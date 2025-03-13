
/*
 * Copyright (C) Vadim Zhestkov
 * Copyright (C) F5, Inc.
 */


#ifndef _NJS_ATOM_H_INCLUDED_
#define _NJS_ATOM_H_INCLUDED_


#define njs_atom_is_number(atom_id) \
    ((atom_id & 0x80000000))


#define njs_atom_number(atom_id) \
    (atom_id & 0x7FFFFFFF)


#define njs_number_atom(n) \
    (n | 0x80000000)


#ifdef NJS_DEF_VW
    #undef NJS_DEF_VW
    #undef NJS_DEF_VS
#endif

#define NJS_DEF_VW(name) \
    const njs_value_t vw_ ## name;

#define NJS_DEF_VS(name) \
    const njs_value_t vs_ ## name;

typedef struct {
    #include <njs_atom_defs.h>
} njs_atom_values_t;


#ifdef NJS_DEF_VW
    #undef NJS_DEF_VW
    #undef NJS_DEF_VS
#endif

#define NJS_DEF_VW(name) \
    NJS_ATOM_SYMBOL_ ## name,

#define NJS_DEF_VS(name) \
    NJS_ATOM_ ## name,

enum {
    #include <njs_atom_defs.h>
    NJS_ATOM_SIZE,
};


#include <njs_atom_map.h>

njs_int_t njs_atom_hash_init(njs_vm_t *vm);
njs_int_t njs_atom_atomize_key(njs_vm_t *vm, njs_value_t *value);
njs_int_t njs_atom_atomize_key_s(njs_vm_t *vm, njs_value_t *value);

extern const njs_atom_values_t    njs_atom;
extern const njs_flathsh_proto_t  njs_atom_hash_proto;


/* Returns string or symbol. */
njs_inline njs_int_t
njs_atom_to_value(njs_vm_t *vm, njs_value_t *dst, uint32_t atom_id)
{
    size_t  size;
    double  num;
    u_char  buf[128];

    if (njs_atom_is_number(atom_id)) {
        num = njs_atom_number(atom_id);
        size = njs_dtoa(num, (char *) buf);

        return njs_string_new(vm, dst, buf, size, size);
    }

    if (atom_id < vm->atom_hash_atom_id_shared_cell) {
        *dst = *((njs_value_t *) (njs_hash_elts(
                      (&vm->atom_hash_shared_cell)->slot))[atom_id].value);

    } else {
        *dst = *((njs_value_t *) (njs_hash_elts(vm->atom_hash->slot))[
                      atom_id - vm->atom_hash_atom_id_shared_cell].value);
    }

    return NJS_OK;
}

#endif /* _NJS_ATOM_H_INCLUDED_ */
