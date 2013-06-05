/*
  This file is part of netcdf-4, a netCDF-like interface for HDF5, or
  a HDF5 backend for netCDF, depending on your point of view.

  This file handles the nc4 file functions.

  Copyright 2003, University Corporation for Atmospheric Research. See
  COPYRIGHT file for copying and redistribution conditions.
*/

#include "nc4internal.h"
#include "nc.h"
#include <H5DSpublic.h>
#include "nc4dispatch.h"
#include "ncdispatch.h"

#ifdef USE_HDF4
#include <mfhdf.h>
#endif

#ifdef USE_PNETCDF
#include <pnetcdf.h>
#endif

/* This is to track opened HDF5 objects to make sure they are
 * closed. */
#ifdef EXTRA_TESTS
extern int num_plists;
extern int num_spaces;
#endif /* EXTRA_TESTS */

#define MIN_DEFLATE_LEVEL 0
#define MAX_DEFLATE_LEVEL 9

/* These are the special attributes added by the HDF5 dimension scale
 * API. They will be ignored by netCDF-4. */
#define REFERENCE_LIST "REFERENCE_LIST"
#define CLASS "CLASS"
#define DIMENSION_LIST "DIMENSION_LIST"
#define NAME "NAME"

static int NC4_enddef(int ncid);

#ifdef IGNORE
/* This extern points to the pointer that holds the list of open
 * netCDF files. */
extern NC_FILE_INFO_T *nc_file;
#endif

/* These are the default chunk cache sizes for HDF5 files created or
 * opened with netCDF-4. */
size_t nc4_chunk_cache_size = CHUNK_CACHE_SIZE;
size_t nc4_chunk_cache_nelems = CHUNK_CACHE_NELEMS;
float nc4_chunk_cache_preemption = CHUNK_CACHE_PREEMPTION;

/* This is set by nc_set_default_format in libsrc/nc.c. */
extern int default_create_format;

/* To turn off HDF5 error messages, I have to catch an early
   invocation of a netcdf function. */
static int virgin = 1;

/* For performance, fill this array only the first time, and keep it
 * in global memory for each further use. */
#define NUM_TYPES 12
static hid_t native_type_constant[NUM_TYPES];

static char nc_type_name[NUM_TYPES][NC_MAX_NAME + 1] = {"char", "byte", "short", "int", "float",
							"double", "ubyte", "ushort", "uint", 
							"int64", "uint64", "string"};
int nc4_free_global_hdf_string_typeid();

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
nc_check_for_hdf(const char *path, int use_parallel, MPI_Comm comm, MPI_Info info, 
		 int *hdf_file)
{
   char blob[MAGIC_NUMBER_LEN];
   
   assert(hdf_file && path);
   LOG((3, "nc_check_for_hdf: path %s", path));

/* Get the 4-byte blob from the beginning of the file. Don't use posix
 * for parallel, use the MPI functions instead. */
#ifdef USE_PARALLEL
   if (use_parallel)
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
#endif /* USE_PARALLEL */
   {
      FILE *fp;
      if (!(fp = fopen(path, "r")) ||
	  fread(blob, MAGIC_NUMBER_LEN, 1, fp) != 1)
	 return errno;
      fclose(fp);
   }

   /* Ignore the first byte for HDF5. */
   if (blob[1] == 'H' && blob[2] == 'D' && blob[3] == 'F')
      *hdf_file = NC_HDF5_FILE;
   else if (!strncmp(blob, "\016\003\023\001", MAGIC_NUMBER_LEN))
      *hdf_file = NC_HDF4_FILE;
   else
      *hdf_file = 0;

   return NC_NOERR;
}
   
/* Create a HDF5/netcdf-4 file. In this case, ncid has already been
 * selected in ncfunc.c. */
static int
nc4_create_file(const char *path, int cmode, MPI_Comm comm, MPI_Info info,
                NC_FILE_INFO_T *nc) 
{
   hid_t fcpl_id, fapl_id;
   unsigned flags = (cmode & NC_NOCLOBBER) ? 
      H5F_ACC_EXCL : H5F_ACC_TRUNC;
   FILE *fp;
   int retval = NC_NOERR;

   LOG((3, "nc4_create_file: path %s mode 0x%x", path, cmode));
   assert(nc && path);

   /* If this file already exists, and NC_NOCLOBBER is specified,
      return an error. */
   if ((cmode & NC_NOCLOBBER) && (fp = fopen(path, "r")))
   {
      fclose(fp);
      return NC_EEXIST;
   }
   
   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, (NC_WRITE | cmode))))
      BAIL(retval);
   assert(nc->nc4_info && nc->nc4_info->root_grp);

   /* Need this access plist to control how HDF5 handles open onjects
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
#ifdef USE_PARALLEL
   /* If this is a parallel file create, set up the file creation
      property list. */
   if ((cmode & NC_MPIIO) || (cmode & NC_MPIPOSIX))
   {
      nc->nc4_info->parallel++;
      if (cmode & NC_MPIIO)  /* MPI/IO */
      {
	 LOG((4, "creating parallel file with MPI/IO"));
	 if (H5Pset_fapl_mpio(fapl_id, comm, info) < 0)
	    BAIL(NC_EPARINIT);
      }
      else /* MPI/POSIX */
      {
	 LOG((4, "creating parallel file with MPI/posix"));
	 if (H5Pset_fapl_mpiposix(fapl_id, comm, 0) < 0)
	    BAIL(NC_EPARINIT);
      }
   }
#else /* only set cache for non-parallel... */
   if (H5Pset_cache(fapl_id, 0, nc4_chunk_cache_nelems, nc4_chunk_cache_size, 
		    nc4_chunk_cache_preemption) < 0)
      BAIL(NC_EHDFERR);
   LOG((4, "nc4_create_file: set HDF raw chunk cache to size %d nelems %d preemption %f", 
	nc4_chunk_cache_size, nc4_chunk_cache_nelems, nc4_chunk_cache_preemption));
#endif /* USE_PARALLEL */
   
   /* Set latest_format in access propertly list and
    * H5P_CRT_ORDER_TRACKED in the creation property list. This turns
    * on HDF5 creation ordering. */
   if (H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) < 0)
      BAIL(NC_EHDFERR);
   if ((fcpl_id = H5Pcreate(H5P_FILE_CREATE)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists++;
#endif
   if (H5Pset_link_creation_order(fcpl_id, (H5P_CRT_ORDER_TRACKED |
					    H5P_CRT_ORDER_INDEXED)) < 0)
      BAIL(NC_EHDFERR);
   if (H5Pset_attr_creation_order(fcpl_id, (H5P_CRT_ORDER_TRACKED |
					    H5P_CRT_ORDER_INDEXED)) < 0)
      BAIL(NC_EHDFERR);

   /* Create the file. */
   if ((nc->nc4_info->hdfid = H5Fcreate(path, flags, fcpl_id, fapl_id)) < 0) 
      BAIL(NC_EFILEMETA);

   /* Open the root group. */
   if ((nc->nc4_info->root_grp->hdf_grpid = H5Gopen2(nc->nc4_info->hdfid, "/", 
						     H5P_DEFAULT)) < 0)
      BAIL(NC_EFILEMETA);

   /* Release the property lists. */
   if (H5Pclose(fapl_id) < 0 ||
       H5Pclose(fcpl_id) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
   num_plists--;
#endif

   /* Define mode gets turned on automatically on create. */
   nc->nc4_info->flags |= NC_INDEF;

   return NC_NOERR;

  exit:
   if (nc->nc4_info->hdfid > 0) H5Fclose(nc->nc4_info->hdfid);
   return retval;
}

int
NC4_create(const char* path, int cmode, size_t initialsz, int basepe, 
	   size_t *chunksizehintp, int use_parallel, void *mpidata,
	   NC_Dispatch *dispatch, NC **ncpp)
{
   NC_FILE_INFO_T *nc_file = NULL;
#ifdef USE_PARALLEL
   MPI_Comm comm = 0; 
   MPI_Info info = 0; 
#else
   int comm = 0, info = 0;
#endif /* USE_PARALLEL */
   int res;

   assert(ncpp && path);

   LOG((1, "nc4_create_file: path %s cmode 0x%x comm %d info %d",
	path, cmode, comm, info));
   
#ifdef USE_PARALLEL
   if (mpidata) 
   { 
      comm = ((NC_MPI_INFO *)mpidata)->comm; 
      info = ((NC_MPI_INFO *)mpidata)->info;	
   }
#endif /* USE_PARALLEL */

   /* If this is our first file, turn off HDF5 error messages. */
   if (virgin)
   {
      if (H5Eset_auto(NULL, NULL) < 0)
	 LOG((0, "Couldn't turn off HDF5 error messages!"));
      LOG((1, "HDF5 error messages have been turned off."));
      virgin = 0;
   }

   /* Check the cmode for validity. */
   if (cmode & ~(NC_NOCLOBBER | NC_64BIT_OFFSET
                 | NC_NETCDF4 | NC_CLASSIC_MODEL
                 | NC_SHARE | NC_MPIIO | NC_MPIPOSIX | NC_LOCK | NC_PNETCDF)
       || (cmode & NC_MPIIO && cmode & NC_MPIPOSIX)
       || (cmode & NC_64BIT_OFFSET && cmode & NC_NETCDF4))
      return NC_EINVAL;

   /* Allocate the storage for this file info struct, and fill it with
      zeros. This add the file metadata to the front of the global
      nc_file list. */
   if ((res = nc4_file_list_add(&nc_file, dispatch)))
      return res;

   /* Apply default create format. */
   if (default_create_format == NC_FORMAT_64BIT)
      cmode |= NC_64BIT_OFFSET;
   else if (default_create_format == NC_FORMAT_NETCDF4)
      cmode |= NC_NETCDF4;
   else if (default_create_format == NC_FORMAT_NETCDF4_CLASSIC)
   {
      cmode |= NC_NETCDF4;
      cmode |= NC_CLASSIC_MODEL;
   }
   LOG((2, "cmode after applying default format: 0x%x", cmode));

   /* Check to see if we want a netcdf3 or netcdf4 file. Open it, and
      call the appropriate nc*_create. */
   if (cmode & NC_NETCDF4) 
   {
      nc_file->int_ncid = nc_file->ext_ncid;
      res = nc4_create_file(path, cmode, comm, info, nc_file);
   } 
#ifdef USE_PNETCDF
   else if (cmode & NC_PNETCDF)
   {
      nc_file->pnetcdf_file++;
      res = ncmpi_create(comm, path, cmode, info, &(nc_file->int_ncid));      
   }
#endif /* USE_PNETCDF */
   else 
   {
      return NC_EINVAL;
   }
   
   /* Delete this file list entry if there was a failure. */
   if (res)
   {
      if (nc_file) 
	 nc4_file_list_del(nc_file);
   }
   else
      *ncpp = (NC *)nc_file;

   return res;
}

/* This function is called by read_dataset when a dimension scale
 * dataset is encountered. It reads in the dimension data (creating a
 * new NC_DIM_INFO_T object), and also checks to see if this is a
 * dimension without a variable - that is, a coordinate dimension
 * which does not have any coordinate data. */
static int
read_scale(NC_GRP_INFO_T *grp, hid_t datasetid, char *obj_name, 
           hsize_t scale_size, hsize_t max_scale_size, 
           int *dim_without_var)
{
   /*char *start_of_len;*/
   char dimscale_name_att[NC_MAX_NAME + 1] = "";
   hid_t attid = 0;
   int max_len;
   int retval;

   /* Add a dimension for this scale. */
   if ((retval = nc4_dim_list_add(&grp->dim)))
      return retval;

   /* Assign dimid and increment number of dimensions. */
   grp->dim->dimid = grp->file->nc4_info->next_dimid++;
   grp->ndims++;

   /* Does this dataset have a hidden attribute that tells us its
    * dimid? If so, read it. */
   H5E_BEGIN_TRY { 
      if ((attid = H5Aopen_by_name(datasetid, ".", NC_DIMID_ATT_NAME, 
				   H5P_DEFAULT, H5P_DEFAULT)) > 0)
      {
	 if (H5Aread(attid, H5T_NATIVE_INT, &grp->dim->dimid) < 0)
	    return NC_EHDFERR;
	 if (H5Aclose(attid) < 0)
	    return NC_EHDFERR;
      }
   } H5E_END_TRY;

   max_len = strlen(obj_name) > NC_MAX_NAME ? NC_MAX_NAME : strlen(obj_name);
   if (!(grp->dim->name = malloc((max_len + 1) * sizeof(char))))
      return NC_ENOMEM;
   strncpy(grp->dim->name, obj_name, max_len + 1);
   if (SIZEOF_SIZE_T < 8 && scale_size > NC_MAX_UINT)
   {
      grp->dim->len = NC_MAX_UINT;
      grp->dim->too_long = 1;
   }
   else
      grp->dim->len = scale_size;
   grp->dim->hdf_dimscaleid = datasetid;

   /* If the dimscale has an unlimited dimension, then this dimension
    * is unlimited. */
   if (max_scale_size == H5S_UNLIMITED)
      grp->dim->unlimited++;

   /* If the scale name is set to DIM_WITHOUT_VARIABLE, then this is a
    * dimension, but not a variable. (If get_scale_name returns an
    * error, just move on, there's no NAME.) */
   if (H5DSget_scale_name(datasetid, dimscale_name_att, NC_MAX_NAME) >= 0)
   {
      if (!strncmp(dimscale_name_att, DIM_WITHOUT_VARIABLE, 
                   strlen(DIM_WITHOUT_VARIABLE)))
      {
         if (grp->dim->unlimited)
         {
            size_t len = 0, *lenp = &len;
            if ((retval = nc4_find_dim_len(grp, grp->dim->dimid, &lenp)))
               return retval;
            grp->dim->len = *lenp;
         }
         (*dim_without_var)++;
      }
   }

   return NC_NOERR;
}

/* This function reads the hacked in coordinates attribute I use for
 * multi-dimensional coordinates. */
static int
read_coord_dimids(NC_VAR_INFO_T *var)
{
   hid_t coord_att_typeid = -1, coord_attid = -1, spaceid = -1;
   hssize_t coord_array_size;
   int ret = 0;

   /* There is a hidden attribute telling us the ids of the
    * dimensions that apply to this multi-dimensional coordinate
    * variable. Read it. */
   if ((coord_attid = H5Aopen_name(var->hdf_datasetid, COORDINATES)) < 0) ret++;
   if (!ret && (coord_att_typeid = H5Aget_type(coord_attid)) < 0) ret++;
   if (!ret && H5Aread(coord_attid, coord_att_typeid, var->dimids) < 0) ret++;
   LOG((4, "dimscale %s is multidimensional and has coords", var->name));

   /* How many dimensions are there? */
   if ((spaceid = H5Aget_space(coord_attid)) < 0) ret++;
#ifdef EXTRA_TESTS
   num_spaces++;
#endif
   if ((coord_array_size = H5Sget_simple_extent_npoints(spaceid)) < 0) ret++;
   
   /* Malloc space to the array of pointers to dims. */
   

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
/*   (*(HDF5_OBJID_T *)dimscale_hdf5_objids).fileno = statbuf.fileno;
     (*(HDF5_OBJID_T *)dimscale_hdf5_objids).objno = statbuf.objno;*/
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
   hid_t class;
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
	       nc_type *xtype, NC_TYPE_INFO_T **type_info)
{
   NC_TYPE_INFO_T *type;
   htri_t is_str, equal = 0;
   hid_t class, native_typeid, hdf_typeid;
   nc_type my_nc_type = 0;
   H5T_order_t order;
   int endianness;
   nc_type nc_type_constant[NUM_TYPES] = {NC_CHAR, NC_BYTE, NC_SHORT, NC_INT, NC_FLOAT,
					  NC_DOUBLE, NC_UBYTE, NC_USHORT, NC_UINT,
					  NC_INT64, NC_UINT64, NC_STRING};
   int type_size[NUM_TYPES] = {sizeof(char), sizeof(char), sizeof(short), 
			       sizeof(int), sizeof(float), sizeof(double),
			       sizeof(unsigned char), sizeof(unsigned short), 
			       sizeof(unsigned int), sizeof(long long), 
			       sizeof(unsigned long long), 0};
   int t;

   assert(h5 && xtype && type_info);

   /* Because these N5T_NATIVE_* constants are actually function calls
    * (!) in H5Tpublic.h, I can't initialize this array in the usual
    * way, because at least some C compilers (like Irix) complain
    * about calling functions when defining constants. So I have to do
    * it like this. Note that there's no native types for char or
    * string. Those are handled later. */
   if (!native_type_constant[1])
   {
      native_type_constant[1] = H5T_NATIVE_SCHAR;
      native_type_constant[2] = H5T_NATIVE_SHORT;
      native_type_constant[3] = H5T_NATIVE_INT;
      native_type_constant[4] = H5T_NATIVE_FLOAT;
      native_type_constant[5] = H5T_NATIVE_DOUBLE;
      native_type_constant[6] = H5T_NATIVE_UCHAR;
      native_type_constant[7] = H5T_NATIVE_USHORT;
      native_type_constant[8] = H5T_NATIVE_UINT;
      native_type_constant[9] = H5T_NATIVE_LLONG;
      native_type_constant[10] = H5T_NATIVE_ULLONG;
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
      (*type_info)->class = class;

      /* H5Tequal doesn't work with H5T_C_S1 for some reason. But
       * H5Tget_class will return H5T_STRING if this is a string. */
      if (class == H5T_STRING)
      {
	 if ((is_str = H5Tis_variable_str(native_typeid)) < 0)
	    return NC_EHDFERR;
	 if (is_str)
	    t = NUM_TYPES - 1;
	 else
	    t = 0;
      }
      else if (class == H5T_INTEGER || class == H5T_FLOAT)
      {
	 for (t = 1; t < NUM_TYPES - 1; t++)
	 {
	    if ((equal = H5Tequal(native_typeid, native_type_constant[t])) < 0)
	       return NC_EHDFERR;
	    if (equal)
	    {
	       my_nc_type = nc_type_constant[t];
	       break;
	    }
	 }

	 /* Find out about endianness. */
	 if (class == H5T_INTEGER)
	 {
	    if ((order = H5Tget_order(hdf_typeid)) < 0) 
	       return NC_EHDFERR;
	    if (order == H5T_ORDER_LE)
	       endianness = NC_ENDIAN_LITTLE;
	    else if (order == H5T_ORDER_BE)
	       endianness = NC_ENDIAN_BIG;
	    /* Copy this into the type_info struct. */
	    (*type_info)->endianness = endianness;
	 }
      }
      *xtype = nc_type_constant[t];
      (*type_info)->nc_typeid = nc_type_constant[t];
      (*type_info)->size = type_size[t];
      if (!((*type_info)->name = malloc((strlen(nc_type_name[t]) + 1) * sizeof(char))))
	 return NC_ENOMEM;
      strcpy((*type_info)->name, nc_type_name[t]);
      (*type_info)->class = class;
      (*type_info)->hdf_typeid = hdf_typeid;
      (*type_info)->native_typeid = native_typeid;
      (*type_info)->close_hdf_typeid = 1;
      return NC_NOERR;
   }
   else
   {
      /* This is a user-defined type. */
      if((type = nc4_rec_find_hdf_type(h5->root_grp, native_typeid)))
      {
         *xtype = type->nc_typeid;
	 *type_info = type;
      }

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

   *xtype = NC_NAT;
   return NC_EBADTYPID;
}

/* Read an attribute. */
static int 
read_hdf5_att(NC_GRP_INFO_T *grp, hid_t attid, NC_ATT_INFO_T *att)
{
   hid_t spaceid = 0, file_typeid = 0;
   hsize_t dims[1]; /* netcdf attributes always 1-D. */
   int retval = NC_NOERR;
   size_t type_size;
   int att_ndims;
   hssize_t att_npoints;
   H5T_class_t att_class;      
   int fixed_len_string = 0;
   size_t fixed_size = 0;

   assert(att->name);
   LOG((5, "read_hdf5_att: att->attnum %d att->name %s "
        "att->xtype %d att->len %d", att->attnum, att->name,
        att->xtype, att->len));

   /* Get type of attribute in file. */
   if ((file_typeid = H5Aget_type(attid)) < 0)
      return NC_EATTMETA;
   if ((att->native_typeid = H5Tget_native_type(file_typeid, H5T_DIR_DEFAULT)) < 0) 
      BAIL(NC_EHDFERR);
   if ((att_class = H5Tget_class(att->native_typeid)) < 0)
      BAIL(NC_EATTMETA);
   if (att_class == H5T_STRING && !H5Tis_variable_str(att->native_typeid))
   {
      fixed_len_string++;
      if (!(fixed_size = H5Tget_size(att->native_typeid)))
	 BAIL(NC_EATTMETA);
   }
   if ((retval = get_netcdf_type(grp->file->nc4_info, att->native_typeid, &(att->xtype))))
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
   {
      dims[0] = 0;
   }
   else if (att->xtype == NC_CHAR)
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
	 att->xtype = NC_STRING;
	 dims[0] = att_npoints;
      }
   } 
   else
   {
      /* All netcdf attributes are 1-D only. */
      if (att_ndims != 1)
	 BAIL(NC_EATTMETA);

      /* Read the size of this attribute. */
      if (H5Sget_simple_extent_dims(spaceid, dims, NULL) < 0)
	 BAIL(NC_EATTMETA);
   }
      
   /* Tell the user what the length if this attribute is. */
   att->len = dims[0];

   /* Allocate some memory if the len is not zero, and read the
      attribute. */
   if (dims[0])
   {
      if ((retval = nc4_get_typelen_mem(grp->file->nc4_info, att->xtype, 0,
					&type_size)))
	 return retval;
      if (att_class == H5T_VLEN)
      {
	 if (!(att->vldata = malloc((unsigned int)(att->len * sizeof(hvl_t)))))
	    BAIL(NC_ENOMEM);
	 if (H5Aread(attid, att->native_typeid, att->vldata) < 0)
	    BAIL(NC_EATTMETA);
      }
      else if (att->xtype == NC_STRING)
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
	    if (H5Aread(attid, att->native_typeid, contig_buf) < 0)
	       BAIL(NC_EATTMETA);
	    
	    /* Copy strings, one at a time, into their new home. Alloc
	       space for each string. The user will later free this
	       space with nc_free_string. */
	    cur = contig_buf;
	    for (i = 0; i < att->len; i++)
	    {
	       if (!(att->stdata[i] = malloc(fixed_size)))
		  BAIL(NC_ENOMEM);
	       strncpy(att->stdata[i], cur, fixed_size);
	       cur += fixed_size;
	    }
	    
	    /* Free contiguous memory buffer. */
	    free(contig_buf);
	 }
	 else
	 {
	    /* Read variable-length string atts. */
	    if (H5Aread(attid, att->native_typeid, att->stdata) < 0)
	       BAIL(NC_EATTMETA);
	 }
      }
      else
      {
	 if (!(att->data = malloc((unsigned int)(att->len * type_size))))
	    BAIL(NC_ENOMEM);
	 if (H5Aread(attid, att->native_typeid, att->data) < 0)
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
 * stash it in the group's list of types. Return the netcdf typeid
 * through a pointer, if caller desires it. */
static int
read_type(NC_GRP_INFO_T *grp, char *type_name)
{
   NC_TYPE_INFO_T *type;
   H5T_class_t class;
   hid_t hdf_typeid, native_typeid = 0;
   int nmembers;
   hid_t member_hdf_typeid, base_hdf_typeid = 0;
   char *member_name = NULL;
   size_t type_size = 0, member_offset;
   unsigned int m;
   nc_type ud_type_type = NC_NAT, base_nc_type = NC_NAT, member_xtype;
   htri_t ret;
   int retval = NC_NOERR;
   void *value;
   int i;

   assert(grp && type_name);

   if (strlen(type_name) > NC_MAX_NAME)
      return NC_EBADNAME;

   LOG((4, "read_type: type_name %s grp->name %s", type_name, grp->name));

   if ((hdf_typeid = H5Topen2(grp->hdf_grpid, type_name, H5P_DEFAULT)) < 0)
      return NC_EHDFERR;

   /* What is the native type for this platform? */
   if ((native_typeid = H5Tget_native_type(hdf_typeid, H5T_DIR_DEFAULT)) < 0) 
      return NC_EHDFERR;
   
   /* What is the size of this type on this platform. */
   if (!(type_size = H5Tget_size(native_typeid)))
      return NC_EHDFERR;
   LOG((5, "type_size %d", type_size));

   /* What is the class of this type, compound, vlen, etc. */
   if ((class = H5Tget_class(hdf_typeid)) < 0)
      return NC_EHDFERR;
   switch (class)
   {
      case H5T_STRING:
         ud_type_type = NC_STRING;
         break;
      case H5T_COMPOUND:
         ud_type_type = NC_COMPOUND; 
         break;
      case H5T_VLEN:
         /* For conveninence we allow user to pass vlens of strings
          * with null terminated strings. This means strings are
          * treated slightly differently by the API, although they are
          * really just VLENs of characters. */
         if ((ret = H5Tis_variable_str(hdf_typeid)) < 0)
            return NC_EHDFERR;
         if (ret)
            ud_type_type = NC_STRING;
         else
         {
            ud_type_type = NC_VLEN;

            /* Find the base type of this vlen (i.e. what is this a
             * vlen of?) */
            if (!(base_hdf_typeid = H5Tget_super(native_typeid)))
               return NC_EHDFERR;

            /* What size is this type? */
            if (!(type_size = H5Tget_size(base_hdf_typeid)))
               return NC_EHDFERR;

            /* What is the netcdf corresponding type. */
            if ((retval = get_netcdf_type(grp->file->nc4_info, base_hdf_typeid, 
					  &base_nc_type)))
               return retval;
            LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d", 
                 base_hdf_typeid, type_size, base_nc_type));
         }
         break;
      case H5T_OPAQUE:
         ud_type_type = NC_OPAQUE;
         /* What size is this type? */
         if (!(type_size = H5Tget_size(hdf_typeid)))
            return NC_EHDFERR;
         LOG((5, "type_size %d", type_size));
         break;
      case H5T_ENUM:
         ud_type_type = NC_ENUM;

         /* Find the base type of this enum (i.e. what is this a
          * enum of?) */
         if (!(base_hdf_typeid = H5Tget_super(hdf_typeid)))
            return NC_EHDFERR;
         /* What size is this type? */
         if (!(type_size = H5Tget_size(base_hdf_typeid)))
            return NC_EHDFERR;
         /* What is the netcdf corresponding type. */
         if ((retval = get_netcdf_type(grp->file->nc4_info, base_hdf_typeid, 
				       &base_nc_type)))
            return retval;
         LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d", 
              base_hdf_typeid, type_size, base_nc_type));
         break;
      default:
         LOG((0, "unknown class"));
         return NC_EBADCLASS;
   }

   /* Add to the list for this new type, and get a local pointer to it. */
   if ((retval = nc4_type_list_add(&grp->type, &type)))
      return retval;
   assert(type);

   /* Remember info about this type. */
   type->nc_typeid = grp->file->nc4_info->next_typeid++;
   type->size = type_size;
   if (!(type->name = malloc((strlen(type_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(type->name, type_name);
   type->class = ud_type_type;
   type->base_nc_type = base_nc_type;
   type->committed++;
   type->hdf_typeid = hdf_typeid;
   type->native_typeid = native_typeid;

   /* Read info about each member of this compound type. */
   if (ud_type_type == NC_COMPOUND)
   {
      if ((nmembers = H5Tget_nmembers(hdf_typeid)) < 0)
         return NC_EHDFERR;
      LOG((5, "compound type has %d members", nmembers));
      for (m = 0; m < nmembers; m++)
      {
         H5T_class_t mem_class;
	 hid_t member_native_typeid;
         int ndims = 0, dim_size[NC_MAX_VAR_DIMS];
         hsize_t dims[NC_MAX_VAR_DIMS];
         int d;

	 /* Get the typeid and native typeid of this member of the
	  * compound type. */
         if ((member_hdf_typeid = H5Tget_member_type(type->native_typeid, m)) < 0)
            return NC_EHDFERR;
	 if ((member_native_typeid = H5Tget_native_type(member_hdf_typeid, H5T_DIR_DEFAULT)) < 0) 
	    return NC_EHDFERR;

	 /* Get the name of the member.*/
         member_name = H5Tget_member_name(type->native_typeid, m);
         if (!member_name || strlen(member_name) > NC_MAX_NAME)
            return NC_EBADNAME;

	 /* Offset in bytes on *this* platform. */
         member_offset = H5Tget_member_offset(type->native_typeid, m);

	 /* Get dimensional data if this member is an array of something. */
         if ((mem_class = H5Tget_class(member_hdf_typeid)) < 0)
            return NC_EHDFERR;
         if (mem_class == H5T_ARRAY)
         {
            if ((ndims = H5Tget_array_ndims(member_hdf_typeid)) < 0)
               return NC_EHDFERR;
            if (H5Tget_array_dims(member_hdf_typeid, dims, NULL) != ndims)
               return NC_EHDFERR;
            for (d = 0; d < ndims; d++)
               dim_size[d] = dims[d];
	    /* What is the netCDF typeid of this member? */
	    if ((retval = get_netcdf_type(grp->file->nc4_info, H5Tget_super(member_hdf_typeid), 
					  &member_xtype)))
	       return retval;
         }
	 else
	 {
	    /* What is the netCDF typeid of this member? */
	    if ((retval = get_netcdf_type(grp->file->nc4_info, member_native_typeid, 
					  &member_xtype)))
	       return retval;
	 }

	 /* Add this member to our list of fields in this compound type. */
         if (ndims)
         {
            if ((retval = nc4_field_list_add(&type->field, type->num_fields++, member_name, 
                                             member_offset, H5Tget_super(member_hdf_typeid), 
					     H5Tget_super(member_native_typeid), 
                                             member_xtype, ndims, dim_size)))
               return retval;
         }
         else
         {
            if ((retval = nc4_field_list_add(&type->field, type->num_fields++, member_name, 
                                             member_offset, member_hdf_typeid, member_native_typeid, 
					     member_xtype, 0, NULL)))
               return retval;
         } 
         
         /* HDF5 allocated this for us. */
         free(member_name);
      }
   }
   else if (ud_type_type == NC_VLEN)
   {
      type->base_hdf_typeid = base_hdf_typeid;
   }
   else if (ud_type_type == NC_ENUM)
   {
      /* Remember the base HDF5 type for this enum. */
      type->base_hdf_typeid = base_hdf_typeid;

      /* Find out how many member are in the enum. */
      if ((type->num_enum_members = H5Tget_nmembers(hdf_typeid)) < 0) 
         return NC_EHDFERR;

      /* Allocate space for one value. */
      if (!(value = calloc(1, type_size)))
         return NC_ENOMEM;

      /* Read each name and value defined in the enum. */
      for (i = 0; i < type->num_enum_members; i++)
      {
         /* Get the name and value from HDF5. */
         if (!(member_name = H5Tget_member_name(hdf_typeid, i)))
            return NC_EHDFERR;
         if (!member_name || strlen(member_name) > NC_MAX_NAME)
            return NC_EBADNAME;
         if (H5Tget_member_value(hdf_typeid, i, value) < 0) 
            return NC_EHDFERR;

         /* Insert new field into this type's list of fields. */
         if ((retval = nc4_enum_member_add(&type->enum_member, type->size, 
                                           member_name, value)))
            return retval;
	 free(member_name);
      }
      
      /* Free the tempory memory for one value, and the member name
       * (which HDF5 allocated for us). */
      free(value);
   }
   
   return retval;
}

/* This function is called by read_dataset, (which is called by
 * nc4_rec_read_metadata) when a netCDF variable is found in the
 * file. This function reads in all the metadata about the var,
 * including the attributes. */
static int
read_var(NC_GRP_INFO_T *grp, hid_t datasetid, char *obj_name, 
         size_t ndims, int is_scale, int num_scales, hid_t access_pid)
{
   NC_VAR_INFO_T *var;
   int natts, a, d;

   NC_ATT_INFO_T *att;
   hid_t attid = 0;
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
   hsize_t chunksize[NC_MAX_VAR_DIMS];
   int retval = NC_NOERR;
   double rdcc_w0;
   int f;

   assert(obj_name && grp);
   LOG((4, "read_var: obj_name %s", obj_name));

   /* Add a variable to the end of the group's var list. */
   if ((retval = nc4_var_list_add(&grp->var, &var)))
      return retval;
            
   /* Fill in what we already know. */
   var->hdf_datasetid = datasetid;
   var->varid = grp->nvars++;
   var->created++;
   var->ndims = ndims;

   /* We need some room to store information about dimensions for this
    * var. */
   if (var->ndims)
   {
      if (!(var->dim = malloc(sizeof(NC_DIM_INFO_T *) * var->ndims)))
	 return NC_ENOMEM;
      if (!(var->dimids = malloc(sizeof(int) * var->ndims)))
	 return NC_ENOMEM;
   }

   /* Learn about current chunk cache settings. */
   if ((H5Pget_chunk_cache(access_pid, &(var->chunk_cache_nelems), 
			   &(var->chunk_cache_size), &rdcc_w0)) < 0)
      return NC_EHDFERR;
   var->chunk_cache_preemption = rdcc_w0;

   /* Allocate space for the name. */
   if (!(var->name = malloc((strlen(obj_name) + 1) * sizeof(char))))
      return NC_ENOMEM;

   /* Check for a weird case: a non-coordinate variable that has the
    * same name as a dimension. It's legal in netcdf, and requires
    * that the HDF5 dataset name be changed. */
   if (!strncmp(obj_name, NON_COORD_PREPEND, strlen(NON_COORD_PREPEND)))
   {
      if (strlen(obj_name) > NC_MAX_NAME)
	 return NC_EMAXNAME;
      strcpy(var->name, &obj_name[strlen(NON_COORD_PREPEND)]);
   }
   else
      strcpy(var->name, obj_name);

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
   else if (layout == H5D_CONTIGUOUS)
      var->contiguous++;

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
            var->shuffle = 1;
            break;
         case H5Z_FILTER_FLETCHER32:
            var->fletcher32 = 1;
            break;
         case H5Z_FILTER_DEFLATE:
            var->deflate++;
            if (cd_nelems != CD_NELEMS_ZLIB ||
                cd_values[0] > MAX_DEFLATE_LEVEL)
               BAIL(NC_EHDFERR);
            var->deflate_level = cd_values[0];
            break;
         case H5Z_FILTER_SZIP:
            var->deflate++;
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
   if ((retval = get_type_info2(grp->file->nc4_info, datasetid, 
				&var->xtype, &var->type_info)))
      BAIL(retval);

   /* Is there a fill value associated with this dataset? */
   if (H5Pfill_value_defined(propid, &fill_status) < 0)
      BAIL(NC_EHDFERR);

   /* Get the fill value, if there is one defined. */
   if (fill_status == H5D_FILL_VALUE_USER_DEFINED)
   {
      /* Allocate space to hold the fill value. */
      if (!var->fill_value)
      {
	 if (var->type_info->class == NC_VLEN)
	 {
	    if (!(var->fill_value = malloc(sizeof(nc_vlen_t))))
	       BAIL(NC_ENOMEM);
	 }
	 else if (var->type_info->size)
	 {
	    if (!(var->fill_value = malloc(var->type_info->size)))
	       BAIL(NC_ENOMEM);
	 }
	 else
	 {
	    if (!(var->fill_value = malloc(sizeof(char *))))
	       BAIL(NC_ENOMEM);
	 }
      }
      
      /* Get the fill value from the HDF5 property lust. */
      if (H5Pget_fill_value(propid, var->type_info->native_typeid, 
			    var->fill_value) < 0)
         BAIL(NC_EHDFERR);
   }
   else
      var->no_fill = 1;

   /* If it's a scale, mark it as such. If not, allocate space to
    * remember whether the dimscale has been attached for each
    * dimension. */
   if (is_scale)
   {
      assert(ndims);
      var->dimscale++;
      if (var->ndims > 1)
      {
	 if ((retval = read_coord_dimids(var)))
	    BAIL(retval);
      }
      else
      {
         var->dimids[0] = grp->dim->dimid;
	 var->dim[0] = grp->dim;
      }
   }
   else
      if (num_scales && ndims && 
	  !(var->dimscale_attached = calloc(ndims, sizeof(int))))
         BAIL(NC_ENOMEM);       
       
   /* If this is not a scale, and has scales, iterate
    * through them. (i.e. this is a variable that is not a
    * coordinate variable) */
   if (!is_scale && num_scales)
   {
      /* Store id information allowing us to match hdf5
       * dimscales to netcdf dimensions. */
      if (!(var->dimscale_hdf5_objids = malloc(ndims * sizeof(struct hdf5_objid))))
         BAIL(NC_ENOMEM);
      for (d = 0; d < var->ndims; d++)
      {
         LOG((5, "read_var: about to iterate over scales for dim %d", d));
         if (H5DSiterate_scales(var->hdf_datasetid, d, NULL, dimscale_visitor,
                                &(var->dimscale_hdf5_objids[d])) < 0)
            BAIL(NC_EHDFERR);
/*       LOG((5, "read_var: collected scale info for dim %d "
         "var %s fileno[0] %d objno[0] %d fileno[1] %d objno[1] %d", 
         d, var->name, var->dimscale_hdf5_objids[d].fileno[0], 
         var->dimscale_hdf5_objids[d].objno[0], 
         var->dimscale_hdf5_objids[d].fileno[1], 
         var->dimscale_hdf5_objids[d].objno[1]));*/
         var->dimscale_attached[d]++;
      }
   }
        
   /* Now read all the attributes of this variable, ignoring the
      ones that hold HDF5 dimension scale information. */
   if ((natts = H5Aget_num_attrs(var->hdf_datasetid)) < 0)
      BAIL(NC_EATTMETA);
   for (a = 0; a < natts; a++)
   {
      /* Close the attribute and try to move on with our
       * lives. Like bits through the network port, so
       * flows the Days of Our Lives! */
      if (attid && H5Aclose(attid) < 0)
         BAIL(NC_EHDFERR);

      /* Open the att and get its name. */
      if ((attid = H5Aopen_idx(var->hdf_datasetid, (unsigned int)a)) < 0)
         BAIL(NC_EATTMETA);
      if (H5Aget_name(attid, NC_MAX_HDF5_NAME, att_name) < 0)
         BAIL(NC_EATTMETA);
      LOG((4, "read_var: a %d att_name %s", a, att_name));

      /* Should we ignore this attribute? */    
      if (strcmp(att_name, REFERENCE_LIST) &&
	  strcmp(att_name, CLASS) &&
	  strcmp(att_name, DIMENSION_LIST) &&
	  strcmp(att_name, NAME) &&
	  strcmp(att_name, COORDINATES))
      {
	 /* Is this the hidden attribute that holds the netCDF
	  * dimension id for a coordinate variable? */
	 if (!strcmp(att_name, NC_DIMID_ATT_NAME))
	 {
	    
	 }
	 else
	 {
	    /* Add to the end of the list of atts for this var. */
	    if ((retval = nc4_att_list_add(&var->att)))
	       BAIL(retval);
	    for (att = var->att; att->next; att = att->next)
	       ;
	    
	    /* Fill in the information we know. */
	    att->attnum = var->natts++;
	    if (!(att->name = malloc((strlen(att_name) + 1) * sizeof(char))))
	       BAIL(NC_ENOMEM);
	    strcpy(att->name, att_name);
	    
	    /* Read the rest of the info about the att,
	     * including its values. */
	    if ((retval = read_hdf5_att(grp, attid, att)))
	       BAIL(retval);
	    
	    att->created++;
	 } /* endif not HDF5 att */
      }
   } /* next attribute */

   /* Is this a deflated variable with a chunksize greater than the
    * current cache size? */
   if ((retval = nc4_adjust_var_cache(grp, var)))
      BAIL(retval);

  exit:
   if (propid > 0 && H5Pclose(propid) < 0)
      BAIL2(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (attid > 0 && H5Aclose(attid) < 0)
      BAIL2(NC_EHDFERR);
   return retval;
}

/* This function is called by nc4_rec_read_metadata to read all the
 * group level attributes (the NC_GLOBAL atts for this group). */
static int
read_grp_atts(NC_GRP_INFO_T *grp)
{
   hid_t attid = 0;
   hsize_t num_obj, i;
   NC_ATT_INFO_T *att;
   NC_TYPE_INFO_T *type;
   char obj_name[NC_MAX_HDF5_NAME + 1];
   int max_len;
   int retval = NC_NOERR;

   num_obj = H5Aget_num_attrs(grp->hdf_grpid);
   for (i = 0; i < num_obj; i++)
   {
      if (attid > 0) 
         H5Aclose(attid);
      if ((attid = H5Aopen_idx(grp->hdf_grpid, (unsigned int)i)) < 0)
         BAIL(NC_EATTMETA);
      if (H5Aget_name(attid, NC_MAX_NAME + 1, obj_name) < 0)
         BAIL(NC_EATTMETA);
      LOG((3, "reading attribute of _netCDF group, named %s", obj_name));

      /* This may be an attribute telling us that strict netcdf-3
       * rules are in effect. If so, we will make note of the fact,
       * but not add this attribute to the metadata. It's not a user
       * attribute, but an internal netcdf-4 one. */
      if (!strcmp(obj_name, NC3_STRICT_ATT_NAME))
         grp->file->nc4_info->cmode |= NC_CLASSIC_MODEL;
      else
      {
         /* Add an att struct at the end of the list, and then go to it. */
         if ((retval = nc4_att_list_add(&grp->att)))
            BAIL(retval);
         for (att = grp->att; att->next; att = att->next)
            ;

	 /* Add the info about this attribute. */
	 max_len = strlen(obj_name) > NC_MAX_NAME ? NC_MAX_NAME : strlen(obj_name);
	 if (!(att->name = malloc((max_len + 1) * sizeof(char))))
	    BAIL(NC_ENOMEM);
         strncpy(att->name, obj_name, max_len);
         att->name[max_len] = 0;
         att->attnum = grp->natts++;
         if ((retval = read_hdf5_att(grp, attid, att)))
            BAIL(retval);
         att->created++;
         if ((retval = nc4_find_type(grp->file->nc4_info, att->xtype, &type)))
            BAIL(retval);
         if (type)
            att->class = type->class;
      }
   }

  exit:
   if (attid > 0 && H5Aclose(attid) < 0)
      BAIL2(NC_EHDFERR);
   return retval;
}

/* This function is called when nc4_rec_read_vars encounters an HDF5
 * dataset when reading a file. */
static int
read_dataset(NC_GRP_INFO_T *grp, char *obj_name)
{
   hid_t datasetid = 0;   
   hid_t spaceid = 0, access_pid = 0;
   int ndims;
   hsize_t dims[NC_MAX_DIMS], max_dims[NC_MAX_DIMS];
   int is_scale = 0;
   int dim_without_var = 0;
   int num_scales = 0;            
   int retval = NC_NOERR;

   /* Open this dataset. */
   if ((datasetid = H5Dopen2(grp->hdf_grpid, obj_name, H5P_DEFAULT)) < 0)
      BAIL(NC_EVARMETA);

   /* Get the current chunk cache settings. */
   if ((access_pid = H5Dget_access_plist(datasetid)) < 0)
      BAIL(NC_EVARMETA);
#ifdef EXTRA_TESTS
   num_plists++;
#endif

   /* Get the dimension information for this dataset. */
   if ((spaceid = H5Dget_space(datasetid)) < 0)
      BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_spaces++;
#endif
   if ((ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
      BAIL(NC_EHDFERR);
   if (ndims > NC_MAX_DIMS)
      BAIL(NC_EMAXDIMS);
   if (H5Sget_simple_extent_dims(spaceid, dims, max_dims) < 0)
      BAIL(NC_EHDFERR);

   /* Is this a dimscale? */
   if ((is_scale = H5DSis_scale(datasetid)) < 0)
      BAIL(NC_EHDFERR);
   if (is_scale)
   {
      /* Read the scale information. */
      if ((retval = read_scale(grp, datasetid, obj_name, dims[0],
                               max_dims[0], &dim_without_var)))
         BAIL(retval);
   }
   else
   {
      /* Find out how many scales are attached to this
       * dataset. H5DSget_num_scales returns an error if there are no
       * scales, so convert a negative return value to zero. */
      num_scales = H5DSget_num_scales(datasetid, 0);
      if (num_scales < 0)
         num_scales = 0;
   }

   /* Add a var to the linked list, and get its metadata,
    * unless this is one of those funny dimscales that are a
    * dimension in netCDF but not a variable. (Spooky!) */
   if (!dim_without_var)
      if ((retval = read_var(grp, datasetid, obj_name, ndims,
                             is_scale, num_scales, access_pid)))
	 BAIL(retval);
   
   if (access_pid && H5Pclose(access_pid) < 0)
      BAIL2(retval);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (spaceid && H5Sclose(spaceid) < 0)
      BAIL2(retval);
#ifdef EXTRA_TESTS
   num_spaces--;
#endif
   return NC_NOERR;

  exit:
   if (access_pid && H5Pclose(access_pid) < 0)
      BAIL2(retval);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (datasetid && H5Dclose(datasetid) < 0)
      BAIL2(retval);
   if (spaceid && H5Sclose(spaceid) <0)
      BAIL2(retval);
#ifdef EXTRA_TESTS
   num_spaces--;
#endif
   return retval;
}

/* Given index, get the HDF5 name of an object and the class of the
 * object (group, type, dataset, etc.). This function will try to use
 * creation ordering, but if that fails it will use default
 * (i.e. alphabetical) ordering. (This is necessary to read existing
 * HDF5 archives without creation ordering). */
/* static int */
/* get_name_by_idx(NC_HDF5_FILE_INFO_T *h5, hid_t hdf_grpid, int i, */
/* 		int *obj_class, char *obj_name) */
/* { */
/*    H5O_info_t obj_info; */
/*    H5_index_t idx_field = H5_INDEX_CRT_ORDER; */
/*    ssize_t size; */
/*    herr_t res; */

/*    /\* These HDF5 macros prevent an HDF5 error message when a */
/*     * non-creation-ordered HDF5 file is opened. *\/ */
/*    H5E_BEGIN_TRY { */
/*       res = H5Oget_info_by_idx(hdf_grpid, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, */
/* 			       i, &obj_info, H5P_DEFAULT); */
/*    } H5E_END_TRY; */
   
/*    /\* Creation ordering not available, so make sure this file is */
/*     * opened for read-only access. This is a plain old HDF5 file being */
/*     * read by netCDF-4. *\/ */
/*    if (res < 0) */
/*    { */
/*       if (H5Oget_info_by_idx(hdf_grpid, ".", H5_INDEX_NAME, H5_ITER_INC, */
/* 			     i, &obj_info, H5P_DEFAULT) < 0) */
/* 	 return NC_EHDFERR; */
/*       if (!h5->no_write) */
/* 	 return NC_ECANTWRITE; */
/*       h5->ignore_creationorder = 1; */
/*       idx_field = H5_INDEX_NAME; */
/*    } */

/*    *obj_class = obj_info.type; */
/*    if ((size = H5Lget_name_by_idx(hdf_grpid, ".", idx_field, H5_ITER_INC, i, */
/* 				  NULL, 0, H5P_DEFAULT)) < 0) */
/*       return NC_EHDFERR; */
/*    if (size > NC_MAX_NAME) */
/*       return NC_EMAXNAME; */
/*    if (H5Lget_name_by_idx(hdf_grpid, ".", idx_field, H5_ITER_INC, i, */
/* 			  obj_name, size+1, H5P_DEFAULT) < 0) */
/*       return NC_EHDFERR; */

/*    LOG((4, "get_name_by_idx: encountered HDF5 object obj_name %s", obj_name)); */

/*    return NC_NOERR; */
/* } */

/* This struct is used to pass information back from the callback
 * function used with H5Literate. */
struct nc_hdf5_link_info 
{
   char name[NC_MAX_NAME + 1];
   H5I_type_t obj_type;   
};   

/* This is a callback function for H5Literate(). 

The parameters of this callback function have the following values or
meanings:

g_id Group that serves as root of the iteration; same value as the
H5Lvisit group_id parameter

name Name of link, relative to g_id, being examined at current step of
the iteration

info H5L_info_t struct containing information regarding that link

op_data User-defined pointer to data required by the application in
processing the link; a pass-through of the op_data pointer provided
with the H5Lvisit function call

*/
static herr_t
visit_link(hid_t g_id, const char *name, const H5L_info_t *info, 
	   void *op_data)  
{
   /* A positive return value causes the visit iterator to immediately
    * return that positive value, indicating short-circuit
    * success. The iterator can be restarted at the next group
    * member. */
   int ret = 1;
   hid_t id;

   /* Get the name, truncating at NC_MAX_NAME. */
   strncpy(((struct nc_hdf5_link_info *)op_data)->name, name, 
	   NC_MAX_NAME);
   
   /* Open this critter. */
   if ((id = H5Oopen_by_addr(g_id, info->u.address)) < 0) 
      return NC_EHDFERR;
   
   /* Is this critter a group, type, data, attribute, or what? */
   if ((((struct nc_hdf5_link_info *)op_data)->obj_type = H5Iget_type(id)) < 0)
      ret = NC_EHDFERR;

   /* Close the critter to release resouces. */
   if (H5Oclose(id) < 0)
      return NC_EHDFERR;
   
   return ret;
}

/* Iterate over one link in the group at a time, returning
 * link_info. The creation_ordering and idx pointers keep track of
 * whether creation ordering works and the most recently examined
 * link. */
static int
nc4_iterate_link(int *ordering_checked, int *creation_ordering, 
		 hid_t grpid, hsize_t *idx, struct nc_hdf5_link_info *link_info)
{
   int res = 0;

   if (*creation_ordering)
   {
      /* These HDF5 macros prevent an HDF5 error message when a
       * non-creation-ordered HDF5 file is opened. */
      H5E_BEGIN_TRY {
	 res = H5Literate(grpid, H5_INDEX_CRT_ORDER, H5_ITER_INC, 
			  idx, visit_link, (void *)link_info);
	 if (res < 0 && *ordering_checked)
	    return NC_EHDFERR;
      } H5E_END_TRY;
   }

   if (!*creation_ordering || res < 0)
   {
      if (H5Literate(grpid, H5_INDEX_NAME, H5_ITER_INC, idx, 
		     visit_link, link_info) != 1)
	 return NC_EHDFERR;
      /* If it didn't work with creation ordering, but did without,
       * then we don't have creation ordering. */
      *creation_ordering = 0;
   }
   
   *ordering_checked = 1;
   return NC_NOERR;
}

/* Recursively open groups and read types. */
int
nc4_rec_read_types(NC_GRP_INFO_T *grp)
{
   hsize_t num_obj, i;
   NC_HDF5_FILE_INFO_T *h5 = grp->file->nc4_info;
   NC_GRP_INFO_T *child_grp;
   hsize_t idx = 0;
   struct nc_hdf5_link_info link_info;
   int ordering_checked = 0;
   int creation_ordering = 1; /* Assume we have it. */
   int retval = NC_NOERR;

   assert(grp && grp->name);
   LOG((3, "nc4_rec_read_types: grp->name %s", grp->name));

   /* Open this HDF5 group and retain its grpid. It will remain open
    * with HDF5 until this file is nc_closed. */
   if (!grp->hdf_grpid)
   {
      if (grp->parent)
      {
         if ((grp->hdf_grpid = H5Gopen2(grp->parent->hdf_grpid, 
					grp->name, H5P_DEFAULT)) < 0)
            return NC_EHDFERR;
      }
      else
      {
         if ((grp->hdf_grpid = H5Gopen2(grp->file->nc4_info->hdfid, 
					"/", H5P_DEFAULT)) < 0)
            return NC_EHDFERR;
      }
   }
   assert(grp->hdf_grpid > 0);

   /* How many objects in this group? */
   if (H5Gget_num_objs(grp->hdf_grpid, &num_obj) < 0)
      return NC_EVARMETA;

   /* For each object in the group... */
   for (i = 0; i < num_obj; i++)
   {
      if ((retval = nc4_iterate_link(&ordering_checked, &creation_ordering, 
				     grp->hdf_grpid, &idx, &link_info)))
	 return retval;

      /* Without creation ordering, file must be read-only. */
      if (!i && !creation_ordering && !h5->no_write)
	 return NC_ECANTWRITE;

      /* Deal with groups and types; ignore the rest. */
      if (link_info.obj_type == H5I_GROUP)
      {
	 LOG((3, "found group %s", link_info.name));
	 if ((retval = nc4_grp_list_add(&(grp->children), h5->next_nc_grpid++, 
					grp, grp->file, link_info.name, &child_grp)))
	    return retval;
	 if ((retval =  nc4_rec_read_types(child_grp)))
	    return retval;
      }
      else if (link_info.obj_type == H5I_DATATYPE)
      {
	 LOG((3, "found datatype %s", link_info.name));
	 if ((retval = read_type(grp, link_info.name)))
	    return retval;
      }
   }

   return NC_NOERR; /* everything worked! */
}

/* This function recursively reads all the var and attribute metadata
   in a HDF5 group, and creates and fills in the netCDF-4 global
   metadata structure. */
int
nc4_rec_read_vars(NC_GRP_INFO_T *grp)
{
   hsize_t num_obj, i;
   NC_GRP_INFO_T *child_grp;
   struct nc_hdf5_link_info link_info;
   hsize_t idx = 0;
   int ordering_checked = 0;
   int creation_ordering = 1; /* Assume we have it. */
   int retval = NC_NOERR;

   assert(grp && grp->name && grp->hdf_grpid > 0);
   LOG((3, "nc4_rec_read_vars: grp->name %s", grp->name));

   /* How many objects in this group? */
   if (H5Gget_num_objs(grp->hdf_grpid, &num_obj) < 0)
      return NC_EVARMETA;

   /* For each object in the group... */
   for (i = 0; i < num_obj; i++)
   {
      if ((retval = nc4_iterate_link(&ordering_checked, &creation_ordering, 
				     grp->hdf_grpid, &idx, &link_info)))
	 return retval;
      
      /* Deal with datasets. */
      switch(link_info.obj_type)
      {
         case H5I_GROUP:
	    LOG((3, "re-encountering group %s", link_info.name));

	    /* The NC_GROUP_INFO_T for this group already exists. Find it. */
	    for (child_grp = grp->children; child_grp; child_grp = child_grp->next)
	       if (!strcmp(child_grp->name, link_info.name))
		  break;
	    if (!child_grp)
	       return NC_EHDFERR;

            /* Recursively read the child group's vars. */
            if ((retval =  nc4_rec_read_vars(child_grp)))
               return retval;
            break;
         case H5I_DATASET:
	    LOG((3, "found dataset %s", link_info.name));

            /* Learn all about this dataset, which may be a dimscale
             * (i.e. dimension metadata), or real data. */
            if ((retval = read_dataset(grp, link_info.name)))
	       return retval;
            break;
         case H5I_DATATYPE:
	    LOG((3, "already handled type %s", link_info.name));
            break;
         default:
            LOG((0, "Unknown object class %d in nc4_rec_read_vars!", 
                 link_info.obj_type));
      }
   }

   /* Scan the group for global (i.e. group-level) attributes. */
   if ((retval = read_grp_atts(grp)))
      return retval;

   return NC_NOERR; /* everything worked! */
}

/* Open a netcdf-4 file. Things have already been kicked off in
 * ncfunc.c in nc_open, but here the netCDF-4 part of opening a file
 * is handled. */
static int
nc4_open_file(const char *path, int mode, MPI_Comm comm,
	      MPI_Info info, NC_FILE_INFO_T *nc)
{
   hid_t fapl_id = H5P_DEFAULT;
   unsigned flags = (mode & NC_WRITE) ? 
      H5F_ACC_RDWR : H5F_ACC_RDONLY;
   int retval;

   LOG((3, "nc4_open_file: path %s mode %d", path, mode));
   assert(path && nc);

   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, mode)))
      BAIL(retval);
   assert(nc->nc4_info && nc->nc4_info->root_grp);
   
   /* Need this access plist to control how HDF5 handles open onjects
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

#ifdef USE_PARALLEL
   /* If this is a parallel file create, set up the file creation
      property list. */
   if (mode & NC_MPIIO || mode & NC_MPIPOSIX)
   {
      nc->nc4_info->parallel++;
      if (mode & NC_MPIIO)  /* MPI/IO */
      {
	 LOG((4, "opening parallel file with MPI/IO"));
	 if (H5Pset_fapl_mpio(fapl_id, comm, info) < 0)
	    BAIL(NC_EPARINIT);
      }
      else /* MPI/POSIX */
      {
	 LOG((4, "opening parallel file with MPI/posix"));
	 if (H5Pset_fapl_mpiposix(fapl_id, comm, 0) < 0)
	    BAIL(NC_EPARINIT);
      }
   }
#else /* only set cache for non-parallel. */
   if (H5Pset_cache(fapl_id, 0, nc4_chunk_cache_nelems, nc4_chunk_cache_size, 
		    nc4_chunk_cache_preemption) < 0)
      BAIL(NC_EHDFERR);
   LOG((4, "nc4_open_file: set HDF raw chunk cache to size %d nelems %d preemption %f", 
	nc4_chunk_cache_size, nc4_chunk_cache_nelems, nc4_chunk_cache_preemption));
#endif /* USE_PARALLEL */
   
   /* The NetCDF-3.x prototype contains an mode option NC_SHARE for
      multiple processes accessing the dataset concurrently.  As there
      is no HDF5 equivalent, NC_SHARE is treated as NC_NOWRITE. */
   if ((nc->nc4_info->hdfid = H5Fopen(path, flags, fapl_id)) < 0)
      BAIL(NC_EHDFERR);

   /* Does the mode specify that this file is read-only? */
   if ((mode & NC_WRITE) == 0)
      nc->nc4_info->no_write++;

   /* Now read in all the metadata. Some types and dimscale
    * information may be difficult to resolve here, if, for example, a
    * dataset of user-defined type is encountered before the
    * definition of that type. */
   if ((retval = nc4_rec_read_types(nc->nc4_info->root_grp)))
      BAIL(retval);
   if ((retval = nc4_rec_read_vars(nc->nc4_info->root_grp)))
      BAIL(retval);

   /* Now figure out which netCDF dims are indicated by the dimscale
    * information. */
   if ((retval = nc4_rec_match_dimscales(nc->nc4_info->root_grp)))
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

   return NC_NOERR;

  exit:
   if (fapl_id != H5P_DEFAULT) H5Pclose(fapl_id);
#ifdef EXTRA_TESTS
   num_plists--;
#endif
   if (nc->nc4_info->hdfid > 0) H5Fclose(nc->nc4_info->hdfid);
   if (nc->nc4_info) free(nc->nc4_info);
   return retval;
}

/* Given an HDF4 type, set a pointer to netcdf type. */
#ifdef USE_HDF4   
static int
get_netcdf_type_from_hdf4(NC_HDF5_FILE_INFO_T *h5, int32 hdf4_typeid, 
			  nc_type *xtype, NC_TYPE_INFO_T *type_info)
{
   int t;
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
      case DFNT_INT8:
	 *xtype = NC_BYTE;
	 t = 1;
	 break;
      case DFNT_INT16:
	 *xtype = NC_SHORT;
	 t = 2;
	 break;
      case DFNT_UINT16:
	 *xtype = NC_USHORT;
	 t = 7;
	 break;
      case DFNT_INT32:
	 *xtype = NC_INT;
	 t = 3;
	 break;
      case DFNT_UINT32:
	 *xtype = NC_UINT;
	 t = 8;
	 break;
      case DFNT_FLOAT32:
	 *xtype = NC_FLOAT;
	 t = 4;
	 break;
      case DFNT_FLOAT64:
	 *xtype = NC_DOUBLE;
	 t = 5;
	 break;
      default:
	 *xtype = NC_NAT;
	 return NC_EBADTYPID;
   }

   if (type_info)
   {
      if (hdf4_typeid == DFNT_FLOAT32 || hdf4_typeid == DFNT_FLOAT64)
	 type_info->class = H5T_FLOAT;
      else if (hdf4_typeid == DFNT_CHAR)
	 type_info->class = H5T_STRING;
      else
	 type_info->class = H5T_INTEGER;
      type_info->endianness = NC_ENDIAN_BIG;
      type_info->nc_typeid = *xtype;
      if (type_info->name)
	 free(type_info->name);
      if (!(type_info->name = malloc((strlen(nc_type_name[t]) + 1) * sizeof(char))))
	 return NC_ENOMEM;
      strcpy(type_info->name, nc_type_name[t]);      
   }

   return NC_NOERR;
}
#endif /* USE_HDF4 */

/* Open a HDF4 file. Things have already been kicked off in nc_open,
 * but here the netCDF-4 part of opening a file is handled. */
static int
nc4_open_hdf4_file(const char *path, int mode, NC_FILE_INFO_T *nc)
{
#ifdef USE_HDF4
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC_ATT_INFO_T *att;
   NC_VAR_INFO_T *var;
   int32 num_datasets, num_gatts;
   int32 rank;
   int v, d, a;
   int retval;

   LOG((3, "nc4_open_hdf4_file: path %s mode %d", path, mode));
   assert(path && nc);

   /* Must be read-only access to hdf4 files. */
   if (mode & NC_WRITE)
      return NC_EINVAL;

   /* Add necessary structs to hold netcdf-4 file data. */
   if ((retval = nc4_nc4f_list_add(nc, path, mode)))
      return retval;
   assert(nc->nc4_info && nc->nc4_info->root_grp);
   h5 = nc->nc4_info;
   h5->hdf4++;
   grp = h5->root_grp;
   h5->no_write++;

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
      if ((retval = nc4_att_list_add(&h5->root_grp->att)))
	 return retval;
      for (att = h5->root_grp->att; att->next; att = att->next)
	 ;
      att->attnum = grp->natts++;
      att->created++;

      /* Learn about this attribute. */
      if (!(att->name = malloc(NC_MAX_HDF4_NAME * sizeof(char))))
	 return NC_ENOMEM;
      if (SDattrinfo(h5->sdid, a, att->name, &att_data_type, &att_count)) 
	 return NC_EATTMETA;
      if ((retval = get_netcdf_type_from_hdf4(h5, att_data_type, 
					      &att->xtype, NULL)))
	 return retval;
      att->len = att_count;

      /* Allocate memory to hold the data. */
      if ((retval = nc4_get_typelen_mem(h5, att->xtype, 0, &att_type_size)))
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
      int32 data_type, num_atts;
      int32 dimsize[NC_MAX_DIMS];
      size_t var_type_size;
      int a;

      /* Add a variable to the end of the group's var list. */
      if ((retval = nc4_var_list_add(&grp->var, &var)))
	 return retval;
      var->varid = grp->nvars++;
      var->created = 1;
      var->written_to = 1;
            
      /* Open this dataset in HDF4 file. */
      if ((var->sdsid = SDselect(h5->sdid, v)) == FAIL)
	 return NC_EVARMETA;

      /* Get shape, name, type, and attribute info about this dataset. */
      if (!(var->name = malloc(NC_MAX_HDF4_NAME + 1)))
	 return NC_ENOMEM;
      if (SDgetinfo(var->sdsid, var->name, &rank, dimsize, &data_type, &num_atts))
	 return NC_EVARMETA;
      var->ndims = rank;
      var->hdf4_data_type = data_type;

      /* Fill special type_info struct for variable type information. */
      if (!(var->type_info = calloc(1, sizeof(NC_TYPE_INFO_T))))
	 return NC_ENOMEM;
      if ((retval = get_netcdf_type_from_hdf4(h5, data_type, &var->xtype, var->type_info)))
	 return retval;
      if ((retval = nc4_get_typelen_mem(h5, var->xtype, 0, &var_type_size)))
	 return retval;
      var->type_info->size = var_type_size;
      LOG((3, "reading HDF4 dataset %s, rank %d netCDF type %d", var->name, 
	   rank, var->xtype));

      /* Get the fill value. */
      if (!(var->fill_value = malloc(var_type_size)))
	 return NC_ENOMEM;
      if (SDgetfillvalue(var->sdsid, var->fill_value))
      {
	 /* Whoops! No fill value! */
	 free(var->fill_value);
	 var->fill_value = NULL;
      }

      /* Allocate storage for dimension info in this variable. */
      if (var->ndims)
      {
	 if (!(var->dim = malloc(sizeof(NC_DIM_INFO_T *) * var->ndims)))
	    return NC_ENOMEM;
	 if (!(var->dimids = malloc(sizeof(int) * var->ndims)))
	    return NC_ENOMEM;
      }

      /* Find its dimensions. */
      for (d = 0; d < var->ndims; d++)
      {
	 int32 dimid, dim_len, dim_data_type, dim_num_attrs;
	 char dim_name[NC_MAX_NAME + 1];
	 NC_DIM_INFO_T *dim;

	 if ((dimid = SDgetdimid(var->sdsid, d)) == FAIL)
	    return NC_EDIMMETA;
	 if (SDdiminfo(dimid, dim_name, &dim_len, &dim_data_type, 
		       &dim_num_attrs))
	    return NC_EDIMMETA;

	 /* Do we already have this dimension? HDF4 explicitly uses
	  * the name to tell. */
	 for (dim = grp->dim; dim; dim = dim->next)
	    if (!strcmp(dim->name, dim_name))
	       break;

	 /* If we didn't find this dimension, add one. */
	 if (!dim)
	 {
	    LOG((4, "adding dimension %s for HDF4 dataset %s", 
		 dim_name, var->name));
	    if ((retval = nc4_dim_list_add(&grp->dim)))
	       return retval;
	    grp->ndims++;
	    dim = grp->dim;
	    dim->dimid = grp->file->nc4_info->next_dimid++;
	    if (strlen(dim_name) > NC_MAX_HDF4_NAME)
	       return NC_EMAXNAME;
	    if (!(dim->name = malloc(NC_MAX_HDF4_NAME + 1)))
	       return NC_ENOMEM;
	    strcpy(dim->name, dim_name);
	    if (dim_len)
	       dim->len = dim_len;
	    else
	       dim->len = *dimsize;
	 }

	 /* Tell the variable the id of this dimension. */
	 var->dimids[d] = dim->dimid;
      }

      /* Read the atts. */
      for (a = 0; a < num_atts; a++)
      {
	 int32 att_data_type, att_count;
	 size_t att_type_size;

	 /* Add to the end of the list of atts for this var. */
         if ((retval = nc4_att_list_add(&var->att)))
            return retval;
         for (att = var->att; att->next; att = att->next)
            ;
	 att->attnum = var->natts++;
	 att->created++;

	 /* Learn about this attribute. */
	 if (!(att->name = malloc(NC_MAX_HDF4_NAME * sizeof(char))))
	    return NC_ENOMEM;
	 if (SDattrinfo(var->sdsid, a, att->name, &att_data_type, &att_count)) 
	    return NC_EATTMETA;
	 if ((retval = get_netcdf_type_from_hdf4(h5, att_data_type, 
						 &att->xtype, NULL)))
	    return retval;
	 att->len = att_count;

	 /* Allocate memory to hold the data. */
	 if ((retval = nc4_get_typelen_mem(h5, att->xtype, 0, &att_type_size)))
	    return retval;
	 if (!(att->data = malloc(att_type_size * att->len)))
	    return NC_ENOMEM;

	 /* Read the data. */
	 if (SDreadattr(var->sdsid, a, att->data)) 
	    return NC_EHDFERR;
      }
   } /* next var */

#ifdef LOGGING
   /* This will print out the names, types, lens, etc of the vars and
      atts in the file, if the logging level is 2 or greater. */ 
   log_metadata_nc(h5->root_grp->file);
#endif
   return NC_NOERR;   
#endif /* USE_HDF4 */
   return NC_ENOTBUILT;
}

int
NC4_open(const char *path, int mode, int basepe, size_t *chunksizehintp, 
	 int use_parallel, void *mpidata, NC_Dispatch *dispatch, NC **ncpp)
{
   int hdf_file = 0;
   NC_FILE_INFO_T *nc_file;
#ifdef USE_PARALLEL
   MPI_Comm comm = 0; 
   MPI_Info info = 0;	
#else
   int comm = 0, info = 0;
#endif /* USE_PARALLEL */
   int res;

   assert(ncpp && path);

   LOG((1, "nc_open_file: path %s mode %d comm %d info %d", 
	path, mode, comm, info));

#ifdef USE_PARALLEL
   if (mpidata) 
   { 
      NC_MPI_INFO *nmi = (NC_MPI_INFO *)mpidata; 
      comm = nmi->comm; info = nmi->info; 
   }
#endif /* USE_PARALLEL */
    
   /* If this is our first file, turn off HDF5 error messages. */
   if (virgin)
   {
      if (H5Eset_auto(NULL, NULL) < 0)
	 LOG((0, "Couldn't turn off HDF5 error messages!"));
      LOG((1, "HDF5 error messages turned off!"));
      virgin = 0;
   }

   /* Check the mode for validity. First make sure only certain bits
    * are turned on. Also MPI I/O and MPI POSIX cannot both be
    * selected at once. */
   if (mode & ~(NC_WRITE | NC_SHARE | NC_MPIIO | NC_MPIPOSIX | 
		NC_PNETCDF | NC_NOCLOBBER | NC_NETCDF4 | NC_CLASSIC_MODEL) ||
       (mode & NC_MPIIO && mode & NC_MPIPOSIX))
      return NC_EINVAL;

   /* Figure out if this is a hdf4 or hdf5 file. */
   if ((res = nc_check_for_hdf(path, use_parallel, comm, info, &hdf_file)))
      return res;

   /* Allocate the storage for this file info struct, and fill it with
      zeros. */
   if ((res = nc4_file_list_add(&nc_file,dispatch)))
      return res;

   /* Depending on the type of file, open it. */
   if (hdf_file == NC_HDF5_FILE)
   {
      nc_file->int_ncid = nc_file->ext_ncid;
      res = nc4_open_file(path, mode, comm, info, nc_file);
   }
   else if (hdf_file == NC_HDF4_FILE)
   {
      nc_file->int_ncid = nc_file->ext_ncid;
      res = nc4_open_hdf4_file(path, mode, nc_file);
   }
#ifdef USE_PNETCDF
   else if (mode & NC_PNETCDF)
   {
      int pnetcdf_nvars, i;

      res = ncmpi_open(comm, path, mode, info, &(nc_file->int_ncid));
      nc_file->pnetcdf_file++;

      /* Default to independent access, like netCDF-4/HDF5 files. */
      if (!res)
	 res = ncmpi_begin_indep_data(nc_file->int_ncid);

      /* I need to keep track of the ndims of each var to translate
       * start, count, and stride arrays to MPI_Offset type. */
      if (!res)
      {
	 res = ncmpi_inq_nvars(nc_file->int_ncid, &pnetcdf_nvars);
	 for (i = 0; i < pnetcdf_nvars; i++)
	    res = ncmpi_inq_varndims(nc_file->int_ncid, i, 
				     &(nc_file->pnetcdf_ndims[i]));

      }
   }
#endif /* USE_PNETCDF */
   else /* netcdf */
   {
      assert(0);
   }

   /* If it succeeds, pass back the new ncid. Otherwise, remove this
      file from the list. */
   if (res)
   {
      if(nc_file != NULL) nc4_file_list_del(nc_file);
   }
   else
   {
      *ncpp = (NC*)nc_file;
   }

   return res;
}

/* Unfortunately HDF only allows specification of fill value only when
   a dataset is created. Whereas in netcdf, you first create the
   variable and then (optionally) specify the fill value. To
   accomplish this in HDF5 I have to delete the dataset, and recreate
   it, with the fill value specified. */
int 
NC4_set_fill(int ncid, int fillmode, int *old_modep)
{
   NC_FILE_INFO_T *nc;
 
   LOG((2, "nc_set_fill: ncid 0x%x fillmode %d", ncid, fillmode));

   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Is this a netcdf-3 file? */
   assert(nc->nc4_info);

   /* Trying to set fill on a read-only file? You sicken me! */
   if (nc->nc4_info->no_write)
      return NC_EPERM;

   /* Did you pass me some weird fillmode? */
   if (fillmode != NC_FILL && fillmode != NC_NOFILL)
      return NC_EINVAL;

   /* If the user wants to know, tell him what the old mode was. */
   if (old_modep)
      *old_modep = nc->nc4_info->fill_mode;

   nc->nc4_info->fill_mode = fillmode;

   return NC_NOERR;
}

/* Put the file back in redef mode. This is done automatically for
 * netcdf-4 files, if the user forgets. */
int
NC4_redef(int ncid)
{
   NC_FILE_INFO_T *nc;

   LOG((1, "nc_redef: ncid 0x%x", ncid));

   /* Find this file's metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_redef(nc->int_ncid);
#endif /* USE_PNETCDF */

   /* Handle netcdf-3 files. */
   assert(nc->nc4_info);

   /* If we're already in define mode, return an error. */
   if (nc->nc4_info->flags & NC_INDEF)
      return NC_EINDEFINE;

   /* If the file is read-only, return an error. */
   if (nc->nc4_info->no_write)
      return NC_EPERM;

   /* Set define mode. */
   nc->nc4_info->flags |= NC_INDEF;

   /* For nc_abort, we need to remember if we're in define mode as a
      redef. */
   nc->nc4_info->redef++;

   return NC_NOERR;
}

/* For netcdf-4 files, this just calls nc_enddef, ignoring the extra
 * parameters. */
int
NC4__enddef(int ncid, size_t h_minfree, size_t v_align,
	    size_t v_minfree, size_t r_align)
{
   NC_FILE_INFO_T *nc;

   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   return NC4_enddef(ncid);
}

/* Take the file out of define mode. This is called automatically for
 * netcdf-4 files, if the user forgets. */
static int NC4_enddef(int ncid)
{
   NC_FILE_INFO_T *nc;

   LOG((1, "nc_enddef: ncid 0x%x", ncid));

   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

#ifdef USE_PNETCDF
   if (nc->pnetcdf_file)
   {
      int res;
      res = ncmpi_enddef(nc->int_ncid);
      if (!res)
      {
	 if (nc->pnetcdf_access_mode == NC_INDEPENDENT)
	    res = ncmpi_begin_indep_data(nc->int_ncid);
      }
      return res;
   }
#endif /* USE_PNETCDF */

   /* Take care of netcdf-3 files. */
   assert(nc->nc4_info);

   return nc4_enddef_netcdf4_file(nc->nc4_info);
}

/* This function will write all changed metadata, and (someday) reread
 * all metadata from the file. */
static int
sync_netcdf4_file(NC_HDF5_FILE_INFO_T *h5)
{
   int retval;

   assert(h5);
   LOG((3, "sync_netcdf4_file"));

   /* If we're in define mode, that's an error, for strict nc3 rules,
    * otherwise, end define mode. */
   if (h5->flags & NC_INDEF)
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
	 return NC_EINDEFINE;

      /* Turn define mode off. */
      h5->flags ^= NC_INDEF;
      
      /* Redef mode needs to be tracked separately for nc_abort. */
      h5->redef = 0;
   }

#ifdef LOGGING
   /* This will print out the names, types, lens, etc of the vars and
      atts in the file, if the logging level is 2 or greater. */ 
   log_metadata_nc(h5->root_grp->file);
#endif

   /* Write any metadata that has changed. */
   if (!(h5->cmode & NC_NOWRITE))
   {
      if ((retval = nc4_rec_write_types(h5->root_grp)))
	 return retval;
      if ((retval = nc4_rec_write_metadata(h5->root_grp)))
	 return retval;
   }

   H5Fflush(h5->hdfid, H5F_SCOPE_GLOBAL);

   /* Reread all the metadata. */
   /*if ((retval = nc4_rec_read_metadata(grp)))
     return retval;*/

   return retval;
}

/* Flushes all buffers associated with the file, after writing all
   changed metadata. This may only be called in data mode. */
int
NC4_sync(int ncid)
{
   NC_FILE_INFO_T *nc;
   int retval;

   LOG((2, "nc_sync: ncid 0x%x", ncid));

   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_sync(nc->int_ncid);
#endif /* USE_PNETCDF */

   /* Take care of netcdf-3 files. */
   assert(nc->nc4_info);

   /* If we're in define mode, we can't sync. */
   if (nc->nc4_info && nc->nc4_info->flags & NC_INDEF)
   {
      if (nc->nc4_info->cmode & NC_CLASSIC_MODEL)
	 return NC_EINDEFINE;
      if ((retval = nc_enddef(ncid)))
	 return retval;
   }

   return sync_netcdf4_file(nc->nc4_info);
}

/* This function will free all allocated metadata memory, and close
   the HDF5 file. The group that is passed in must be the root group
   of the file. */
static int
close_netcdf4_file(NC_HDF5_FILE_INFO_T *h5, int abort)
{
   int retval;

   assert(h5 && h5->root_grp);
   LOG((3, "close_netcdf4_file: h5->path %s abort %d", 
	h5->path, abort));

   /* According to the docs, always end define mode on close. */
   if (h5->flags & NC_INDEF)
      h5->flags ^= NC_INDEF;

   /* Sync the file, unless we're aborting, or this is a read-only
    * file. */
   if (!h5->no_write && !abort)
      if ((retval = sync_netcdf4_file(h5)))
	 return retval;

   /* Delete all the list contents for vars, dims, and atts, in each
    * group. */
   if ((retval = nc4_rec_grp_del(&h5->root_grp, h5->root_grp)))
      return retval;

   /* Close hdf file. */
   if (h5->hdf4)
   {
#ifdef USE_HDF4
      if (SDend(h5->sdid))
	 return NC_EHDFERR;
#endif /* USE_HDF4 */
   } 
   else
   {
      if (H5Fclose(h5->hdfid) < 0) 
      {
#ifdef LOGGING
	 /* If the close doesn't work, probably there are still some HDF5
	  * objects open, which means there's a bug in the library. So
	  * print out some info on to help the poor programmer figure it
	  * out. */
	 {
	    int nobjs;
	    if ((nobjs = H5Fget_obj_count(h5->hdfid, H5F_OBJ_ALL) < 0))
	       return NC_EHDFERR;
	    LOG((0, "There are %d HDF5 objects open!", nobjs));
	 }
#endif      
	 return NC_EHDFERR;
      }
/*      if (H5garbage_collect() < 0)
	return NC_EHDFERR;	 */
   }

   /* Delete the memory for the path, if it's been allocated. */
   if (h5->path)
      free(h5->path);

   /* Free the nc4_info struct. */
   free(h5);
   return NC_NOERR;
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
   NC_FILE_INFO_T *nc;
   int delete_file = 0;
   char path[NC_MAX_NAME + 1];
   int retval = NC_NOERR;

   LOG((2, "nc_abort: ncid 0x%x", ncid));

   /* Find metadata for this file. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_abort(nc->int_ncid);
#endif /* USE_PNETCDF */

   /* If this is a netcdf-3 file, let the netcdf-3 library handle it. */
   assert(nc->nc4_info);

   /* If we're in define mode, but not redefing the file, delete it. */
   if (nc->nc4_info->flags & NC_INDEF && !nc->nc4_info->redef)
   {
      delete_file++;
      strcpy(path, nc->nc4_info->path);
      /*strcpy(path, nc->path);*/
   }

   /* Free any resources the netcdf-4 library has for this file's
    * metadata. */
   if ((retval = close_netcdf4_file(nc->nc4_info, 1)))
      return retval;
   
   /* Delete the file, if we should. */
   if (delete_file)
      remove(path);

   /* Delete this entry from our list of open files. */
   nc4_file_list_del(nc);

   return retval;
}

/* Close the netcdf file, writing any changes first. */
int
NC4_close(int ncid)
{
   NC_GRP_INFO_T *grp;
   NC_FILE_INFO_T *nc;
   NC_HDF5_FILE_INFO_T *h5;
   int retval;

   LOG((1, "nc_close: ncid 0x%x", ncid));

   /* Find our metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_close(nc->int_ncid);
#endif /* USE_PNETCDF */

   assert(h5 && nc);

   /* This must be the root group. */
   if (grp->parent)
      return NC_EBADGRPID;

   /* Call the nc4 close. */
   if ((retval = close_netcdf4_file(grp->file->nc4_info, 0)))
      return retval;

   /* Delete this entry from our list of open files. */
   if (nc->path)
      free(nc->path);
   nc4_file_list_del(nc);

   /* Reset the ncid numbers if there are no more files open. */
   if(count_NCList() == 0)
      nc4_file_list_free();

   return NC_NOERR;
}

/* It's possible for any of these pointers to be NULL, in which case
   don't try to figure out that value. */
int
NC4_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp)
{
   NC_FILE_INFO_T *nc;
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC_DIM_INFO_T *dim;
   NC_ATT_INFO_T *att;
   NC_VAR_INFO_T *var;
   int retval;

   LOG((2, "nc_inq: ncid 0x%x", ncid)); 

   /* Find file metadata. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_inq(nc->int_ncid, ndimsp, nvarsp, nattsp, unlimdimidp);
#endif /* USE_PNETCDF */

   /* Netcdf-3 files are already taken care of. */
   assert(h5 && grp && nc);

   /* Count the number of dims, vars, and global atts. */
   if (ndimsp)
   {
      *ndimsp = 0;
      for (dim = grp->dim; dim; dim = dim->next)
	 (*ndimsp)++;
   }
   if (nvarsp)
   {
      *nvarsp = 0;
      for (var = grp->var; var; var= var->next)
	 (*nvarsp)++;
   }
   if (nattsp)
   {
      *nattsp = 0;
      for (att = grp->att; att; att = att->next)
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
      for (dim = grp->dim; dim; dim = dim->next)
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
   LOG((3, "nc4_enddef_netcdf4_file"));

   /* If we're not in define mode, return an error. */
   if (!(h5->flags & NC_INDEF))
      return NC_ENOTINDEFINE;

   /* Turn define mode off. */
   h5->flags ^= NC_INDEF;

   /* Redef mode needs to be tracked separately for nc_abort. */
   h5->redef = 0;

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


