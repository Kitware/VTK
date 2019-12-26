/* Copyright 2005, University Corporation for Atmospheric Research. See
 * the COPYRIGHT file for copying and redistribution conditions. */
/**
 * @file
 * @internal This file is part of netcdf-4, a netCDF-like interface
 * for HDF5, or a HDF5 backend for netCDF, depending on your point of
 * view.
 *
 * This file handles the nc4 user-defined type functions
 * (i.e. compound and opaque types).
 *
 * @author Ed Hartnett
 */
#include "nc4internal.h"
#include "nc4dispatch.h"

/** @internal Names of atomic types. */
const char* nc4_atomic_name[NUM_ATOMIC_TYPES] = {"none", "byte", "char",
                                           "short", "int", "float",
                                           "double", "ubyte",
                                           "ushort", "uint",
                                           "int64", "uint64", "string"};

/* The sizes of types may vary from platform to platform, but within
 * netCDF files, type sizes are fixed. */
#define NC_CHAR_LEN sizeof(char)      /**< @internal Size of char. */
#define NC_STRING_LEN sizeof(char *)  /**< @internal Size of char *. */
#define NC_BYTE_LEN 1     /**< @internal Size of byte. */
#define NC_SHORT_LEN 2    /**< @internal Size of short. */
#define NC_INT_LEN 4      /**< @internal Size of int. */
#define NC_FLOAT_LEN 4    /**< @internal Size of float. */
#define NC_DOUBLE_LEN 8   /**< @internal Size of double. */
#define NC_INT64_LEN 8    /**< @internal Size of int64. */

/**
 * @internal Find all user-defined types for a location. This finds
 * all user-defined types in a group.
 *
 * @param ncid File and group ID.
 * @param ntypes Pointer that gets the number of user-defined
 * types. Ignored if NULL
 * @param typeids Array that gets the typeids. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
NC4_inq_typeids(int ncid, int *ntypes, int *typeids)
{
    NC_GRP_INFO_T *grp;
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    int num = 0;
    int retval;

    LOG((2, "nc_inq_typeids: ncid 0x%x", ncid));

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;
    assert(h5 && grp);

    /* Count types. */
    if (grp->type) {
        int i;
        for(i=0;i<ncindexsize(grp->type);i++)
        {
            if((type = (NC_TYPE_INFO_T*)ncindexith(grp->type,i)) == NULL) continue;
            if (typeids)
                typeids[num] = type->hdr.id;
            num++;
        }
    }

    /* Give the count to the user. */
    if (ntypes)
        *ntypes = num;

    return NC_NOERR;
}

/**
 * @internal Get the name and size of a type. For strings, 1 is
 * returned. For VLEN the base type len is returned.
 *
 * @param ncid File and group ID.
 * @param typeid1 Type ID.
 * @param name Gets the name of the type.
 * @param size Gets the size of one element of the type in bytes.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADTYPE Type not found.
 * @author Ed Hartnett
 */
int
NC4_inq_type(int ncid, nc_type typeid1, char *name, size_t *size)
{
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    static const int atomic_size[NUM_ATOMIC_TYPES] = {0, NC_BYTE_LEN, NC_CHAR_LEN, NC_SHORT_LEN,
                                                      NC_INT_LEN, NC_FLOAT_LEN, NC_DOUBLE_LEN,
                                                      NC_BYTE_LEN, NC_SHORT_LEN, NC_INT_LEN, NC_INT64_LEN,
                                                      NC_INT64_LEN, NC_STRING_LEN};

    int retval;

    LOG((2, "nc_inq_type: ncid 0x%x typeid %d", ncid, typeid1));

    /* If this is an atomic type, the answer is easy. */
    if (typeid1 < NUM_ATOMIC_TYPES)
    {
        if (name)
            strcpy(name, nc4_atomic_name[typeid1]);
        if (size)
            *size = atomic_size[typeid1];
        return NC_NOERR;
    }

    /* Not an atomic type - so find group. */
    if ((retval = nc4_find_nc4_grp(ncid, &grp)))
        return retval;

    /* Find this type. */
    if (!(type = nclistget(grp->nc4_info->alltypes, typeid1)))
        return NC_EBADTYPE;

    if (name)
        strcpy(name, type->hdr.name);

    if (size)
    {
        if (type->nc_type_class == NC_VLEN)
            *size = sizeof(nc_vlen_t);
        else if (type->nc_type_class == NC_STRING)
            *size = 1;
        else
            *size = type->size;
    }

    return NC_NOERR;
}

/**
 * @internal Find info about any user defined type.
 *
 * @param ncid File and group ID.
 * @param typeid1 Type ID.
 * @param name Gets name of the type.
 * @param size Gets size in bytes of one element of type.
 * @param base_nc_typep Gets the base nc_type.
 * @param nfieldsp Gets the number of fields.
 * @param classp Gets the type class (NC_COMPOUND, NC_ENUM, NC_VLEN).
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADTYPE Type not found.
 * @author Ed Hartnett
 */
int
NC4_inq_user_type(int ncid, nc_type typeid1, char *name, size_t *size,
                  nc_type *base_nc_typep, size_t *nfieldsp, int *classp)
{
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    int retval;

    LOG((2, "nc_inq_user_type: ncid 0x%x typeid %d", ncid, typeid1));

    /* Find group metadata. */
    if ((retval = nc4_find_nc4_grp(ncid, &grp)))
        return retval;

    /* Find this type. */
    if (!(type = nclistget(grp->nc4_info->alltypes, typeid1)))
        return NC_EBADTYPE;

    /* Count the number of fields. */
    if (nfieldsp)
    {
        if (type->nc_type_class == NC_COMPOUND)
            *nfieldsp = nclistlength(type->u.c.field);
        else if (type->nc_type_class == NC_ENUM)
            *nfieldsp = nclistlength(type->u.e.enum_member);
        else
            *nfieldsp = 0;
    }

    /* Fill in size and name info, if desired. */
    if (size)
    {
        if (type->nc_type_class == NC_VLEN)
            *size = sizeof(nc_vlen_t);
        else if (type->nc_type_class == NC_STRING)
            *size = 1;
        else
            *size = type->size;
    }
    if (name)
        strcpy(name, type->hdr.name);

    /* VLENS and ENUMs have a base type - that is, they type they are
     * arrays of or enums of. */
    if (base_nc_typep)
    {
        if (type->nc_type_class == NC_ENUM)
            *base_nc_typep = type->u.e.base_nc_typeid;
        else if (type->nc_type_class == NC_VLEN)
            *base_nc_typep = type->u.v.base_nc_typeid;
        else
            *base_nc_typep = NC_NAT;
    }

    /* If the user wants it, tell whether this is a compound, opaque,
     * vlen, enum, or string class of type. */
    if (classp)
        *classp = type->nc_type_class;

    return NC_NOERR;
}

/**
 * @internal Given the ncid, typeid and fieldid, get info about the
 * field.
 *
 * @param ncid File and group ID.
 * @param typeid1 Type ID.
 * @param fieldid Field ID.
 * @param name Gets name of field.
 * @param offsetp Gets offset of field.
 * @param field_typeidp Gets field type ID.
 * @param ndimsp Gets number of dims for this field.
 * @param dim_sizesp Gets the dim sizes for this field.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
NC4_inq_compound_field(int ncid, nc_type typeid1, int fieldid, char *name,
                       size_t *offsetp, nc_type *field_typeidp, int *ndimsp,
                       int *dim_sizesp)
{
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    NC_FIELD_INFO_T *field;
    int d, retval;

    /* Find file metadata. */
    if ((retval = nc4_find_nc4_grp(ncid, &grp)))
        return retval;

    /* Find this type. */
    if (!(type = nclistget(grp->nc4_info->alltypes, typeid1)))
        return NC_EBADTYPE;

    /* Find the field. */
    if (!(field = nclistget(type->u.c.field,fieldid)))
        return NC_EBADFIELD;

    if (name)
        strcpy(name, field->hdr.name);
    if (offsetp)
        *offsetp = field->offset;
    if (field_typeidp)
        *field_typeidp = field->nc_typeid;
    if (ndimsp)
        *ndimsp = field->ndims;
    if (dim_sizesp)
        for (d = 0; d < field->ndims; d++)
            dim_sizesp[d] = field->dim_size[d];

    return NC_NOERR;
}

/**
 * @internal Given the typeid and the name, get the fieldid.
 *
 * @param ncid File and group ID.
 * @param typeid1 Type ID.
 * @param name Name of field.
 * @param fieldidp Pointer that gets new field ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADTYPE Type not found.
 * @return ::NC_EBADFIELD Field not found.
 * @author Ed Hartnett
 */
int
NC4_inq_compound_fieldindex(int ncid, nc_type typeid1, const char *name, int *fieldidp)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    NC_FIELD_INFO_T *field;
    char norm_name[NC_MAX_NAME + 1];
    int retval;
    int i;

    LOG((2, "nc_inq_compound_fieldindex: ncid 0x%x typeid %d name %s",
         ncid, typeid1, name));

    /* Find file metadata. */
    if ((retval = nc4_find_grp_h5(ncid, NULL, &h5)))
        return retval;

    /* Find the type. */
    if ((retval = nc4_find_type(h5, typeid1, &type)))
        return retval;

    /* Did the user give us a good compound type typeid? */
    if (!type || type->nc_type_class != NC_COMPOUND)
        return NC_EBADTYPE;

    /* Normalize name. */
    if ((retval = nc4_normalize_name(name, norm_name)))
        return retval;

    /* Find the field with this name. */
    for (i = 0; i < nclistlength(type->u.c.field); i++)
    {
        field = nclistget(type->u.c.field, i);
        assert(field);
        if (!strcmp(field->hdr.name, norm_name))
            break;
        field = NULL; /* because this is the indicator of not found */
    }

    if (!field)
        return NC_EBADFIELD;

    if (fieldidp)
        *fieldidp = field->hdr.id;
    return NC_NOERR;
}

/**
 * @internal Get enum name from enum value. Name size will be <=
 * NC_MAX_NAME.
 *
 * @param ncid File and group ID.
 * @param xtype Type ID.
 * @param value Value of enum.
 * @param identifier Gets the identifier for this enum value.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADTYPE Type not found.
 * @return ::NC_EINVAL Invalid type data.
 * @author Ed Hartnett
 */
int
NC4_inq_enum_ident(int ncid, nc_type xtype, long long value, char *identifier)
{
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    NC_ENUM_MEMBER_INFO_T *enum_member;
    long long ll_val;
    int i;
    int retval;
    int found;

    LOG((3, "nc_inq_enum_ident: xtype %d value %d\n", xtype, value));

    /* Find group metadata. */
    if ((retval = nc4_find_nc4_grp(ncid, &grp)))
        return retval;

    /* Find this type. */
    if (!(type = nclistget(grp->nc4_info->alltypes, xtype)))
        return NC_EBADTYPE;

    /* Complain if they are confused about the type. */
    if (type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;

    /* Move to the desired enum member in the list. */
    for (found = 0, i = 0; i < nclistlength(type->u.e.enum_member); i++)
    {
        enum_member = nclistget(type->u.e.enum_member, i);
        assert(enum_member);
        switch (type->u.e.base_nc_typeid)
        {
        case NC_BYTE:
            ll_val = *(char *)enum_member->value;
            break;
        case NC_UBYTE:
            ll_val = *(unsigned char *)enum_member->value;
            break;
        case NC_SHORT:
            ll_val = *(short *)enum_member->value;
            break;
        case NC_USHORT:
            ll_val = *(unsigned short *)enum_member->value;
            break;
        case NC_INT:
            ll_val = *(int *)enum_member->value;
            break;
        case NC_UINT:
            ll_val = *(unsigned int *)enum_member->value;
            break;
        case NC_INT64:
        case NC_UINT64:
            ll_val = *(long long *)enum_member->value;
            break;
        default:
            return NC_EINVAL;
        }
        LOG((4, "ll_val=%d", ll_val));
        if (ll_val == value)
        {
            if (identifier)
                strcpy(identifier, enum_member->name);
            found = 1;
            break;
        }
    }

    /* If we didn't find it, life sucks for us. :-( */
    if (!found)
        return NC_EINVAL;

    return NC_NOERR;
}

/**
 * @internal Get information about an enum member: an identifier and
 * value. Identifier size will be <= NC_MAX_NAME.
 *
 * @param ncid File and group ID.
 * @param typeid1 Type ID.
 * @param idx Enum member index.
 * @param identifier Gets the identifier.
 * @param value Gets the enum value.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADTYPE Type not found.
 * @return ::NC_EINVAL Bad idx.
 * @author Ed Hartnett
 */
int
NC4_inq_enum_member(int ncid, nc_type typeid1, int idx, char *identifier,
                    void *value)
{
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    NC_ENUM_MEMBER_INFO_T *enum_member;
    int retval;

    LOG((2, "nc_inq_enum_member: ncid 0x%x typeid %d", ncid, typeid1));

    /* Find group metadata. */
    if ((retval = nc4_find_nc4_grp(ncid, &grp)))
        return retval;

    /* Find this type. */
    if (!(type = nclistget(grp->nc4_info->alltypes, typeid1)))
        return NC_EBADTYPE;

    /* Complain if they are confused about the type. */
    if (type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;

    /* Move to the desired enum member in the list. */
    if (!(enum_member = nclistget(type->u.e.enum_member, idx)))
        return NC_EINVAL;

    /* Give the people what they want. */
    if (identifier)
        strcpy(identifier, enum_member->name);
    if (value)
        memcpy(value, enum_member->value, type->size);

    return NC_NOERR;
}
