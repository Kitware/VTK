/*
 * Copyright 1993-2011 University Corporation for Atmospheric
 * Research/Unidata
 * 
 */

#include "config.h"
#include <stdlib.h>
#include "nc.h"

#ifndef MPI_INCLUDED
typedef int MPI_Comm;
typedef int MPI_Info;
#endif

int
nc3_create(const char *path, int cmode, size_t initialsz, int basepe,
	   size_t *chunksizehintp,
	   MPI_Comm comm, MPI_Info info,
	   NC** ncp) {abort();}

int
nc3_open(const char *path, int mode, int basepe, size_t *chunksizehintp, 
	 int use_parallel, MPI_Comm comm, MPI_Info info,
	 NC** ncp) {abort();}

int
nc3_redef(int ncid) {abort();}

int
nc3__enddef(int ncid, size_t h_minfree, size_t v_align,
	size_t v_minfree, size_t r_align) {abort();}

int
nc3_sync(int ncid) {abort();}

int
nc3_abort(int ncid) {abort();}

int
nc3_close(int ncid) {abort();}

int
nc3_set_fill(int ncid, int fillmode, int *old_modep) {abort();}

int
nc3_set_base_pe(int ncid, int pe) {abort();}

int
nc3_inq_base_pe(int ncid, int *pe) {abort();}

int
nc3_inq_format(int ncid, int *formatp) {abort();}

int
nc3_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp) {abort();}

int
nc3_inq_type(int ncid, nc_type xtype,char* name,size_t* sizep) {abort();}

/* Begin _dim */

int
nc3_def_dim(int ncid, const char *name, size_t len, int *idp) {abort();}

int
nc3_inq_dimid(int ncid, const char *name, int *idp) {abort();}

int
nc3_inq_dim(int ncid, int dimid, char *name, size_t *lenp) {abort();}

int
nc3_rename_dim(int ncid, int dimid, const char *name) {abort();}

/* End _dim */
/* Begin _att */

int
nc3_inq_att(int ncid, int varid, const char *name,
	 nc_type *xtypep, size_t *lenp) {abort();}

int 
nc3_inq_attid(int ncid, int varid, const char *name, int *idp) {abort();}

int
nc3_inq_attname(int ncid, int varid, int attnum, char *name) {abort();}

int
nc3_rename_att(int ncid, int varid, const char *name, const char *newname) {abort();}

int
nc3_del_att(int ncid, int varid, const char* name) {abort();}

/* End _att */
/* Begin {put,get}_att */

int
nc3_get_att(int ncid, int varid, const char *name, void *value, nc_type xtype) {abort();}

int
nc3_put_att(int ncid, int varid, const char *name, nc_type datatype,
	   size_t len, const void *value, nc_type xtype) {abort();}

/* End {put,get}_att */
/* Begin _var */

int
nc3_def_var(int ncid, const char *name,
	 nc_type xtype, int ndims, const int *dimidsp, int *varidp) {abort();}

int
nc3_inq_var(int ncid, int varid, char *name,
	 nc_type *xtypep, int *ndimsp, int *dimidsp, int *nattsp) {abort();}

int
nc3_inq_varid(int ncid, const char *name, int *varidp) {abort();}

int
nc3_rename_var(int ncid, int varid, const char *name) {abort();}

int
nc3_put_vara(int ncid, int varid,
   	     const size_t *start, const size_t *count,
             const void *value, nc_type xtype) {abort();}

int
nc3_get_vara(int ncid, int varid,
	     const size_t *start, const size_t *count,
             void *value, nc_type xtype) {abort();}

int
nc3_put_var(int ncid, int varid,  const void *op) {abort();}

int
nc3_get_var(int ncid, int varid,  void *ip) {abort();}

int
nc3_put_var1(int ncid, int varid,  const size_t *indexp,
	    const void *op) {abort();}

int
nc3_get_var1(int ncid, int varid,  const size_t *indexp, void *ip) {abort();}

int
nc3_put_vars(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const void *op) {abort();}

int
nc3_get_vars(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    void *ip) {abort();}

int
nc3_put_varm(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const ptrdiff_t *imapp, const void *op) {abort();}

int
nc3_get_varm(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const ptrdiff_t *imapp, void *ip) {abort();}

/* End _var */

/* netCDF4 API only */
int
nc3_var_par_access(int ncid,int varid,int pint) {abort();}

int
nc3_inq_ncid(int ncid,const char* pnm,int* pintp) {abort();}

int
nc3_inq_grps(int ncid,int* pintp,int* pintp2) {abort();}

int
nc3_inq_grpname(int ncid,char* pcharp) {abort();}

int
nc3_inq_grpname_full(int ncid,size_t* psize_tp,char* pcharp) {abort();}

int
nc3_inq_grp_parent(int ncid,int* pintp) {abort();}

int
nc3_inq_grp_full_ncid(int ncid,const char* pnm,int* pintp) {abort();}

int
nc3_inq_varids(int ncid,int* nvars,int* pintp) {abort();}

int
nc3_inq_dimids(int ncid,int* ndims,int* pintp,int pint) {abort();}

int
nc3_inq_typeids(int ncid,int* ntypes,int* pintp) {abort();}

int
nc3_inq_type_equal(int ncid,nc_type pnc_type,int pint,nc_type pnc_type2,int* pintp) {abort();}

int
nc3_def_grp(int ncid,const char* pnm,int* pintp) {abort();}

int
nc3_inq_user_type(int ncid,nc_type pnc_type,char* pnm,size_t* psize_tp,nc_type* pnc_typep,size_t* psize_tp2,int* pintp) {abort();}


int
nc3_def_compound(int ncid,size_t psize_t,const char* pnm,nc_type* pnc_typep) {abort();}

int
nc3_insert_compound(int ncid,nc_type pnc_type,const char* pnm,size_t psize_t,nc_type pnc_type2) {abort();}

int
nc3_insert_array_compound(int ncid,nc_type pnc_type,const char* pnm,size_t psize_t,nc_type pnc_type2,int pint,const int* pintp) {abort();}

int
nc3_inq_typeid(int ncid,const char* pnm,nc_type* pnc_typep) {abort();}

int
nc3_inq_compound_field(int ncid,nc_type pnc_type,int pint,char* pnm,size_t* psize_tp,nc_type* pnc_typep,int* pintp,int* pintp2) {abort();}

int
nc3_inq_compound_fieldindex(int ncid,nc_type pnc_type,const char* pnm,int* pintp) {abort();}

int
nc3_def_vlen(int ncid,const char* pnm,nc_type base_typeid,nc_type* pnc_typep) {abort();}

int
nc3_put_vlen_element(int ncid,int varid,void* pvoidp,size_t psize_t,const void* voidp) {abort();}

int
nc3_get_vlen_element(int ncid,int varid,const void* pvoidp,size_t* psize_tp,void* pvoidp2) {abort();}

int
nc3_def_enum(int ncid,nc_type pnc_type,const char* pnm,nc_type* pnc_typep) {abort();}

int
nc3_insert_enum(int ncid,nc_type pnc_type,const char* pnm,const void* voidp) {abort();}

int
nc3_inq_enum_member(int ncid,nc_type pnc_type,int pint,char* pnm,void* pvoidp) {abort();}

int
nc3_inq_enum_ident(int ncid,nc_type pnc_type,long long plonglong,char* pcharp) {abort();}

int
nc3_def_opaque(int ncid,size_t psize_t,const char* pnm,nc_type* pnc_typep) {abort();}

int
nc3_def_var_deflate(int ncid,int varid,int pint,int pint2,int pint3) {abort();}

int
nc3_inq_var_deflate(int ncid,int varid,int* pintp,int* pintp2,int* pintp3) {abort();}

int
nc3_inq_var_szip(int ncid,int varid,int* pintp,int* pintp2) {abort();}

int
nc3_def_var_fletcher32(int ncid,int varid,int pint) {abort();}

int
nc3_inq_var_fletcher32(int ncid,int varid,int* pintp) {abort();}

int
nc3_def_var_chunking(int ncid,int varid,int pint,const size_t* size_tp) {abort();}

int
nc3_inq_var_chunking(int ncid,int varid,int* pintp,size_t* psize_tp) {abort();}

int
nc3_def_var_fill(int ncid,int varid,int pint,const void* pvoidp) {abort();}

int
nc3_inq_var_fill(int ncid,int varid,int* pintp,void* pvoidp) {abort();}

int
nc3_def_var_endian(int ncid,int varid,int pint) {abort();}

int
nc3_inq_var_endian(int ncid,int varid,int* pintp) {abort();}

int
nc3_set_var_chunk_cache(int ncid,int varid,size_t psize_t,size_t psize_t2,float pfloat) {abort();}

int
nc3_get_var_chunk_cache(int ncid,int varid,size_t* psize_tp,size_t* psize_tp2, size_t* psize_tp3, float* pfloatp) {abort();}

int
nc3_inq_unlimdims(int ncid ,int* nump,int* dimsp) {abort();}

int 
nc3_inq_unlimdim(int ncid, int *unlimdimidp) {abort();}

int
nc3_show_metadata(int ncid) {abort();}

int
nc3_put_att_text(int ncid, int varid, const char *name,
		size_t len, const char *op) {abort();}

int
nc3_get_att_text(int ncid, int varid, const char *name, char *ip) {abort();}

int
nc3_put_att_uchar(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const unsigned char *op) {abort();}

int
nc3_get_att_uchar(int ncid, int varid, const char *name, unsigned char *ip) {abort();}

int
nc3_put_att_schar(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const signed char *op) {abort();}

int
nc3_get_att_schar(int ncid, int varid, const char *name, signed char *ip) {abort();}

int
nc3_put_att_short(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const short *op) {abort();}

int
nc3_get_att_short(int ncid, int varid, const char *name, short *ip) {abort();}

int
nc3_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
	       size_t len, const int *op) {abort();}

int
nc3_get_att_int(int ncid, int varid, const char *name, int *ip) {abort();}

int
nc3_put_att_long(int ncid, int varid, const char *name, nc_type xtype,
		size_t len, const long *op) {abort();}

int
nc3_get_att_long(int ncid, int varid, const char *name, long *ip) {abort();}

int
nc3_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const float *op) {abort();}

int
nc3_get_att_float(int ncid, int varid, const char *name, float *ip) {abort();}

int
nc3_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const double *op) {abort();}

int
nc3_get_att_double(int ncid, int varid, const char *name, double *ip) {abort();}

int
nc3_put_att_ubyte(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const unsigned char *op) {abort();}

int
nc3_get_att_ubyte(int ncid, int varid, const char *name, 
		 unsigned char *ip) {abort();}

int
nc3_put_att_ushort(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const unsigned short *op) {abort();}

int
nc3_get_att_ushort(int ncid, int varid, const char *name, unsigned short *ip) {abort();}

int
nc3_put_att_uint(int ncid, int varid, const char *name, nc_type xtype,
		size_t len, const unsigned int *op) {abort();}

int
nc3_get_att_uint(int ncid, int varid, const char *name, unsigned int *ip) {abort();}

int
nc3_put_att_longlong(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const long long *op) {abort();}

int
nc3_get_att_longlong(int ncid, int varid, const char *name, long long *ip) {abort();}

int
nc3_put_att_ulonglong(int ncid, int varid, const char *name, nc_type xtype,
		     size_t len, const unsigned long long *op) {abort();}

int
nc3_get_att_ulonglong(int ncid, int varid, const char *name, 
		     unsigned long long *ip) {abort();}

int
nc3_put_att_string(int ncid, int varid, const char *name, 
		  size_t len, const char **op) {abort();}

int
nc3_get_att_string(int ncid, int varid, const char *name, char **ip) {abort();}


int
nc3_put_var1_text(int ncid, int varid, const size_t *indexp, const char *op) {abort();}

int
nc3_get_var1_text(int ncid, int varid, const size_t *indexp, char *ip) {abort();}

int
nc3_put_var1_uchar(int ncid, int varid, const size_t *indexp,
		  const unsigned char *op) {abort();}

int
nc3_get_var1_uchar(int ncid, int varid, const size_t *indexp,
		  unsigned char *ip) {abort();}

int
nc3_put_var1_schar(int ncid, int varid, const size_t *indexp,
		  const signed char *op) {abort();}

int
nc3_get_var1_schar(int ncid, int varid, const size_t *indexp,
		  signed char *ip) {abort();}

int
nc3_put_var1_short(int ncid, int varid, const size_t *indexp,
		  const short *op) {abort();}

int
nc3_get_var1_short(int ncid, int varid, const size_t *indexp,
		  short *ip) {abort();}

int
nc3_put_var1_int(int ncid, int varid, const size_t *indexp, const int *op) {abort();}

int
nc3_get_var1_int(int ncid, int varid, const size_t *indexp, int *ip) {abort();}

int
nc3_put_var1_long(int ncid, int varid, const size_t *indexp, const long *op) {abort();}

int
nc3_get_var1_long(int ncid, int varid, const size_t *indexp, long *ip) {abort();}

int
nc3_put_var1_float(int ncid, int varid, const size_t *indexp, const float *op) {abort();}

int
nc3_get_var1_float(int ncid, int varid, const size_t *indexp, float *ip) {abort();}

int
nc3_put_var1_double(int ncid, int varid, const size_t *indexp, const double *op) {abort();}

int
nc3_get_var1_double(int ncid, int varid, const size_t *indexp, double *ip) {abort();}

int
nc3_put_var1_ubyte(int ncid, int varid, const size_t *indexp, 
		  const unsigned char *op) {abort();}

int
nc3_get_var1_ubyte(int ncid, int varid, const size_t *indexp, 
		  unsigned char *ip) {abort();}

int
nc3_put_var1_ushort(int ncid, int varid, const size_t *indexp, 
		   const unsigned short *op) {abort();}

int
nc3_get_var1_ushort(int ncid, int varid, const size_t *indexp, 
		   unsigned short *ip) {abort();}

int
nc3_put_var1_uint(int ncid, int varid, const size_t *indexp, 
		 const unsigned int *op) {abort();}

int
nc3_get_var1_uint(int ncid, int varid, const size_t *indexp, 
		 unsigned int *ip) {abort();}

int
nc3_put_var1_longlong(int ncid, int varid, const size_t *indexp, 
		     const long long *op) {abort();}

int
nc3_get_var1_longlong(int ncid, int varid, const size_t *indexp, 
		  long long *ip) {abort();}

int
nc3_put_var1_ulonglong(int ncid, int varid, const size_t *indexp, 
		   const unsigned long long *op) {abort();}

int
nc3_get_var1_ulonglong(int ncid, int varid, const size_t *indexp, 
		   unsigned long long *ip) {abort();}

int
nc3_put_var1_string(int ncid, int varid, const size_t *indexp, 
		   const char **op) {abort();}

int
nc3_get_var1_string(int ncid, int varid, const size_t *indexp, 
		   char **ip) {abort();}

/* End {put,get}_var1 */
/* Begin {put,get}_vara */

int
nc3_put_vara_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const char *op) {abort();}

int
nc3_get_vara_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, char *ip) {abort();}

int
nc3_put_vara_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const unsigned char *op) {abort();}

int
nc3_get_vara_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, unsigned char *ip) {abort();}

int
nc3_put_vara_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const signed char *op) {abort();}

int
nc3_get_vara_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, signed char *ip) {abort();}

int
nc3_put_vara_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const short *op) {abort();}

int
nc3_get_vara_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, short *ip) {abort();}

int
nc3_put_vara_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const int *op) {abort();}

int
nc3_get_vara_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, int *ip) {abort();}

int
nc3_put_vara_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const long *op) {abort();}

int
nc3_get_vara_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, long *ip) {abort();}

int
nc3_put_vara_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const float *op) {abort();}

int
nc3_get_vara_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, float *ip) {abort();}

int
nc3_put_vara_double(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const double *op) {abort();}

int
nc3_get_vara_double(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, double *ip) {abort();}

int
nc3_put_vara_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const unsigned char *op) {abort();}

int
nc3_get_vara_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, unsigned char *ip) {abort();}

int
nc3_put_vara_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const unsigned short *op) {abort();}

int
nc3_get_vara_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, unsigned short *ip) {abort();}

int
nc3_put_vara_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const unsigned int *op) {abort();}

int
nc3_get_vara_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, unsigned int *ip) {abort();}

int
nc3_put_vara_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const long long *op) {abort();}

int
nc3_get_vara_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, long long *ip) {abort();}

int
nc3_put_vara_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const unsigned long long *op) {abort();}

int
nc3_get_vara_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, unsigned long long *ip) {abort();}

int
nc3_put_vara_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const char **op) {abort();}

int
nc3_get_vara_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, char **ip) {abort();}

/* End {put,get}_vara */
/* Begin {put,get}_vars */

int
nc3_put_vars_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const char *op) {abort();}

int
nc3_get_vars_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	char *ip) {abort();}

int
nc3_put_vars_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const unsigned char *op) {abort();}

int
nc3_get_vars_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	unsigned char *ip) {abort();}

int
nc3_put_vars_schar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const signed char *op) {abort();}

int
nc3_get_vars_schar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	signed char *ip) {abort();}

int
nc3_put_vars_short(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const short *op) {abort();}

int
nc3_get_vars_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  short *ip) {abort();}

int
nc3_put_vars_int(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const int *op) {abort();}

int
nc3_get_vars_int(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	int *ip) {abort();}

int
nc3_put_vars_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const long *op) {abort();}

int
nc3_get_vars_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	long *ip) {abort();}

int
nc3_put_vars_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const float *op) {abort();}

int
nc3_get_vars_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	float *ip) {abort();}

int
nc3_put_vars_double(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const double *op) {abort();}

int
nc3_get_vars_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   double *ip) {abort();}

int
nc3_put_vars_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const unsigned char *op) {abort();}

int
nc3_get_vars_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  unsigned char *ip) {abort();}

int
nc3_put_vars_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const unsigned short *op) {abort();}

int
nc3_get_vars_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   unsigned short *ip) {abort();}

int
nc3_put_vars_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const unsigned int *op) {abort();}

int
nc3_get_vars_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 unsigned int *ip) {abort();}

int
nc3_put_vars_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const long long *op) {abort();}

int
nc3_get_vars_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  long long *ip) {abort();}

int
nc3_put_vars_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const unsigned long long *op) {abort();}

int
nc3_get_vars_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   unsigned long long *ip) {abort();}

int
nc3_put_vars_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const char **op) {abort();}

int
nc3_get_vars_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   char **ip) {abort();}

/* End {put,get}_vars */
/* Begin {put,get}_varm */

int
nc3_put_varm_text(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, const char *op) {abort();}

int
nc3_get_varm_text(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, char *ip) {abort();}

int
nc3_put_varm_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const unsigned char *op) {abort();}

int
nc3_get_varm_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, unsigned char *ip) {abort();}

int
nc3_put_varm_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const signed char *op) {abort();}

int
nc3_get_varm_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, signed char *ip) {abort();}

int
nc3_put_varm_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const short *op) {abort();}

int
nc3_get_varm_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, short *ip) {abort();}

int
nc3_put_varm_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const ptrdiff_t *stridep,
		const ptrdiff_t *imapp, const int *op) {abort();}

int
nc3_get_varm_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const ptrdiff_t *stridep,
		const ptrdiff_t *imapp, int *ip) {abort();}

int
nc3_put_varm_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, const long *op) {abort();}

int
nc3_get_varm_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, long *ip) {abort();}

int
nc3_put_varm_float(int ncid, int varid,const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const float *op) {abort();}

int
nc3_get_varm_float(int ncid, int varid,const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, float *ip) {abort();}

int
nc3_put_varm_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   const ptrdiff_t *imapp, const double *op) {abort();}

int
nc3_get_varm_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   const ptrdiff_t * imapp, double *ip) {abort();}

int
nc3_put_varm_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, const unsigned char *op) {abort();}

int
nc3_get_varm_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, unsigned char *ip) {abort();}

int
nc3_put_varm_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const unsigned short *op) {abort();}

int
nc3_get_varm_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, unsigned short *ip) {abort();}

int
nc3_put_varm_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const ptrdiff_t * imapp, const unsigned int *op) {abort();}

int
nc3_get_varm_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const ptrdiff_t * imapp, unsigned int *ip) {abort();}

int
nc3_put_varm_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, const long long *op) {abort();}

int
nc3_get_varm_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, long long *ip) {abort();}

int
nc3_put_varm_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const unsigned long long *op) {abort();}

int
nc3_get_varm_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, unsigned long long *ip) {abort();}

int
nc3_put_varm_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const char **op) {abort();}

int
nc3_get_varm_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, char **ip) {abort();}

/* End {put,get}_varm */
/* Begin {put,get}_var */

int
nc3_put_var_text(int ncid, int varid, const char *op) {abort();}

int
nc3_get_var_text(int ncid, int varid, char *ip) {abort();}

int
nc3_put_var_uchar(int ncid, int varid, const unsigned char *op) {abort();}

int
nc3_get_var_uchar(int ncid, int varid, unsigned char *ip) {abort();}

int
nc3_put_var_schar(int ncid, int varid, const signed char *op) {abort();}

int
nc3_get_var_schar(int ncid, int varid, signed char *ip) {abort();}

int
nc3_put_var_short(int ncid, int varid, const short *op) {abort();}

int
nc3_get_var_short(int ncid, int varid, short *ip) {abort();}

int
nc3_put_var_int(int ncid, int varid, const int *op) {abort();}

int
nc3_get_var_int(int ncid, int varid, int *ip) {abort();}

int
nc3_put_var_long(int ncid, int varid, const long *op) {abort();}

int
nc3_get_var_long(int ncid, int varid, long *ip) {abort();}

int
nc3_put_var_float(int ncid, int varid, const float *op) {abort();}

int
nc3_get_var_float(int ncid, int varid, float *ip) {abort();}

int
nc3_put_var_double(int ncid, int varid, const double *op) {abort();}

int
nc3_get_var_double(int ncid, int varid, double *ip) {abort();}

int
nc3_put_var_ubyte(int ncid, int varid, const unsigned char *op) {abort();}

int
nc3_get_var_ubyte(int ncid, int varid, unsigned char *ip) {abort();}

int
nc3_put_var_ushort(int ncid, int varid, const unsigned short *op) {abort();}

int
nc3_get_var_ushort(int ncid, int varid, unsigned short *ip) {abort();}

int
nc3_put_var_uint(int ncid, int varid, const unsigned int *op) {abort();}

int
nc3_get_var_uint(int ncid, int varid, unsigned int *ip) {abort();}

int
nc3_put_var_longlong(int ncid, int varid, const long long *op) {abort();}

int
nc3_get_var_longlong(int ncid, int varid, long long *ip) {abort();}

int
nc3_put_var_ulonglong(int ncid, int varid, const unsigned long long *op) {abort();}

int
nc3_get_var_ulonglong(int ncid, int varid, unsigned long long *ip) {abort();}

int
nc3_put_var_string(int ncid, int varid, const char **op) {abort();}

int
nc3_get_var_string(int ncid, int varid, char **ip) {abort();}

int
nc3__create_mp(const char *path, int cmode, size_t initialsz, int basepe,
	 size_t *chunksizehintp, int *ncidp) {abort();}

int
nc3__open_mp(const char *path, int mode, int basepe,
	size_t *chunksizehintp, int *ncidp) {abort();}

int
nc3_enddef(int ncid) {abort();}
