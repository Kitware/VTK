/** \file
The netCDF-4 file functions.

This file is part of netcdf-4, a netCDF-like interface for HDF5, or
a HDF5 backend for netCDF, depending on your point of view.

Copyright 2003, University Corporation for Atmospheric Research. See
COPYRIGHT file for copying and redistribution conditions.

*/

#include "config.h"
#include <errno.h>  /* netcdf functions sometimes return system errors */

#define H5Tget_array_dims_vers 1

#include "nc.h"
#include "nc4internal.h"
#include "nc4dispatch.h"

extern int nc4_vararray_add(NC_GRP_INFO_T *grp,
			    NC_VAR_INFO_T *var);

/* must be after nc4internal.h */
#include <H5DSpublic.h>
#include <H5Fpublic.h>
#ifdef USE_HDF4
#include <mfhdf.h>
#endif
#include <vtk_hdf5.h>

/* When we have open objects at file close, should
   we log them or print to stdout. Default is to log
*/
#define LOGOPEN 1

/* This is to track opened HDF5 objects to make sure they are
 * closed. */
#ifdef EXTRA_TESTS
extern int num_plists;
extern int num_spaces;
#endif /* EXTRA_TESTS */

#define MIN_DEFLATE_LEVEL 0
#define MAX_DEFLATE_LEVEL 9

/*Forward*/
static int read_hdf5_att(NC_GRP_INFO_T *grp, hid_t attid, NC_ATT_INFO_T *att);
static void hdf5free(void* memory);

/* Custom iteration callback data */
typedef struct {
  NC_GRP_INFO_T *grp;
  NC_VAR_INFO_T *var;
} att_iter_info;

static herr_t
att_read_var_callbk(hid_t loc_id, const char *att_name, const H5A_info_t *ainfo, void *att_data)
{

  hid_t attid = 0;
  int retval = NC_NOERR;
  NC_ATT_INFO_T *att;
  att_iter_info *att_info = (att_iter_info *)att_data;
  const char** reserved;


  /* Should we ignore this attribute? */
  for(reserved=NC_RESERVED_VARATT_LIST;*reserved;reserved++) {
    if (strcmp(att_name, *reserved)==0) break;
  }

  if(*reserved == NULL) {
      /* Open the att by name. */
      if ((attid = H5Aopen(loc_id, att_name, H5P_DEFAULT)) < 0)
	BAIL(NC_EATTMETA);
      LOG((4, "%s::  att_name %s", __func__, att_name));
      /* Add to the end of the list of atts for this var. */
      if ((retval = nc4_att_list_add(&att_info->var->att, &att)))
	BAIL(retval);
      /* Fill in the information we know. */
      att->attnum = att_info->var->natts++;
      if (!(att->name = strdup(att_name)))
	BAIL(NC_ENOMEM);

      /* Read the rest of the info about the att,
       * including its values. */
      if ((retval = read_hdf5_att(att_info->grp, attid, att)))
	{
	  if (NC_EBADTYPID == retval)
            {
	      if ((retval = nc4_att_list_del(&att_info->var->att, att)))
		BAIL(retval);
            }
	  else
	    BAIL(retval);
	}

      att->created = NC_TRUE;

      if (attid > 0 && H5Aclose(attid) < 0)
	BAIL2(NC_EHDFERR);

    } /* endif not HDF5 att */
  
  return NC_NOERR;

 exit:
  if (attid > 0 && H5Aclose(attid) < 0)
    BAIL2(NC_EHDFERR);

  return retval;
}

/* Define the illegal mode flags */
static const int ILLEGAL_OPEN_FLAGS = (NC_MMAP|NC_64BIT_OFFSET);

static const int ILLEGAL_CREATE_FLAGS = (NC_NOWRITE|NC_MMAP|NC_INMEMORY|NC_64BIT_OFFSET|NC_CDF5);

extern void reportopenobjects(int log, hid_t);

/*! Struct to track information about objects in a group, for nc4_rec_read_metadata()

 \internal

*/
typedef struct NC4_rec_read_metadata_obj_info
{
    hid_t oid;                          /* HDF5 object ID */
    char oname[NC_MAX_NAME + 1];        /* Name of object */
    H5G_stat_t statbuf;                 /* Information about the object */
    struct NC4_rec_read_metadata_obj_info *next;        /* Pointer to next node in list */
} NC4_rec_read_metadata_obj_info_t;

/*! User data struct for call to H5Literate() in nc4_rec_read_metadata()

\internal

Tracks the groups, named datatypes and datasets in the group, for later use.
*/
typedef struct NC4_rec_read_metadata_ud
{
   NC4_rec_read_metadata_obj_info_t *grps_head, *grps_tail;      /* Pointers to head & tail of list of groups */
   NC_GRP_INFO_T *grp;                                          /* Pointer to parent group */
} NC4_rec_read_metadata_ud_t;

/* Forward */
static int NC4_enddef(int ncid);
static int nc4_rec_read_metadata(NC_GRP_INFO_T *grp);
static int close_netcdf4_file(NC_HDF5_FILE_INFO_T *h5, int abort);

/* Define the names of attributes to ignore
 * added by the HDF5 dimension scale; these
 * attached to variables.
 * They cannot be modified thru the netcdf-4 API.
 */
const char* NC_RESERVED_VARATT_LIST[] = {
NC_ATT_REFERENCE_LIST,
NC_ATT_CLASS,
NC_ATT_DIMENSION_LIST,
NC_ATT_NAME,
NC_ATT_COORDINATES,
NC_DIMID_ATT_NAME,
NULL
};

/* Define the names of attributes to ignore
 * because they are "hidden" global attributes.
 * They can be read, but not modified thru the netcdf-4 API.
 */
const char* NC_RESERVED_ATT_LIST[] = {
NC_ATT_FORMAT,
NC3_STRICT_ATT_NAME,
NCPROPS,
ISNETCDF4ATT,
SUPERBLOCKATT,
NULL
};

/* Define the subset of the reserved list that is readable by name only */
const char* NC_RESERVED_SPECIAL_LIST[] = {
ISNETCDF4ATT,
SUPERBLOCKATT,
NCPROPS,
NULL
};

/* These are the default chunk cache sizes for HDF5 files created or
 * opened with netCDF-4. */
size_t nc4_chunk_cache_size = CHUNK_CACHE_SIZE;
size_t nc4_chunk_cache_nelems = CHUNK_CACHE_NELEMS;
float nc4_chunk_cache_preemption = CHUNK_CACHE_PREEMPTION;

/* For performance, fill this array only the first time, and keep it
 * in global memory for each further use. */
#define NUM_TYPES 12
static hid_t h5_native_type_constant_g[NUM_TYPES];
static const char nc_type_name_g[NUM_TYPES][NC_MAX_NAME + 1] = {"char", "byte", "short",
    "int", "float", "double", "ubyte",
    "ushort", "uint", "int64",
    "uint64", "string"};
static const nc_type nc_type_constant_g[NUM_TYPES] = {NC_CHAR, NC_BYTE, NC_SHORT,
    NC_INT, NC_FLOAT, NC_DOUBLE, NC_UBYTE,
    NC_USHORT, NC_UINT, NC_INT64,
    NC_UINT64, NC_STRING};
static const int nc_type_size_g[NUM_TYPES] = {sizeof(char), sizeof(char), sizeof(short),
    sizeof(int), sizeof(float), sizeof(double), sizeof(unsigned char),
    sizeof(unsigned short), sizeof(unsigned int), sizeof(long long),
    sizeof(unsigned long long), sizeof(char *)};

/* Set chunk cache size. Only affects files opened/created *after* it
 * is called.  */
int
nc_set_chunk_cache(size_t size, size_t nelems, float preemption)
{
   if (preemption < 0 || preemption > 1)
      return NC_EINVAL;
   nc4_chunk_cache_size = size;
   nc4_chunk_cache_nelems = nelems;
   nc4_chunk_cache_preemption = preemption;
   return NC_NOERR;
}

/* Get chunk cache size. Only affects files opened/created *after* it
 * is called.  */
int
nc_get_chunk_cache(size_t *sizep, size_t *nelemsp, float *preemptionp)
{
   if (sizep)
      *sizep = nc4_chunk_cache_size;

   if (nelemsp)
      *nelemsp = nc4_chunk_cache_nelems;

   if (preemptionp)
      *preemptionp = nc4_chunk_cache_preemption;
   return NC_NOERR;
}

/* Required for fortran to avoid size_t issues. */
int
nc_set_chunk_cache_ints(int size, int nelems, int preemption)
{
   if (size <= 0 || nelems <= 0 || preemption < 0 || preemption > 100)
      return NC_EINVAL;
   nc4_chunk_cache_size = size;
   nc4_chunk_cache_nelems = nelems;
   nc4_chunk_cache_preemption = (float)preemption / 100;
   return NC_NOERR;
}

int
nc_get_chunk_cache_ints(int *sizep, int *nelemsp, int *preemptionp)
{
   if (sizep)
      *sizep = (int)nc4_chunk_cache_size;
   if (nelemsp)
      *nelemsp = (int)nc4_chunk_cache_nelems;
   if (preemptionp)
      *preemptionp = (int)(nc4_chunk_cache_preemption * 100);

   return NC_NOERR;
}

/* This will return the length of a netcdf data type in bytes. */
int
nc4typelen(nc_type type)
{
   switch(type){
      case NC_BYTE:
      case NC_CHAR:
      case NC_UBYTE:
	 return 1;
      case NC_USHORT:
      case NC_SHORT:
	 return 2;
      case NC_FLOAT:
      case NC_INT:
      case NC_UINT:
	 return 4;
      case NC_DOUBLE:
      case NC_INT64:
      case NC_UINT64:
	 return 8;
   }
   return -1;
}

/* Given a filename, check to see if it is a HDF5 file. */
#define MAGIC_NUMBER_LEN 4
#define NC_HDF5_FILE 1
#define NC_HDF4_FILE 2
static int
nc_check_for_hdf(const char *path, int flags, void* parameters, int *hdf_file)
{
   char blob[MAGIC_NUMBER_LEN];
#ifdef USE_PARALLEL4
   int use_parallel = ((flags & NC_MPIIO) == NC_MPIIO);
   NC_MPI_INFO* mpiinfo = (NC_MPI_INFO*)parameters;
   MPI_Comm comm = MPI_COMM_WORLD;
   MPI_Info info = MPI_INFO_NULL;
#endif
   int inmemory = ((flags & NC_INMEMORY) == NC_INMEMORY);
   NC_MEM_INFO* meminfo = (NC_MEM_INFO*)parameters;

#ifdef USE_PARALLEL4
   if(use_parallel) {
       comm = mpiinfo->comm;
       info = mpiinfo->info;
   }
#endif

   assert(hdf_file);
   LOG((3, "%s: path %s", __func__, path));

   /* HDF5 function handles possible user block at beginning of file */
   if(!inmemory && H5Fis_hdf5(path))
   {
       *hdf_file = NC_HDF5_FILE;
   } else {

/* Get the 4-byte blob from the beginning of the file. Don't use posix
 * for parallel, use the MPI functions instead. */
#ifdef USE_PARALLEL4
       if (!inmemory && use_parallel)
       {
	   MPI_File fh;
	   MPI_Status status;
	   int retval;
	   if ((retval = MPI_File_open(comm, (char *)path, MPI_MODE_RDONLY,
				       info, &fh)) != MPI_SUCCESS)
	       return NC_EPARINIT;
	   if ((retval = MPI_File_read(fh, blob, MAGIC_NUMBER_LEN, MPI_CHAR,
				       &status)) != MPI_SUCCESS)
	       return NC_EPARINIT;
	   if ((retval = MPI_File_close(&fh)) != MPI_SUCCESS)
	       return NC_EPARINIT;
       }
       else
#endif /* USE_PARALLEL4 */
       if(!inmemory) {
	   FILE *fp;
	   if (!(fp = fopen(path, "r")) ||
	       fread(blob, MAGIC_NUMBER_LEN, 1, fp) != 1) {

	     if(fp) fclose(fp);
	     return errno;
	   }
	   fclose(fp);
       } else { /*inmemory*/
	  if(meminfo->size < MAGIC_NUMBER_LEN)
	    return NC_ENOTNC;
	  memcpy(blob,meminfo->memory,MAGIC_NUMBER_LEN);
       }

       /* Check for HDF4. */
       if (memcmp(blob, "\016\003\023\001", 4)==0)
	   *hdf_file = NC_HDF4_FILE;
       else if (memcmp(blob, "HDF", 3)==0)
	   *hdf_file = NC_HDF5_FILE;
       else
	   *hdf_file = 0;
   }
   return NC_NOERR;
}

/* Create a HDF5/netcdf-4 file. */

static int
nc4_create_file(const char *path, int cmode, MPI_Comm comm, MPI_Info info,
                NC *nc)
{
   hid_t fcpl_id, fapl_id = -1;
   unsigned flags;
   FILE *fp;
   int retval = NC_NOERR;
   NC_HDF5_FILE_INFO_T* nc4_info = NULL;
#ifdef USE_PARALLEL4
   int comm_duped = 0;          /* Whether the MPI Communicator was duplicated */
   int info_duped = 0;          /* Whether the MPI Info object was duplicated */
#else /* !USE_PARALLEL4 */
   int persist = 0; /* Should diskless try to persist its data into file?*/
#endif

   assert(nc);

   if(cmode & NC_DISKLESS)
       flags = H5F_ACC_TRUNC;
   else if(cmode & NC_NOCLOBBER)
       flags = H5F_ACC_EXCL;
   else
       flags = H5F_ACC_TRUNC;

   LOG((3, "%s: path %s mode 0x%x", __func__, path, cmode));
   assert(nc && path);

   /* If this file already exists, and NC_NOCLOBBER is specified,
      return an error. */
   if (cmode & NC_DISKLESS) {
#ifndef USE_PARALLEL4
	if(cmode & NC_WRITE)
	    persist = 1;
#endif
   } else if ((cmode & NC_NOCLOBBER) && (fp = fopen(path, "r"))) {
      fclose(fp);
      return NC_EEXIST;
   }

   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, (NC_WRITE | cmode))))
      BAIL(retval);
   nc4_info = NC4_DATA(nc);
   assert(nc4_info && nc4_info->root_grp);

   /* Need this access plist to control how HDF5 handles open objects
    * on file close. (Setting H5F_CLOSE_SEMI will cause H5Fclose to
    * fail if there are any open objects in the file. */
   if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists++;
#endif
#ifdef EXTRA_TESTS
   if (H5Pset_fclose_degree(fapl_id, H5F_CLOSE_SEMI))
      BAIL(NC_EHDFERR);
#else
   if (H5Pset_fclose_degree(fapl_id, H5F_CLOSE_STRONG))
      BAIL(NC_EHDFERR);
#endif /* EXTRA_TESTS */

#ifdef USE_PARALLEL4
   /* If this is a parallel file create, set up the file creation
      property list. */
   if ((cmode & NC_MPIIO) || (cmode & NC_MPIPOSIX))
   {
      nc4_info->parallel = NC_TRUE;
      if (cmode & NC_MPIIO)  /* MPI/IO */
      {
	 LOG((4, "creating parallel file with MPI/IO"));
	 if (H5Pset_fapl_mpio(fapl_id, comm, info) < 0)
	    BAIL(NC_EPARINIT);
      }
#ifdef USE_PARALLEL_POSIX
      else /* MPI/POSIX */
      {
	 LOG((4, "creating parallel file with MPI/posix"));
	 if (H5Pset_fapl_mpiposix(fapl_id, comm, 0) < 0)
	    BAIL(NC_EPARINIT);
      }
#else /* USE_PARALLEL_POSIX */
      /* Should not happen! Code in NC4_create/NC4_open should alias the
       *        NC_MPIPOSIX flag to NC_MPIIO, if the MPI-POSIX VFD is not
       *        available in HDF5. -QAK
       */
      else /* MPI/POSIX */
         BAIL(NC_EPARINIT);
#endif /* USE_PARALLEL_POSIX */

      /* Keep copies of the MPI Comm & Info objects */
      if (MPI_SUCCESS != MPI_Comm_dup(comm, &nc4_info->comm))
         BAIL(NC_EMPI);
      comm_duped++;
      if (MPI_INFO_NULL != info)
      {
         if (MPI_SUCCESS != MPI_Info_dup(info, &nc4_info->info))
            BAIL(NC_EMPI);
         info_duped++;
      }
      else
      {
         /* No dup, just copy it. */
         nc4_info->info = info;
      }
   }
#else /* only set cache for non-parallel... */
   if(cmode & NC_DISKLESS) {
	 if (H5Pset_fapl_core(fapl_id, 4096, persist))
	    BAIL(NC_EDISKLESS);
   }
   if (H5Pset_cache(fapl_id, 0, nc4_chunk_cache_nelems, nc4_chunk_cache_size,
		    nc4_chunk_cache_preemption) < 0)
      BAIL(NC_EHDFERR);
   LOG((4, "%s: set HDF raw chunk cache to size %d nelems %d preemption %f",
	__func__, nc4_chunk_cache_size, nc4_chunk_cache_nelems, nc4_chunk_cache_preemption));
#endif /* USE_PARALLEL4 */

#ifdef HDF5_HAS_LIBVER_BOUNDS
   if (H5Pset_libver_bounds(fapl_id, H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST) < 0)
      BAIL(NC_EHDFERR);
#endif

   /* Create the property list. */
   if ((fcpl_id = H5Pcreate(H5P_FILE_CREATE)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists++;
#endif

   /* RJ: this suppose to be FALSE that is defined in H5 private.h as 0 */
   if (H5Pset_obj_track_times(fcpl_id,0)<0)
      BAIL(NC_EHDFERR);

   /* Set latest_format in access propertly list and
    * H5P_CRT_ORDER_TRACKED in the creation property list. This turns
    * on HDF5 creation ordering. */
   if (H5Pset_link_creation_order(fcpl_id, (H5P_CRT_ORDER_TRACKED |
					    H5P_CRT_ORDER_INDEXED)) < 0)
      BAIL(NC_EHDFERR);
   if (H5Pset_attr_creation_order(fcpl_id, (H5P_CRT_ORDER_TRACKED |
					    H5P_CRT_ORDER_INDEXED)) < 0)
      BAIL(NC_EHDFERR);

   /* Create the file. */
#ifdef HDF5_HAS_COLL_METADATA_OPS
   H5Pset_all_coll_metadata_ops(fapl_id, 1 );
   H5Pset_coll_metadata_write(fapl_id, 1);
#endif

   if ((nc4_info->hdfid = H5Fcreate(path, flags, fcpl_id, fapl_id)) < 0)
        /*Change the return error from NC_EFILEMETADATA to
          System error EACCES because that is the more likely problem */
      BAIL(EACCES);

   /* Open the root group. */
   if ((nc4_info->root_grp->hdf_grpid = H5Gopen2(nc4_info->hdfid, "/",
						     H5P_DEFAULT)) < 0)
      BAIL(NC_EFILEMETA);

   /* Release the property lists. */
   if (H5Pclose(fapl_id) < 0 || H5Pclose(fcpl_id) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
   num_plists--;
#endif

   /* Define mode gets turned on automatically on create. */
   nc4_info->flags |= NC_INDEF;

   NC4_get_fileinfo(nc4_info,&globalpropinfo);
   NC4_put_propattr(nc4_info);

   return NC_NOERR;

exit: /*failure exit*/
#ifdef USE_PARALLEL4
   if (comm_duped) MPI_Comm_free(&nc4_info->comm);
   if (info_duped) MPI_Info_free(&nc4_info->info);
#endif
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (fapl_id != H5P_DEFAULT) H5Pclose(fapl_id);
   if(!nc4_info) return retval;
   close_netcdf4_file(nc4_info,1); /* treat like abort */
   return retval;
}

/** \ingroup netcdf4
Create a netCDF-4/HDF5 file.

\param path The file name of the new file.
\param cmode The creation mode flag.
\param initialsz Ignored by this function.
\param basepe Ignored by this function.
\param chunksizehintp Ignored by this function.
\param use_parallel 0 for sequential, non-zero for parallel I/O.
\param parameters pointer to struct holding extra data (e.g. for parallel I/O)
layer. Ignored if NULL.
\param dispatch Pointer to the dispatch table for this file.
\param nc_file Pointer to an instance of NC.
\return NC_INVAL Invalid input (check cmode).
*/
int
NC4_create(const char* path, int cmode, size_t initialsz, int basepe,
	   size_t *chunksizehintp, int use_parallel, void *parameters,
	   NC_Dispatch *dispatch, NC* nc_file)
{
   MPI_Comm comm = MPI_COMM_WORLD;
   MPI_Info info = MPI_INFO_NULL;
   int res;

   assert(nc_file && path);

   LOG((1, "%s: path %s cmode 0x%x comm %d info %d",
	__func__, path, cmode, comm, info));

#ifdef USE_PARALLEL4
   if (parameters)
   {
      comm = ((NC_MPI_INFO *)parameters)->comm;
      info = ((NC_MPI_INFO *)parameters)->info;
   }
#endif /* USE_PARALLEL4 */

   /* If this is our first file, turn off HDF5 error messages. */
   if (!nc4_hdf5_initialized)
	nc4_hdf5_initialize();

   /* Check the cmode for validity. */
   if((cmode & ILLEGAL_CREATE_FLAGS) != 0)
      {res = NC_EINVAL; goto done;}

   /* Cannot have both */
   if((cmode & (NC_MPIIO|NC_MPIPOSIX)) == (NC_MPIIO|NC_MPIPOSIX))
      {res = NC_EINVAL; goto done;}

   /* Currently no parallel diskless io */
   if((cmode & (NC_MPIIO | NC_MPIPOSIX)) && (cmode & NC_DISKLESS))
      {res = NC_EINVAL; goto done;}

#ifndef USE_PARALLEL_POSIX
/* If the HDF5 library has been compiled without the MPI-POSIX VFD, alias
 *      the NC_MPIPOSIX flag to NC_MPIIO. -QAK
 */
   if(cmode & NC_MPIPOSIX)
   {
      cmode &= ~NC_MPIPOSIX;
      cmode |= NC_MPIIO;
   }
#endif /* USE_PARALLEL_POSIX */

   cmode |= NC_NETCDF4;

   /* Apply default create format. */
   if (nc_get_default_format() == NC_FORMAT_CDF5)
      cmode |= NC_CDF5;
   else if (nc_get_default_format() == NC_FORMAT_64BIT_OFFSET)
      cmode |= NC_64BIT_OFFSET;
   else if (nc_get_default_format() == NC_FORMAT_NETCDF4_CLASSIC)
      cmode |= NC_CLASSIC_MODEL;

   LOG((2, "cmode after applying default format: 0x%x", cmode));

   nc_file->int_ncid = nc_file->ext_ncid;

   res = nc4_create_file(path, cmode, comm, info, nc_file);

done:
   return res;
}

/* This function is called by read_dataset when a dimension scale
 * dataset is encountered. It reads in the dimension data (creating a
 * new NC_DIM_INFO_T object), and also checks to see if this is a
 * dimension without a variable - that is, a coordinate dimension
 * which does not have any coordinate data. */
static int
read_scale(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
        const H5G_stat_t *statbuf, hsize_t scale_size, hsize_t max_scale_size,
        NC_DIM_INFO_T **dim)
{
   NC_DIM_INFO_T *new_dim;              /* Dimension added to group */
   char dimscale_name_att[NC_MAX_NAME + 1] = "";    /* Dimscale name, for checking if dim without var */
   htri_t attr_exists = -1;             /* Flag indicating hidden attribute exists */
   hid_t attid = -1;                    /* ID of hidden attribute (to store dim ID) */
   int dimscale_created = 0;            /* Remember if a dimension was created (for error recovery) */
   short initial_next_dimid = grp->nc4_info->next_dimid;/* Retain for error recovery */
   int retval;

   /* Add a dimension for this scale. */
   if ((retval = nc4_dim_list_add(&grp->dim, &new_dim)))
      BAIL(retval);
   dimscale_created++;

   /* Does this dataset have a hidden attribute that tells us its
    * dimid? If so, read it. */
   if ((attr_exists = H5Aexists(datasetid, NC_DIMID_ATT_NAME)) < 0)
      BAIL(NC_EHDFERR);
   if (attr_exists)
   {
      if ((attid = H5Aopen_name(datasetid, NC_DIMID_ATT_NAME)) < 0)
         BAIL(NC_EHDFERR);

      if (H5Aread(attid, H5T_NATIVE_INT, &new_dim->dimid) < 0)
         BAIL(NC_EHDFERR);

      /* Check if scale's dimid should impact the group's next dimid */
      if (new_dim->dimid >= grp->nc4_info->next_dimid)
         grp->nc4_info->next_dimid = new_dim->dimid + 1;
   }
   else
   {
      /* Assign dimid */
      new_dim->dimid = grp->nc4_info->next_dimid++;
   }

   if (!(new_dim->name = strdup(obj_name)))
      BAIL(NC_ENOMEM);
   if (SIZEOF_SIZE_T < 8 && scale_size > NC_MAX_UINT)
   {
      new_dim->len = NC_MAX_UINT;
      new_dim->too_long = NC_TRUE;
   }
   else
      new_dim->len = scale_size;
   new_dim->hdf5_objid.fileno[0] = statbuf->fileno[0];
   new_dim->hdf5_objid.fileno[1] = statbuf->fileno[1];
   new_dim->hdf5_objid.objno[0] = statbuf->objno[0];
   new_dim->hdf5_objid.objno[1] = statbuf->objno[1];
   new_dim->hash = hash_fast(obj_name, strlen(obj_name));

   /* If the dimscale has an unlimited dimension, then this dimension
    * is unlimited. */
   if (max_scale_size == H5S_UNLIMITED)
      new_dim->unlimited = NC_TRUE;

   /* If the scale name is set to DIM_WITHOUT_VARIABLE, then this is a
    * dimension, but not a variable. (If get_scale_name returns an
    * error, just move on, there's no NAME.) */
   if (H5DSget_scale_name(datasetid, dimscale_name_att, NC_MAX_NAME) >= 0)
   {
      if (!strncmp(dimscale_name_att, DIM_WITHOUT_VARIABLE,
                   strlen(DIM_WITHOUT_VARIABLE)))
      {
         if (new_dim->unlimited)
         {
            size_t len = 0, *lenp = &len;

            if ((retval = nc4_find_dim_len(grp, new_dim->dimid, &lenp)))
               BAIL(retval);
            new_dim->len = *lenp;
         }

         /* Hold open the dataset, since the dimension doesn't have a coordinate variable */
         new_dim->hdf_dimscaleid = datasetid;
         H5Iinc_ref(new_dim->hdf_dimscaleid);        /* Increment number of objects using ID */
      }
   }

   /* Set the dimension created */
   *dim = new_dim;

exit:
   /* Close the hidden attribute, if it was opened (error, or no error) */
   if (attid > 0 && H5Aclose(attid) < 0)
      BAIL2(NC_EHDFERR);

   /* On error, undo any dimscale creation */
   if (retval < 0 && dimscale_created)
   {
       /* Delete the dimension */
       if ((retval = nc4_dim_list_del(&grp->dim, new_dim)))
           BAIL2(retval);

      /* Reset the group's information */
      grp->nc4_info->next_dimid = initial_next_dimid;
   }

   return retval;
}

/* This function reads the hacked in coordinates attribute I use for
 * multi-dimensional coordinates. */
static int
read_coord_dimids(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
   hid_t coord_att_typeid = -1, coord_attid = -1, spaceid = -1;
   hssize_t npoints;
   int ret = 0;
   int d;

   /* There is a hidden attribute telling us the ids of the
    * dimensions that apply to this multi-dimensional coordinate
    * variable. Read it. */
   if ((coord_attid = H5Aopen_name(var->hdf_datasetid, COORDINATES)) < 0) ret++;
   if (!ret && (coord_att_typeid = H5Aget_type(coord_attid)) < 0) ret++;

   /* How many dimensions are there? */
   if (!ret && (spaceid = H5Aget_space(coord_attid)) < 0) ret++;
#ifdef EXTRA_TESTS
   num_spaces++;
#endif
   if (!ret && (npoints = H5Sget_simple_extent_npoints(spaceid)) < 0) ret++;

   /* Check that the number of points is the same as the number of dimensions
    *   for the variable */
   if (!ret && npoints != var->ndims) ret++;

   if (!ret && H5Aread(coord_attid, coord_att_typeid, var->dimids) < 0) ret++;
   LOG((4, "dimscale %s is multidimensional and has coords", var->name));

   /* Update var->dim field based on the var->dimids */
   for (d = 0; d < var->ndims; d++) {
     /* Ok if does not find a dim at this time, but if found set it */
     nc4_find_dim(grp, var->dimids[d], &var->dim[d], NULL);
   }

   /* Set my HDF5 IDs free! */
   if (spaceid >= 0 && H5Sclose(spaceid) < 0) ret++;
#ifdef EXTRA_TESTS
   num_spaces--;
#endif
   if (coord_att_typeid >= 0 && H5Tclose(coord_att_typeid) < 0) ret++;
   if (coord_attid >= 0 && H5Aclose(coord_attid) < 0) ret++;
   return ret ? NC_EATTMETA : NC_NOERR;
}

/* This function is called when reading a file's metadata for each
 * dimension scale attached to a variable.*/
static herr_t
dimscale_visitor(hid_t did, unsigned dim, hid_t dsid,
                 void *dimscale_hdf5_objids)
{
   H5G_stat_t statbuf;

   /* Get more info on the dimscale object.*/
   if (H5Gget_objinfo(dsid, ".", 1, &statbuf) < 0)
      return -1;

   /* Pass this information back to caller. */
   (*(HDF5_OBJID_T *)dimscale_hdf5_objids).fileno[0] = statbuf.fileno[0];
   (*(HDF5_OBJID_T *)dimscale_hdf5_objids).fileno[1] = statbuf.fileno[1];
   (*(HDF5_OBJID_T *)dimscale_hdf5_objids).objno[0] = statbuf.objno[0];
   (*(HDF5_OBJID_T *)dimscale_hdf5_objids).objno[1] = statbuf.objno[1];
   return 0;
}

/* Given an HDF5 type, set a pointer to netcdf type. */
static int
get_netcdf_type(NC_HDF5_FILE_INFO_T *h5, hid_t native_typeid,
		nc_type *xtype)
{
   NC_TYPE_INFO_T *type;
   H5T_class_t class;
   htri_t is_str, equal = 0;

   assert(h5 && xtype);

   if ((class = H5Tget_class(native_typeid)) < 0)
      return NC_EHDFERR;

   /* H5Tequal doesn't work with H5T_C_S1 for some reason. But
    * H5Tget_class will return H5T_STRING if this is a string. */
   if (class == H5T_STRING)
   {
      if ((is_str = H5Tis_variable_str(native_typeid)) < 0)
         return NC_EHDFERR;
      if (is_str)
         *xtype = NC_STRING;
      else
         *xtype = NC_CHAR;
      return NC_NOERR;
   }
   else if (class == H5T_INTEGER || class == H5T_FLOAT)
   {
      /* For integers and floats, we don't have to worry about
       * endianness if we compare native types. */
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_SCHAR)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_BYTE;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_SHORT)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_SHORT;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_INT)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_INT;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_FLOAT)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_FLOAT;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_DOUBLE)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_DOUBLE;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_UCHAR)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_UBYTE;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_USHORT)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_USHORT;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_UINT)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_UINT;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_LLONG)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_INT64;
         return NC_NOERR;
      }
      if ((equal = H5Tequal(native_typeid, H5T_NATIVE_ULLONG)) < 0)
         return NC_EHDFERR;
      if (equal)
      {
         *xtype = NC_UINT64;
         return NC_NOERR;
      }
   }

   /* Maybe we already know about this type. */
   if (!equal)
      if((type = nc4_rec_find_hdf_type(h5->root_grp, native_typeid)))
      {
         *xtype = type->nc_typeid;
         return NC_NOERR;
      }

   *xtype = NC_NAT;
   return NC_EBADTYPID;
}

/* Given an HDF5 type, set a pointer to netcdf type_info struct,
 * either an existing one (for user-defined types) or a newly created
 * one. */
static int
get_type_info2(NC_HDF5_FILE_INFO_T *h5, hid_t datasetid,
	       NC_TYPE_INFO_T **type_info)
{
   htri_t is_str, equal = 0;
   H5T_class_t class;
   hid_t native_typeid, hdf_typeid;
   H5T_order_t order;
   int t;

   assert(h5 && type_info);

   /* Because these N5T_NATIVE_* constants are actually function calls
    * (!) in H5Tpublic.h, I can't initialize this array in the usual
    * way, because at least some C compilers (like Irix) complain
    * about calling functions when defining constants. So I have to do
    * it like this. Note that there's no native types for char or
    * string. Those are handled later. */
   if (!h5_native_type_constant_g[1])
   {
      h5_native_type_constant_g[1] = H5T_NATIVE_SCHAR;
      h5_native_type_constant_g[2] = H5T_NATIVE_SHORT;
      h5_native_type_constant_g[3] = H5T_NATIVE_INT;
      h5_native_type_constant_g[4] = H5T_NATIVE_FLOAT;
      h5_native_type_constant_g[5] = H5T_NATIVE_DOUBLE;
      h5_native_type_constant_g[6] = H5T_NATIVE_UCHAR;
      h5_native_type_constant_g[7] = H5T_NATIVE_USHORT;
      h5_native_type_constant_g[8] = H5T_NATIVE_UINT;
      h5_native_type_constant_g[9] = H5T_NATIVE_LLONG;
      h5_native_type_constant_g[10] = H5T_NATIVE_ULLONG;
   }

   /* Get the HDF5 typeid - we'll need it later. */
   if ((hdf_typeid = H5Dget_type(datasetid)) < 0)
      return NC_EHDFERR;

   /* Get the native typeid. Will be equivalent to hdf_typeid when
    * creating but not necessarily when reading, a variable. */
   if ((native_typeid = H5Tget_native_type(hdf_typeid, H5T_DIR_DEFAULT)) < 0)
      return NC_EHDFERR;

   /* Is this type an integer, string, compound, or what? */
   if ((class = H5Tget_class(native_typeid)) < 0)
      return NC_EHDFERR;

   /* Is this an atomic type? */
   if (class == H5T_STRING || class == H5T_INTEGER || class == H5T_FLOAT)
   {
      /* Allocate a phony NC_TYPE_INFO_T struct to hold type info. */
      if (!(*type_info = calloc(1, sizeof(NC_TYPE_INFO_T))))
	 return NC_ENOMEM;

      /* H5Tequal doesn't work with H5T_C_S1 for some reason. But
       * H5Tget_class will return H5T_STRING if this is a string. */
      if (class == H5T_STRING)
      {
	 if ((is_str = H5Tis_variable_str(native_typeid)) < 0)
	    return NC_EHDFERR;
	 /* Make sure fixed-len strings will work like variable-len strings */
	 if (is_str || H5Tget_size(hdf_typeid) > 1)
         {
            /* Set a class for the type */
	    t = NUM_TYPES - 1;
            (*type_info)->nc_type_class = NC_STRING;
         }
	 else
         {
            /* Set a class for the type */
	    t = 0;
            (*type_info)->nc_type_class = NC_CHAR;
         }
      }
      else if (class == H5T_INTEGER || class == H5T_FLOAT)
      {
	 for (t = 1; t < NUM_TYPES - 1; t++)
	 {
	    if ((equal = H5Tequal(native_typeid, h5_native_type_constant_g[t])) < 0)
	       return NC_EHDFERR;
	    if (equal)
	       break;
	 }

     /* Find out about endianness.
      * As of HDF 1.8.6, this works with all data types
      * Not just H5T_INTEGER.
      *
      * See https://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-GetOrder
      */
     if((order = H5Tget_order(hdf_typeid)) < 0)
         return NC_EHDFERR;

     if(order == H5T_ORDER_LE)
         (*type_info)->endianness = NC_ENDIAN_LITTLE;
     else if(order == H5T_ORDER_BE)
         (*type_info)->endianness = NC_ENDIAN_BIG;
     else
         return NC_EBADTYPE;

     if(class == H5T_INTEGER)
          (*type_info)->nc_type_class = NC_INT;
     else
        (*type_info)->nc_type_class = NC_FLOAT;
    }
      (*type_info)->nc_typeid = nc_type_constant_g[t];
      (*type_info)->size = nc_type_size_g[t];
      if (!((*type_info)->name = strdup(nc_type_name_g[t])))
	 return NC_ENOMEM;
      (*type_info)->hdf_typeid = hdf_typeid;
      (*type_info)->native_hdf_typeid = native_typeid;
      return NC_NOERR;
   }
   else
   {
      NC_TYPE_INFO_T *type;

      /* This is a user-defined type. */
      if((type = nc4_rec_find_hdf_type(h5->root_grp, native_typeid)))
	 *type_info = type;

      /* The type entry in the array of user-defined types already has
       * an open data typeid (and native typeid), so close the ones we
       * opened above. */
      if (H5Tclose(native_typeid) < 0)
	 return NC_EHDFERR;
      if (H5Tclose(hdf_typeid) < 0)
	 return NC_EHDFERR;

      if (type)
         return NC_NOERR;
   }

   return NC_EBADTYPID;
}

/* Read an attribute. */
static int
read_hdf5_att(NC_GRP_INFO_T *grp, hid_t attid, NC_ATT_INFO_T *att)
{
   hid_t spaceid = 0, file_typeid = 0;
   hsize_t dims[1] = {0}; /* netcdf attributes always 1-D. */
   int retval = NC_NOERR;
   size_t type_size;
   int att_ndims;
   hssize_t att_npoints;
   H5T_class_t att_class;
   int fixed_len_string = 0;
   size_t fixed_size = 0;

   assert(att->name);
   LOG((5, "%s: att->attnum %d att->name %s att->nc_typeid %d att->len %d",
        __func__, att->attnum, att->name, (int)att->nc_typeid, att->len));

   /* Get type of attribute in file. */
   if ((file_typeid = H5Aget_type(attid)) < 0)
      return NC_EATTMETA;
   if ((att->native_hdf_typeid = H5Tget_native_type(file_typeid, H5T_DIR_DEFAULT)) < 0)
      BAIL(NC_EHDFERR);
   if ((att_class = H5Tget_class(att->native_hdf_typeid)) < 0)
      BAIL(NC_EATTMETA);
   if (att_class == H5T_STRING && !H5Tis_variable_str(att->native_hdf_typeid))
   {
      fixed_len_string++;
      if (!(fixed_size = H5Tget_size(att->native_hdf_typeid)))
	 BAIL(NC_EATTMETA);
   }
   if ((retval = get_netcdf_type(grp->nc4_info, att->native_hdf_typeid, &(att->nc_typeid))))
      BAIL(retval);


   /* Get len. */
   if ((spaceid = H5Aget_space(attid)) < 0)
      BAIL(NC_EATTMETA);
#ifdef EXTRA_TESTS
   num_spaces++;
#endif
   if ((att_ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
      BAIL(NC_EATTMETA);
   if ((att_npoints = H5Sget_simple_extent_npoints(spaceid)) < 0)
      BAIL(NC_EATTMETA);

   /* If both att_ndims and att_npoints are zero, then this is a
    * zero length att. */
   if (att_ndims == 0 && att_npoints == 0)
      dims[0] = 0;
   else if (att->nc_typeid == NC_STRING)
       dims[0] = att_npoints;
   else if (att->nc_typeid == NC_CHAR)
   {
      /* NC_CHAR attributes are written as a scalar in HDF5, of type
       * H5T_C_S1, of variable length. */
     if (att_ndims == 0)
       {
         if (!(dims[0] = H5Tget_size(file_typeid)))
           BAIL(NC_EATTMETA);
       }
     else
       {
         /* This is really a string type! */
         att->nc_typeid = NC_STRING;
         dims[0] = att_npoints;
       }
   }
   else
   {
      H5S_class_t space_class;

      /* All netcdf attributes are scalar or 1-D only. */
      if (att_ndims > 1)
	 BAIL(NC_EATTMETA);

      /* Check class of HDF5 dataspace */
      if ((space_class = H5Sget_simple_extent_type(spaceid)) < 0)
	 BAIL(NC_EATTMETA);

      /* Check for NULL HDF5 dataspace class (should be weeded out earlier) */
      if (H5S_NULL == space_class)
	 BAIL(NC_EATTMETA);

      /* check for SCALAR HDF5 dataspace class */
      if (H5S_SCALAR == space_class)
          dims[0] = 1;
      else /* Must be "simple" dataspace */
      {
          /* Read the size of this attribute. */
          if (H5Sget_simple_extent_dims(spaceid, dims, NULL) < 0)
             BAIL(NC_EATTMETA);
      }
   }

   /* Tell the user what the length if this attribute is. */
   att->len = dims[0];

   /* Allocate some memory if the len is not zero, and read the
      attribute. */
   if (dims[0])
   {
      if ((retval = nc4_get_typelen_mem(grp->nc4_info, att->nc_typeid, 0,
					&type_size)))
	 return retval;
      if (att_class == H5T_VLEN)
      {
	 if (!(att->vldata = malloc((unsigned int)(att->len * sizeof(hvl_t)))))
	    BAIL(NC_ENOMEM);
	 if (H5Aread(attid, att->native_hdf_typeid, att->vldata) < 0)
	    BAIL(NC_EATTMETA);
      }
      else if (att->nc_typeid == NC_STRING)
      {
	 if (!(att->stdata = calloc(att->len, sizeof(char *))))
	    BAIL(NC_ENOMEM);
	 /* For a fixed length HDF5 string, the read requires
	  * contiguous memory. Meanwhile, the netCDF API requires that
	  * nc_free_string be called on string arrays, which would not
	  * work if one contiguous memory block were used. So here I
	  * convert the contiguous block of strings into an array of
	  * malloced strings (each string with its own malloc). Then I
	  * copy the data and free the contiguous memory. This
	  * involves copying the data, which is bad, but this only
	  * occurs for fixed length string attributes, and presumably
	  * these are small. (And netCDF-4 does not create them - it
	  * always uses variable length strings. */
	 if (fixed_len_string)
	 {
	    int i;
	    char *contig_buf, *cur;

	    /* Alloc space for the contiguous memory read. */
	    if (!(contig_buf = malloc(att->len * fixed_size * sizeof(char))))
	       BAIL(NC_ENOMEM);

	    /* Read the fixed-len strings as one big block. */
	    if (H5Aread(attid, att->native_hdf_typeid, contig_buf) < 0) {
          free(contig_buf);
          BAIL(NC_EATTMETA);
        }

	    /* Copy strings, one at a time, into their new home. Alloc
	       space for each string. The user will later free this
	       space with nc_free_string. */
	    cur = contig_buf;
	    for (i = 0; i < att->len; i++)
	    {
          if (!(att->stdata[i] = malloc(fixed_size))) {
            free(contig_buf);
            BAIL(NC_ENOMEM);
          }
           strncpy(att->stdata[i], cur, fixed_size);
	       cur += fixed_size;
	    }

	    /* Free contiguous memory buffer. */
	    free(contig_buf);
	 }
	 else
	 {
	    /* Read variable-length string atts. */
	    if (H5Aread(attid, att->native_hdf_typeid, att->stdata) < 0)
	       BAIL(NC_EATTMETA);
	 }
      }
      else
      {
	 if (!(att->data = malloc((unsigned int)(att->len * type_size))))
	    BAIL(NC_ENOMEM);
	 if (H5Aread(attid, att->native_hdf_typeid, att->data) < 0)
	    BAIL(NC_EATTMETA);
      }
   }

   if (H5Tclose(file_typeid) < 0)
      BAIL(NC_EHDFERR);
   if (H5Sclose(spaceid) < 0)
      return NC_EHDFERR;
#ifdef EXTRA_TESTS
   num_spaces--;
#endif

   return NC_NOERR;

  exit:
   if (H5Tclose(file_typeid) < 0)
      BAIL2(NC_EHDFERR);
   if (spaceid > 0 && H5Sclose(spaceid) < 0)
      BAIL2(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_spaces--;
#endif
   return retval;
}

/* Read information about a user defined type from the HDF5 file, and
 * stash it in the group's list of types. */
static int
read_type(NC_GRP_INFO_T *grp, hid_t hdf_typeid, char *type_name)
{
   NC_TYPE_INFO_T *type;
   H5T_class_t class;
   hid_t native_typeid;
   size_t type_size;
   int retval = NC_NOERR;

   assert(grp && type_name);

   LOG((4, "%s: type_name %s grp->name %s", __func__, type_name, grp->name));

   /* What is the native type for this platform? */
   if ((native_typeid = H5Tget_native_type(hdf_typeid, H5T_DIR_DEFAULT)) < 0)
      return NC_EHDFERR;

   /* What is the size of this type on this platform. */
   if (!(type_size = H5Tget_size(native_typeid)))
      return NC_EHDFERR;
   LOG((5, "type_size %d", type_size));

   /* Add to the list for this new type, and get a local pointer to it. */
   if ((retval = nc4_type_list_add(grp, type_size, type_name, &type)))
      return retval;

   /* Remember common info about this type. */
   type->committed = NC_TRUE;
   type->hdf_typeid = hdf_typeid;
   H5Iinc_ref(type->hdf_typeid);        /* Increment number of objects using ID */
   type->native_hdf_typeid = native_typeid;

   /* What is the class of this type, compound, vlen, etc. */
   if ((class = H5Tget_class(hdf_typeid)) < 0)
      return NC_EHDFERR;
   switch (class)
   {
      case H5T_STRING:
         type->nc_type_class = NC_STRING;
         break;

      case H5T_COMPOUND:
         {
            int nmembers;
            unsigned int m;
	    char* member_name = NULL;
#ifdef JNA
            char jna[1001];
#endif

            type->nc_type_class = NC_COMPOUND;

            if ((nmembers = H5Tget_nmembers(hdf_typeid)) < 0)
               return NC_EHDFERR;
            LOG((5, "compound type has %d members", nmembers));
            for (m = 0; m < nmembers; m++)
            {
               hid_t member_hdf_typeid;
               hid_t member_native_typeid;
               size_t member_offset;
               H5T_class_t mem_class;
               nc_type member_xtype;


               /* Get the typeid and native typeid of this member of the
                * compound type. */
               if ((member_hdf_typeid = H5Tget_member_type(type->native_hdf_typeid, m)) < 0)
                  return NC_EHDFERR;

               if ((member_native_typeid = H5Tget_native_type(member_hdf_typeid, H5T_DIR_DEFAULT)) < 0)
                  return NC_EHDFERR;

               /* Get the name of the member.*/
               member_name = H5Tget_member_name(type->native_hdf_typeid, m);
               if (!member_name || strlen(member_name) > NC_MAX_NAME) {
                  retval = NC_EBADNAME;
		  break;
	       }
#ifdef JNA
	       else {
		strncpy(jna,member_name,1000);
		member_name = jna;
               }
#endif

               /* Offset in bytes on *this* platform. */
               member_offset = H5Tget_member_offset(type->native_hdf_typeid, m);

               /* Get dimensional data if this member is an array of something. */
               if ((mem_class = H5Tget_class(member_hdf_typeid)) < 0)
                  return NC_EHDFERR;
               if (mem_class == H5T_ARRAY)
               {
                  int ndims, dim_size[NC_MAX_VAR_DIMS];
                  hsize_t dims[NC_MAX_VAR_DIMS];
                  int d;

                  if ((ndims = H5Tget_array_ndims(member_hdf_typeid)) < 0) {
                     retval = NC_EHDFERR;
		     break;
		  }
                  if (H5Tget_array_dims(member_hdf_typeid, dims, NULL) != ndims) {
                     retval = NC_EHDFERR;
		     break;
		  }
                  for (d = 0; d < ndims; d++)
                     dim_size[d] = dims[d];

                  /* What is the netCDF typeid of this member? */
                  if ((retval = get_netcdf_type(grp->nc4_info, H5Tget_super(member_hdf_typeid),
                                                &member_xtype)))
		     break;

                  /* Add this member to our list of fields in this compound type. */
                  if ((retval = nc4_field_list_add(&type->u.c.field, type->u.c.num_fields++, member_name,
                                                   member_offset, H5Tget_super(member_hdf_typeid),
                                                   H5Tget_super(member_native_typeid),
                                                   member_xtype, ndims, dim_size)))
                     break;
               }
               else
               {
                  /* What is the netCDF typeid of this member? */
                  if ((retval = get_netcdf_type(grp->nc4_info, member_native_typeid,
                                                &member_xtype)))
                     break;

                  /* Add this member to our list of fields in this compound type. */
                  if ((retval = nc4_field_list_add(&type->u.c.field, type->u.c.num_fields++, member_name,
                                                   member_offset, member_hdf_typeid, member_native_typeid,
                                                   member_xtype, 0, NULL)))
                     break;
               }

	       hdf5free(member_name);
	       member_name = NULL;
            }
	    hdf5free(member_name);
	    member_name = NULL;	    	
	    if(retval) /* error exit from loop */
		return retval;
         }
         break;

      case H5T_VLEN:
         {
            htri_t ret;

            /* For conveninence we allow user to pass vlens of strings
             * with null terminated strings. This means strings are
             * treated slightly differently by the API, although they are
             * really just VLENs of characters. */
            if ((ret = H5Tis_variable_str(hdf_typeid)) < 0)
               return NC_EHDFERR;
            if (ret)
               type->nc_type_class = NC_STRING;
            else
            {
               hid_t base_hdf_typeid;
               nc_type base_nc_type = NC_NAT;

               type->nc_type_class = NC_VLEN;

               /* Find the base type of this vlen (i.e. what is this a
                * vlen of?) */
               if (!(base_hdf_typeid = H5Tget_super(native_typeid)))
                  return NC_EHDFERR;

               /* What size is this type? */
               if (!(type_size = H5Tget_size(base_hdf_typeid)))
                  return NC_EHDFERR;

               /* What is the netcdf corresponding type. */
               if ((retval = get_netcdf_type(grp->nc4_info, base_hdf_typeid,
                                             &base_nc_type)))
                  return retval;
               LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d",
                    base_hdf_typeid, type_size, base_nc_type));

               /* Remember the base types for this vlen */
               type->u.v.base_nc_typeid = base_nc_type;
               type->u.v.base_hdf_typeid = base_hdf_typeid;
            }
         }
         break;

      case H5T_OPAQUE:
         type->nc_type_class = NC_OPAQUE;
         break;

      case H5T_ENUM:
         {
            hid_t base_hdf_typeid;
            nc_type base_nc_type = NC_NAT;
            void *value;
            int i;
	    char *member_name = NULL;
#ifdef JNA
            char jna[1001];
#endif

            type->nc_type_class = NC_ENUM;

            /* Find the base type of this enum (i.e. what is this a
             * enum of?) */
            if (!(base_hdf_typeid = H5Tget_super(hdf_typeid)))
               return NC_EHDFERR;
            /* What size is this type? */
            if (!(type_size = H5Tget_size(base_hdf_typeid)))
               return NC_EHDFERR;
            /* What is the netcdf corresponding type. */
            if ((retval = get_netcdf_type(grp->nc4_info, base_hdf_typeid,
                                          &base_nc_type)))
               return retval;
            LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d",
                 base_hdf_typeid, type_size, base_nc_type));

            /* Remember the base types for this enum */
            type->u.e.base_nc_typeid = base_nc_type;
            type->u.e.base_hdf_typeid = base_hdf_typeid;

            /* Find out how many member are in the enum. */
            if ((type->u.e.num_members = H5Tget_nmembers(hdf_typeid)) < 0)
               return NC_EHDFERR;

            /* Allocate space for one value. */
            if (!(value = calloc(1, type_size)))
               return NC_ENOMEM;

            /* Read each name and value defined in the enum. */
            for (i = 0; i < type->u.e.num_members; i++)
            {

               /* Get the name and value from HDF5. */
               if (!(member_name = H5Tget_member_name(hdf_typeid, i)))
               {
                  retval = NC_EHDFERR;
		  break;
               }
#ifdef JNA
		strncpy(jna,member_name,1000);
		member_name = jna;
#endif

               if (strlen(member_name) > NC_MAX_NAME)
               {
                  retval = NC_EBADNAME;
		  break;
               }
               if (H5Tget_member_value(hdf_typeid, i, value) < 0)
               {
                  retval = NC_EHDFERR;
		  break;
               }

               /* Insert new field into this type's list of fields. */
               if ((retval = nc4_enum_member_add(&type->u.e.enum_member, type->size,
                                                 member_name, value)))
               {
		  break;
               }

	       hdf5free(member_name);
	       member_name = NULL;
            }
	    hdf5free(member_name);
	    member_name = NULL;
        if(value) free(value);
        if(retval) /* error exit from loop */
          return retval;
         }
         break;

      default:
         LOG((0, "unknown class"));
         return NC_EBADCLASS;
   }
   return retval;
}

/* This function is called by read_dataset, (which is called by
 * nc4_rec_read_metadata) when a netCDF variable is found in the
 * file. This function reads in all the metadata about the var,
 * including the attributes. */
static int
read_var(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
         size_t ndims, NC_DIM_INFO_T *dim)
{
   NC_VAR_INFO_T *var = NULL;
   hid_t access_pid = 0;
   int incr_id_rc = 0;          /* Whether the dataset ID's ref count has been incremented */
   int natts, a, d;
   const char** reserved;

   NC_ATT_INFO_T *att;
   att_iter_info att_info;         /* Custom iteration information */
   char att_name[NC_MAX_HDF5_NAME + 1];

#define CD_NELEMS_ZLIB 1
#define CD_NELEMS_SZIP 4
   H5Z_filter_t filter;
   int num_filters;
   unsigned int cd_values[CD_NELEMS_SZIP];
   size_t cd_nelems = CD_NELEMS_SZIP;
   hid_t propid = 0;
   H5D_fill_value_t fill_status;
   H5D_layout_t layout;
   hsize_t chunksize[NC_MAX_VAR_DIMS] = {0};
   int retval = NC_NOERR;
   double rdcc_w0;
   int f;

   assert(obj_name && grp);
   LOG((4, "%s: obj_name %s", __func__, obj_name));

   /* Add a variable to the end of the group's var list. */
   if ((retval = nc4_var_add(&var)))
      BAIL(retval);

   /* Fill in what we already know. */
   var->hdf_datasetid = datasetid;
   H5Iinc_ref(var->hdf_datasetid);      /* Increment number of objects using ID */
   incr_id_rc++;                        /* Indicate that we've incremented the ref. count (for errors) */
   var->varid = grp->nvars++;
   var->created = NC_TRUE;
   var->ndims = ndims;

   /* We need some room to store information about dimensions for this
    * var. */
   if (var->ndims)
   {
      if (!(var->dim = calloc(var->ndims, sizeof(NC_DIM_INFO_T *))))
	 BAIL(NC_ENOMEM);
      if (!(var->dimids = calloc(var->ndims, sizeof(int))))
	 BAIL(NC_ENOMEM);
   }

   /* Get the current chunk cache settings. */
   if ((access_pid = H5Dget_access_plist(datasetid)) < 0)
      BAIL(NC_EVARMETA);
#ifdef EXTRA_TESTS
   num_plists++;
#endif

   /* Learn about current chunk cache settings. */
   if ((H5Pget_chunk_cache(access_pid, &(var->chunk_cache_nelems),
			   &(var->chunk_cache_size), &rdcc_w0)) < 0)
      BAIL(NC_EHDFERR);
   var->chunk_cache_preemption = rdcc_w0;

   /* Check for a weird case: a non-coordinate variable that has the
    * same name as a dimension. It's legal in netcdf, and requires
    * that the HDF5 dataset name be changed. */
   if (strlen(obj_name) > strlen(NON_COORD_PREPEND) &&
         !strncmp(obj_name, NON_COORD_PREPEND, strlen(NON_COORD_PREPEND)))
   {
      /* Allocate space for the name. */
      if (!(var->name = malloc(((strlen(obj_name) - strlen(NON_COORD_PREPEND))+ 1) * sizeof(char))))
         BAIL(NC_ENOMEM);

      strcpy(var->name, &obj_name[strlen(NON_COORD_PREPEND)]);

      /* Allocate space for the HDF5 name. */
      if (!(var->hdf5_name = malloc((strlen(obj_name) + 1) * sizeof(char))))
         BAIL(NC_ENOMEM);

      strcpy(var->hdf5_name, obj_name);
   }
   else
   {
      /* Allocate space for the name. */
      if (!(var->name = malloc((strlen(obj_name) + 1) * sizeof(char))))
         BAIL(NC_ENOMEM);

      strcpy(var->name, obj_name);
   }

   var->hash = hash_fast(var->name, strlen(var->name));
   /* Find out what filters are applied to this HDF5 dataset,
    * fletcher32, deflate, and/or shuffle. All other filters are
    * ignored. */
   if ((propid = H5Dget_create_plist(datasetid)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists++;
#endif /* EXTRA_TESTS */

   /* Get the chunking info for non-scalar vars. */
   if ((layout = H5Pget_layout(propid)) < -1)
      BAIL(NC_EHDFERR);
   if (layout == H5D_CHUNKED)
   {
      if (H5Pget_chunk(propid, NC_MAX_VAR_DIMS, chunksize) < 0)
         BAIL(NC_EHDFERR);
      if (!(var->chunksizes = malloc(var->ndims * sizeof(size_t))))
	 BAIL(NC_ENOMEM);
      for (d = 0; d < var->ndims; d++)
         var->chunksizes[d] = chunksize[d];
   }
   else if (layout == H5D_CONTIGUOUS || layout == H5D_COMPACT)
      var->contiguous = NC_TRUE;

   /* The possible values of filter (which is just an int) can be
    * found in H5Zpublic.h. */
   if ((num_filters = H5Pget_nfilters(propid)) < 0)
      BAIL(NC_EHDFERR);
   for (f = 0; f < num_filters; f++)
   {
      if ((filter = H5Pget_filter2(propid, f, NULL, &cd_nelems,
                                   cd_values, 0, NULL, NULL)) < 0)
         BAIL(NC_EHDFERR);
      switch (filter)
      {
         case H5Z_FILTER_SHUFFLE:
            var->shuffle = NC_TRUE;
            break;

         case H5Z_FILTER_FLETCHER32:
            var->fletcher32 = NC_TRUE;
            break;

         case H5Z_FILTER_DEFLATE:
            var->deflate = NC_TRUE;
            if (cd_nelems != CD_NELEMS_ZLIB || cd_values[0] > MAX_DEFLATE_LEVEL)
               BAIL(NC_EHDFERR);
            var->deflate_level = cd_values[0];
            break;

         case H5Z_FILTER_SZIP:
            var->szip = NC_TRUE;
            if (cd_nelems != CD_NELEMS_SZIP)
               BAIL(NC_EHDFERR);
	    var->options_mask = cd_values[0];
            var->pixels_per_block = cd_values[1];
            break;

         default:
            LOG((1, "Yikes! Unknown filter type found on dataset!"));
            break;
      }
   }

   /* Learn all about the type of this variable. */
   if ((retval = get_type_info2(grp->nc4_info, datasetid,
				&var->type_info)))
      BAIL(retval);

   /* Indicate that the variable has a pointer to the type */
   var->type_info->rc++;

   /* Is there a fill value associated with this dataset? */
   if (H5Pfill_value_defined(propid, &fill_status) < 0)
      BAIL(NC_EHDFERR);

   /* Get the fill value, if there is one defined. */
   if (fill_status == H5D_FILL_VALUE_USER_DEFINED)
   {
      /* Allocate space to hold the fill value. */
      if (!var->fill_value)
      {
	 if (var->type_info->nc_type_class == NC_VLEN)
	 {
	    if (!(var->fill_value = malloc(sizeof(nc_vlen_t))))
	       BAIL(NC_ENOMEM);
	 }
	 else if (var->type_info->nc_type_class == NC_STRING)
	 {
	    if (!(var->fill_value = malloc(sizeof(char *))))
	       BAIL(NC_ENOMEM);
	 }
	 else
	 {
            assert(var->type_info->size);
	    if (!(var->fill_value = malloc(var->type_info->size)))
	       BAIL(NC_ENOMEM);
	 }
      }

      /* Get the fill value from the HDF5 property lust. */
      if (H5Pget_fill_value(propid, var->type_info->native_hdf_typeid,
			    var->fill_value) < 0)
         BAIL(NC_EHDFERR);
   }
   else
      var->no_fill = NC_TRUE;

   /* If it's a scale, mark it as such. */
   if (dim)
   {
      assert(ndims);
      var->dimscale = NC_TRUE;
      if (var->ndims > 1)
      {
	 if ((retval = read_coord_dimids(grp, var)))
	    BAIL(retval);
      }
      else
      {
         /* sanity check */
         assert(0 == strcmp(var->name, dim->name));

         var->dimids[0] = dim->dimid;
	 var->dim[0] = dim;
      }
      dim->coord_var = var;
   }
   /* If this is not a scale, but has scales, iterate
    * through them. (i.e. this is a variable that is not a
    * coordinate variable) */
   else
   {
      int num_scales = 0;

      /* Find out how many scales are attached to this
       * dataset. H5DSget_num_scales returns an error if there are no
       * scales, so convert a negative return value to zero. */
      num_scales = H5DSget_num_scales(datasetid, 0);
      if (num_scales < 0)
         num_scales = 0;

      if (num_scales && ndims)
      {
         /* Allocate space to remember whether the dimscale has been attached
          * for each dimension. */
         if (NULL == (var->dimscale_attached = calloc(ndims, sizeof(nc_bool_t))))
            BAIL(NC_ENOMEM);

         /* Store id information allowing us to match hdf5
          * dimscales to netcdf dimensions. */
         if (NULL == (var->dimscale_hdf5_objids = malloc(ndims * sizeof(struct hdf5_objid))))
            BAIL(NC_ENOMEM);
         for (d = 0; d < var->ndims; d++)
         {
            if (H5DSiterate_scales(var->hdf_datasetid, d, NULL, dimscale_visitor,
                                   &(var->dimscale_hdf5_objids[d])) < 0)
               BAIL(NC_EHDFERR);
            var->dimscale_attached[d] = NC_TRUE;
         }
      }
   }

   /* Now read all the attributes of this variable, ignoring the
      ones that hold HDF5 dimension scale information. */

   att_info.var = var;
   att_info.grp = grp;

   if ((H5Aiterate2(var->hdf_datasetid, H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, att_read_var_callbk, &att_info)) < 0)
     BAIL(NC_EATTMETA);

   nc4_vararray_add(grp, var);

   /* Is this a deflated variable with a chunksize greater than the
    * current cache size? */
   if ((retval = nc4_adjust_var_cache(grp, var)))
      BAIL(retval);

exit:
   if (retval)
   {
       if (incr_id_rc && H5Idec_ref(datasetid) < 0)
          BAIL2(NC_EHDFERR);
       if (var && nc4_var_del(var))
          BAIL2(NC_EHDFERR);
   }
   if (access_pid && H5Pclose(access_pid) < 0)
      BAIL2(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (propid > 0 && H5Pclose(propid) < 0)
      BAIL2(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   return retval;
}

/* This function is called by nc4_rec_read_metadata to read all the
 * group level attributes (the NC_GLOBAL atts for this group). */
static int
read_grp_atts(NC_GRP_INFO_T *grp)
{
   hid_t attid = -1;
   hsize_t num_obj, i;
   NC_ATT_INFO_T *att;
   NC_TYPE_INFO_T *type;
   char obj_name[NC_MAX_HDF5_NAME + 1];
   int max_len;
   int retval = NC_NOERR;
   int hidden = 0;

   num_obj = H5Aget_num_attrs(grp->hdf_grpid);
   for (i = 0; i < num_obj; i++)
   {
      if ((attid = H5Aopen_idx(grp->hdf_grpid, (unsigned int)i)) < 0)
         BAIL(NC_EATTMETA);
      if (H5Aget_name(attid, NC_MAX_NAME + 1, obj_name) < 0)
         BAIL(NC_EATTMETA);
      LOG((3, "reading attribute of _netCDF group, named %s", obj_name));

      /* See if this a hidden, global attribute */
      if(grp->nc4_info->root_grp == grp) {
	const char** reserved = NC_RESERVED_ATT_LIST;
	hidden = 0;
	for(;*reserved;reserved++) {
	    if(strcmp(*reserved,obj_name)==0) {
		hidden = 1;
		break;
	    }
	}
      }

      /* This may be an attribute telling us that strict netcdf-3
       * rules are in effect. If so, we will make note of the fact,
       * but not add this attribute to the metadata. It's not a user
       * attribute, but an internal netcdf-4 one. */
      if(strcmp(obj_name, NC3_STRICT_ATT_NAME)==0)
             grp->nc4_info->cmode |= NC_CLASSIC_MODEL;
      else if(!hidden) {
         /* Add an att struct at the end of the list, and then go to it. */
         if ((retval = nc4_att_list_add(&grp->att, &att)))
            BAIL(retval);

	 /* Add the info about this attribute. */
	 max_len = strlen(obj_name) > NC_MAX_NAME ? NC_MAX_NAME : strlen(obj_name);
	 if (!(att->name = malloc((max_len + 1) * sizeof(char))))
	    BAIL(NC_ENOMEM);
         strncpy(att->name, obj_name, max_len);
         att->name[max_len] = 0;
         att->attnum = grp->natts++;
         retval = read_hdf5_att(grp, attid, att);
         if(retval == NC_EBADTYPID) {
               if((retval = nc4_att_list_del(&grp->att, att)))
                  BAIL(retval);
	 } else if(retval) {
               BAIL(retval);
         } else {
             att->created = NC_TRUE;
	     if ((retval = nc4_find_type(grp->nc4_info, att->nc_typeid, &type)))
            	BAIL(retval);
	 }
      }
      /* Unconditionally close the open attribute */
      H5Aclose(attid);
      attid = -1;
   }

exit:
   if (attid > 0) {
	if(H5Aclose(attid) < 0)
            BAIL2(NC_EHDFERR);
   }
   return retval;
}

/* This function is called when nc4_rec_read_metadata encounters an HDF5
 * dataset when reading a file. */
static int
read_dataset(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
    const H5G_stat_t *statbuf)
{
   NC_DIM_INFO_T *dim = NULL;   /* Dimension created for scales */
   hid_t spaceid = 0;
   int ndims;
   htri_t is_scale;
   int retval = NC_NOERR;

   /* Get the dimension information for this dataset. */
   if ((spaceid = H5Dget_space(datasetid)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_spaces++;
#endif
   if ((ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
      BAIL(NC_EHDFERR);

   /* Is this a dimscale? */
   if ((is_scale = H5DSis_scale(datasetid)) < 0)
      BAIL(NC_EHDFERR);
   if (is_scale)
   {
      hsize_t dims[H5S_MAX_RANK];
      hsize_t max_dims[H5S_MAX_RANK];

      /* Query the scale's size & max. size */
      if (H5Sget_simple_extent_dims(spaceid, dims, max_dims) < 0)
         BAIL(NC_EHDFERR);

      /* Read the scale information. */
      if ((retval = read_scale(grp, datasetid, obj_name, statbuf, dims[0],
                               max_dims[0], &dim)))
         BAIL(retval);
   }

   /* Add a var to the linked list, and get its metadata,
    * unless this is one of those funny dimscales that are a
    * dimension in netCDF but not a variable. (Spooky!) */
   if (NULL == dim || (dim && !dim->hdf_dimscaleid))
      if ((retval = read_var(grp, datasetid, obj_name, ndims, dim)))
	 BAIL(retval);

exit:
   if (spaceid && H5Sclose(spaceid) <0)
      BAIL2(retval);
#ifdef EXTRA_TESTS
   num_spaces--;
#endif

   return retval;
}

static int
nc4_rec_read_metadata_cb_list_add(NC4_rec_read_metadata_obj_info_t **head,
                  NC4_rec_read_metadata_obj_info_t **tail,
                  const NC4_rec_read_metadata_obj_info_t *oinfo)
{
   NC4_rec_read_metadata_obj_info_t *new_oinfo;    /* Pointer to info for object */

   /* Allocate memory for the object's info */
   if (!(new_oinfo = calloc(1, sizeof(*new_oinfo))))
      return NC_ENOMEM;

   /* Make a copy of the object's info */
   memcpy(new_oinfo, oinfo, sizeof(*oinfo));

   if (*tail)
   {
       assert(*head);
       (*tail)->next = new_oinfo;
       *tail = new_oinfo;
   }
   else
   {
       assert(NULL == *head);
       *head = *tail = new_oinfo;
   }

   return (NC_NOERR);
}

static int
nc4_rec_read_metadata_cb(hid_t grpid, const char *name, const H5L_info_t *info,
		      void *_op_data)
{
    NC4_rec_read_metadata_ud_t *udata = (NC4_rec_read_metadata_ud_t *)_op_data; /* Pointer to user data for callback */
    NC4_rec_read_metadata_obj_info_t oinfo;    /* Pointer to info for object */
    int retval = H5_ITER_CONT;

   /* Reset the memory for the object's info */
   memset(&oinfo, 0, sizeof(oinfo));

   /* Open this critter. */
   if ((oinfo.oid = H5Oopen(grpid, name, H5P_DEFAULT)) < 0)
      BAIL(H5_ITER_ERROR);

   /* Get info about the object.*/
   if (H5Gget_objinfo(oinfo.oid, ".", 1, &oinfo.statbuf) < 0)
      BAIL(H5_ITER_ERROR);

   strncpy(oinfo.oname, name, NC_MAX_NAME);

   /* Add object to list, for later */
   switch(oinfo.statbuf.type)
   {
      case H5G_GROUP:
         LOG((3, "found group %s", oinfo.oname));

         /* Defer descending into child group immediately, so that the types
          *     in the current group can be processed and be ready for use by
          *     vars in the child group(s).
          */
         if (nc4_rec_read_metadata_cb_list_add(&udata->grps_head, &udata->grps_tail, &oinfo))
             BAIL(H5_ITER_ERROR);
         break;

      case H5G_DATASET:
         LOG((3, "found dataset %s", oinfo.oname));

         /* Learn all about this dataset, which may be a dimscale
          * (i.e. dimension metadata), or real data. */
         if ((retval = read_dataset(udata->grp, oinfo.oid, oinfo.oname, &oinfo.statbuf)))
         {
            /* Allow NC_EBADTYPID to transparently skip over datasets
             *  which have a datatype that netCDF-4 doesn't undertand
             *  (currently), but break out of iteration for other
             *  errors.
             */
            if(NC_EBADTYPID != retval)
               BAIL(H5_ITER_ERROR);
            else
               retval = H5_ITER_CONT;
         }

         /* Close the object */
         if (H5Oclose(oinfo.oid) < 0)
	    BAIL(H5_ITER_ERROR);
         break;

      case H5G_TYPE:
         LOG((3, "found datatype %s", oinfo.oname));

         /* Process the named datatype */
         if (read_type(udata->grp, oinfo.oid, oinfo.oname))
            BAIL(H5_ITER_ERROR);

         /* Close the object */
         if (H5Oclose(oinfo.oid) < 0)
	    BAIL(H5_ITER_ERROR);
         break;

      default:
         LOG((0, "Unknown object class %d in %s!", oinfo.statbuf.type, __func__));
         BAIL(H5_ITER_ERROR);
    }

exit:
   if (retval)
   {
      if (oinfo.oid > 0 && H5Oclose(oinfo.oid) < 0)
         BAIL2(H5_ITER_ERROR);
   }

   return (retval);
}

/* This is the main function to recursively read all the metadata for the file. */
/* The links in the 'grp' are iterated over and added to the file's metadata
 *      information.  Note that child groups are not immediately processed, but
 *      are deferred until all the other links in the group are handled (so that
 *      vars in the child groups are guaranteed to have types that they use in
 *      a parent group in place).
 */
static int
nc4_rec_read_metadata(NC_GRP_INFO_T *grp)
{
    NC4_rec_read_metadata_ud_t udata;   /* User data for iteration */
    NC4_rec_read_metadata_obj_info_t *oinfo;    /* Pointer to info for object */
    hsize_t idx=0;
    hid_t pid = 0;
    unsigned crt_order_flags = 0;
    H5_index_t iter_index;
    int i, retval = NC_NOERR; /* everything worked! */

    assert(grp && grp->name);
    LOG((3, "%s: grp->name %s", __func__, grp->name));

    /* Portably initialize user data for later */
    memset(&udata, 0, sizeof(udata));

    /* Open this HDF5 group and retain its grpid. It will remain open
     * with HDF5 until this file is nc_closed. */
    if (!grp->hdf_grpid)
    {
        if (grp->parent)
        {
            if ((grp->hdf_grpid = H5Gopen2(grp->parent->hdf_grpid,
					grp->name, H5P_DEFAULT)) < 0)
                BAIL(NC_EHDFERR);
        }
        else
        {
	    if ((grp->hdf_grpid = H5Gopen2(grp->nc4_info->hdfid,
					   "/", H5P_DEFAULT)) < 0)
                BAIL(NC_EHDFERR);
        }
    }
    assert(grp->hdf_grpid > 0);

    /* Get the group creation flags, to check for creation ordering */
    pid = H5Gget_create_plist(grp->hdf_grpid);
    H5Pget_link_creation_order(pid, &crt_order_flags);
    if (H5Pclose(pid) < 0)
	BAIL(NC_EHDFERR);

    /* Set the iteration index to use */
    if (crt_order_flags & H5P_CRT_ORDER_TRACKED)
        iter_index = H5_INDEX_CRT_ORDER;
    else
    {
        NC_HDF5_FILE_INFO_T *h5 = grp->nc4_info;

        /* Without creation ordering, file must be read-only. */
        if (!h5->no_write)
            BAIL(NC_ECANTWRITE);

        iter_index = H5_INDEX_NAME;
    }

    /* Set user data for iteration */
    udata.grp = grp;

    /* Iterate over links in this group, building lists for the types,
     *  datasets and groups encountered
     */
    if (H5Literate(grp->hdf_grpid, iter_index, H5_ITER_INC, &idx,
            nc4_rec_read_metadata_cb, (void *)&udata) < 0)
	BAIL(NC_EHDFERR);

    /* Process the child groups found */
    /* (Deferred until now, so that the types in the current group get
     *  processed and are available for vars in the child group(s).)
     */
    for (oinfo = udata.grps_head; oinfo; oinfo = udata.grps_head)
    {
        NC_GRP_INFO_T *child_grp;
        NC_HDF5_FILE_INFO_T *h5 = grp->nc4_info;

        /* Add group to file's hierarchy */
        if ((retval = nc4_grp_list_add(&(grp->children), h5->next_nc_grpid++,
                        grp, grp->nc4_info->controller, oinfo->oname, &child_grp)))
            BAIL(retval);

        /* Recursively read the child group's metadata */
        if ((retval = nc4_rec_read_metadata(child_grp)))
            BAIL(retval);

        /* Close the object */
        if (H5Oclose(oinfo->oid) < 0)
	    BAIL(NC_EHDFERR);

        /* Advance to next node, free current node */
        udata.grps_head = oinfo->next;
        free(oinfo);
    }

    /* Scan the group for global (i.e. group-level) attributes. */
    if ((retval = read_grp_atts(grp)))
	BAIL(retval);

   /* when exiting define mode, mark all variable written */
   for (i=0; i<grp->vars.nelems; i++)
      grp->vars.value[i]->written_to = NC_TRUE;

exit:
    /* Clean up local information on error, if anything remains */
    if (retval)
    {
        for (oinfo = udata.grps_head; oinfo; oinfo = udata.grps_head)
        {
            /* Close the object */
            if (H5Oclose(oinfo->oid) < 0)
                BAIL2(NC_EHDFERR);

            /* Advance to next node, free current node */
            udata.grps_head = oinfo->next;
            free(oinfo);
        }
    }

    return retval;
}

/* Open a netcdf-4 file. Things have already been kicked off in
 * ncfunc.c in nc_open, but here the netCDF-4 part of opening a file
 * is handled. */
static int
nc4_open_file(const char *path, int mode, void* parameters, NC *nc)
{
   hid_t fapl_id = H5P_DEFAULT;
   unsigned flags = (mode & NC_WRITE) ?
      H5F_ACC_RDWR : H5F_ACC_RDONLY;
   int retval;
   NC_HDF5_FILE_INFO_T* nc4_info = NULL;
   int inmemory = ((mode & NC_INMEMORY) == NC_INMEMORY);
   NC_MEM_INFO* meminfo = (NC_MEM_INFO*)parameters;
#ifdef USE_PARALLEL4
   NC_MPI_INFO* mpiinfo = (NC_MPI_INFO*)parameters;
   int comm_duped = 0;          /* Whether the MPI Communicator was duplicated */
   int info_duped = 0;          /* Whether the MPI Info object was duplicated */
#endif /* !USE_PARALLEL4 */

   LOG((3, "%s: path %s mode %d", __func__, path, mode));
   assert(path && nc);

   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, mode)))
      BAIL(retval);
   nc4_info = NC4_DATA(nc);
   assert(nc4_info && nc4_info->root_grp);

   /* Need this access plist to control how HDF5 handles open objects
    * on file close. (Setting H5F_CLOSE_SEMI will cause H5Fclose to
    * fail if there are any open objects in the file. */
   if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
      BAIL(NC_EHDFERR);

#ifdef EXTRA_TESTS
   num_plists++;
#endif
#ifdef EXTRA_TESTS
   if (H5Pset_fclose_degree(fapl_id, H5F_CLOSE_SEMI))
      BAIL(NC_EHDFERR);
#else
   if (H5Pset_fclose_degree(fapl_id, H5F_CLOSE_STRONG))
      BAIL(NC_EHDFERR);
#endif

#ifdef USE_PARALLEL4
   /* If this is a parallel file create, set up the file creation
      property list. */
   if (mode & NC_MPIIO || mode & NC_MPIPOSIX)
   {
      nc4_info->parallel = NC_TRUE;
      if (mode & NC_MPIIO)  /* MPI/IO */
      {
	 LOG((4, "opening parallel file with MPI/IO"));
	 if (H5Pset_fapl_mpio(fapl_id, mpiinfo->comm, mpiinfo->info) < 0)
	    BAIL(NC_EPARINIT);
      }
#ifdef USE_PARALLEL_POSIX
      else /* MPI/POSIX */
      {
	 LOG((4, "opening parallel file with MPI/posix"));
	 if (H5Pset_fapl_mpiposix(fapl_id, mpiinfo->comm, 0) < 0)
	    BAIL(NC_EPARINIT);
      }
#else /* USE_PARALLEL_POSIX */
      /* Should not happen! Code in NC4_create/NC4_open should alias the
       *        NC_MPIPOSIX flag to NC_MPIIO, if the MPI-POSIX VFD is not
       *        available in HDF5. -QAK
       */
      else /* MPI/POSIX */
         BAIL(NC_EPARINIT);
#endif /* USE_PARALLEL_POSIX */

      /* Keep copies of the MPI Comm & Info objects */
      if (MPI_SUCCESS != MPI_Comm_dup(mpiinfo->comm, &nc4_info->comm))
         BAIL(NC_EMPI);
      comm_duped++;
      if (MPI_INFO_NULL != mpiinfo->info)
      {
         if (MPI_SUCCESS != MPI_Info_dup(mpiinfo->info, &nc4_info->info))
            BAIL(NC_EMPI);
         info_duped++;
      }
      else
      {
         /* No dup, just copy it. */
         nc4_info->info = mpiinfo->info;
      }
   }
#else /* only set cache for non-parallel. */
   if (H5Pset_cache(fapl_id, 0, nc4_chunk_cache_nelems, nc4_chunk_cache_size,
		    nc4_chunk_cache_preemption) < 0)
      BAIL(NC_EHDFERR);
   LOG((4, "%s: set HDF raw chunk cache to size %d nelems %d preemption %f",
	__func__, nc4_chunk_cache_size, nc4_chunk_cache_nelems, nc4_chunk_cache_preemption));
#endif /* USE_PARALLEL4 */

   /* The NetCDF-3.x prototype contains an mode option NC_SHARE for
      multiple processes accessing the dataset concurrently.  As there
      is no HDF5 equivalent, NC_SHARE is treated as NC_NOWRITE. */
#ifdef HDF5_HAS_COLL_METADATA_OPS
   H5Pset_all_coll_metadata_ops(fapl_id, 1 );
#endif
   if(inmemory) {
       if((nc4_info->hdfid = H5LTopen_file_image(meminfo->memory,meminfo->size,
			H5LT_FILE_IMAGE_DONT_COPY|H5LT_FILE_IMAGE_DONT_RELEASE
			)) < 0)
           BAIL(NC_EHDFERR);
       nc4_info->no_write = NC_TRUE;
   } else if ((nc4_info->hdfid = H5Fopen(path, flags, fapl_id)) < 0)
      BAIL(NC_EHDFERR);

   /* Does the mode specify that this file is read-only? */
   if ((mode & NC_WRITE) == 0)
      nc4_info->no_write = NC_TRUE;

   /* Now read in all the metadata. Some types and dimscale
    * information may be difficult to resolve here, if, for example, a
    * dataset of user-defined type is encountered before the
    * definition of that type. */
   if ((retval = nc4_rec_read_metadata(nc4_info->root_grp)))
      BAIL(retval);

   /* Now figure out which netCDF dims are indicated by the dimscale
    * information. */
   if ((retval = nc4_rec_match_dimscales(nc4_info->root_grp)))
      BAIL(retval);

#ifdef LOGGING
   /* This will print out the names, types, lens, etc of the vars and
      atts in the file, if the logging level is 2 or greater. */
   log_metadata_nc(nc);
#endif

   /* Close the property list. */
   if (H5Pclose(fapl_id) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
#endif

   NC4_get_fileinfo(nc4_info,NULL);

   return NC_NOERR;

exit:
#ifdef USE_PARALLEL4
   if (comm_duped) MPI_Comm_free(&nc4_info->comm);
   if (info_duped) MPI_Info_free(&nc4_info->info);
#endif
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (fapl_id != H5P_DEFAULT) H5Pclose(fapl_id);
   if (!nc4_info) return retval;
   close_netcdf4_file(nc4_info,1); /*  treat like abort*/
   return retval;
}

#ifdef USE_HDF4
/*! Given an HDF4 type, set a pointer to netcdf type.
 *
 * Given an HDF4 type, set a pointer to a netcdf type.
 * See http://www.hdfgroup.org/training/HDFtraining/UsersGuide/Fundmtls.fm3.html
 * for more information re: HDF4 types.
 *
 * \param NC_HDF5_FILE_INFO_T* Pointer to h5
 * \param int32 TypeID for hdf4 datatype.
 * \param nc_type* Pointer to netcdf type, where result will be stored.
 * \param NC_TYPE_INFO_T* (Optiona) Type info for the variable.
 * \return Error code, 0 on success.
 *
 */
static int
get_netcdf_type_from_hdf4(NC_HDF5_FILE_INFO_T *h5, int32 hdf4_typeid,
			  nc_type *xtype, NC_TYPE_INFO_T *type_info)
{
   int t = 0;

   /* Added this variable in the course of fixing NCF-332.
    * Prior to the fix, all data types were assigned
    * NC_ENDIAN_BIG, so I am preserving that here for now.
    * Not sure why it wouldn't be NC_ENDIAN_NATIVE, although
    * I can hazard a guess or two.
    */

   int endianness = NC_ENDIAN_BIG;
   assert(h5 && xtype);

   switch(hdf4_typeid)
   {
   case DFNT_CHAR:
       *xtype = NC_CHAR;
       t = 0;
       break;
   case DFNT_UCHAR:
   case DFNT_UINT8:
       *xtype = NC_UBYTE;
       t = 6;
       break;
   case DFNT_LUINT8:
       *xtype = NC_UBYTE;
       t = 6;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_INT8:
       *xtype = NC_BYTE;
       t = 1;
       break;
   case DFNT_LINT8:
       *xtype = NC_BYTE;
       t = 1;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_INT16:
       *xtype = NC_SHORT;
       t = 2;
       break;
   case DFNT_LINT16:
       *xtype = NC_SHORT;
       t = 2;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_UINT16:
       *xtype = NC_USHORT;
       t = 7;
       break;
   case DFNT_LUINT16:
       *xtype = NC_USHORT;
       t = 7;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_INT32:
       *xtype = NC_INT;
       t = 3;
       break;
   case DFNT_LINT32:
        *xtype = NC_INT;
       t = 3;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_UINT32:
       *xtype = NC_UINT;
       t = 8;
       break;
   case DFNT_LUINT32:
        *xtype = NC_UINT;
       t = 8;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_FLOAT32:
       *xtype = NC_FLOAT;
       t = 4;
       break;
   case DFNT_LFLOAT32:
       *xtype = NC_FLOAT;
       t = 4;
       endianness = NC_ENDIAN_LITTLE;
       break;
   case DFNT_FLOAT64:
       *xtype = NC_DOUBLE;
       t = 5;
       break;
   case DFNT_LFLOAT64:
       *xtype = NC_DOUBLE;
       t = 5;
       endianness = NC_ENDIAN_LITTLE;
       break;
   default:
       *xtype = NC_NAT;
       return NC_EBADTYPID;
   }

   if (type_info)
   {
      if (hdf4_typeid == DFNT_FLOAT32)
	 type_info->nc_type_class = NC_FLOAT;
      else if (hdf4_typeid == DFNT_FLOAT64)
	 type_info->nc_type_class = NC_DOUBLE;
      else if (hdf4_typeid == DFNT_CHAR)
	 type_info->nc_type_class = NC_STRING;
      else
	 type_info->nc_type_class = NC_INT;
      type_info->endianness = endianness;
      type_info->nc_typeid = *xtype;
      type_info->size = nc_type_size_g[t];
      if (!(type_info->name = strdup(nc_type_name_g[t])))
        return NC_ENOMEM;
   }

   return NC_NOERR;
}

/* Open a HDF4 file. Things have already been kicked off in nc_open,
 * but here the netCDF-4 part of opening a file is handled. */
static int
nc4_open_hdf4_file(const char *path, int mode, NC *nc)
{
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC_ATT_INFO_T *att;
   int32 num_datasets, num_gatts;
   int32 rank;
   int v, d, a;
   int retval;
   NC_HDF5_FILE_INFO_T* nc4_info = NULL;

   LOG((3, "%s: path %s mode %d", __func__, path, mode));
   assert(path && nc);

   /* Must be read-only access to hdf4 files. */
   if (mode & NC_WRITE)
      return NC_EINVAL;

   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, mode)))
      return retval;
   nc4_info = NC4_DATA(nc);
   assert(nc4_info && nc4_info->root_grp);
   h5 = nc4_info;
   h5->hdf4 = NC_TRUE;
   grp = h5->root_grp;
   h5->no_write = NC_TRUE;

   /* Open the file and initialize SD interface. */
   if ((h5->sdid = SDstart(path, DFACC_READ)) == FAIL)
      return NC_EHDFERR;

   /* Learn how many datasets and global atts we have. */
   if (SDfileinfo(h5->sdid, &num_datasets, &num_gatts))
      return NC_EHDFERR;

   /* Read the atts. */
   for (a = 0; a < num_gatts; a++)
   {
      int32 att_data_type, att_count;
      size_t att_type_size;

      /* Add to the end of the list of atts for this var. */
      if ((retval = nc4_att_list_add(&h5->root_grp->att, &att)))
	 return retval;
      att->attnum = grp->natts++;
      att->created = NC_TRUE;

      /* Learn about this attribute. */
      if (!(att->name = malloc(NC_MAX_HDF4_NAME * sizeof(char))))
	 return NC_ENOMEM;
      if (SDattrinfo(h5->sdid, a, att->name, &att_data_type, &att_count))
	 return NC_EATTMETA;
      if ((retval = get_netcdf_type_from_hdf4(h5, att_data_type,
					      &att->nc_typeid, NULL)))
	 return retval;
      att->len = att_count;

      /* Allocate memory to hold the data. */
      if ((retval = nc4_get_typelen_mem(h5, att->nc_typeid, 0, &att_type_size)))
	 return retval;
      if (!(att->data = malloc(att_type_size * att->len)))
	 return NC_ENOMEM;

      /* Read the data. */
      if (SDreadattr(h5->sdid, a, att->data))
	 return NC_EHDFERR;
   }

   /* Read each dataset. */
   for (v = 0; v < num_datasets; v++)
   {
      NC_VAR_INFO_T *var;
      int32 data_type, num_atts;
      /* Problem: Number of dims is returned by the call that requires
	 a pre-allocated array, 'dimsize'.
       From SDS_SD website:
       http://www.hdfgroup.org/training/HDFtraining/UsersGuide/SDS_SD.fm3.html
       The maximum rank is 32, or MAX_VAR_DIMS (as defined in netcdf.h).

       int32 dimsize[MAX_VAR_DIMS];
      */
      int32 *dimsize = NULL;
      size_t var_type_size;
      int a;

      /* Add a variable. */
      if ((retval = nc4_var_add(&var)))
	return retval;

      var->varid = grp->nvars++;
      var->created = NC_TRUE;
      var->written_to = NC_TRUE;

      nc4_vararray_add(grp, var);

      /* Open this dataset in HDF4 file. */
      if ((var->sdsid = SDselect(h5->sdid, v)) == FAIL)
	return NC_EVARMETA;

      /* Get shape, name, type, and attribute info about this dataset. */
      if (!(var->name = malloc(NC_MAX_HDF4_NAME + 1)))
	return NC_ENOMEM;

      /* Invoke SDgetInfo with null dimsize to get rank. */
      if (SDgetinfo(var->sdsid, var->name, &rank, NULL, &data_type, &num_atts))
	return NC_EVARMETA;

      var->hash = hash_fast(var->name, strlen(var->name));

      if(!(dimsize = (int32*)malloc(sizeof(int32)*rank)))
	return NC_ENOMEM;

      if (SDgetinfo(var->sdsid, var->name, &rank, dimsize, &data_type, &num_atts)) {
	if(dimsize) free(dimsize);
	return NC_EVARMETA;
      }

      var->ndims = rank;
      var->hdf4_data_type = data_type;

      /* Fill special type_info struct for variable type information. */
      if (!(var->type_info = calloc(1, sizeof(NC_TYPE_INFO_T)))) {
	if(dimsize) free(dimsize);
	return NC_ENOMEM;
      }

      if ((retval = get_netcdf_type_from_hdf4(h5, data_type, &var->type_info->nc_typeid, var->type_info))) {
	if(dimsize) free(dimsize);
	return retval;
      }

      /* Indicate that the variable has a pointer to the type */
      var->type_info->rc++;

      if ((retval = nc4_get_typelen_mem(h5, var->type_info->nc_typeid, 0, &var_type_size))) {
	if(dimsize) free(dimsize);
	return retval;
      }

      var->type_info->size = var_type_size;
      LOG((3, "reading HDF4 dataset %s, rank %d netCDF type %d", var->name,
	   rank, var->type_info->nc_typeid));

      /* Get the fill value. */
      if (!(var->fill_value = malloc(var_type_size))) {
	if(dimsize) free(dimsize);
	return NC_ENOMEM;
      }

      if (SDgetfillvalue(var->sdsid, var->fill_value))
      {
	 /* Whoops! No fill value! */
	 free(var->fill_value);
	 var->fill_value = NULL;
      }

      /* Allocate storage for dimension info in this variable. */
      if (var->ndims)
      {
	if (!(var->dim = malloc(sizeof(NC_DIM_INFO_T *) * var->ndims))) {
	  if(dimsize) free(dimsize);
	  return NC_ENOMEM;
	}

	if (!(var->dimids = malloc(sizeof(int) * var->ndims))) {
	  if(dimsize) free(dimsize);
	  return NC_ENOMEM;
	}
      }


      /* Find its dimensions. */
      for (d = 0; d < var->ndims; d++)
      {
	 int32 dimid, dim_len, dim_data_type, dim_num_attrs;
	 char dim_name[NC_MAX_NAME + 1];
	 NC_DIM_INFO_T *dim;

	 if ((dimid = SDgetdimid(var->sdsid, d)) == FAIL) {
	   if(dimsize) free(dimsize);
	   return NC_EDIMMETA;
	 }
	 if (SDdiminfo(dimid, dim_name, &dim_len, &dim_data_type,
		       &dim_num_attrs))
	   {
	     if(dimsize) free(dimsize);
	     return NC_EDIMMETA;
	   }

	 /* Do we already have this dimension? HDF4 explicitly uses
	  * the name to tell. */
	 for (dim = grp->dim; dim; dim = dim->l.next)
	    if (!strcmp(dim->name, dim_name))
	       break;

	 /* If we didn't find this dimension, add one. */
	 if (!dim)
	 {
	    LOG((4, "adding dimension %s for HDF4 dataset %s",
		 dim_name, var->name));
	    if ((retval = nc4_dim_list_add(&grp->dim, &dim)))
	       return retval;
	    dim->dimid = grp->nc4_info->next_dimid++;
	    if (strlen(dim_name) > NC_MAX_HDF4_NAME)
	       return NC_EMAXNAME;
	    if (!(dim->name = strdup(dim_name)))
	       return NC_ENOMEM;
	    if (dim_len)
	       dim->len = dim_len;
	    else
	       dim->len = *dimsize;
	    dim->hash = hash_fast(dim_name, strlen(dim_name));
	 }

	 /* Tell the variable the id of this dimension. */
	 var->dimids[d] = dim->dimid;
	 var->dim[d] = dim;
      }

      /* Read the atts. */
      for (a = 0; a < num_atts; a++)
      {
	 int32 att_data_type, att_count;
	 size_t att_type_size;

	 /* Add to the end of the list of atts for this var. */
         if ((retval = nc4_att_list_add(&var->att, &att))) {
	   if(dimsize) free(dimsize);
	   return retval;
	 }
	 att->attnum = var->natts++;
	 att->created = NC_TRUE;

	 /* Learn about this attribute. */
	 if (!(att->name = malloc(NC_MAX_HDF4_NAME * sizeof(char)))) {
	   if(dimsize) free(dimsize);
	   return NC_ENOMEM;
	 }
	 if (SDattrinfo(var->sdsid, a, att->name, &att_data_type, &att_count)) {
	   if(dimsize) free(dimsize);
	    return NC_EATTMETA;
	 }
	 if ((retval = get_netcdf_type_from_hdf4(h5, att_data_type,
						 &att->nc_typeid, NULL))) {
	   if(dimsize) free(dimsize);
	   return retval;
	 }

	 att->len = att_count;

	 /* Allocate memory to hold the data. */
	 if ((retval = nc4_get_typelen_mem(h5, att->nc_typeid, 0, &att_type_size))) {
	   if(dimsize) free(dimsize);
	   return retval;
	 }
	 if (!(att->data = malloc(att_type_size * att->len))) {
	   if(dimsize) free(dimsize);
	   return NC_ENOMEM;
	 }

	 /* Read the data. */
	 if (SDreadattr(var->sdsid, a, att->data)) {
	   if(dimsize) free(dimsize);
	   return NC_EHDFERR;
	 }
      }
      if(dimsize) free(dimsize);

      {
        /* HDF4 files can be chunked */
	HDF_CHUNK_DEF chunkdefs;
	int flag;
        if(!SDgetchunkinfo(var->sdsid, &chunkdefs, &flag)) {
	    if(flag == HDF_NONE)
		var->contiguous = NC_TRUE;
            else if((flag & HDF_CHUNK) != 0) {
		var->contiguous = NC_FALSE;
		if (!(var->chunksizes = malloc(var->ndims * sizeof(size_t))))
	 	    return NC_ENOMEM;
	        for (d = 0; d < var->ndims; d++) {
		    var->chunksizes[d] = chunkdefs.chunk_lengths[d];
		}
	    }
	}
      }

   } /* next var */

#ifdef LOGGING
   /* This will print out the names, types, lens, etc of the vars and
      atts in the file, if the logging level is 2 or greater. */
   log_metadata_nc(h5->root_grp->nc4_info->controller);
#endif
   return NC_NOERR;
   return NC_ENOTBUILT;
}
#endif /* USE_HDF4 */

int
NC4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
	 int use_parallel, void *parameters, NC_Dispatch *dispatch, NC *nc_file)
{
   int res;
   int hdf_file = 0;
#ifdef USE_PARALLEL4
   NC_MPI_INFO mpidfalt = {MPI_COMM_WORLD, MPI_INFO_NULL};
#endif
#if defined USE_PARALLEL4 || defined USE_HDF4
   int inmemory = ((mode & NC_INMEMORY) == NC_INMEMORY);
#endif

   assert(nc_file && path);

   LOG((1, "%s: path %s mode %d params %x",
	__func__, path, mode, parameters));

#ifdef USE_PARALLEL4
   if (!inmemory && use_parallel && parameters == NULL)
	parameters = &mpidfalt;
#endif /* USE_PARALLEL4 */

   /* If this is our first file, initialize HDF5. */
   if (!nc4_hdf5_initialized)
	nc4_hdf5_initialize();

   /* Check the mode for validity */
   if((mode & ILLEGAL_OPEN_FLAGS) != 0)
      {res = NC_EINVAL; goto done;}

   /* Cannot have both */
   if((mode & (NC_MPIIO|NC_MPIPOSIX)) == (NC_MPIIO|NC_MPIPOSIX))
      {res = NC_EINVAL; goto done;}

#ifndef USE_PARALLEL_POSIX
/* If the HDF5 library has been compiled without the MPI-POSIX VFD, alias
 *      the NC_MPIPOSIX flag to NC_MPIIO. -QAK
 */
   if(mode & NC_MPIPOSIX)
   {
      mode &= ~NC_MPIPOSIX;
      mode |= NC_MPIIO;
   }
#endif /* USE_PARALLEL_POSIX */

   /* Figure out if this is a hdf4 or hdf5 file. */
   if ((res = nc_check_for_hdf(path, use_parallel, parameters, &hdf_file)))
	goto done;

   /* Depending on the type of file, open it. */
   nc_file->int_ncid = nc_file->ext_ncid;
   if (hdf_file == NC_HDF5_FILE)
       res = nc4_open_file(path, mode, parameters, nc_file);
#ifdef USE_HDF4
   else if (hdf_file == NC_HDF4_FILE && inmemory)
	{res = NC_EDISKLESS; goto done;}
   else if (hdf_file == NC_HDF4_FILE)
       res = nc4_open_hdf4_file(path, mode, nc_file);
#endif /* USE_HDF4 */
   else
       assert(0); /* should never happen */
done:
   return res;
}

/* Unfortunately HDF only allows specification of fill value only when
   a dataset is created. Whereas in netcdf, you first create the
   variable and then (optionally) specify the fill value. To
   accomplish this in HDF5 I have to delete the dataset, and recreate
   it, with the fill value specified. */
/* QAK: This looks completely unused in the code. (?) */
int
NC4_set_fill(int ncid, int fillmode, int *old_modep)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T* nc4_info;

   LOG((2, "%s: ncid 0x%x fillmode %d", __func__, ncid, fillmode));

   if (!(nc = nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;
   assert(nc4_info);

   /* Trying to set fill on a read-only file? You sicken me! */
   if (nc4_info->no_write)
      return NC_EPERM;

   /* Did you pass me some weird fillmode? */
   if (fillmode != NC_FILL && fillmode != NC_NOFILL)
      return NC_EINVAL;

   /* If the user wants to know, tell him what the old mode was. */
   if (old_modep)
      *old_modep = nc4_info->fill_mode;

   nc4_info->fill_mode = fillmode;


   return NC_NOERR;
}

/* Put the file back in redef mode. This is done automatically for
 * netcdf-4 files, if the user forgets. */
int
NC4_redef(int ncid)
{
  NC_HDF5_FILE_INFO_T* nc4_info;

   LOG((1, "%s: ncid 0x%x", __func__, ncid));

   /* Find this file's metadata. */
   if (!(nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;
   assert(nc4_info);

   /* If we're already in define mode, return an error. */
   if (nc4_info->flags & NC_INDEF)
      return NC_EINDEFINE;

   /* If the file is read-only, return an error. */
   if (nc4_info->no_write)
      return NC_EPERM;

   /* Set define mode. */
   nc4_info->flags |= NC_INDEF;

   /* For nc_abort, we need to remember if we're in define mode as a
      redef. */
   nc4_info->redef = NC_TRUE;

   return NC_NOERR;
}

/* For netcdf-4 files, this just calls nc_enddef, ignoring the extra
 * parameters. */
int
NC4__enddef(int ncid, size_t h_minfree, size_t v_align,
	    size_t v_minfree, size_t r_align)
{
   if (nc4_find_nc_file(ncid,NULL) == NULL)
      return NC_EBADID;

   return NC4_enddef(ncid);
}

/* Take the file out of define mode. This is called automatically for
 * netcdf-4 files, if the user forgets. */
static int NC4_enddef(int ncid)
{
  NC *nc;
   NC_HDF5_FILE_INFO_T* nc4_info;
   NC_GRP_INFO_T *grp;
   int i;

   LOG((1, "%s: ncid 0x%x", __func__, ncid));

   if (!(nc = nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;
   assert(nc4_info);

   /* Find info for this file and group */
   if (!(grp = nc4_rec_find_grp(nc4_info->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADGRPID;

   /* when exiting define mode, mark all variable written */
   for (i=0; i<grp->vars.nelems; i++)
      grp->vars.value[i]->written_to = NC_TRUE;

   return nc4_enddef_netcdf4_file(nc4_info);
}

/* This function will write all changed metadata, and (someday) reread
 * all metadata from the file. */
static int
sync_netcdf4_file(NC_HDF5_FILE_INFO_T *h5)
{
   int retval;

   assert(h5);
   LOG((3, "%s", __func__));

   /* If we're in define mode, that's an error, for strict nc3 rules,
    * otherwise, end define mode. */
   if (h5->flags & NC_INDEF)
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
	 return NC_EINDEFINE;

      /* Turn define mode off. */
      h5->flags ^= NC_INDEF;

      /* Redef mode needs to be tracked separately for nc_abort. */
      h5->redef = NC_FALSE;
   }

#ifdef LOGGING
   /* This will print out the names, types, lens, etc of the vars and
      atts in the file, if the logging level is 2 or greater. */
   log_metadata_nc(h5->root_grp->nc4_info->controller);
#endif

   /* Write any metadata that has changed. */
   if (!(h5->cmode & NC_NOWRITE))
   {
      nc_bool_t bad_coord_order = NC_FALSE;	/* if detected, propagate to all groups to consistently store dimids */

      if ((retval = nc4_rec_write_groups_types(h5->root_grp)))
	 return retval;
      if ((retval = nc4_rec_detect_need_to_preserve_dimids(h5->root_grp, &bad_coord_order)))
	 return retval;
      if ((retval = nc4_rec_write_metadata(h5->root_grp, bad_coord_order)))
	 return retval;
   }

   if (H5Fflush(h5->hdfid, H5F_SCOPE_GLOBAL) < 0)
      return NC_EHDFERR;

   return retval;
}

/* Flushes all buffers associated with the file, after writing all
   changed metadata. This may only be called in data mode. */
int
NC4_sync(int ncid)
{
   NC *nc;
   int retval;
   NC_HDF5_FILE_INFO_T* nc4_info;

   LOG((2, "%s: ncid 0x%x", __func__, ncid));

   if (!(nc = nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;
   assert(nc4_info);

   /* If we're in define mode, we can't sync. */
   if (nc4_info && nc4_info->flags & NC_INDEF)
   {
      if (nc4_info->cmode & NC_CLASSIC_MODEL)
	 return NC_EINDEFINE;
      if ((retval = NC4_enddef(ncid)))
	 return retval;
   }

   return sync_netcdf4_file(nc4_info);
}

/* This function will free all allocated metadata memory, and close
   the HDF5 file. The group that is passed in must be the root group
   of the file. */
static int
close_netcdf4_file(NC_HDF5_FILE_INFO_T *h5, int abort)
{
   int retval = NC_NOERR;

   assert(h5 && h5->root_grp);
   LOG((3, "%s: h5->path %s abort %d", __func__, h5->controller->path, abort));

   /* According to the docs, always end define mode on close. */
   if (h5->flags & NC_INDEF)
      h5->flags ^= NC_INDEF;

   /* Sync the file, unless we're aborting, or this is a read-only
    * file. */
   if (!h5->no_write && !abort)
      if ((retval = sync_netcdf4_file(h5)))
	goto exit;

   /* Delete all the list contents for vars, dims, and atts, in each
    * group. */
   if ((retval = nc4_rec_grp_del(&h5->root_grp, h5->root_grp)))
	goto exit;

   /* Close hdf file. */
#ifdef USE_HDF4
   if (h5->hdf4)
   {
      if (SDend(h5->sdid))
         BAIL_QUIET(NC_EHDFERR);
   }
   else
#endif /* USE_HDF4 */
   {
#ifdef USE_PARALLEL4
      /* Free the MPI Comm & Info objects, if we opened the file in parallel */
      if(h5->parallel)
      {
          if(MPI_COMM_NULL != h5->comm)
              MPI_Comm_free(&h5->comm);
          if(MPI_INFO_NULL != h5->info)
              MPI_Info_free(&h5->info);
      }
#endif

      if(h5->fileinfo) free(h5->fileinfo);

      if (H5Fclose(h5->hdfid) < 0)
      {
	int nobjs;

	nobjs = H5Fget_obj_count(h5->hdfid, H5F_OBJ_ALL);
	/* Apparently we can get an error even when nobjs == 0 */
	if(nobjs < 0) {
          BAIL_QUIET(NC_EHDFERR);
	} else if(nobjs > 0) {
#ifdef LOGGING
	 char msg[1024];
	 int logit = 1;
	 /* If the close doesn't work, probably there are still some HDF5
	  * objects open, which means there's a bug in the library. So
	  * print out some info on to help the poor programmer figure it
	  * out. */
	snprintf(msg,sizeof(msg),"There are %d HDF5 objects open!", nobjs);
#ifdef LOGOPEN
         LOG((0, msg));
#else
         fprintf(stdout,msg);
	 logit = 0;
#endif
	 reportopenobjects(logit,h5->hdfid);
#endif
      }
    }
   }
exit:
   /* Free the nc4_info struct; above code should have reclaimed
      everything else */
   if(h5 != NULL)
       free(h5);
   return retval;
}

/* From the netcdf-3 docs: The function nc_abort just closes the
   netCDF dataset, if not in define mode. If the dataset is being
   created and is still in define mode, the dataset is deleted. If
   define mode was entered by a call to nc_redef, the netCDF dataset
   is restored to its state before definition mode was entered and the
   dataset is closed. */
int
NC4_abort(int ncid)
{
   NC *nc;
   int delete_file = 0;
   char path[NC_MAX_NAME + 1];
   int retval = NC_NOERR;
   NC_HDF5_FILE_INFO_T* nc4_info;

   LOG((2, "%s: ncid 0x%x", __func__, ncid));

   /* Find metadata for this file. */
   if (!(nc = nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;

   assert(nc4_info);

   /* If we're in define mode, but not redefing the file, delete it. */
   if (nc4_info->flags & NC_INDEF && !nc4_info->redef)
   {
      delete_file++;
      strncpy(path, nc->path,NC_MAX_NAME);
   }

   /* Free any resources the netcdf-4 library has for this file's
    * metadata. */
   if ((retval = close_netcdf4_file(nc4_info, 1)))
      return retval;

   /* Delete the file, if we should. */
   if (delete_file)
      if (remove(path) < 0)
          return NC_ECANTREMOVE;

   return retval;
}

/* Close the netcdf file, writing any changes first. */
int
NC4_close(int ncid)
{
   NC_GRP_INFO_T *grp;
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   int retval;

   LOG((1, "%s: ncid 0x%x", __func__, ncid));

   /* Find our metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(nc && h5 && grp);

   /* This must be the root group. */
   if (grp->parent)
      return NC_EBADGRPID;

   /* Call the nc4 close. */
   if ((retval = close_netcdf4_file(grp->nc4_info, 0)))
      return retval;

   return NC_NOERR;
}

/* It's possible for any of these pointers to be NULL, in which case
   don't try to figure out that value. */
int
NC4_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC_DIM_INFO_T *dim;
   NC_ATT_INFO_T *att;
   int retval;

   LOG((2, "%s: ncid 0x%x", __func__, ncid));

   /* Find file metadata. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(h5 && grp && nc);

   /* Count the number of dims, vars, and global atts. */
   if (ndimsp)
   {
      *ndimsp = 0;
      for (dim = grp->dim; dim; dim = dim->l.next)
	 (*ndimsp)++;
   }
   if (nvarsp)
   {
      int i;
      *nvarsp = 0;
      for (i=0; i < grp->vars.nelems; i++)
      {
	if (grp->vars.value[i])
	  (*nvarsp)++;
      }
   }
   if (nattsp)
     {
      *nattsp = 0;
      for (att = grp->att; att; att = att->l.next)
	 (*nattsp)++;
   }

   if (unlimdimidp)
   {
      /* Default, no unlimited dimension */
      *unlimdimidp = -1;

      /* If there's more than one unlimited dim, which was not possible
	 with netcdf-3, then only the last unlimited one will be reported
	 back in xtendimp. */
      /* Note that this code is inconsistent with nc_inq_unlimid() */
      for (dim = grp->dim; dim; dim = dim->l.next)
	 if (dim->unlimited)
	 {
	    *unlimdimidp = dim->dimid;
	    break;
	 }
   }

   return NC_NOERR;
}

/* This function will do the enddef stuff for a netcdf-4 file. */
int
nc4_enddef_netcdf4_file(NC_HDF5_FILE_INFO_T *h5)
{
   assert(h5);
   LOG((3, "%s", __func__));

   /* If we're not in define mode, return an error. */
   if (!(h5->flags & NC_INDEF))
      return NC_ENOTINDEFINE;

   /* Turn define mode off. */
   h5->flags ^= NC_INDEF;

   /* Redef mode needs to be tracked separately for nc_abort. */
   h5->redef = NC_FALSE;

   return sync_netcdf4_file(h5);
}

#ifdef EXTRA_TESTS
int
nc_exit()
{
   if (num_plists || num_spaces)
      return NC_EHDFERR;

   return NC_NOERR;
}
#endif /* EXTRA_TESTS */

#ifdef USE_PARALLEL4
int
nc_use_parallel_enabled()
{
   return 0;
}
#endif /* USE_PARALLEL4 */


/*
Wrap HDF5 allocated memory free operations
*/

static void
hdf5free(void* memory)
{
#ifndef JNA
    /* On Windows using the microsoft runtime, it is an error
       for one library to free memory allocated by a different library.*/
#ifdef HDF5_HAS_H5FREE
    if(memory != NULL) H5free_memory(memory);
#else
#ifndef _MSC_VER
	if(memory != NULL) free(memory);
#endif
#endif
#endif
}
