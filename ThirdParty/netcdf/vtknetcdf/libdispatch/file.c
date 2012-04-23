/*
  Copyright 2010 University Corporation for Atmospheric
  Research/Unidata. See COPYRIGHT file for more info.

  This file defines the file create and open functions.
*/

#include "ncdispatch.h"

static int nc_initialized = 0;

static int
NC_check_file_type(const char *path, int use_parallel, void *mpi_info,
                   int *cdf, int *hdf)
{
   char magic[MAGIC_NUMBER_LEN];

   *hdf = 0; *cdf = 0;

   /* Get the 4-byte magic from the beginning of the file. Don't use posix
    * for parallel, use the MPI functions instead. */
#ifdef USE_PARALLEL_MPIO
   if (use_parallel)
   {
      MPI_File fh;
      MPI_Status status;
      int retval;
      MPI_Comm comm = 0;
      MPI_Info info = 0;

      if(mpi_info != NULL) {
      comm = ((NC_MPI_INFO*)mpi_info)->comm;
      info = ((NC_MPI_INFO*)mpi_info)->info;
      }
      if((retval = MPI_File_open(comm, (char *)path, MPI_MODE_RDONLY,info,
                                 &fh)) != MPI_SUCCESS)
        return NC_EPARINIT;
      if((retval = MPI_File_read(fh, magic, MAGIC_NUMBER_LEN, MPI_CHAR,
                                 &status)) != MPI_SUCCESS)
        return NC_EPARINIT;
      if((retval = MPI_File_close(&fh)) != MPI_SUCCESS)
        return NC_EPARINIT;
   } else
#endif /* USE_PARALLEL */
   {
      FILE *fp;
      int i;

      if (!path)
        return 0;
      if (!(fp = fopen(path, "r")))
        return errno;
      i = fread(magic, MAGIC_NUMBER_LEN, 1, fp);
      fclose(fp);
      if(i != 1)
        return errno;
   }

   /* Ignore the first byte for HDF */
   if(magic[1] == 'H' && magic[2] == 'D' && magic[3] == 'F')
      *hdf = 5;
   else if(magic[0] == '\016' && magic[1] == '\003'
           && magic[2] == '\023' && magic[3] == '\001')
      *hdf = 4;
   else if(magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F')
   {
      if(magic[3] == '\001')
        *cdf = 1;
      else if(magic[3] == '\002')
        *cdf = 2;
   }

   return NC_NOERR;
}

int
nc_create(const char *path, int cmode, int *ncidp)
{
   return NC_create(path, cmode, 0, 0, NULL, 0, NULL, ncidp);
}

int
nc__create(const char *path, int cmode, size_t initialsz,
           size_t *chunksizehintp, int *ncidp)
{
   return NC_create(path, cmode, initialsz, 0,
                    chunksizehintp, 0, NULL, ncidp);

}

int
nc__create_mp(const char *path, int cmode, size_t initialsz,
              int basepe, size_t *chunksizehintp, int *ncidp)
{
   return NC_create(path, cmode, initialsz, basepe,
                    chunksizehintp, 0, NULL, ncidp);
}

/*
  For create, we have the following pieces of information
  to use to determine the dispatch table.
  1. table specified by override
  2. path
  3. cmode
*/
int
NC_create(const char *path, int cmode, size_t initialsz,
          int basepe, size_t *chunksizehintp, int useparallel,
          void* mpi_info, int *ncidp)
{
   int stat = NC_NOERR;
   NC* ncp = NULL;
   NC_Dispatch* dispatcher = NULL;
   /* Need three pieces of information for now */
   int model = 0; /* one of the NC_DISPATCH_XXX values */
   int isurl = 0;   /* dap or cdmremote or neither */
   int xcmode = 0; /* for implied cmode flags */
   extern int default_create_format;

   /* Initialize the dispatch table. The function pointers in the
    * dispatch table will depend on how netCDF was built
    * (with/without netCDF-4, DAP, CDMREMOTE). */
   if(!nc_initialized)
   {
      if ((stat = NC_initialize()))
        return stat;
      nc_initialized = 1;
   }

   if((isurl = NC_testurl(path)))
     model = NC_urlmodel(path);

   /* Look to the incoming cmode for hints */
   if(model == 0) {
      if(cmode & NC_NETCDF4 || cmode & NC_PNETCDF)
        model = NC_DISPATCH_NC4;
   }

   if(model == 0) {
      /* Check default format */
      int format = default_create_format;
      switch (format) {
#ifdef USE_NETCDF4
      case NC_FORMAT_NETCDF4:
        xcmode |= NC_NETCDF4;
        model = NC_DISPATCH_NC4;
        break;
      case NC_FORMAT_NETCDF4_CLASSIC:
        xcmode |= NC_CLASSIC_MODEL;
        model = NC_DISPATCH_NC4;
        break;
#endif
      case NC_FORMAT_64BIT:
        xcmode |= NC_64BIT_OFFSET;
        /* fall thru */
      case NC_FORMAT_CLASSIC:
      default:
        model = NC_DISPATCH_NC3;
        break;
      }
   }

   /* Add inferred flags */
   cmode |= xcmode;

#ifdef USE_NETCDF4
   if((cmode & NC_MPIIO && cmode & NC_MPIPOSIX))
      return  NC_EINVAL;
#endif

   dispatcher = NC_get_dispatch_override();
   if(dispatcher != NULL) goto havetable;

   /* Figure out what dispatcher to use */
#ifdef USE_NETCDF4
#ifdef USE_CDMREMOTE
   if(model == (NC_DISPATCH_NC4 | NC_DISPATCH_NCR))
     dispatcher = NCCR_dispatch_table;
   else
#endif
#ifdef USE_DAP
   if(model == (NC_DISPATCH_NC4 | NC_DISPATCH_NCD))
     dispatcher = NCD4_dispatch_table;
   else
#endif
   if(model == (NC_DISPATCH_NC4))
     dispatcher = NC4_dispatch_table;
   else
#endif /*USE_NETCDF4*/
#ifdef USE_DAP
   if(model == (NC_DISPATCH_NC3 | NC_DISPATCH_NCD))
     dispatcher = NCD3_dispatch_table;
   else
#endif
   if(model == (NC_DISPATCH_NC3))
     dispatcher = NC3_dispatch_table;
   else
      return  NC_ENOTNC;

  havetable:
   stat = dispatcher->create(path,cmode,initialsz,basepe,chunksizehintp,
                             useparallel,mpi_info,dispatcher,&ncp);
   if(stat == NC_NOERR) {
      ncp->dispatch = dispatcher;
      if(ncidp) *ncidp = ncp->ext_ncid;
      ncp->path = strdup(path);
      if(path == NULL) stat = NC_ENOMEM;
   }
   return stat;
}

int
nc_open(const char *path, int mode, int *ncidp)
{
   return NC_open(path, mode, 0, NULL, 0, NULL, ncidp);
}

int
nc__open(const char *path, int cmode,
         size_t *chunksizehintp, int *ncidp)
{
   return NC_open(path, cmode, 0, chunksizehintp, 0,
                  NULL, ncidp);
}

int
nc__open_mp(const char *path, int cmode, int basepe,
            size_t *chunksizehintp, int *ncidp)
{
   return NC_open(path, cmode, basepe, chunksizehintp,
                  0, NULL, ncidp);
}

/*
  For create, we have the following pieces of information
  to use to determine the dispatch table.
  1. table specified by override
  2. path
  3. cmode
  4. the contents of the file (if it exists);
  basically checking its magic number
*/

int
NC_open(const char *path, int cmode,
        int basepe, size_t *chunksizehintp,
        int useparallel, void* mpi_info,
        int *ncidp)
{
   int stat = NC_NOERR;
   NC* ncp = NULL;
   NC_Dispatch* dispatcher = NULL;
   /* Need two pieces of information for now */
   int model = 0;
   int isurl = 0;
   int cdfversion = 0;
   int hdfversion = 0;
   extern int default_create_format;

   if(!nc_initialized)
   {stat = NC_initialize(); if(stat) return stat; nc_initialized = 1;}

   if((isurl = NC_testurl(path)))
      model = NC_urlmodel(path);

   if(isurl == 0) {
      /* Look at the file if it exists */
      stat = NC_check_file_type(path,useparallel,mpi_info,&cdfversion,&hdfversion);
      if(stat == NC_NOERR) {
      if(hdfversion != 0) {
      model = NC_DISPATCH_NC4;
      } else if(cdfversion != 0) {
      model = NC_DISPATCH_NC3;
      }
      }
      /* else ignore the file */
   }

   /* Look to the incoming cmode for hints */
   if(model == 0) {
      if(cmode & NC_NETCDF4 || cmode & NC_PNETCDF) model = NC_DISPATCH_NC4;
   }

   if(model == 0) model = NC_DISPATCH_NC3; /* final default */

   /* Force flag consistentcy */
   if(model & NC_DISPATCH_NC4)
      cmode |= NC_NETCDF4;
   else if(model & NC_DISPATCH_NC3) {
      cmode &= ~NC_NETCDF4; /* must be netcdf-3 */
      if(cdfversion == 2) cmode |= NC_64BIT_OFFSET;
   }

   if((cmode & NC_MPIIO && cmode & NC_MPIPOSIX))
      return  NC_EINVAL;

   /* override overrides any other table choice */
   dispatcher = NC_get_dispatch_override();
   if(dispatcher != NULL) goto havetable;

   /* Figure out what dispatcher to use */
#if  defined(USE_CDMREMOTE)
   if(model == (NC_DISPATCH_NC4 | NC_DISPATCH_NCR))
     dispatcher = NCCR_dispatch_table;
   else
#endif
#if defined(USE_NETCDF4) && defined(USE_DAP)
   if(model == (NC_DISPATCH_NC4 | NC_DISPATCH_NCD))
     dispatcher = NCD4_dispatch_table;
   else
#endif
#if defined(USE_DAP)
   if(model == (NC_DISPATCH_NC3 | NC_DISPATCH_NCD))
     dispatcher = NCD3_dispatch_table;
   else
#endif
#if defined(USE_NETCDF4)
   if(model == (NC_DISPATCH_NC4))
     dispatcher = NC4_dispatch_table;
   else
#endif
   if(model == (NC_DISPATCH_NC3))
     dispatcher = NC3_dispatch_table;
   else
      return  NC_ENOTNC;

  havetable:
   stat = dispatcher->open(path, cmode, basepe, chunksizehintp,
                           useparallel, mpi_info, dispatcher, &ncp);
   if(stat == NC_NOERR) {
      ncp->dispatch = dispatcher;
      if(ncidp) *ncidp = ncp->ext_ncid;
      ncp->path = strdup(path);
      if(path == NULL) stat = NC_ENOMEM;
   }
   return stat;
}

/* This function returns the file pathname (or the opendap URL) which
 * was used to open/create the ncid's file. */
int
nc_inq_path(int ncid, size_t *pathlen, char *path)
{
   NC* ncp;
   int stat = NC_NOERR;
   if ((stat = NC_check_id(ncid, &ncp)))
      return stat;
   if(ncp->path == NULL) {
   if(pathlen) *pathlen = 0;
   if(path) path[0] = '\0';
   } else {
       if (pathlen) *pathlen = strlen(ncp->path);
       if (path) strcpy(path, ncp->path);
   }
   return stat;
}

int
nc_redef(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->redef(ncid);
}

int
nc__enddef(int ncid, size_t h_minfree, size_t v_align, size_t v_minfree, size_t r_align)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->_enddef(ncid,h_minfree,v_align,v_minfree,r_align);
}

int
nc_enddef(int ncid)
{
   int status;
   NC *ncp;
   status = NC_check_id(ncid, &ncp);
   if(status != NC_NOERR) return status;
   return ncp->dispatch->_enddef(ncid,0,1,0,1);
}

int
nc_sync(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->sync(ncid);
}

static void
NC_reclaim(NC* ncp)
{
   /* reclaim the path */
   if(ncp->path != NULL) free(ncp->path);
   ncp->path = NULL;
}

int
nc_abort(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   NC_reclaim(ncp);
   return ncp->dispatch->abort(ncid);
}

int
nc_close(int ncid)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->close(ncid);
}

int
nc_set_fill(int ncid, int fillmode, int *old_modep)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->set_fill(ncid,fillmode,old_modep);
}

int
nc_inq_base_pe(int ncid, int *pe)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_base_pe(ncid,pe);
}

int
nc_set_base_pe(int ncid, int pe)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->set_base_pe(ncid,pe);
}


int
nc_inq_format(int ncid, int *formatp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_format(ncid,formatp);
}

int
nc_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq(ncid,ndimsp,nvarsp,nattsp,unlimdimidp);
}

int
nc_inq_type(int ncid, nc_type xtype, char *name, size_t *size)
{
   NC* ncp;
   /* For compatibility, we need to allow inq about
      atomic types, even if ncid is ill-defined */
   if(xtype <= ATOMICTYPEMAX) {
      if(xtype <= NC_NAT) return NC_EBADTYPE;
      if(name) strncpy(name,NC_atomictypename(xtype),NC_MAX_NAME);
      if(size) *size = NC_atomictypelen(xtype);
      return NC_NOERR;
   } else {
      int stat = NC_check_id(ncid, &ncp);
      if(stat != NC_NOERR) return NC_EBADTYPE; /* compatibility */
      return ncp->dispatch->inq_type(ncid,xtype,name,size);
   }
}
