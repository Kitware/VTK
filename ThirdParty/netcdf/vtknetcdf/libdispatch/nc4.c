/*
 Copyright 2010 University Corporation for Atmospheric
 Research/Unidata. See COPYRIGHT file for more info.

 This file defines the netcdf-4 functions.

 "$Id: nc4.c,v 1.1 2010/06/01 15:46:50 ed Exp $" 
*/

#include "ncdispatch.h"

/* When you read string type the library will allocate the storage
 * space for the data. This storage space must be freed, so pass the
 * pointer back to this function, when you're done with the data, and
 * it will free the string memory. */
int
nc_free_string(size_t len, char **data)
{
   int i;
   for (i = 0; i < len; i++)
      free(data[i]);
   return NC_NOERR;
}

/* When you read VLEN type the library will actually allocate the
 * storage space for the data. This storage space must be freed, so
 * pass the pointer back to this function, when you're done with the
 * data, and it will free the vlen memory. */
int
nc_free_vlen(nc_vlen_t *vl)
{
   free(vl->p);
   return NC_NOERR;
}

/* Free an array of vlens given the number of elements and an
 * array. */ 
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

/* Wrappers for nc_inq_var_all */

int
nc_inq_var_deflate(int ncid, int varid, int *shufflep, int *deflatep, int *deflate_levelp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      shufflep, /*shufflep*/
      deflatep, /*deflatep*/
      deflate_levelp, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

int
nc_inq_var_szip(int ncid, int varid, int *options_maskp, int *pixels_per_blockp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      options_maskp, /*optionsmaskp*/
      pixels_per_blockp /*pixelsp*/
      );
}

int
nc_inq_var_fletcher32(int ncid, int varid, int *fletcher32p)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      fletcher32p, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

int
nc_inq_var_chunking(int ncid, int varid, int *contiguousp, size_t *chunksizesp)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(ncid, varid, NULL, NULL, NULL, NULL, 
				     NULL, NULL, NULL, NULL, NULL, contiguousp, 
				     chunksizesp, NULL, NULL, NULL, NULL, NULL);
}

int
nc_inq_var_fill(int ncid, int varid, int *no_fill, void *fill_value)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      no_fill, /*nofillp*/
      fill_value, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

int
nc_inq_var_endian(int ncid, int varid, int *endianp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      endianp, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}


int
nc_inq_ncid(int ncid, const char *name, int *grp_ncid)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_ncid(ncid,name,grp_ncid);
}

int
nc_inq_grps(int ncid, int *numgrps, int *ncids)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_grps(ncid,numgrps,ncids);
}

int
nc_inq_grpname(int ncid, char *name)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_grpname(ncid,name);
}

int
nc_inq_grpname_full(int ncid, size_t *lenp, char *full_name)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_grpname_full(ncid,lenp,full_name);
}

int
nc_inq_grpname_len(int ncid, size_t *lenp)
{
    int stat = nc_inq_grpname_full(ncid,lenp,NULL);    
    return stat;
}

int
nc_inq_grp_parent(int ncid, int *parent_ncid)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_grp_parent(ncid,parent_ncid);
}

/* This has same semantics as nc_inq_ncid */ 
int
nc_inq_grp_ncid(int ncid, const char *grp_name, int *grp_ncid)
{
    return nc_inq_ncid(ncid,grp_name,grp_ncid);    
}

int
nc_inq_grp_full_ncid(int ncid, const char *full_name, int *grp_ncid)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_grp_full_ncid(ncid,full_name,grp_ncid);
}

int 
nc_inq_varids(int ncid, int *nvars, int *varids)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_varids(ncid,nvars,varids);
}

int 
nc_inq_dimids(int ncid, int *ndims, int *dimids, int include_parents)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_dimids(ncid,ndims,dimids,include_parents);
}

int 
nc_inq_typeids(int ncid, int *ntypes, int *typeids)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_typeids(ncid,ntypes,typeids);
}

int
nc_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, 
		  nc_type typeid2, int *equal)
{
    NC* ncp1;
    int stat = NC_check_id(ncid1,&ncp1);
    if(stat != NC_NOERR) return stat;
    return ncp1->dispatch->inq_type_equal(ncid1,typeid1,ncid2,typeid2,equal);
}

int
nc_def_grp(int parent_ncid, const char *name, int *new_ncid)
{
    NC* ncp;
    int stat = NC_check_id(parent_ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_grp(parent_ncid,name,new_ncid);
}

int
nc_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_compound(ncid,size,name,typeidp);
}

int
nc_insert_compound(int ncid, nc_type xtype, const char *name, size_t offset, nc_type field_typeid)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->insert_compound(ncid,xtype,name,offset,field_typeid);
}

int
nc_insert_array_compound(int ncid, nc_type xtype, const char *name, size_t offset, nc_type field_typeid, int ndims, const int *dim_sizes)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->insert_array_compound(ncid,xtype,name,offset,field_typeid,ndims,dim_sizes);
}

int
nc_inq_typeid(int ncid, const char *name, nc_type *typeidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_typeid(ncid,name,typeidp);
}

int
nc_inq_compound(int ncid, nc_type xtype, char *name, size_t *sizep, size_t *nfieldsp)
{
    int class = 0;
    int stat = nc_inq_user_type(ncid,xtype,name,sizep,NULL,nfieldsp,&class);
    if(stat != NC_NOERR) return stat;
    if(class != NC_COMPOUND) stat = NC_EBADTYPE;
    return stat;
}

int
nc_inq_compound_name(int ncid, nc_type xtype, char *name)
{
    return nc_inq_compound(ncid,xtype,name,NULL,NULL);
}

int
nc_inq_compound_size(int ncid, nc_type xtype, size_t *sizep)
{
    return nc_inq_compound(ncid,xtype,NULL,sizep,NULL);
}

int
nc_inq_compound_nfields(int ncid, nc_type xtype, size_t *nfieldsp)
{
    return nc_inq_compound(ncid,xtype,NULL,NULL,nfieldsp);
}

int
nc_inq_compound_field(int ncid, nc_type xtype, int fieldid, char *name, size_t *offsetp, nc_type *field_typeidp, int *ndimsp, int *dim_sizesp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,name,offsetp,field_typeidp,ndimsp,dim_sizesp);
}

int
nc_inq_compound_fieldname(int ncid, nc_type xtype, int fieldid, 
			  char *name)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,name,NULL,NULL,NULL,NULL);
}

int
nc_inq_compound_fieldoffset(int ncid, nc_type xtype, int fieldid, 
			    size_t *offsetp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,NULL,offsetp,NULL,NULL,NULL);
}

int
nc_inq_compound_fieldtype(int ncid, nc_type xtype, int fieldid, 
			  nc_type *field_typeidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,NULL,NULL,field_typeidp,NULL,NULL);
}

int
nc_inq_compound_fieldndims(int ncid, nc_type xtype, int fieldid, 
			   int *ndimsp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,NULL,NULL,NULL,ndimsp,NULL);
}

int
nc_inq_compound_fielddim_sizes(int ncid, nc_type xtype, int fieldid, 
			       int *dim_sizes)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_field(ncid,xtype,fieldid,NULL,NULL,NULL,NULL,dim_sizes);
}

int
nc_inq_compound_fieldindex(int ncid, nc_type xtype, const char *name, 
			   int *fieldidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_compound_fieldindex(ncid,xtype,name,fieldidp);
}

int
nc_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *xtypep)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_vlen(ncid,name,base_typeid,xtypep);
}

int
nc_inq_vlen(int ncid, nc_type xtype, char *name, size_t *datum_sizep, nc_type *base_nc_typep)
{
    int class = 0;
    int stat = nc_inq_user_type(ncid,xtype,name,datum_sizep,base_nc_typep,NULL,&class);
    if(stat != NC_NOERR) return stat;
    if(class != NC_VLEN) stat = NC_EBADTYPE;
    return stat;
}

int
nc_put_vlen_element(int ncid, int typeid1, void *vlen_element, size_t len, const void *data)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->put_vlen_element(ncid,typeid1,vlen_element,len,data);
}

int
nc_get_vlen_element(int ncid, int typeid1, const void *vlen_element, size_t *len, void *data)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->get_vlen_element(ncid,typeid1,vlen_element,len,data);
}

int
nc_inq_user_type(int ncid, nc_type xtype, char *name, size_t *size, nc_type *base_nc_typep, size_t *nfieldsp, int *classp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_user_type(ncid,xtype,name,size,base_nc_typep,nfieldsp,classp);
}

int
nc_def_enum(int ncid, nc_type base_typeid, const char *name, nc_type *typeidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_enum(ncid,base_typeid,name,typeidp);
}

int
nc_insert_enum(int ncid, nc_type xtype, const char *name, const void *value)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->insert_enum(ncid,xtype,name,value);
}

int
nc_inq_enum(int ncid, nc_type xtype, char *name, nc_type *base_nc_typep, size_t *base_sizep, size_t *num_membersp)
{
    int class = 0;
    int stat = nc_inq_user_type(ncid,xtype,name,base_sizep,base_nc_typep,num_membersp,&class);
    if(stat != NC_NOERR) return stat;
    if(class != NC_ENUM) stat = NC_EBADTYPE;
    return stat;
}

int
nc_inq_enum_member(int ncid, nc_type xtype, int idx, char *name, void *value)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_enum_member(ncid,xtype,idx,name,value);
}

int
nc_inq_enum_ident(int ncid, nc_type xtype, long long value, char *identifier)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_enum_ident(ncid,xtype,value,identifier);
}

int
nc_def_opaque(int ncid, size_t size, const char *name, nc_type *xtypep)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_opaque(ncid,size,name,xtypep);
}

int
nc_inq_opaque(int ncid, nc_type xtype, char *name, size_t *sizep)
{
    int class = 0;
    int stat = nc_inq_user_type(ncid,xtype,name,sizep,NULL,NULL,&class);
    if(stat != NC_NOERR) return stat;
    if(class != NC_OPAQUE) stat = NC_EBADTYPE;
    return stat;
}

int
nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate, int deflate_level)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_deflate(ncid,varid,shuffle,deflate,deflate_level);
}

int
nc_def_var_fletcher32(int ncid, int varid, int fletcher32)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_fletcher32(ncid,varid,fletcher32);
}

int
nc_def_var_chunking(int ncid, int varid, int storage, const size_t *chunksizesp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_chunking(ncid,varid,storage,chunksizesp);
}

int
nc_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_fill(ncid,varid,no_fill,fill_value);
}

int
nc_def_var_endian(int ncid, int varid, int endian)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_endian(ncid,varid,endian);
}

int
nc_set_var_chunk_cache(int ncid, int varid, size_t size, size_t nelems, float preemption)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->set_var_chunk_cache(ncid,varid,size,nelems,preemption);
}

int
nc_get_var_chunk_cache(int ncid, int varid, size_t *sizep, size_t *nelemsp, float *preemptionp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->get_var_chunk_cache(ncid,varid,sizep,nelemsp,preemptionp);
}

int 
nc_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_unlimdims(ncid,nunlimdimsp,unlimdimidsp);
}

int 
nc_show_metadata(int ncid)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->show_metadata(ncid);
}

