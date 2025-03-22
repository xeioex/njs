
/*
 * Copyright (C) Vadim Zhestkov
 * Copyright (C) F5, Inc.
 */


#include <njs_main.h>


const njs_value_t njs_atom[] = {
#define NJS_DEF_VW(_id, _s) njs_symval(_id, _s),
#define NJS_DEF_VS(_id, _s, _typ, _tok) (njs_value_t) {                       \
    .string = {                                                               \
        .type = NJS_STRING,                                                   \
        .truth = njs_length(_s) ? 1 : 0,                                      \
        .atom_id = NJS_ATOM_ ## _id,                                          \
        .token_type = _typ,                                                   \
        .token_id = _tok,                                                     \
        .data = & (njs_string_t) {                                            \
            .start = (u_char *) _s,                                           \
            .length = njs_length(_s),                                         \
            .size = njs_length(_s),                                           \
        },                                                                    \
    }                                                                         \
},

    #include <njs_atom_defs.h>
};


static njs_int_t
njs_atom_hash_test(njs_flathsh_query_t *lhq, void *data)
{
    size_t       size;
    u_char       *start;
    njs_value_t  *name;

    name = data;

    if (name->type == NJS_STRING
        && ((njs_value_t *) lhq->value)->type == NJS_STRING)
    {
        size = name->string.data->length;

        if (lhq->key.length != size) {
            return NJS_DECLINED;
        }

        start = (u_char *) name->string.data->start;

        if (memcmp(start, lhq->key.start, lhq->key.length) == 0) {
           return NJS_OK;
        }
    }

    if (name->type == NJS_SYMBOL
        && ((njs_value_t *) lhq->value)->type == NJS_SYMBOL)
    {
        if (lhq->key_hash == name->atom_id) {
            return NJS_OK;
        }
    }

    return NJS_DECLINED;
}


const njs_flathsh_proto_t  njs_atom_hash_proto
    njs_aligned(64) =
{
    0,
    njs_atom_hash_test,
    njs_lvlhsh_alloc,
    njs_lvlhsh_free,
};


njs_int_t
njs_atom_hash_init(njs_vm_t *vm)
{
    u_char               *start;
    size_t               len;
    njs_int_t            ret;
    njs_uint_t           n;
    const njs_value_t    *value, *values;
    njs_flathsh_query_t  lhq;

    values = &njs_atom[0];

    njs_lvlhsh_init(vm->atom_hash);

    lhq.replace = 0;
    lhq.proto = &njs_atom_hash_proto;
    lhq.pool = vm->mem_pool;

    for (n = 0; n < NJS_ATOM_SIZE; n++) {
        value = &values[n];

        if (value->type == NJS_SYMBOL) {
            lhq.key_hash = value->string.atom_id;
            lhq.value = (void *) value;

            ret = njs_flathsh_insert(vm->atom_hash, &lhq);
            if (njs_slow_path(ret != NJS_OK)) {
                njs_internal_error(vm, "flathsh insert/replace failed");
                return NJS_ERROR;
            }
        }

        if (value->type == NJS_STRING) {
            start = value->string.data->start;
            len = value->string.data->length;

            lhq.key_hash = njs_djb_hash(start, len);
            lhq.key.length = len;
            lhq.key.start = start;

            lhq.value = (void *) value;

            ret = njs_flathsh_insert(vm->atom_hash, &lhq);
            if (njs_slow_path(ret != NJS_OK)) {
                njs_internal_error(vm, "flathsh insert/replace failed");
                return NJS_ERROR;
            }
        }
    }

    return NJS_OK;
};


/*
 * value is always key: string or number or symbol.
 *
 * symbol always contain atom_id by construction. do nothing;
 * number if short number it is atomized by "| 0x80000000";
 * string if represents short number it is atomized by "| 0x80000000";
 * for string and symbol atom_ids common range is uint32_t < 0x80000000.
 */

njs_int_t
njs_atom_atomize_key(njs_vm_t *vm, njs_value_t *value)
{
    double             num;
    uint32_t           hash_id;
    njs_int_t          ret;
    njs_value_t        val_str;
    const njs_value_t  *entry;

    njs_assert(value->atom_id == 0);

    switch (value->type) {
    case NJS_STRING:
        num = njs_key_to_index(value);
        if (njs_fast_path(njs_key_is_integer_index(num, value)) &&
            ((uint32_t) num) < 0x80000000)
        {
            value->atom_id = njs_number_atom(((uint32_t) num));

        } else {
            hash_id = njs_djb_hash(value->string.data->start,
                                   value->string.data->size);

            entry = njs_lexer_keyword_find(vm, value->string.data->start,
                                           value->string.data->size,
                                           value->string.data->length,
                                           hash_id);
            if (njs_slow_path(entry == NULL)) {
                return NJS_ERROR;
            }

            *value = *entry;
        }

        break;

    case NJS_NUMBER:
        num = njs_number(value);
        if (njs_fast_path(njs_key_is_integer_index(num, value)) &&
            ((uint32_t) num) < 0x80000000)
        {
            value->atom_id = njs_number_atom(((uint32_t) num));

        } else {
            ret = njs_number_to_string(vm, &val_str, value);
            if (ret != NJS_OK) {
                return ret;
            }

            if (val_str.atom_id == 0) {
                hash_id = njs_djb_hash(val_str.string.data->start,
                                       val_str.string.data->size);

                entry = njs_lexer_keyword_find(vm, val_str.string.data->start,
                                               val_str.string.data->size,
                                               val_str.string.data->length,
                                               hash_id);
                if (njs_slow_path(entry == NULL)) {
                    return NJS_ERROR;
                }

                value->atom_id = entry->atom_id;

            } else {
                value->atom_id = val_str.atom_id;
            }
        }

        break;

    case NJS_SYMBOL:
    default:
        /* do nothing. */
        break;
    }

    return NJS_OK;
}


njs_int_t
njs_atom_atomize_key_s(njs_vm_t *vm, njs_value_t *value)
{
    njs_int_t            ret;
    njs_flathsh_query_t  lhq;

    lhq.replace = 0;
    lhq.proto = &njs_lexer_hash_proto;
    lhq.pool = vm->atom_hash_mem_pool;

    value->string.atom_id = (*vm->atom_hash_atom_id)++;

    if (value->type == NJS_SYMBOL) {
        lhq.key_hash = value->string.atom_id;
        lhq.value = (void *) value;

        ret = njs_flathsh_insert(vm->atom_hash, &lhq);
        if (njs_slow_path(ret != NJS_OK)) {
            njs_internal_error(vm, "flathsh insert/replace failed");
            return NJS_ERROR;
        }
    }

    return NJS_OK;
}
