/*! \file
  Functions for VLEN Types

  Copyright 2011 University Corporation for Atmospheric
  Research/Unidata. See \ref copyright file for more info. */

#include "ncdispatch.h"

/** \name Variable Length Array Types

    Functions to create and learn about VLEN types. */
/*! \{ */ /* All these functions are part of this named group... */

/** 
\ingroup user_types 
Free memory in a VLEN object. 

When you read VLEN type the library will actually allocate the storage
space for the data. This storage space must be freed, so pass the
pointer back to this function, when you're done with the data, and it
will free the vlen memory.

The function nc_free_vlens() is more useful than this function,
because it can free an array of VLEN objects.

\param vl pointer to the vlen object.

\returns ::NC_NOERR No error.
*/
int
nc_free_vlen(nc_vlen_t *vl)
{
   free(vl->p);
   return NC_NOERR;
}

/** 
\ingroup user_types 
Free an array of vlens given the number of elements and an array. 

When you read VLEN type the library will actually allocate the storage
space for the data. This storage space must be freed, so pass the
pointer back to this function, when you're done with the data, and it
will free the vlen memory.

\param len number of elements in the array.
\param vlens pointer to the vlen object.

\returns ::NC_NOERR No error.
*/ 
int
nc_free_vlens(size_t len, nc_vlen_t vlens[])
{
   int ret;
   size_t i;

   for(i = 0; i < len; i++) 
      if ((ret = nc_free_vlen(&vlens[i])))
	 return ret;

   return NC_NOERR;
}

/** 
\ingroup user_types
Use this function to define a variable length array type.

\param ncid \ref ncid
\param name \ref object_name of new type.

\param base_typeid The typeid of the base type of the VLEN. For
example, for a VLEN of shorts, the base type is ::NC_SHORT. This can be
a user defined type.

\param xtypep A pointer to an nc_type variable. The typeid of the new
VLEN type will be set here.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad \ref ncid.
\returns ::NC_EBADTYPE Bad type id.
\returns ::NC_ENOTNC4 Not an netCDF-4 file, or classic model enabled.
\returns ::NC_EHDFERR An error was reported by the HDF5 layer.
\returns ::NC_ENAMEINUSE That name is in use.
\returns ::NC_EMAXNAME Name exceeds max length NC_MAX_NAME.
\returns ::NC_EBADNAME Name contains illegal characters.
\returns ::NC_EPERM Attempt to write to a read-only file.
\returns ::NC_ENOTINDEFINE Not in define mode. 
 */
int
nc_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *xtypep)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_vlen(ncid,name,base_typeid,xtypep);
}

/** \ingroup user_types
Learn about a VLEN type.

\param ncid \ref ncid
\param xtype The type of the VLEN to inquire about. 
\param name \ref object_name of the type. \ref ignored_if_null.

\param datum_sizep A pointer to a size_t, this will get the size of
one element of this vlen. \ref ignored_if_null.

\param base_nc_typep Pointer to get the base type of the VLEN. \ref
ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad \ref ncid.
\returns ::NC_EBADTYPE Bad type id.
\returns ::NC_ENOTNC4 Not an netCDF-4 file, or classic model enabled.
\returns ::NC_EHDFERR An error was reported by the HDF5 layer.
 */
int
nc_inq_vlen(int ncid, nc_type xtype, char *name, size_t *datum_sizep, nc_type *base_nc_typep)
{
    int class = 0;
    int stat = nc_inq_user_type(ncid,xtype,name,datum_sizep,base_nc_typep,NULL,&class);
    if(stat != NC_NOERR) return stat;
    if(class != NC_VLEN) stat = NC_EBADTYPE;
    return stat;
}
/*! \} */  /* End of named group ...*/

/** \internal
\ingroup user_types

Put a VLEN element. This function writes an element of a VLEN for the
Fortran APIs.

\param ncid \ref ncid
\param typeid1 Typeid of the VLEN.
\param vlen_element Pointer to the element of the VLEN.
\param len Lenth of the VLEN element.
\param data VLEN data.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad \ref ncid.
\returns ::NC_EBADTYPE Bad type id.
\returns ::NC_ENOTNC4 Not an netCDF-4 file, or classic model enabled.
\returns ::NC_EHDFERR An error was reported by the HDF5 layer.
\returns ::NC_EPERM Attempt to write to a read-only file.
 */
int
nc_put_vlen_element(int ncid, int typeid1, void *vlen_element, size_t len, const void *data)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->put_vlen_element(ncid,typeid1,vlen_element,len,data);
}

/** 
\internal
\ingroup user_types

Get a VLEN element. This function reads an element of a VLEN for the
Fortran APIs.

\param ncid \ref ncid
\param typeid1 Typeid of the VLEN.
\param vlen_element Pointer to the element of the VLEN.
\param len Lenth of the VLEN element.
\param data VLEN data.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad \ref ncid.
\returns ::NC_EBADTYPE Bad type id.
\returns ::NC_ENOTNC4 Not an netCDF-4 file, or classic model enabled.
\returns ::NC_EHDFERR An error was reported by the HDF5 layer.
 */
int
nc_get_vlen_element(int ncid, int typeid1, const void *vlen_element, 
		    size_t *len, void *data)
{
    NC *ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->get_vlen_element(ncid, typeid1, vlen_element, 
					   len, data);
}
