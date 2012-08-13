/*
  Copyright 2010 University Corporation for Atmospheric
  Research/Unidata. See COPYRIGHT file for more info.

  This file defines the attribute functions.

  "$Id: nc4.c,v 1.1 2010/06/01 15:46:50 ed Exp $" 
*/

#include "ncdispatch.h"

static nc_type longtype = (sizeof(long) == sizeof(int) ? NC_INT : NC_INT64);

int
nc_inq_att(int ncid, int varid, const char *name, nc_type *xtypep, 
	   size_t *lenp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, xtypep, lenp);
}

int
nc_inq_attid(int ncid, int varid, const char *name, int *idp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_attid(ncid, varid, name, idp);
}

int
nc_inq_attname(int ncid, int varid, int attnum, char *name)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_attname(ncid, varid, attnum, name);
}

int
nc_rename_att(int ncid, int varid, const char *name, const char *newname)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->rename_att(ncid, varid, name, newname);
}

int
nc_del_att(int ncid, int varid, const char *name)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->del_att(ncid, varid, name);
}

int
nc_inq_natts(int ncid, int *nattsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   if(nattsp == NULL) return NC_NOERR;
   return ncp->dispatch->inq(ncid, NULL, NULL, nattsp, NULL);
}

int
nc_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, xtypep, NULL);
}

int
nc_inq_attlen(int ncid, int varid, const char *name, size_t *lenp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, NULL, lenp);
}

/* This function is no longer deprecated; it's used to support the 2.x
 * interface and also the netcdf-4 api. */
int
nc_get_att(int ncid, int varid, const char *name, void *value)
{
   NC* ncp;
   int stat;
   nc_type xtype;

   if ((stat = NC_check_id(ncid, &ncp)))
      return stat;

   /* Need to get the type */
   if ((stat = nc_inq_atttype(ncid, varid, name, &xtype)))
      return stat;

   return ncp->dispatch->get_att(ncid, varid, name, value, xtype);
}

int
nc_put_att(int ncid, int varid, const char *name, nc_type type,
	   size_t nelems, const void *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems, 
				 value, type);
}

int
nc_get_att_text(int ncid, int varid, const char *name, char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_CHAR);
}

int
nc_get_att_schar(int ncid, int varid, const char *name, signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_BYTE);
}

int
nc_get_att_uchar(int ncid, int varid, const char *name, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UBYTE);
}

int
nc_get_att_short(int ncid, int varid, const char *name, short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_SHORT);
}

int
nc_get_att_int(int ncid, int varid, const char *name, int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_INT);
}

int
nc_get_att_long(int ncid, int varid, const char *name, long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, longtype);
}

int
nc_get_att_float(int ncid, int varid, const char *name, float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_FLOAT);
}

int
nc_get_att_double(int ncid, int varid, const char *name, double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_DOUBLE);
}

int
nc_get_att_ubyte(int ncid, int varid, const char *name, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UBYTE);
}

int
nc_get_att_ushort(int ncid, int varid, const char *name, unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_USHORT);
}

int
nc_get_att_uint(int ncid, int varid, const char *name, unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UINT);
}

int
nc_get_att_longlong(int ncid, int varid, const char *name, long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_INT64);
}

int
nc_get_att_ulonglong(int ncid, int varid, const char *name, unsigned long long *value)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UINT64);
}

int
nc_get_att_string(int ncid, int varid, const char *name, char **value)
{
    NC *ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->get_att(ncid,varid,name,(void*)value, NC_STRING);
}

int
nc_put_att_schar(int ncid, int varid, const char *name,
		 nc_type type, size_t nelems, const signed char *value)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems, 
				 (void *)value, NC_BYTE);
}

int
nc_put_att_uchar(int ncid, int varid, const char *name,
		 nc_type type, size_t nelems, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid,varid,name,type,nelems,(void *)value, NC_UBYTE);
}

int
nc_put_att_short(int ncid, int varid, const char *name,
		 nc_type type, size_t nelems, const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems, 
				 (void *)value, NC_SHORT);
}

int
nc_put_att_int(int ncid, int varid, const char *name,
	       nc_type type, size_t nelems, const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_INT);
}

int
nc_put_att_long(int ncid, int varid, const char *name,
		nc_type type, size_t nelems, const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, longtype);
}

int
nc_put_att_float(int ncid, int varid, const char *name,
		 nc_type type, size_t nelems, const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_FLOAT);
}

int
nc_put_att_double(int ncid, int varid, const char *name,
		  nc_type type, size_t nelems, const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_DOUBLE);
}

int
nc_put_att_ubyte(int ncid, int varid, const char *name,
		 nc_type type, size_t nelems, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_UBYTE);
}

int
nc_put_att_ushort(int ncid, int varid, const char *name,
		  nc_type type, size_t nelems, const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_USHORT);
}

int
nc_put_att_uint(int ncid, int varid, const char *name,
		nc_type type, size_t nelems, const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_UINT);
}

int
nc_put_att_longlong(int ncid, int varid, const char *name,
		    nc_type type, size_t nelems, 
		    const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_INT64);
}

int
nc_put_att_ulonglong(int ncid, int varid, const char *name,
		     nc_type type, size_t nelems, 
		     const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, type, nelems,
				 (void *)value, NC_UINT64);
}

int
nc_put_att_string(int ncid, int varid, const char *name,
		  size_t len, const char** value)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->put_att(ncid,varid,name,NC_STRING,len,(void*)value,NC_STRING);
}

int
nc_put_att_text(int ncid, int varid, const char *name,
		size_t len, const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, NC_CHAR, len, 
				 (void *)value, NC_CHAR);
}

