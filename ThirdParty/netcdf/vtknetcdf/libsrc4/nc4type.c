/*

This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 user-defined type functions (i.e. compound
and opaque types).

Copyright 2005, University Corporation for Atmospheric Research. See
the COPYRIGHT file for copying and redistribution conditions.

$Id: nc4type.c,v 1.73 2010/05/25 17:54:24 dmh Exp $
*/

#include "nc4internal.h"

#define NUM_ATOMIC_TYPES 13
char atomic_name[NUM_ATOMIC_TYPES][NC_MAX_NAME + 1] = {"none", "byte", "char", 
						       "short", "int", "float", 
						       "double", "ubyte",
						       "ushort", "uint",
						       "int64", "uint64", "string"};

EXTERNL int
NC4_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, 
		  nc_type typeid2, int *equalp)
{
   NC_GRP_INFO_T *grp1, *grp2;
   NC_TYPE_INFO_T *type1, *type2;
   int retval;
   
   LOG((2, "nc_inq_type_equal: ncid1 0x%x typeid1 %d ncid2 0x%x typeid2 %d", 
	ncid1, typeid1, ncid2, typeid2));

   /* Check input. */
   if(equalp == NULL) return NC_NOERR;

   if (typeid1 <= NC_NAT || typeid2 <= NC_NAT)
      return NC_EINVAL;

   /* If one is atomic, and the other user-defined, the types are not
    * equal. */
   if ((typeid1 <= NC_STRING && typeid2 > NC_STRING) ||
       (typeid2 <= NC_STRING && typeid1 > NC_STRING))
   {
      if (equalp) *equalp = 0;
      return NC_NOERR;
   }

   /* If both are atomic types, the answer is easy. */
   if (typeid1 <= NUM_ATOMIC_TYPES)
   {
      if (equalp)
      {
	 if (typeid1 == typeid2)
	    *equalp = 1;
	 else
	    *equalp = 0;
      }	    
      return NC_NOERR;
   }

   /* Not atomic types - so find type1 and type2 information. */
   if ((retval = nc4_find_nc4_grp(ncid1, &grp1)))
      return retval;
   if (!(type1 = nc4_rec_find_nc_type(grp1->file->nc4_info->root_grp, 
				      typeid1)))
      return NC_EBADTYPE;
   if ((retval = nc4_find_nc4_grp(ncid2, &grp2)))
      return retval;
   if (!(type2 = nc4_rec_find_nc_type(grp2->file->nc4_info->root_grp, 
				      typeid2)))
      return NC_EBADTYPE;

   /* Are the two types equal? */
   if (equalp)
      *equalp = (int)H5Tequal(type1->native_typeid, type2->native_typeid);
   
   return NC_NOERR;
}

/* Get the id of a type from the name. */
EXTERNL int
NC4_inq_typeid(int ncid, const char *name, nc_type *typeidp)
{
   NC_GRP_INFO_T *grp, *grp2;
   NC_HDF5_FILE_INFO_T *h5;
   NC_TYPE_INFO_T *type = NULL;
   char *norm_name;
   int i, retval;

   for (i = 0; i < NUM_ATOMIC_TYPES; i++)
      if (!strcmp(name, atomic_name[i]))
      {
	 if (typeidp)
	    *typeidp = i;
	 return NC_NOERR;
      }

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* Must be a netCDF-4 file. */
   if (!h5)
      return NC_ENOTNC4;

   /* If the first char is a /, this is a fully-qualified
    * name. Otherwise, this had better be a local name (i.e. no / in
    * the middle). */
   if (name[0] != '/' && strstr(name, "/"))
      return NC_EINVAL;

   /* Normalize name. */
   if (!(norm_name = malloc(strlen(name) + 1)))
      return NC_ENOMEM;
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   /* Is the type in this group? If not, search parents. */
   for (grp2 = grp; grp2; grp2 = grp2->parent)
      for (type = grp2->type; type; type = type->next)
	 if (!strcmp(norm_name, type->name))
	 {
	    if (typeidp)
	       *typeidp = type->nc_typeid;
	    break;
	 }

   /* Still didn't find type? Search file recursively, starting at the
    * root group. */
   if (!type)
      if ((type = nc4_rec_find_named_type(grp->file->nc4_info->root_grp, norm_name)))
	 if (typeidp)
	    *typeidp = type->nc_typeid;

   free(norm_name);

   /* OK, I give up already! */
   if (!type)
      return NC_EBADTYPE;
   
   return NC_NOERR;
}

/* Find all user-defined types for a location. This finds all
 * user-defined types in a group. */
int 
NC4_inq_typeids(int ncid, int *ntypes, int *typeids)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_TYPE_INFO_T *type;
   int num = 0;
   int retval;

   LOG((2, "nc_inq_typeids: ncid 0x%x", ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* If this is a netCDF-4 file, count types. */
   if (h5 && grp->type)
      for (type = grp->type; type; type = type->next)
      {
	 if (typeids)
	    typeids[num] = type->nc_typeid;
	 num++;
      }

   /* Give the count to the user. */
   if (ntypes)
      *ntypes = num;

   return NC_NOERR;
}


/* This internal function adds a new user defined type to the metadata
 * of a group of an open file. */
static int
add_user_type(int ncid, size_t size, const char *name, nc_type base_typeid,
	      nc_type type_class, nc_type *typeidp)
{
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   LOG((2, "add_user_type: ncid 0x%x size %d name %s base_typeid %d ", 
	ncid, size, norm_name, base_typeid));

   /* Find group metadata. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* Only netcdf-4 files! */
   if (!h5)
      return NC_ENOTNC4;

   /* Turn on define mode if it is not on. */
   if (!(h5->cmode & NC_INDEF))
      if ((retval = nc_redef(ncid)))
	 return retval;

   /* No size is provided for vlens or enums, get it from the base type. */
   if (type_class == NC_VLEN || type_class == NC_ENUM)
   {
      if ((retval = nc4_get_typelen_mem(grp->file->nc4_info, base_typeid, 0, 
					&size)))
	 return retval;
   }
   else if (size <= 0)
      return NC_EINVAL;

   /* Check that this name is not in use as a var, grp, or type. */
   if ((retval = nc4_check_dup_name(grp, norm_name)))
      return retval;
   
   /* Add to our list of types. */
   if ((retval = nc4_type_list_add(&(grp->type), &type)))
      return retval;

   /* Remember info about this type. */
   type->nc_typeid = grp->file->nc4_info->next_typeid++;
   type->size = size;
   if (!(type->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(type->name, norm_name);
   type->class = type_class;
   type->base_nc_type = base_typeid;
   
   /* Return the typeid to the user. */
   if (typeidp)
      *typeidp = type->nc_typeid;

   return NC_NOERR;
}


/* The sizes of types may vary from platform to platform, but within
 * netCDF files, type sizes are fixed. */
#define NC_CHAR_LEN sizeof(char)
#define NC_STRING_LEN sizeof(char *)
#define NC_BYTE_LEN 1
#define NC_SHORT_LEN 2
#define NC_INT_LEN 4
#define NC_FLOAT_LEN 4
#define NC_DOUBLE_LEN 8
#define NC_INT64_LEN 8

/* Get the name and size of a type. For strings, 1 is returned. For
 * VLEN the base type len is returned. */
int
NC4_inq_type(int ncid, nc_type typeid, char *name, size_t *size)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   int atomic_size[NUM_ATOMIC_TYPES] = {0, NC_BYTE_LEN, NC_CHAR_LEN, NC_SHORT_LEN, 
					NC_INT_LEN, NC_FLOAT_LEN, NC_DOUBLE_LEN, 
					NC_BYTE_LEN, NC_SHORT_LEN, NC_INT_LEN, NC_INT64_LEN, 
					NC_INT64_LEN, NC_STRING_LEN};
					
   int retval;
   
   LOG((2, "nc_inq_type: ncid 0x%x typeid %d", ncid, typeid));

   /* If this is an atomic type, the answer is easy. */
   if (typeid <= NUM_ATOMIC_TYPES)
   {
      if (name)
	 strcpy(name, atomic_name[typeid]);
      if (size)
	 *size = atomic_size[typeid];
      return NC_NOERR;
   }

   /* Not an atomic type - so find group. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;
   
   /* Find this type. */
   if (!(type = nc4_rec_find_nc_type(grp->file->nc4_info->root_grp, typeid)))
      return NC_EBADTYPE;

   if (name)
      strcpy(name, type->name);
   
   if (size)
   {
      if (type->class != NC_VLEN)
	 *size = type->size;
      else
	 *size = sizeof(nc_vlen_t);
   }
   
   return NC_NOERR;
}

/* Create a compound type. */
int
NC4_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp)
{
   return add_user_type(ncid, size, name, 0, NC_COMPOUND, typeidp);
}

/* Insert a named field into a compound type. */
int
NC4_insert_compound(int ncid, nc_type typeid, const char *name, size_t offset, 
		   nc_type field_typeid)
{
   return nc_insert_array_compound(ncid, typeid, name, offset, 
				   field_typeid, 0, NULL);
}

/* Insert a named array into a compound type. */
EXTERNL int
NC4_insert_array_compound(int ncid, int typeid, const char *name, 
			 size_t offset, nc_type field_typeid,
			 int ndims, const int *dim_sizesp)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "nc_insert_array_compound: ncid 0x%x, typeid %d name %s "
	"offset %d field_typeid %d ndims %d", ncid, typeid, 
	name, offset, field_typeid, ndims));

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Find file metadata. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;

   /* Find type metadata. */
   if ((retval = nc4_find_type(grp->file->nc4_info, typeid, &type)))
      return retval;

   /* Did the user give us a good compound type typeid? */
   if (!type || type->class != NC_COMPOUND)
      return NC_EBADTYPE;

   /* If this type has already been written to the file, you can't
    * change it. */
   if (type->committed)
      return NC_ETYPDEFINED;

   /* Insert new field into this type's list of fields. */
   if ((retval = nc4_field_list_add(&type->field, type->num_fields, 
				    norm_name, offset, 0, 0, field_typeid,
				    ndims, dim_sizesp)))
      return retval;

   type->num_fields++;
   
   return NC_NOERR;
}

/* Find info about any user defined type. */
int
NC4_inq_user_type(int ncid, nc_type typeid, char *name, size_t *size, 
		 nc_type *base_nc_typep, size_t *nfieldsp, int *classp)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   NC_FIELD_INFO_T *field;
   int retval;
   
   LOG((2, "nc_inq_user_type: ncid 0x%x typeid %d", ncid, typeid));

   /* Find group metadata. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;
   
   /* Find this type. */
   if (!(type = nc4_rec_find_nc_type(grp->file->nc4_info->root_grp, typeid)))
      return NC_EBADTYPE;

   /* Count the number of fields. */
   if (nfieldsp)
   {
      *nfieldsp = 0;
      if (type->class == NC_COMPOUND)
	 for (field = type->field; field; field = field->next)
	    (*nfieldsp)++;
      else if (type->class == NC_ENUM)
	 *nfieldsp = type->num_enum_members;
   }

   /* Fill in size and name info, if desired. */
   if (size)
   {
      if (type->class != NC_VLEN)
	 *size = type->size;
      else
	 *size = sizeof(nc_vlen_t);
   }
   if (name)
      strcpy(name, type->name);

   /* VLENS and ENUMs have a base type - that is, they type they are
    * arrays of or enums of. */
   if (base_nc_typep)
      *base_nc_typep = type->base_nc_type;

   /* If the user wants it, tell whether this is a compound, opaque,
    * vlen, enum, or string class of type. */
   if (classp)
      *classp = type->class;

   return NC_NOERR;
}

/* Given the ncid, typeid and fieldid, get info about the field. */
int
NC4_inq_compound_field(int ncid, nc_type typeid, int fieldid, char *name, 
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
   if (!(type = nc4_rec_find_nc_type(grp->file->nc4_info->root_grp, typeid)))
      return NC_EBADTYPE;

   /* Find the field. */
   for (field = type->field; field; field = field->next)
      if (field->fieldid == fieldid)
      {
	 if (name)
	    strcpy(name, field->name);
	 if (offsetp)
	    *offsetp = field->offset;
	 if (field_typeidp)
	    *field_typeidp = field->nctype;
	 if (ndimsp)
	    *ndimsp = field->ndims;
	 if (dim_sizesp)
	    for (d = 0; d < field->ndims; d++)
	       dim_sizesp[d] = field->dim_size[d];
	 return NC_NOERR;
      }

   return NC_EBADFIELD;
}

/* Find a netcdf-4 file. THis will return an error if it finds a
 * netcdf-3 file, or a netcdf-4 file with strict nc3 rules. */
static int
find_nc4_file(int ncid, NC_FILE_INFO_T **nc)
{
   
   /* Find file metadata. */
   if (!((*nc) = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Check for netcdf-3 files or netcdf-3 rules. */
   if (!(*nc)->nc4_info)
      return NC_ENOTNC4;
   if ((*nc)->nc4_info->cmode & NC_CLASSIC_MODEL)
      return NC_ESTRICTNC3;

   return NC_NOERR;
}

/* Given the typeid and the name, get the fieldid. */
int
NC4_inq_compound_fieldindex(int ncid, nc_type typeid, const char *name, int *fieldidp)
{
   NC_FILE_INFO_T *nc;
   NC_TYPE_INFO_T *type;
   NC_FIELD_INFO_T *field;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "nc_inq_compound_fieldindex: ncid 0x%x typeid %d name %s",
	ncid, typeid, name));

   /* Find file metadata. */
   if ((retval = find_nc4_file(ncid, &nc)))
      return retval;

   /* Find the type. */
   if ((retval = nc4_find_type(nc->nc4_info, typeid, &type)))
      return retval;

   /* Did the user give us a good compound type typeid? */
   if (!type || type->class != NC_COMPOUND)
      return NC_EBADTYPE;

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   /* Find the field with this name. */
   for (field = type->field; field; field = field->next)
      if (!strcmp(field->name, norm_name))
	 break;

   if (!field)
      return NC_EBADFIELD;

   if (fieldidp)
      *fieldidp = field->fieldid;
   return NC_NOERR;
}


/* Opaque type. */

/* Create an opaque type. Provide a size and a name. */
int
NC4_def_opaque(int ncid, size_t datum_size, const char *name, 
	      nc_type *typeidp)
{
   return add_user_type(ncid, datum_size, name, 0, NC_OPAQUE, typeidp);
}


/* Define a variable length type. */
int
NC4_def_vlen(int ncid, const char *name, nc_type base_typeid, 
	    nc_type *typeidp)
{
   return add_user_type(ncid, 0, name, base_typeid, NC_VLEN, typeidp);
}

/* Create an enum type. Provide a base type and a name. At the moment
 * only ints are accepted as base types. */
int
NC4_def_enum(int ncid, nc_type base_typeid, const char *name, 
	    nc_type *typeidp)
{
   return add_user_type(ncid, 0, name, base_typeid, NC_ENUM, typeidp);
}


/* Get enum name from enum value. Name size will be <= NC_MAX_NAME. */
int
NC4_inq_enum_ident(int ncid, nc_type xtype, long long value, char *identifier)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   NC_ENUM_MEMBER_INFO_T *enum_member;
   long long ll_val;
   int i;
   int retval;

   LOG((3, "nc_inq_enum_ident: xtype %d value %d\n", xtype, value));
   
   /* Find group metadata. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;
   
   /* Find this type. */
   if (!(type = nc4_rec_find_nc_type(grp->file->nc4_info->root_grp, xtype)))
      return NC_EBADTYPE;
   
   /* Complain if they are confused about the type. */
   if (type->class != NC_ENUM)
      return NC_EBADTYPE;
   
   /* Move to the desired enum member in the list. */
   enum_member = type->enum_member;
   for (i = 0; i < type->num_enum_members; i++)
   {
      switch (type->base_nc_type)
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
	 break;
      }
      else
	 enum_member = enum_member->next;
   }

   /* If we didn't find it, life sucks for us. :-( */
   if (i == type->num_enum_members)
      return NC_EINVAL;

   return NC_NOERR;
}

/* Get information about an enum member: an identifier and
 * value. Identifier size will be <= NC_MAX_NAME. */
int
NC4_inq_enum_member(int ncid, nc_type typeid, int idx, char *identifier, 
		   void *value)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   NC_ENUM_MEMBER_INFO_T *enum_member;
   int i;
   int retval;
   
   LOG((2, "nc_inq_enum_member: ncid 0x%x typeid %d", ncid, typeid));

   /* Find group metadata. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;
   
   /* Find this type. */
   if (!(type = nc4_rec_find_nc_type(grp->file->nc4_info->root_grp, typeid)))
      return NC_EBADTYPE;
   
   /* Complain if they are confused about the type. */
   if (type->class != NC_ENUM)
      return NC_EBADTYPE;
   
   /* Check index. */
   if (idx >= type->num_enum_members)
      return NC_EINVAL;
   
   /* Move to the desired enum member in the list. */
   enum_member = type->enum_member;
   for (i = 0; i < idx; i++)
      enum_member = enum_member->next;

   /* Give the people what they want. */
   if (identifier)
      strcpy(identifier, enum_member->name);
   if (value)
      memcpy(value, enum_member->value, type->size);

   return NC_NOERR;
}

/* Insert a identifierd value into an enum type. The value must fit within
 * the size of the enum type, the identifier size must be <= NC_MAX_NAME. */
int
NC4_insert_enum(int ncid, nc_type typeid, const char *identifier, 
	       const void *value)
{
   NC_GRP_INFO_T *grp;
   NC_TYPE_INFO_T *type;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "nc_insert_enum: ncid 0x%x, typeid %d identifier %s value %d", ncid, 
	typeid, identifier, value));

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(identifier, norm_name)))
      return retval;

   /* Find file metadata. */
   if ((retval = nc4_find_nc4_grp(ncid, &grp)))
      return retval;

   /* Find type metadata. */
   if ((retval = nc4_find_type(grp->file->nc4_info, typeid, &type)))
      return retval;

   /* Did the user give us a good enum typeid? */
   if (!type || type->class != NC_ENUM)
      return NC_EBADTYPE;

   /* If this type has already been written to the file, you can't
    * change it. */
   if (type->committed)
      return NC_ETYPDEFINED;

   /* Insert new field into this type's list of fields. */
   if ((retval = nc4_enum_member_add(&type->enum_member, type->size, 
				     norm_name, value)))
      return retval;

      type->num_enum_members++;
   
   return NC_NOERR;
}

/* Insert one element into an already allocated vlen array element. */
int
NC4_put_vlen_element(int ncid, int typeid, void *vlen_element, 
		    size_t len, const void *data)
{
   nc_vlen_t *tmp = vlen_element;
   tmp->len = len;
   tmp->p = (void *)data;
   return NC_NOERR;
}

/* Insert one element into an already allocated vlen array element. */
int
NC4_get_vlen_element(int ncid, int typeid, const void *vlen_element, 
		    size_t *len, void *data)
{
   const nc_vlen_t *tmp = vlen_element;
   int type_size = 4;

   *len = tmp->len;
   memcpy(data, tmp->p, tmp->len * type_size);
   return NC_NOERR;
}

