/*
  Copyright 2010 University Corporation for Atmospheric
  Research/Unidata. See COPYRIGHT file for more info.

  This file defines the variable functions.
*/

#include "ncdispatch.h"
#include "netcdf_f.h"

#if defined(__cplusplus)
/* C++ consts default to internal linkage and must be initialized */
const size_t coord_zero[NC_MAX_VAR_DIMS] = {0};
const size_t coord_one[NC_MAX_VAR_DIMS] = {1};
#else
static const size_t coord_zero[NC_MAX_VAR_DIMS];
/* initialized int put/get_var1 below */
static size_t coord_one[NC_MAX_VAR_DIMS];
#endif

#define INITCOORD1 if(coord_one[0] != 1) {int i; for(i=0;i<NC_MAX_VAR_DIMS;i++) coord_one[i] = 1;}

static nc_type longtype = (sizeof(long) == sizeof(int) ? NC_INT : NC_INT64);

#define MINVARSSPACE 1024;

static int
getshape(int ncid, int varid, int ndims, size_t* shape)
{
   int dimids[NC_MAX_VAR_DIMS];
   int i;
   int status;

   if ((status = nc_inq_vardimid(ncid, varid, dimids)))
      return status;
   for(i = 0; i < ndims; i++) 
      if ((status = nc_inq_dimlen(ncid, dimids[i], &shape[i])))
	 break;

   return status;
}

int
nc_def_var(int ncid, const char *name, nc_type xtype, 
	   int ndims,  const int *dimidsp, int *varidp)
{
   NC* ncp;
   int stat;

   if ((stat = NC_check_id(ncid, &ncp)))
      return stat;
   return ncp->dispatch->def_var(ncid, name, xtype, ndims,
				 dimidsp, varidp);
}

int
nc_inq_varid(int ncid, const char *name, int *varidp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_varid(ncid, name, varidp);
}

int
nc_rename_var(int ncid, int varid, const char *name)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->rename_var(ncid, varid, name);
}

int 
nc_inq_varname(int ncid, int varid, char *name)
{
   return nc_inq_var(ncid, varid, name, NULL, NULL,
		     NULL, NULL);
}

int 
nc_inq_vartype(int ncid, int varid, nc_type *typep)
{
   return nc_inq_var(ncid, varid, NULL, typep, NULL,
		     NULL, NULL);
}

int 
nc_inq_varndims(int ncid, int varid, int *ndimsp)
{
   return nc_inq_var(ncid, varid, NULL, NULL, ndimsp, NULL, NULL);
}

int 
nc_inq_vardimid(int ncid, int varid, int *dimids)
{
   return nc_inq_var(ncid, varid, NULL, NULL, NULL, 
		     dimids, NULL);
}

int 
nc_inq_varnatts(int ncid, int varid, int *nattsp)
{
   if (varid == NC_GLOBAL)
      return nc_inq_natts(ncid,nattsp);
   /*else*/
   return nc_inq_var(ncid, varid, NULL, NULL, NULL, NULL, 
		     nattsp);
}

int
nc_inq_nvars(int ncid, int *nvarsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq(ncid, NULL, nvarsp, NULL, NULL);
}


int
nc_inq_var(int ncid, int varid, char *name, nc_type *xtypep,  
	   int *ndimsp, int *dimidsp, int *nattsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(ncid, varid, name, xtypep, ndimsp, 
				     dimidsp, nattsp, NULL, NULL, NULL, 
				     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

static int
NC_put_vara(int ncid, int varid, const size_t *start, 
	    const size_t *edges, const void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   if(edges == NULL) {
      size_t shape[NC_MAX_VAR_DIMS];
      int ndims;
      stat = nc_inq_varndims(ncid, varid, &ndims); 
      if(stat != NC_NOERR) return stat;
      stat = getshape(ncid,varid,ndims,shape);
      if(stat != NC_NOERR) return stat;
      return ncp->dispatch->put_vara(ncid,varid,start,shape,value,memtype);
   } else
      return ncp->dispatch->put_vara(ncid,varid,start,edges,value,memtype);
}

int
NC_get_vara(int ncid, int varid,
	    const size_t *start, const size_t *edges,
            void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
#ifdef USE_NETCDF4
   if(memtype >= NC_FIRSTUSERTYPEID) memtype = NC_NAT;
#endif
   if(edges == NULL) {
      size_t shape[NC_MAX_VAR_DIMS];
      int ndims;
      stat = nc_inq_varndims(ncid, varid, &ndims); 
      if(stat != NC_NOERR) return stat;
      stat = getshape(ncid,varid,ndims,shape);
      if(stat != NC_NOERR) return stat;
      return ncp->dispatch->get_vara(ncid,varid,start,shape,value,memtype);
   } else
      return ncp->dispatch->get_vara(ncid,varid,start,edges,value,memtype);
}

static int
NC_get_var(int ncid, int varid, void *value, nc_type memtype)
{
   int ndims;
   size_t shape[NC_MAX_VAR_DIMS];
   int stat = nc_inq_varndims(ncid,varid, &ndims);
   if(stat) return stat;
   stat = getshape(ncid,varid, ndims, shape);
   if(stat) return stat;
   return NC_get_vara(ncid, varid, coord_zero, shape, value, memtype);
}

static int
NC_put_var(int ncid, int varid, const void *value, nc_type memtype)
{
   int ndims;
   size_t shape[NC_MAX_VAR_DIMS];
   int stat = nc_inq_varndims(ncid,varid, &ndims);
   if(stat) return stat;
   stat = getshape(ncid,varid, ndims, shape);
   if(stat) return stat;
   return NC_put_vara(ncid, varid, coord_zero, shape, value, memtype);
}

static int
NC_get_var1(int ncid, int varid, const size_t *coord, void* value, 
	    nc_type memtype)
{
   INITCOORD1;
   return NC_get_vara(ncid, varid, coord, coord_one, value, memtype);
}

static int
NC_put_var1(int ncid, int varid, const size_t *coord, const void* value, 
	    nc_type memtype)
{
   INITCOORD1;
   return NC_put_vara(ncid, varid, coord, coord_one, value, memtype);
}

static int
is_recvar(int ncid, int varid, size_t* nrecs)
{
   int status;
   int unlimid;
   int ndims;
   int dimset[NC_MAX_VAR_DIMS];
    
   status = nc_inq_unlimdim(ncid,&unlimid);
   if(status != NC_NOERR) return 0; /* no unlimited defined */
   status = nc_inq_varndims(ncid,varid,&ndims);
   if(status != NC_NOERR) return 0; /* no unlimited defined */
   if(ndims == 0) return 0; /* scalar */
   status = nc_inq_vardimid(ncid,varid,dimset);
   if(status != NC_NOERR) return 0; /* no unlimited defined */
   status = nc_inq_dim(ncid,dimset[0],NULL,nrecs);
   if(status != NC_NOERR) return 0;
   return (dimset[0] == unlimid ? 1: 0);
}

/* Most dispatch tables will use the default procedures */
int
NCDEFAULT_get_vars(int ncid, int varid, const size_t * start,
	    const size_t * edges, const ptrdiff_t * stride,
	    void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->get_varm(ncid,varid,start,edges,stride,NULL,value,memtype);
}

int
NCDEFAULT_put_vars(int ncid, int varid, const size_t * start,
	    const size_t * edges, const ptrdiff_t * stride,
	    const void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_varm(ncid,varid,start,edges,stride,NULL,value,memtype);
}

int
NCDEFAULT_get_varm(int ncid, int varid, const size_t *start,
	    const size_t *edges, const ptrdiff_t *stride,
	    const ptrdiff_t *imapp, void *value0, nc_type memtype)
{
   int status;
   nc_type vartype;
   int varndims,maxidim;
   NC* ncp;
   size_t memtypelen;
   ptrdiff_t cvtmap[NC_MAX_VAR_DIMS];
   char* value = (char*)value0;

   status = NC_check_id (ncid, &ncp);
   if(status != NC_NOERR) return status;

/*
  if(NC_indef(ncp)) return NC_EINDEFINE;
*/

   status = nc_inq_vartype(ncid, varid, &vartype); 
   if(status != NC_NOERR) return status;
   /* Check that this is an atomic type */
   if(vartype >= NC_MAX_ATOMIC_TYPE)
	return NC_EMAPTYPE;

   status = nc_inq_varndims(ncid, varid, &varndims); 
   if(status != NC_NOERR) return status;

   if(memtype == NC_NAT) {
      if(imapp != NULL && varndims != 0) {
	 /*
	  * convert map units from bytes to units of sizeof(type)
	  */
	 size_t ii;
	 const ptrdiff_t szof = (ptrdiff_t) nctypelen(vartype);
	 for(ii = 0; ii < varndims; ii++) {
	    if(imapp[ii] % szof != 0) {
	       /*free(cvtmap);*/
	       return NC_EINVAL;
	    }
	    cvtmap[ii] = imapp[ii] / szof;
	 }
	 imapp = cvtmap;
      }
      memtype = vartype;
   }

   if(memtype == NC_CHAR && vartype != NC_CHAR)
      return NC_ECHAR;
   else if(memtype != NC_CHAR && vartype == NC_CHAR)  
      return NC_ECHAR;

   memtypelen = nctypelen(memtype);

   maxidim = (int) varndims - 1;

   if (maxidim < 0)
   {
      /*
       * The variable is a scalar; consequently,
       * there s only one thing to get and only one place to put it.
       * (Why was I called?)
       */
      size_t edge1[1] = {1};
      return NC_get_vara(ncid, varid, start, edge1, value, memtype);
   }

   /*
    * else
    * The variable is an array.
    */
   {
      int idim;
      size_t *mystart = NULL;
      size_t *myedges;
      size_t *iocount;    /* count vector */
      size_t *stop;   /* stop indexes */
      size_t *length; /* edge lengths in bytes */
      ptrdiff_t *mystride;
      ptrdiff_t *mymap;
      size_t varshape[NC_MAX_VAR_DIMS];
      int isrecvar;
      size_t numrecs;

      /* Compute some dimension related values */
      isrecvar = is_recvar(ncid,varid,&numrecs);
      getshape(ncid,varid,varndims,varshape);	

      /*
       * Verify stride argument; also see if stride is all ones
       */
      if(stride != NULL) {
	 int stride1 = 1;
	 for (idim = 0; idim <= maxidim; ++idim)
	 {
            if (stride[idim] == 0
		/* cast needed for braindead systems with signed size_t */
                || ((unsigned long) stride[idim] >= X_INT_MAX))
            {
	       return NC_ESTRIDE;
            }
	    if(stride[idim] != 1) stride1 = 0;
	 }
         /* If stride1 is true, and there is no imap 
            then call get_vara directly.
         */
         if(stride1 && imapp == NULL) {
	     return NC_get_vara(ncid, varid, start, edges, value, memtype);
	 }
      }

      /* assert(sizeof(ptrdiff_t) >= sizeof(size_t)); */
      /* Allocate space for mystart,mystride,mymap etc.all at once */
      mystart = (size_t *)calloc(varndims * 7, sizeof(ptrdiff_t));
      if(mystart == NULL) return NC_ENOMEM;
      myedges = mystart + varndims;
      iocount = myedges + varndims;
      stop = iocount + varndims;
      length = stop + varndims;
      mystride = (ptrdiff_t *)(length + varndims);
      mymap = mystride + varndims;

      /*
       * Initialize I/O parameters.
       */
      for (idim = maxidim; idim >= 0; --idim)
      {
	 mystart[idim] = start != NULL
	    ? start[idim]
	    : 0;

	 if (edges != NULL && edges[idim] == 0)
	 {
	    status = NC_NOERR;    /* read/write no data */
	    goto done;
	 }

#ifdef COMPLEX
	 myedges[idim] = edges != NULL
	    ? edges[idim]
	    : idim == 0 && isrecvar
	    ? numrecs - mystart[idim]
	    : varshape[idim] - mystart[idim];
#else
	 if(edges != NULL)
	    myedges[idim] = edges[idim];
	 else if (idim == 0 && isrecvar)
	    myedges[idim] = numrecs - mystart[idim];
	 else
	    myedges[idim] = varshape[idim] - mystart[idim];
#endif

	 mystride[idim] = stride != NULL
	    ? stride[idim]
	    : 1;

	 /* Remember: imapp is byte oriented, not index oriented */
#ifdef COMPLEX
	 mymap[idim] = (imapp != NULL
			? imapp[idim]
			: (idim == maxidim ? 1
			   : mymap[idim + 1] * (ptrdiff_t) myedges[idim + 1]));
#else
	 if(imapp != NULL)
	    mymap[idim] = imapp[idim];
	 else if (idim == maxidim)
	    mymap[idim] = 1;
	 else
	    mymap[idim] = 
	       mymap[idim + 1] * (ptrdiff_t) myedges[idim + 1];
#endif
	 iocount[idim] = 1;
	 length[idim] = mymap[idim] * myedges[idim];
	 stop[idim] = mystart[idim] + myedges[idim] * mystride[idim];
      }

      /*
       * Check start, edges
       */
      for (idim = maxidim; idim >= 0; --idim)
      {
	 size_t dimlen = 
	    idim == 0 && isrecvar
	    ? numrecs
	    : varshape[idim];
	 if (mystart[idim] >= dimlen)
	 {
	    status = NC_EINVALCOORDS;
	    goto done;
	 }

	 if (mystart[idim] + myedges[idim] > dimlen)
	 {
	    status = NC_EEDGE;
	    goto done;
	 }

      }


      /* Lower body */
      /*
       * As an optimization, adjust I/O parameters when the fastest 
       * dimension has unity stride both externally and internally.
       * In this case, the user could have called a simpler routine
       * (i.e. ncvar$1()
       */
      if (mystride[maxidim] == 1
	  && mymap[maxidim] == 1)
      {
	 iocount[maxidim] = myedges[maxidim];
	 mystride[maxidim] = (ptrdiff_t) myedges[maxidim];
	 mymap[maxidim] = (ptrdiff_t) length[maxidim];
      }

      /* 
       * Perform I/O.  Exit when done.
       */
      for (;;)
      {
	 /* TODO: */
	 int lstatus = NC_get_vara(ncid, varid, mystart, iocount,
				   value, memtype);
	 if (lstatus != NC_NOERR) {
	    if(status == NC_NOERR || lstatus != NC_ERANGE)
	       status = lstatus;
	 }
	 /*
	  * The following code permutes through the variable s
	  * external start-index space and it s internal address
	  * space.  At the UPC, this algorithm is commonly
	  * called "odometer code".
	  */
	 idim = maxidim;
        carry:
	 value += (mymap[idim] * memtypelen);
	 mystart[idim] += mystride[idim];
	 if (mystart[idim] == stop[idim])
	 {
	    mystart[idim] = start[idim];
	    value -= (length[idim] * memtypelen);
	    if (--idim < 0)
	       break; /* normal return */
	    goto carry;
	 }
      } /* I/O loop */
     done:
      free(mystart);
   } /* variable is array */
   return status;
}


int
NCDEFAULT_put_varm(
   int ncid,
   int varid,
   const size_t * start,
   const size_t * edges,
   const ptrdiff_t * stride,
   const ptrdiff_t * imapp,
   const void *value0,
   nc_type memtype)
{
   int status;
   nc_type vartype;
   int varndims,maxidim;
   NC* ncp;
   size_t memtypelen;
   ptrdiff_t cvtmap[NC_MAX_VAR_DIMS];
   const char* value = (char*)value0;

   status = NC_check_id (ncid, &ncp);
   if(status != NC_NOERR) return status;

/*
  if(NC_indef(ncp)) return NC_EINDEFINE;
  if(NC_readonly (ncp)) return NC_EPERM;
*/

   /* mid body */
   status = nc_inq_vartype(ncid, varid, &vartype); 
   if(status != NC_NOERR) return status;
   /* Check that this is an atomic type */
   if(vartype >= NC_MAX_ATOMIC_TYPE)
	return NC_EMAPTYPE;

   status = nc_inq_varndims(ncid, varid, &varndims); 
   if(status != NC_NOERR) return status;

   if(memtype == NC_NAT) {
      if(imapp != NULL && varndims != 0) {
	 /*
	  * convert map units from bytes to units of sizeof(type)
	  */
	 size_t ii;
	 const ptrdiff_t szof = (ptrdiff_t) nctypelen(vartype);
	 for(ii = 0; ii < varndims; ii++) {
	    if(imapp[ii] % szof != 0) {
	       /*free(cvtmap);*/
	       return NC_EINVAL;
	    }
	    cvtmap[ii] = imapp[ii] / szof;
	 }
	 imapp = cvtmap;
      }
      memtype = vartype;
   }

   if(memtype == NC_CHAR && vartype != NC_CHAR)
      return NC_ECHAR;
   else if(memtype != NC_CHAR && vartype == NC_CHAR)  
      return NC_ECHAR;

   memtypelen = nctypelen(memtype);

   maxidim = (int) varndims - 1;

   if (maxidim < 0)
   {
      /*
       * The variable is a scalar; consequently,
       * there s only one thing to get and only one place to put it.
       * (Why was I called?)
       */
      size_t edge1[1] = {1};
      return NC_put_vara(ncid, varid, start, edge1, value, memtype);
   }

   /*
    * else
    * The variable is an array.
    */
   {
      int idim;
      size_t *mystart = NULL;
      size_t *myedges;
      size_t *iocount;    /* count vector */
      size_t *stop;   /* stop indexes */
      size_t *length; /* edge lengths in bytes */
      ptrdiff_t *mystride;
      ptrdiff_t *mymap;
      size_t varshape[NC_MAX_VAR_DIMS];
      int isrecvar;
      size_t numrecs;
      int stride1; /* is stride all ones? */

      /*
       * Verify stride argument.
       */
      if(stride != NULL)
	 stride1 = 1;
	 for (idim = 0; idim <= maxidim; ++idim)
	 {
            if ((stride[idim] == 0)
		/* cast needed for braindead systems with signed size_t */
                || ((unsigned long) stride[idim] >= X_INT_MAX))
            {
	       return NC_ESTRIDE;
            }
	    if(stride[idim] != 1) stride1 = 0;
	 }

      /* If stride1 is true, and there is no imap, then call get_vara
         directly
      */
      if(stride1 && imapp == NULL) {
	 return NC_put_vara(ncid, varid, start, edges, value, memtype);
      }

      /* Compute some dimension related values */
      isrecvar = is_recvar(ncid,varid,&numrecs);
      getshape(ncid,varid,varndims,varshape);	

      /* assert(sizeof(ptrdiff_t) >= sizeof(size_t)); */
      mystart = (size_t *)calloc(varndims * 7, sizeof(ptrdiff_t));
      if(mystart == NULL) return NC_ENOMEM;
      myedges = mystart + varndims;
      iocount = myedges + varndims;
      stop = iocount + varndims;
      length = stop + varndims;
      mystride = (ptrdiff_t *)(length + varndims);
      mymap = mystride + varndims;

      /*
       * Initialize I/O parameters.
       */
      for (idim = maxidim; idim >= 0; --idim)
      {
	 mystart[idim] = start != NULL
	    ? start[idim]
	    : 0;

	 if (edges != NULL && edges[idim] == 0)
	 {
	    status = NC_NOERR;    /* read/write no data */
	    goto done;
	 }

	 myedges[idim] = edges != NULL
	    ? edges[idim]
	    : idim == 0 && isrecvar
	    ? numrecs - mystart[idim]
	    : varshape[idim] - mystart[idim];
	 mystride[idim] = stride != NULL
	    ? stride[idim]
	    : 1;
	 mymap[idim] = imapp != NULL
	    ? imapp[idim]
	    : idim == maxidim
	    ? 1
	    : mymap[idim + 1] * (ptrdiff_t) myedges[idim + 1];

	 iocount[idim] = 1;
	 length[idim] = mymap[idim] * myedges[idim];
	 stop[idim] = mystart[idim] + myedges[idim] * mystride[idim];
      }

      /*
       * Check start, edges
       */
      for (idim = isrecvar; idim < maxidim; ++idim)
      {
	 if (mystart[idim] > varshape[idim])
	 {
	    status = NC_EINVALCOORDS;
	    goto done;
	 }
	 if (mystart[idim] + myedges[idim] > varshape[idim])
	 {
	    status = NC_EEDGE;
	    goto done;
	 }
      }

      /* Lower body */
      /*
       * As an optimization, adjust I/O parameters when the fastest 
       * dimension has unity stride both externally and internally.
       * In this case, the user could have called a simpler routine
       * (i.e. ncvar$1()
       */
      if (mystride[maxidim] == 1
	  && mymap[maxidim] == 1)
      {
	 iocount[maxidim] = myedges[maxidim];
	 mystride[maxidim] = (ptrdiff_t) myedges[maxidim];
	 mymap[maxidim] = (ptrdiff_t) length[maxidim];
      }

      /*
       * Perform I/O.  Exit when done.
       */
      for (;;)
      {
	 /* TODO: */
	 int lstatus = NC_put_vara(ncid, varid, mystart, iocount,
				   value, memtype);
	 if (lstatus != NC_NOERR) {
	    if(status == NC_NOERR || lstatus != NC_ERANGE)
	       status = lstatus;
	 }	    

	 /*
	  * The following code permutes through the variable s
	  * external start-index space and it s internal address
	  * space.  At the UPC, this algorithm is commonly
	  * called "odometer code".
	  */
	 idim = maxidim;
        carry:
	 value += (mymap[idim] * memtypelen);
	 mystart[idim] += mystride[idim];
	 if (mystart[idim] == stop[idim])
	 {
	    mystart[idim] = start[idim];
	    value -= (length[idim] * memtypelen);
	    if (--idim < 0)
	       break; /* normal return */
	    goto carry;
	 }
      } /* I/O loop */
     done:
      free(mystart);
   } /* variable is array */
   return status;
}

/* Called by externally visible nc_get_vars_xxx routines */
static int
NC_get_vars(int ncid, int varid, const size_t *start, 
	    const size_t *edges, const ptrdiff_t *stride, void *value,
	    nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
#ifdef USE_NETCDF4
   if(memtype >= NC_FIRSTUSERTYPEID) memtype = NC_NAT;
#endif
   return ncp->dispatch->get_vars(ncid,varid,start,edges,stride,value,memtype);
}

static int
NC_put_vars(int ncid, int varid, const size_t *start,
	    const size_t *edges, const ptrdiff_t *stride,
	    const void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
#ifdef USE_NETCDF4
   if(memtype >= NC_FIRSTUSERTYPEID) memtype = NC_NAT;
#endif
   return ncp->dispatch->put_vars(ncid,varid,start,edges,stride,value,memtype);
}

/* Called by externally visible nc_get_vars_xxx routines */
static int
NC_get_varm(int ncid, int varid, const size_t *start, 
	    const size_t *edges, const ptrdiff_t *stride, const ptrdiff_t* map,
	    void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
#ifdef USE_NETCDF4
   if(memtype >= NC_FIRSTUSERTYPEID) memtype = NC_NAT;
#endif
   return ncp->dispatch->get_varm(ncid,varid,start,edges,stride,map,value,memtype);
}

static int
NC_put_varm(int ncid, int varid, const size_t *start, 
	    const size_t *edges, const ptrdiff_t *stride, const ptrdiff_t* map,
	    const void *value, nc_type memtype)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);

   if(stat != NC_NOERR) return stat;
#ifdef USE_NETCDF4
   if(memtype >= NC_FIRSTUSERTYPEID) memtype = NC_NAT;
#endif
   return ncp->dispatch->put_varm(ncid,varid,start,edges,stride,map,value,memtype);
}

/* Ok to use NC pointers because
   all IOSP's will use that structure,
   but not ok to use e.g. NC_Var pointers
   because they may be different structure
   entirely.
*/

/*
 *  This is how much space is required by the user, as in
 *
 *   vals = malloc(nel * nctypelen(var.type));
 *   ncvarget(cdfid, varid, cor, edg, vals);
 */
int
nctypelen(nc_type type) 
{
   switch(type){
      case NC_CHAR :
	 return((int)sizeof(char));
      case NC_BYTE :
	 return((int)sizeof(signed char));
      case NC_SHORT :
	 return(int)(sizeof(short));
      case NC_INT :
	 return((int)sizeof(int));
      case NC_FLOAT :
	 return((int)sizeof(float));
      case NC_DOUBLE : 
	 return((int)sizeof(double));

	 /* These can occur in netcdf-3 code */ 
      case NC_UBYTE :
	 return((int)sizeof(unsigned char));
      case NC_USHORT :
	 return((int)(sizeof(unsigned short)));
      case NC_UINT :
	 return((int)sizeof(unsigned int));
      case NC_INT64 :
	 return((int)sizeof(signed long long));
      case NC_UINT64 :
	 return((int)sizeof(unsigned long long));
#ifdef USE_NETCDF4
      case NC_STRING :
	 return((int)sizeof(char*));
#endif /*USE_NETCDF4*/

      default:
	 return -1;
   }
}

/* utility functions */
/* Redunant over nctypelen above */
int
NC_atomictypelen(nc_type xtype)
{
   int sz = 0;
   switch(xtype) {
      case NC_NAT: sz = 0; break;
      case NC_BYTE: sz = sizeof(signed char); break;
      case NC_CHAR: sz = sizeof(char); break;
      case NC_SHORT: sz = sizeof(short); break;
      case NC_INT: sz = sizeof(int); break;
      case NC_FLOAT: sz = sizeof(float); break;
      case NC_DOUBLE: sz = sizeof(double); break;
      case NC_INT64: sz = sizeof(signed long long); break;
      case NC_UBYTE: sz = sizeof(unsigned char); break;
      case NC_USHORT: sz = sizeof(unsigned short); break;
      case NC_UINT: sz = sizeof(unsigned int); break;
      case NC_UINT64: sz = sizeof(unsigned long long); break;
#ifdef USE_NETCDF4
      case NC_STRING: sz = sizeof(char*); break;
#endif
      default: break;
   }	
   return sz;
}

char*
NC_atomictypename(nc_type xtype)
{
   char* nm = NULL;
   switch(xtype) {
      case NC_NAT: nm = "undefined"; break;
      case NC_BYTE: nm = "byte"; break;
      case NC_CHAR: nm = "char"; break;
      case NC_SHORT: nm = "short"; break;
      case NC_INT: nm = "int"; break;
      case NC_FLOAT: nm = "float"; break;
      case NC_DOUBLE: nm = "double"; break;
      case NC_INT64: nm = "int64"; break;
      case NC_UBYTE: nm = "ubyte"; break;
      case NC_USHORT: nm = "ushort"; break;
      case NC_UINT: nm = "uint"; break;
      case NC_UINT64: nm = "uint64"; break;
#ifdef USE_NETCDF4
      case NC_STRING: nm = "string"; break;
#endif
      default: break;
   }	
   return nm;
}

int
nc_put_vara(int ncid, int varid,
	    const size_t *start, const size_t *edges,
            const void *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   nc_type xtype;
   if(stat != NC_NOERR) return stat;
   stat = nc_inq_vartype(ncid, varid, &xtype);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,value,xtype);
}

int
nc_get_vara(int ncid, int varid,
	    const size_t *start, const size_t *edges,
            void *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   nc_type xtype;
   if(stat != NC_NOERR) return stat;
   stat = nc_inq_vartype(ncid, varid, &xtype);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,value,xtype);
}

int
nc_get_var(int ncid, int varid, void *value)
{
   return NC_get_var(ncid, varid, value, NC_NAT);
}

int
nc_put_var(int ncid, int varid, const void *value)
{
   return NC_put_var(ncid, varid, value, NC_NAT);
}

int
nc_get_var1(int ncid, int varid, const size_t *coord, void *value)
{
   return NC_get_var1(ncid, varid, coord, value, NC_NAT);
}

int
nc_put_var1(int ncid, int varid, const size_t *coord, const void *value)
{
   return NC_put_var1(ncid, varid, coord, value, NC_NAT);
}

int
nc_get_varm(int ncid, int varid, const size_t * start,
	    const size_t * edges, const ptrdiff_t * stride,
	    const ptrdiff_t * imapp, void *value)
{
   NC* ncp;
   int stat;

   if ((stat = NC_check_id(ncid, &ncp)))
       return stat;
   return ncp->dispatch->get_varm(ncid, varid, start, edges, stride, imapp,
		      value, NC_NAT);
}

int
nc_put_varm (int ncid, int varid, const size_t * start,
	     const size_t * edges, const ptrdiff_t * stride,
	     const ptrdiff_t * imapp, const void *value)
{
   NC* ncp;
   int stat;

   if ((stat = NC_check_id(ncid, &ncp)))
       return stat;
   return ncp->dispatch->put_varm(ncid, varid, start, edges, stride, imapp,
		      value, NC_NAT);
}

int
nc_get_vars (int ncid, int varid, const size_t * start,
	     const size_t * edges, const ptrdiff_t * stride,
	     void *value)
{
   NC* ncp;
   int stat;

   if ((stat = NC_check_id(ncid, &ncp)))
       return stat;
   return ncp->dispatch->get_vars(ncid, varid, start, edges, stride,
		      value, NC_NAT);
}

int
nc_put_vars (int ncid, int varid, const size_t * start,
	     const size_t * edges, const ptrdiff_t * stride,
	     const void *value)
{
   NC* ncp;
   int stat;

   if ((stat = NC_check_id(ncid, &ncp)))
       return stat;
   return ncp->dispatch->put_vars(ncid, varid, start, edges, stride,
		      value, NC_NAT);
}

int
nc_get_var1_text(int ncid, int varid, const size_t *coord, char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid, varid, coord, (void*)value, NC_CHAR);
}

int
nc_get_var1_schar(int ncid, int varid, const size_t *coord, signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid, varid, coord, (void*)value, NC_BYTE);
}

int
nc_get_var1_uchar(int ncid, int varid, const size_t *coord, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid, varid, coord, (void*)value, NC_UBYTE);
}

int
nc_get_var1_short(int ncid, int varid, const size_t *coord, short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid, varid, coord, (void*)value, NC_SHORT);
}

int
nc_get_var1_int(int ncid, int varid, const size_t *coord, int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid, varid, coord, (void*)value, NC_INT);
}

int
nc_get_var1_long(int ncid, int varid, const size_t *coord, long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, longtype);
}

int
nc_get_var1_float(int ncid, int varid, const size_t *coord, float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_FLOAT);
}

int
nc_get_var1_double(int ncid, int varid, const size_t *coord, double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_DOUBLE);
}

int
nc_get_var1_ubyte(int ncid, int varid, const size_t *coord, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_UBYTE);
}

int
nc_get_var1_ushort(int ncid, int varid, const size_t *coord, 
		   unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_USHORT);
}

int
nc_get_var1_uint(int ncid, int varid, const size_t *coord, unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_INT);
}

int
nc_get_var1_longlong(int ncid, int varid, const size_t *coord, long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_INT64);
}

int
nc_get_var1_ulonglong(int ncid, int varid, const size_t *coord, unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_get_var1_string(int ncid, int varid, const size_t *coord, char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_get_var1(ncid,varid,coord,(void*)value, NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_put_var1_text(int ncid, int varid, const size_t *coord, const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_CHAR);
}

int
nc_put_var1_schar(int ncid, int varid, const size_t *coord, const signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_BYTE);
}

int
nc_put_var1_uchar(int ncid, int varid, const size_t *coord, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_UBYTE);
}

int
nc_put_var1_short(int ncid, int varid, const size_t *coord, const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_SHORT);
}

int
nc_put_var1_int(int ncid, int varid, const size_t *coord, const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_INT);
}

int
nc_put_var1_long(int ncid, int varid, const size_t *coord, const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, longtype);
}

int
nc_put_var1_float(int ncid, int varid, const size_t *coord, const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_FLOAT);
}

int
nc_put_var1_double(int ncid, int varid, const size_t *coord, const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_DOUBLE);
}

int
nc_put_var1_ubyte(int ncid, int varid, const size_t *coord, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_UBYTE);
}

int
nc_put_var1_ushort(int ncid, int varid, const size_t *coord, const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_USHORT);
}

int
nc_put_var1_uint(int ncid, int varid, const size_t *coord, const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_UINT);
}

int
nc_put_var1_longlong(int ncid, int varid, const size_t *coord, const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_INT64);
}

int
nc_put_var1_ulonglong(int ncid, int varid, const size_t *coord, const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid,varid,coord,(void*)value, NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_put_var1_string(int ncid, int varid, const size_t *coord, const char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   INITCOORD1;
   return NC_put_var1(ncid, varid, coord, (void*)value, NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_get_var_text(int ncid, int varid, char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid, varid, (void*)value, NC_CHAR);
}

int
nc_get_var_schar(int ncid, int varid, signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_BYTE);
}

int
nc_get_var_uchar(int ncid, int varid, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_UBYTE);
}

int
nc_get_var_short(int ncid, int varid, short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_SHORT);
}

int
nc_get_var_int(int ncid, int varid, int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_INT);
}

int
nc_get_var_long(int ncid, int varid, long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, longtype);
}

int
nc_get_var_float(int ncid, int varid, float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_FLOAT);
}

int
nc_get_var_double(int ncid, int varid, double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_DOUBLE);
}

int
nc_get_var_ubyte(int ncid, int varid, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_UBYTE);
}

int
nc_get_var_ushort(int ncid, int varid, unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_USHORT);
}

int
nc_get_var_uint(int ncid, int varid, unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_UINT);
}

int
nc_get_var_longlong(int ncid, int varid, long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value, NC_INT64);
}

int
nc_get_var_ulonglong(int ncid, int varid, unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_get_var_string(int ncid, int varid, char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_var(ncid,varid,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_put_var_text(int ncid, int varid, const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_CHAR);
}

int
nc_put_var_schar(int ncid, int varid, const signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_BYTE);
}

int
nc_put_var_uchar(int ncid, int varid, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_uchar);
}

int
nc_put_var_short(int ncid, int varid, const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_SHORT);
}

int
nc_put_var_int(int ncid, int varid, const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_INT);
}

int
nc_put_var_long(int ncid, int varid, const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_long);
}

int
nc_put_var_float(int ncid, int varid, const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_float);
}

int
nc_put_var_double(int ncid, int varid, const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_double);
}

int
nc_put_var_ubyte(int ncid, int varid, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_ubyte);
}

int
nc_put_var_ushort(int ncid, int varid, const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_ushort);
}

int
nc_put_var_uint(int ncid, int varid, const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_uint);
}

int
nc_put_var_longlong(int ncid, int varid, const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,T_longlong);
}

int
nc_put_var_ulonglong(int ncid, int varid, const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_put_var_string(int ncid, int varid, const char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_var(ncid,varid,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/


int
nc_put_vara_text(int ncid, int varid, const size_t *start, 
		 const size_t *edges, const char *value)
{
   return NC_put_vara(ncid, varid, start, edges, 
		      (void*)value, NC_CHAR);
}

int
nc_put_vara_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges, const signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,NC_BYTE);
}

int
nc_put_vara_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_uchar);
}

int
nc_put_vara_short(int ncid, int varid,
		  const size_t *start, const size_t *edges, const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,NC_SHORT);
}

int
nc_put_vara_int(int ncid, int varid,
		const size_t *start, const size_t *edges, const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,NC_INT);
}

int
nc_put_vara_long(int ncid, int varid,
		 const size_t *start, const size_t *edges, const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_long);
}

int
nc_put_vara_float(int ncid, int varid,
		  const size_t *start, const size_t *edges, const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_float);
}

int
nc_put_vara_double(int ncid, int varid,
		   const size_t *start, const size_t *edges, const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_double);
}

int
nc_put_vara_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_ubyte);
}

int
nc_put_vara_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges, const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_ushort);
}

int
nc_put_vara_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges, const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_uint);
}

int
nc_put_vara_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges, const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,T_longlong);
}

int
nc_put_vara_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges, const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_put_vara_string(int ncid, int varid,
		   const size_t *start, const size_t *edges, const char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vara(ncid,varid,start,edges,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_get_vara_text(int ncid, int varid,
		 const size_t *start, const size_t *edges, char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_CHAR);
}

int
nc_get_vara_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges, signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_BYTE);
}

int
nc_get_vara_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_uchar);
}

int
nc_get_vara_short(int ncid, int varid,
		  const size_t *start, const size_t *edges, short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_SHORT);
}

int
nc_get_vara_int(int ncid, int varid,
		const size_t *start, const size_t *edges, int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_INT);
}

int
nc_get_vara_long(int ncid, int varid,
		 const size_t *start, const size_t *edges, long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_long);
}

int
nc_get_vara_float(int ncid, int varid,
		  const size_t *start, const size_t *edges, float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_float);
}

int
nc_get_vara_double(int ncid, int varid,
		   const size_t *start, const size_t *edges, double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_double);
}

int
nc_get_vara_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_ubyte);
}

int
nc_get_vara_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges, unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_ushort);
}

int
nc_get_vara_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges, unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_uint);
}

int
nc_get_vara_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges, long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,T_longlong);
}

int
nc_get_vara_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges, unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_get_vara_string(int ncid, int varid,
		   const size_t *start, const size_t *edges, char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vara(ncid,varid,start,edges,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_put_varm_text(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_CHAR);
}

int
nc_put_varm_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  const signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_BYTE);
}

int
nc_put_varm_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_uchar);
}

int
nc_put_varm_short(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_SHORT);
}

int
nc_put_varm_int(int ncid, int varid,
		const size_t *start, const size_t *edges,
		const ptrdiff_t * stride, const ptrdiff_t * imapp,
		const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_INT);
}

int
nc_put_varm_long(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_long);
}

int
nc_put_varm_float(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_float);
}

int
nc_put_varm_double(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_double);
}

int
nc_put_varm_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_ubyte);
}

int
nc_put_varm_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_ushort);
}

int
nc_put_varm_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_uint);
}

int
nc_put_varm_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges,
		     const ptrdiff_t * stride, const ptrdiff_t * imapp,
		     const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_longlong);
}

int
nc_put_varm_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges,
		      const ptrdiff_t * stride, const ptrdiff_t * imapp,
		      const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_put_varm_string(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   const char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_get_varm_text(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_CHAR);
}

int
nc_get_varm_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_BYTE);
}

int
nc_get_varm_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_uchar);
}

int
nc_get_varm_short(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_SHORT);
}

int
nc_get_varm_int(int ncid, int varid,
		const size_t *start, const size_t *edges,
		const ptrdiff_t * stride, const ptrdiff_t * imapp,
		int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_INT);
}

int
nc_get_varm_long(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_long);
}

int
nc_get_varm_float(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_float);
}

int
nc_get_varm_double(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_double);
}

int
nc_get_varm_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride, const ptrdiff_t * imapp,
		  unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_ubyte);
}

int
nc_get_varm_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_ushort);
}

int
nc_get_varm_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride, const ptrdiff_t * imapp,
		 unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_uint);
}

int
nc_get_varm_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges,
		     const ptrdiff_t * stride, const ptrdiff_t * imapp,
		     long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,T_longlong);
}

int
nc_get_varm_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges,
		      const ptrdiff_t * stride, const ptrdiff_t * imapp,
		      unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_get_varm_string(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride, const ptrdiff_t * imapp,
		   char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_varm(ncid,varid,start,edges,stride,imapp,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_put_vars_text(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_CHAR);
}

int
nc_put_vars_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  const signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_BYTE);
}

int
nc_put_vars_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_uchar);
}

int
nc_put_vars_short(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_SHORT);
}

int
nc_put_vars_int(int ncid, int varid,
		const size_t *start, const size_t *edges,
		const ptrdiff_t * stride,
		const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_INT);
}

int
nc_put_vars_long(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_long);
}

int
nc_put_vars_float(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_float);
}

int
nc_put_vars_double(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_double);
}

int
nc_put_vars_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_ubyte);
}

int
nc_put_vars_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_ushort);
}

int
nc_put_vars_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_uint);
}

int
nc_put_vars_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges,
		     const ptrdiff_t * stride,
		     const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,T_longlong);
}

int
nc_put_vars_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges,
		      const ptrdiff_t * stride,
		      const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_put_vars_string(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   const char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_put_vars(ncid,varid,start,edges,stride,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/

int
nc_get_vars_text(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_CHAR);
}

int
nc_get_vars_schar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_BYTE);
}

int
nc_get_vars_uchar(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_uchar);
}

int
nc_get_vars_short(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_SHORT);
}

int
nc_get_vars_int(int ncid, int varid,
		const size_t *start, const size_t *edges,
		const ptrdiff_t * stride,
		int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_INT);
}

int
nc_get_vars_long(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_long);
}

int
nc_get_vars_float(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_float);
}

int
nc_get_vars_double(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_double);
}

int
nc_get_vars_ubyte(int ncid, int varid,
		  const size_t *start, const size_t *edges,
		  const ptrdiff_t * stride,
		  unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_ubyte);
}

int
nc_get_vars_ushort(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_ushort);
}

int
nc_get_vars_uint(int ncid, int varid,
		 const size_t *start, const size_t *edges,
		 const ptrdiff_t * stride,
		 unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_uint);
}

int
nc_get_vars_longlong(int ncid, int varid,
		     const size_t *start, const size_t *edges,
		     const ptrdiff_t * stride,
		     long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,T_longlong);
}

int
nc_get_vars_ulonglong(int ncid, int varid,
		      const size_t *start, const size_t *edges,
		      const ptrdiff_t * stride,
		      unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_UINT64);
}

#ifdef USE_NETCDF4
int
nc_get_vars_string(int ncid, int varid,
		   const size_t *start, const size_t *edges,
		   const ptrdiff_t * stride,
		   char* *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return NC_get_vars(ncid,varid,start,edges,stride,(void*)value,NC_STRING);
}

#endif /*USE_NETCDF4*/


