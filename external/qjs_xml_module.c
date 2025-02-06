
/*
 * Copyright (C) Dmitry Volyntsev
 * Copyright (C) F5, Inc.
 */

#include <qjs.h>
#include <njs_sprintf.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/c14n.h>
#include <libxml/xpathInternals.h>


typedef struct {
    xmlDoc         *doc;
    xmlParserCtxt  *ctx;
    int            ref_count;
} qjs_xml_doc_t;


typedef struct {
    xmlNode        *node;
    qjs_xml_doc_t  *doc;
} qjs_xml_node_t;


typedef enum {
    XML_NSET_TREE = 0,
    XML_NSET_TREE_NO_COMMENTS,
    XML_NSET_TREE_INVERT,
} qjs_xml_nset_type_t;


typedef struct qjs_xml_nset_s  qjs_xml_nset_t;

struct qjs_xml_nset_s {
    xmlNodeSet           *nodes;
    xmlDoc               *doc;
    qjs_xml_nset_type_t  type;
    qjs_xml_nset_t       *next;
    qjs_xml_nset_t       *prev;
};


static JSValue qjs_xml_parse(JSContext *cx, JSValueConst this_val,
    int argc, JSValueConst *argv);


static int qjs_xml_doc_get_own_property(JSContext *cx,
    JSPropertyDescriptor *pdesc, JSValueConst obj, JSAtom prop);
static int qjs_xml_doc_get_own_property_names(JSContext *cx,
    JSPropertyEnum **ptab, uint32_t *plen, JSValueConst obj);
static void qjs_xml_doc_finalizer(JSRuntime *rt, JSValue val);

static JSValue qjs_xml_node_name(JSContext *cx, JSValueConst this_val);
static JSValue qjs_xml_node_text_get(JSContext *cx, JSValueConst this_val);
static int qjs_xml_node_get_own_property(JSContext *cx,
    JSPropertyDescriptor *pdesc, JSValueConst obj, JSAtom prop);
static int qjs_xml_node_get_own_property_names(JSContext *cx,
    JSPropertyEnum **ptab, uint32_t *plen, JSValueConst obj);
static JSValue qjs_xml_node_make(JSContext *cx, qjs_xml_doc_t *doc,
    xmlNode *node);
static void qjs_xml_node_finalizer(JSRuntime *rt, JSValue val);

static void qjs_xml_error(JSContext *cx, qjs_xml_doc_t *current,
    const char *fmt, ...);
static JSModuleDef *qjs_xml_init(JSContext *ctx, const char *name);


static const JSCFunctionListEntry qjs_xml_export[] = {
    JS_CFUNC_DEF("parse", 2, qjs_xml_parse),
};


static const JSCFunctionListEntry qjs_xml_doc_proto[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "XMLDoc", JS_PROP_CONFIGURABLE),
};


static const JSCFunctionListEntry qjs_xml_node_proto[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "XMLNode", JS_PROP_CONFIGURABLE),
    JS_CGETSET_DEF("$name", qjs_xml_node_name, NULL),
    JS_CGETSET_DEF("$text", qjs_xml_node_text_get, NULL),
};


static JSClassDef qjs_xml_doc_class = {
    "XMLDoc",
    .finalizer = qjs_xml_doc_finalizer,
    .exotic = & (JSClassExoticMethods) {
        .get_own_property = qjs_xml_doc_get_own_property,
        .get_own_property_names = qjs_xml_doc_get_own_property_names,
    },
};


static JSClassDef qjs_xml_node_class = {
    "XMLNode",
    .finalizer = qjs_xml_node_finalizer,
    .exotic = & (JSClassExoticMethods) {
        .get_own_property = qjs_xml_node_get_own_property,
        .get_own_property_names = qjs_xml_node_get_own_property_names,
    },
};


qjs_module_t  qjs_xml_module = {
    .name = "xml",
    .init = qjs_xml_init,
};


static JSValue
qjs_xml_parse(JSContext *cx, JSValueConst this_val, int argc,
    JSValueConst *argv)
{
    JSValue        ret;
    qjs_bytes_t    data;
    qjs_xml_doc_t  *tree;

    if (qjs_to_bytes(cx, &data, argv[0]) < 0) {
        return JS_EXCEPTION;
    }

    tree = js_malloc(cx, sizeof(qjs_xml_doc_t));
    if (tree == NULL) {
        qjs_bytes_free(cx, &data);
        JS_ThrowOutOfMemory(cx);
        return JS_EXCEPTION;
    }

    tree->ref_count = 1;

    tree->ctx = xmlNewParserCtxt();
    if (tree->ctx == NULL) {
        qjs_bytes_free(cx, &data);
        JS_ThrowInternalError(cx, "xmlNewParserCtxt() failed");
        return JS_EXCEPTION;
    }

    tree->doc = xmlCtxtReadMemory(tree->ctx, (char *) data.start, data.length,
                                  NULL, NULL, XML_PARSE_NOWARNING
                                              | XML_PARSE_NOERROR);
    qjs_bytes_free(cx, &data);
    if (tree->doc == NULL) {
        qjs_xml_error(cx, tree, "failed to parse XML");
        return JS_EXCEPTION;
    }

    ret = JS_NewObjectClass(cx, QJS_CORE_CLASS_ID_XML_DOC);
    if (JS_IsException(ret)) {
        return ret;
    }

    JS_SetOpaque(ret, tree);

    return ret;
}


static int
qjs_xml_doc_get_own_property(JSContext *cx, JSPropertyDescriptor *pdesc,
    JSValueConst obj, JSAtom prop)
{
    xmlNode        *node;
    njs_str_t      name;
    qjs_xml_doc_t  *tree;

    tree = JS_GetOpaque(obj, QJS_CORE_CLASS_ID_XML_DOC);
    if (tree == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLDoc");
        return -1;
    }

    name.start = (u_char *) JS_AtomToCString(cx, prop);
    if (name.start == NULL) {
        return -1;
    }

    name.length = njs_strlen(name.start);

    for (node = xmlDocGetRootElement(tree->doc);
         node != NULL;
         node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (name.length != njs_strlen(node->name)
            || njs_strncmp(name.start, node->name, name.length) != 0)
        {
            continue;
        }

        JS_FreeCString(cx, (char *) name.start);

        if (pdesc != NULL) {
            pdesc->flags = JS_PROP_ENUMERABLE;
            pdesc->getter = JS_UNDEFINED;
            pdesc->setter = JS_UNDEFINED;
            pdesc->value  = qjs_xml_node_make(cx, tree, node);
            if (JS_IsException(pdesc->value)) {
                return -1;
            }
        }

        return 1;
    }

    JS_FreeCString(cx, (char *) name.start);

    return 0;
}


static int
qjs_xml_push_string(JSContext *cx, JSValue obj, const char *start)
{
    JSAtom  key;

    key = JS_NewAtomLen(cx, start, njs_strlen(start));
    if (key == JS_ATOM_NULL) {
        return -1;
    }

    if (JS_DefinePropertyValue(cx, obj, key, JS_UNDEFINED,
                               JS_PROP_ENUMERABLE) < 0)
    {
        JS_FreeAtom(cx, key);
        return -1;
    }

    JS_FreeAtom(cx, key);

    return 0;
}


static int
qjs_xml_doc_get_own_property_names(JSContext *cx, JSPropertyEnum **ptab,
    uint32_t *plen, JSValueConst obj)
{
    int            rc;
    JSValue        keys;
    xmlNode        *node;
    qjs_xml_doc_t  *tree;

    tree = JS_GetOpaque(obj, QJS_CORE_CLASS_ID_XML_DOC);
    if (tree == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLDoc");
        return -1;
    }

    keys = JS_NewObject(cx);
    if (JS_IsException(keys)) {
        return -1;
    }

    for (node = xmlDocGetRootElement(tree->doc);
         node != NULL;
         node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (qjs_xml_push_string(cx, keys, (char *) node->name) < 0) {
            JS_FreeValue(cx, keys);
            return -1;
        }
    }

    rc = JS_GetOwnPropertyNames(cx, ptab, plen, keys, JS_GPN_STRING_MASK);

    JS_FreeValue(cx, keys);

    return rc;
}


static void
qjs_xml_doc_free(JSRuntime *rt, qjs_xml_doc_t *current)
{
    current->ref_count--;

    if (current->ref_count == 0) {
        xmlFreeDoc(current->doc);
        xmlFreeParserCtxt(current->ctx);
        js_free_rt(rt, current);
    }
}


static void
qjs_xml_doc_finalizer(JSRuntime *rt, JSValue val)
{
    qjs_xml_doc_t  *current;

    current = JS_GetOpaque(val, QJS_CORE_CLASS_ID_XML_DOC);

    qjs_xml_doc_free(rt, current);
}


static JSValue
qjs_xml_node_name(JSContext *cx, JSValueConst this_val)
{
    xmlNode         *node;
    qjs_xml_node_t  *current;

    current = JS_GetOpaque(this_val, QJS_CORE_CLASS_ID_XML_NODE);
    if (current == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLNode");
        return JS_EXCEPTION;
    }

    node = current->node;

    if (node == NULL || node->type != XML_ELEMENT_NODE) {
        return JS_UNDEFINED;
    }

    return JS_NewString(cx, (char *) node->name);
}


static JSValue
qjs_xml_node_text_get(JSContext *cx, JSValueConst this_val)
{
    u_char          *text;
    JSValue         ret;
    qjs_xml_node_t  *current;

    current = JS_GetOpaque(this_val, QJS_CORE_CLASS_ID_XML_NODE);
    if (current == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLNode");
        return JS_EXCEPTION;
    }

    text = xmlNodeGetContent(current->node);
    if (text == NULL) {
        return JS_UNDEFINED;
    }

    ret = JS_NewString(cx, (char *) text);

    xmlFree(text);

    return ret;
}


static int
qjs_xml_node_get_own_property(JSContext *cx, JSPropertyDescriptor *pdesc,
    JSValueConst obj, JSAtom prop)
{
    size_t          size;
    xmlNode         *node;
    njs_str_t       name;
    qjs_xml_node_t  *current;

    /*
     * $tag$foo - the first tag child with the name "foo"
     * $tags$foo - the all children with the name "foo" as an array
     * $attr$foo - the attribute with the name "foo"
     * foo - the same as $tag$foo
     */

    current = JS_GetOpaque(obj, QJS_CORE_CLASS_ID_XML_NODE);
    if (current == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLNode");
        return -1;
    }

    name.start = (u_char *) JS_AtomToCString(cx, prop);
    if (name.start == NULL) {
        return -1;
    }

    name.length = njs_strlen(name.start);

#if 0
    if (name.length > 1 && name.start[0] == '$') {
    }
#endif

    for (node = current->node->children; node != NULL; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }

        size = njs_strlen(node->name);

        if (name.length != size
            || njs_strncmp(name.start, node->name, size) != 0)
        {
            continue;
        }

        if (pdesc != NULL) {
            pdesc->flags = JS_PROP_ENUMERABLE;
            pdesc->getter = JS_UNDEFINED;
            pdesc->setter = JS_UNDEFINED;
            pdesc->value  = qjs_xml_node_make(cx, current->doc, node);
            if (JS_IsException(pdesc->value)) {
                return -1;
            }
        }

        return 1;
    }

    return 0;
}


static int
qjs_xml_node_get_own_property_names(JSContext *cx, JSPropertyEnum **ptab,
    uint32_t *plen, JSValueConst obj)
{
    int             rc;
    JSValue         keys;
    xmlNode         *node, *current;
    qjs_xml_node_t  *tree;

    tree = JS_GetOpaque(obj, QJS_CORE_CLASS_ID_XML_NODE);
    if (tree == NULL) {
        (void) JS_ThrowInternalError(cx, "\"this\" is not an XMLNode");
        return -1;
    }

    keys = JS_NewObject(cx);
    if (JS_IsException(keys)) {
        return -1;
    }

    current = tree->node;

    if (current->name != NULL && current->type == XML_ELEMENT_NODE) {
        if (qjs_xml_push_string(cx, keys, "$name") < 0) {
            goto fail;
        }
    }

    if (current->ns != NULL) {
        if (qjs_xml_push_string(cx, keys, "$ns") < 0) {
            goto fail;
        }
    }

    if (current->properties != NULL) {
        if (qjs_xml_push_string(cx, keys, "$attrs") < 0) {
            goto fail;
        }
    }

    if (current->children != NULL && current->children->content != NULL) {
        if (qjs_xml_push_string(cx, keys, "$text") < 0) {
            goto fail;
        }
    }

    for (node = current->children; node != NULL; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (qjs_xml_push_string(cx, keys, "$tags") < 0) {
            goto fail;
        }

        break;
    }

    rc = JS_GetOwnPropertyNames(cx, ptab, plen, keys, JS_GPN_STRING_MASK);

    JS_FreeValue(cx, keys);

    return rc;

fail:

    JS_FreeValue(cx, keys);

    return -1;
}


static void
qjs_xml_node_finalizer(JSRuntime *rt, JSValue val)
{
    qjs_xml_node_t  *node;

    node = JS_GetOpaque(val, QJS_CORE_CLASS_ID_XML_NODE);

    qjs_xml_doc_free(rt, node->doc);

    js_free_rt(rt, node);
}


static JSValue
qjs_xml_node_make(JSContext *cx, qjs_xml_doc_t *doc, xmlNode *node)
{
    JSValue         ret;
    qjs_xml_node_t  *current;

    current = js_malloc(cx, sizeof(qjs_xml_node_t));
    if (current == NULL) {
        JS_ThrowOutOfMemory(cx);
        return JS_EXCEPTION;
    }

    current->node = node;
    current->doc = doc;
    doc->ref_count++;

    ret = JS_NewObjectClass(cx, QJS_CORE_CLASS_ID_XML_NODE);
    if (JS_IsException(ret)) {
        return ret;
    }

    JS_SetOpaque(ret, current);

    return ret;
}


static void
qjs_xml_error(JSContext *cx, qjs_xml_doc_t *current, const char *fmt, ...)
{
    u_char          *p, *last;
    va_list         args;
    const xmlError  *err;
    u_char          errstr[NJS_MAX_ERROR_STR];

    last = &errstr[NJS_MAX_ERROR_STR];

    va_start(args, fmt);
    p = njs_vsprintf(errstr, last - 1, fmt, args);
    va_end(args);

    err = xmlCtxtGetLastError(current->ctx);

    if (err != NULL) {
        p = njs_sprintf(p, last - 1, " (libxml2: \"%*s\" at %d:%d)",
                        njs_strlen(err->message) - 1, err->message, err->line,
                        err->int2);
    }

    JS_ThrowSyntaxError(cx, "%*s", (int) (p - errstr), errstr);
}


static int
qjs_xml_module_init(JSContext *cx, JSModuleDef *m)
{
    int      rc;
    JSValue  proto;

    proto = JS_NewObject(cx);
    JS_SetPropertyFunctionList(cx, proto, qjs_xml_export,
                               njs_nitems(qjs_xml_export));

    rc = JS_SetModuleExport(cx, m, "default", proto);
    if (rc != 0) {
        return -1;
    }

    return JS_SetModuleExportList(cx, m, qjs_xml_export,
                                  njs_nitems(qjs_xml_export));
}


static JSModuleDef *
qjs_xml_init(JSContext *cx, const char *name)
{
    int          rc;
    JSValue      proto;
    JSModuleDef  *m;

    if (!JS_IsRegisteredClass(JS_GetRuntime(cx),
                              QJS_CORE_CLASS_ID_XML_DOC))
    {
        if (JS_NewClass(JS_GetRuntime(cx), QJS_CORE_CLASS_ID_XML_DOC,
                        &qjs_xml_doc_class) < 0)
        {
            return NULL;
        }

        proto = JS_NewObject(cx);
        if (JS_IsException(proto)) {
            return NULL;
        }

        JS_SetPropertyFunctionList(cx, proto, qjs_xml_doc_proto,
                                   njs_nitems(qjs_xml_doc_proto));

        JS_SetClassProto(cx, QJS_CORE_CLASS_ID_XML_DOC, proto);

        if (JS_NewClass(JS_GetRuntime(cx), QJS_CORE_CLASS_ID_XML_NODE,
                        &qjs_xml_node_class) < 0)
        {
            return NULL;
        }

        proto = JS_NewObject(cx);
        if (JS_IsException(proto)) {
            return NULL;
        }

        JS_SetPropertyFunctionList(cx, proto, qjs_xml_node_proto,
                                   njs_nitems(qjs_xml_node_proto));

        JS_SetClassProto(cx, QJS_CORE_CLASS_ID_XML_NODE, proto);
    }

    m = JS_NewCModule(cx, name, qjs_xml_module_init);
    if (m == NULL) {
        return NULL;
    }

    JS_AddModuleExport(cx, m, "default");
    rc = JS_AddModuleExportList(cx, m, qjs_xml_export,
                                njs_nitems(qjs_xml_export));
    if (rc != 0) {
        return NULL;
    }

    return m;
}
