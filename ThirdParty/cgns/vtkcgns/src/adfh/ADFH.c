/*-------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * HDF5 interface to ADF
 *-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

#if defined(_WIN32) && !defined(__NUTC__)
# include <io.h>
# define ACCESS _access
# define UNLINK _unlink
#else
# include <unistd.h>
# define ACCESS access
# define UNLINK unlink
#endif

#include "ADFH.h"
#include "vtk_hdf5.h"
#include "cgns_io.h" /* for cgio_find_file */

#if CG_BUILD_PARALLEL
#include "mpi.h"
extern int pcg_mpi_initialized;
extern MPI_Info pcg_mpi_info;
extern hid_t default_pio_mode;
#endif

#define ADFH_FORCE_ID_CLOSE
/*#define ADFH_H5F_CLOSE_STRONG*/
/*#define ADFH_DEBUG_ON*/

#define ADFH_NO_ORDER
#define ADFH_USE_STRINGS
#define ADFH_FORTRAN_INDEXING

static int CompressData = -1;

#if CG_BUILD_PARALLEL
static MPI_Comm ParallelMPICommunicator = MPI_COMM_NULL;
#endif

#define TO_UPPER( c ) ((islower(c))?(toupper(c)):(c))

/* HDF5 compact storage limit */
#define CGNS_64KB (64 * 1024)

/*
 * ADF names are not allowed to start with a space.
 * Since HDF5 allows this, use the space to hide data
 */

/* dataset and group names */

#define D_PREFIX  ' '
#ifdef ADFH_FORTRAN_INDEXING
#define D_VERSION " hdf5version"
#define D_OLDVERS " version"
#else
#define D_VERSION " version"
#endif
#define D_FORMAT  " format"
#define D_DATA    " data"
#define D_FILE    " file"
#define D_PATH    " path"
#define D_LINK    " link"

/* attribute names */

#define A_NAME    "name"
#define A_LABEL   "label"
#define A_TYPE    "type"
#define A_ORDER   "order"
#define A_MOUNT   "mount"
#define A_FILE    "file"
#define A_REFCNT  "refcnt"
#define A_FLAGS   "flags"

/* debugging */
#define ADFH_CHECK_HID( hid ) \
if (hid <0) {printf("#### BAD ID [%5d] ",__LINE__);fflush(stdout); }

#define ADFH_PREFIX       "#### DBG         "
#ifdef ADFH_DEBUG_ON
#define ADFH_DEBUG(aaa) \
printf("#### DBG [%5d] ",__LINE__);fflush(stdout); \
printf aaa ; printf("\n"); fflush(stdout);
#define DROP( msg ) printf("XX " msg "\n");fflush(stdout);
#else
#define ADFH_DEBUG(a) {;}
#define DROP( msg ) {;}
#endif

/* ADF data types */

#define ADFH_MT "MT"
#define ADFH_LK "LK"
#define ADFH_B1 "B1"
#define ADFH_C1 "C1"
#define ADFH_I4 "I4"
#define ADFH_I8 "I8"
#define ADFH_U4 "U4"
#define ADFH_U8 "U8"
#define ADFH_R4 "R4"
#define ADFH_R8 "R8"
/* these are not supported */
#define ADFH_X4 "X4"
#define ADFH_X8 "X8"

/* file open modes */

#define ADFH_MODE_NEW 1
#define ADFH_MODE_OLD 2
#define ADFH_MODE_RDO 3

/* the following keeps track of open and mounted files */

#define ADFH_MAXIMUM_FILES 128

/* Start to prepare re-entrance into lib, gather statics in one global struct  */
/* Then, you'll just have to handle struct with something else but a static... */
/* MTA stands for... Multi-Threads-Aware */
typedef struct _ADFH_MTA {
  int g_init;               /* set when initialization done */
  int g_error_state;        /* zero means do not stop on error (one stops) */
  int i_start;
  int i_len;
  int n_length;
  int n_names;
#ifdef ADFH_NO_ORDER
  int i_count;
#endif
  /* HDF5 property lists */
  hid_t g_proplink;
  hid_t g_propgroupcreate;
  hid_t g_propdataset;

  int   g_flags;
  hid_t g_files[ADFH_MAXIMUM_FILES];

#ifndef ADFH_FORCE_ID_CLOSE
  /* object ids returned to API user that should be closed */
  hid_t *g_extids[ADFH_MAXIMUM_FILES];
  int    n_extids[ADFH_MAXIMUM_FILES]; /* number of extids */
  int    x_extids[ADFH_MAXIMUM_FILES]; /* max allocated in g_extids */
#endif
} ADFH_MTA;

static ADFH_MTA  *mta_root=NULL;

/* error codes and messages - do not care about multi-threading here */
static struct _ErrorList {
  int errcode;
  char *errmsg;
} ErrorList[] = {
  {NO_ERROR,                "No Error"},
  {STRING_LENGTH_ZERO,      "String length of zero or blank string detected"},
  {STRING_LENGTH_TOO_BIG,   "String length longer than maximum allowable length"},
  {TOO_MANY_ADF_FILES_OPENED,"Too many files opened"},
  {ADF_FILE_STATUS_NOT_RECOGNIZED,"File status was not recognized"},
  {FILE_OPEN_ERROR,         "File-open error"},
  {NULL_STRING_POINTER,     "A string pointer is NULL"},
  {REQUESTED_NEW_FILE_EXISTS,"File Open Error: NEW - File already exists"},
  {ADF_FILE_FORMAT_NOT_RECOGNIZED,"File format was not recognized"},
  {REQUESTED_OLD_FILE_NOT_FOUND,"File Open Error: OLD - File does not exist"},
  {MEMORY_ALLOCATION_FAILED,"Memory allocation failed"},
  {DUPLICATE_CHILD_NAME,    "Duplicate child name under a parent node"},
  {ZERO_DIMENSIONS,         "Node has no dimensions"},
  {BAD_NUMBER_OF_DIMENSIONS,"Node's number-of-dimensions is not in legal range"},
  {CHILD_NOT_OF_GIVEN_PARENT,"Specified child is NOT a child of the specified parent"},
  {INVALID_DATA_TYPE,       "Invalid Data-Type"},
  {NULL_POINTER,            "A pointer is NULL"},
  {NO_DATA,                 "Node has no data associated with it"},
  {END_OUT_OF_DEFINED_RANGE,"Bad end value"},
  {BAD_STRIDE_VALUE,        "Bad stride value"},
  {MINIMUM_GT_MAXIMUM,      "Minimum value is greater than the maximum value"},
  {DATA_TYPE_NOT_SUPPORTED, "The data format is not support on a particular machine"},
  {FILE_CLOSE_ERROR,        "File Close error"},
  {START_OUT_OF_DEFINED_RANGE,"Bad start value"},
  {ZERO_LENGTH_VALUE,       "A value of zero is not allowable"},
  {BAD_DIMENSION_VALUE,     "Bad dimension value"},
  {BAD_ERROR_STATE,         "Error state must be either a 0 (zero) or a 1 (one)"},
  {UNEQUAL_MEMORY_AND_DISK_DIMS,"Unequal dimensional specifications for disk and memory"},
  {NODE_IS_NOT_A_LINK,      "The node is not a link.  It was expected to be a link"},
  {LINK_TARGET_NOT_THERE,   "The linked-to node does not exist"},
  {LINKED_TO_FILE_NOT_THERE,"The file of a linked-node is not accessible"},
  {INVALID_NODE_NAME,       "Node name contains invalid characters"},
  {FFLUSH_ERROR,            "H5Fflush:flush error"},
  {NULL_NODEID_POINTER,     "The node ID pointer is NULL"},
  {MAX_FILE_SIZE_EXCEEDED,  "The maximum size for a file exceeded"},
  {MAX_INT32_SIZE_EXCEEDED, "dimensions exceed that for a 32-bit integer"},

  {ADFH_ERR_GLINK,          "H5Glink:soft link creation failed"},
  {ADFH_ERR_NO_ATT,         "Node attribute doesn't exist"},
  {ADFH_ERR_AOPEN,          "H5Aopen:open of node attribute failed"},
  {ADFH_ERR_IGET_NAME,      "H5Iget_name:failed to get node path from ID"},
  {ADFH_ERR_GMOVE,          "H5Gmove:moving a node group failed"},
  {ADFH_ERR_GUNLINK,        "H5Gunlink:node group deletion failed"},
  {ADFH_ERR_GOPEN,          "H5Gopen:open of a node group failed"},
  {ADFH_ERR_DGET_SPACE,     "H5Dget_space:couldn't get node dataspace"},
  {ADFH_ERR_DOPEN,          "H5Dopen:open of the node data failed"},
  {ADFH_ERR_DEXTEND,        "H5Dextend:couldn't extend the node dataspace"},
  {ADFH_ERR_DCREATE,        "H5Dcreate:node data creation failed"},
  {ADFH_ERR_SCREATE_SIMPLE, "H5Screate_simple:dataspace creation failed"},
  {ADFH_ERR_ACREATE,        "H5Acreate:node attribute creation failed"},
  {ADFH_ERR_GCREATE,        "H5Gcreate:node group creation failed"},
  {ADFH_ERR_DWRITE,         "H5Dwrite:write to node data failed"},
  {ADFH_ERR_DREAD,          "H5Dread:read of node data failed"},
  {ADFH_ERR_AWRITE,         "H5Awrite:write to node attribute failed"},
  {ADFH_ERR_AREAD,          "H5Aread:read of node attribute failed"},
  {ADFH_ERR_FMOUNT,         "H5Fmount:file mount failed"},
  {ADFH_ERR_LINK_MOVE,      "Can't move a linked-to node"},
  {ADFH_ERR_LINK_DATA,      "Can't change the data for a linked-to node"},
  {ADFH_ERR_LINK_NODE,      "Parent of node is a link"},
  {ADFH_ERR_LINK_DELETE,    "Can't delete a linked-to node"},
  {ADFH_ERR_NOT_HDF5_FILE,  "File does not exist or is not a HDF5 file"},
  {ADFH_ERR_FILE_DELETE,    "unlink (delete) of file failed"},
  {ADFH_ERR_FILE_INDEX,     "couldn't get file index from node ID"},
  {ADFH_ERR_TCOPY,          "H5Tcopy:copy of existing datatype failed"},
  {ADFH_ERR_AGET_TYPE,      "H5Aget_type:couldn't get attribute datatype"},
  {ADFH_ERR_TSET_SIZE,      "H5Tset_size:couldn't set datatype size"},
  {ADFH_ERR_NOT_IMPLEMENTED,"routine not implemented"},
  {ADFH_ERR_NOTXLINK,       "H5L: Link target is not an HDF5 external link"},
  {ADFH_ERR_LIBREG,         "HDF5: No external link feature available"},
  {ADFH_ERR_OBJINFO_FAILED, "HDF5: Internal problem with objinfo"},
  {ADFH_ERR_XLINK_NOVAL,    "HDF5: No value for external link"},
  {ADFH_ERR_XLINK_UNPACK,   "HDF5: Cannot unpack external link"},
  {ADFH_ERR_ROOTNULL,       "HDF5: Root descriptor is NULL"},
  {ADFH_ERR_NEED_TRANSPOSE, "dimensions need transposed - open in modify mode"},
  {ADFH_ERR_INVALID_OPTION, "invalid configuration option"},
  {ADFH_ERR_INVALID_USER_DATA, "invalid configuration data passed in"},

  {ADFH_ERR_SENTINEL,       "<None>"}
};

#define NUM_ERRORS ((int)(sizeof(ErrorList)/sizeof(struct _ErrorList)))
#define ROOT_OR_DIE(err) \
if (mta_root == NULL){set_error(ADFH_ERR_ROOTNULL, err);return;}
#define ROOT_OR_DIE_ERR(err) \
if (mta_root == NULL){set_error(ADFH_ERR_ROOTNULL, err);return 1;}

/* useful macros */

#define CMP_OSTAT(r,n) ((r)->objno[0]==(n)->objno[0] && \
                        (r)->objno[1]==(n)->objno[1] && \
                        (r)->fileno[0]==(n)->fileno[0] && \
                        (r)->fileno[1]==(n)->fileno[1])

static herr_t gfind_by_name(hid_t, const char *, void *);
static herr_t find_by_name(hid_t, const char *, const H5A_info_t*, void *);

#define has_child(ID,NAME) H5Giterate(ID,".",NULL,gfind_by_name,(void *)NAME)
#define has_data(ID)       H5Giterate(ID,".",NULL,gfind_by_name,(void *)D_DATA)

#define has_att(ID,NAME)   H5Aiterate2(ID,H5_INDEX_NAME,H5_ITER_NATIVE,NULL,find_by_name,(void *)NAME)

#if 0
static herr_t gprint_name(hid_t, const char *, void *);
static herr_t print_name(hid_t, const char *, const H5A_info_t*, void *);

#define show_grp(ID)       H5Giterate(ID,".",NULL,gprint_name,(void *)"GRP")
#define show_att(ID,NAME)  H5Aiterate2(ID,H5_INDEX_NAME,H5_ITER_NATIVE,NULL,print_name,(void *)NAME)
#endif

/* ----------------------------------------------------------------
 * set error and terminate if error state set
 * ---------------------------------------------------------------- */

static void set_error(int errcode, int *err)
{
  if ((mta_root != NULL)&&(errcode != NO_ERROR)&&(mta_root->g_error_state))
  {
    char errmsg[ADF_MAX_ERROR_STR_LENGTH+1];
    ADFH_Error_Message(errcode, errmsg);
    fprintf(stderr, "ERROR:%s\n", errmsg);
    exit(1);
  }
  *err = errcode;
}

/* ----- handle HDF5 errors --------------------------------------- */

static herr_t print_H5_error(int n, H5E_error2_t *desc, void *data)
{
  const char *p;

  if ((p = strrchr(desc->file_name, '/')) == NULL &&
      (p = strrchr(desc->file_name, '\\')) == NULL)
    p = desc->file_name;
  else
    p++;
  fprintf(stderr, "%s line %u in %s(): %s\n", p,
    desc->line, desc->func_name, desc->desc);
  return 0;
}

/* ----------------------------------------------------------------- */

static herr_t walk_H5_error(hid_t estack, void *data)
{
  if ((mta_root != NULL) && (mta_root->g_error_state)) {
    fflush(stdout);
    fprintf(stderr, "\nHDF5 Error Trace Back\n");
    return H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD,
                    (H5E_walk2_t)print_H5_error, data);
  }
  return 0;
}

/* -----------------------------------------------------------------
 * get file ID from node ID
 * ----------------------------------------------------------------- */

static hid_t get_file_id (hid_t id)
{
  ssize_t n, nobj;
  hid_t *objs, fid = -1;
  H5G_stat_t gstat, rstat;

  /* find the file ID from the root ID */

  if (H5Gget_objinfo(id, "/", 0, &gstat) >= 0) {
    nobj = H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_FILE);
    if (nobj > 0) {
      objs = (hid_t *) malloc (nobj * sizeof(hid_t));
      if (objs == NULL) return fid;
      H5Fget_obj_ids(H5F_OBJ_ALL, H5F_OBJ_FILE, -1, objs);
      for (n = 0; n < nobj; n++) {
        H5Gget_objinfo(objs[n], "/", 0, &rstat);
        if (CMP_OSTAT(&gstat, &rstat)) {
          fid = objs[n];
          break;
        }
      }
      free (objs);
    }
  }
  return fid;
}

/* ----------------------------------------------------------------- */

static int get_file_number (hid_t id, int *err)
{
  int n;
  hid_t fid = get_file_id(id);

  if (mta_root == NULL) return -1;
  for (n = 0; n < ADFH_MAXIMUM_FILES; n++) {
    if (fid == mta_root->g_files[n]) {
      set_error(NO_ERROR, err);
      return n;
    }
  }
  set_error(ADFH_ERR_FILE_INDEX, err);
  return -1;
}

#ifndef ADFH_FORCE_ID_CLOSE
/* ----------------------------------------------------------------- */

static track_id(hid_t refid, hid_t trackid)
{
  int fn,er;
  int sname;
  char oname[256];
  size_t maxhid;

  memset(oname,'\0',256);
  sname=H5Iget_name(trackid,oname,0);
  sname=H5Iget_name(trackid,oname,sname+1);
  fn=get_file_number(refid,&er);
  if (fn==-1)
  {
    ADFH_DEBUG((">ADFH track_ids cannot stat [%d][%s]",trackid,oname));
  }
  else
  {
    if (mta_root->n_extids[fn]>mta_root->x_extids[fn])
    {
      mta_root->x_extids[fn]+=256;
      maxhid=mta_root->x_extids[fn]*sizeof(hid_t*);
      mta_root->g_extids[fn]=(hid_t*)realloc(mta_root->g_extids[fn],maxhid);
      ADFH_DEBUG((">ADFH track_ids realloc up to [%d]",\
                  mta_root->x_extids[fn]));

    }
    mta_root->g_extids[fn][mta_root->n_extids[fn]]=trackid;
    ADFH_DEBUG((">ADFH track_ids [%d][%d][%s]",\
                mta_root->n_extids[fn],trackid,oname));
    mta_root->n_extids[fn]++;
  }
}
#endif

/*-----------------------------------------------------------------
 * get the native format - returns pointer to static storage
 *----------------------------------------------------------------- */

static char *native_format(void)
{
  static char format[ADF_FORMAT_LENGTH+1];
  hid_t type = H5Tcopy(H5T_NATIVE_FLOAT);

  ADFH_CHECK_HID(type);
  if (H5Tequal(type, H5T_IEEE_F32BE))
    strcpy(format, "IEEE_BIG_32");
  else if (H5Tequal(type, H5T_IEEE_F32LE))
    strcpy(format, "IEEE_LITTLE_32");
  else if (H5Tequal(type, H5T_IEEE_F64BE))
    strcpy(format, "IEEE_BIG_64");
  else if (H5Tequal(type, H5T_IEEE_F64LE))
    strcpy(format, "IEEE_LITTLE_64");
  else
    sprintf(format, "NATIVE_%d", (int)H5Tget_precision(type));
  H5Tclose(type);
  return format;
}

/* -----------------------------------------------------------------
 * set/get attribute values
 * ----------------------------------------------------------------- */

static hid_t get_att_id(hid_t id, const char *name, int *err)
{
  hid_t aid = H5Aopen_name(id, name);

  /* H5Aclose() performed elsewhere */
  if (aid < 0) {
    if (!has_att(id, name))
      set_error(ADFH_ERR_NO_ATT, err);
    else
      set_error(ADFH_ERR_AOPEN, err);
  }
  else
    set_error(NO_ERROR, err);
  return aid;
}

/* ----------------------------------------------------------------- */
static int new_str_att(hid_t id, const char *name, const char *value,
                       int max_size, int *err)
{
#ifdef ADFH_USE_STRINGS
  hid_t sid, tid, aid;
  herr_t status;

  /* [1] the attribute is set on the GROUP (id is a group id) */
  /* [2] all datatypes should be H5T_STRING and not H5T_NATIVE_CHAR
         which requires an array (see case below with a H5Screate_simple
         and providse an array of chars)
  */
  sid = H5Screate(H5S_SCALAR);
  if (sid < 0) {
    set_error(ADFH_ERR_SCREATE_SIMPLE, err);
    return 1;
  }
  tid = H5Tcopy(H5T_C_S1);
  if (tid < 0) {
    H5Sclose(sid);
    set_error(ADFH_ERR_TCOPY, err);
    return 1;
  }
  if (H5Tset_size(tid, max_size + 1) < 0) {
    H5Tclose(tid);
    H5Sclose(sid);
    set_error(ADFH_ERR_TSET_SIZE, err);
    return 1;
  }
  aid = H5Acreate2(id, name, tid, sid, H5P_DEFAULT, H5P_DEFAULT);
  if (aid < 0) {
    H5Tclose(tid);
    H5Sclose(sid);
    set_error(ADFH_ERR_ACREATE, err);
    return 1;
  }
  status = H5Awrite(aid, tid, value);
  H5Aclose(aid);
  H5Tclose(tid);
  H5Sclose(sid);
  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  set_error(NO_ERROR, err);
  return 0;
#else
  /* CAUTION: only use this for strings <= ADF_FILENAME_LENGTH */
  hid_t sid, aid;
  hsize_t dim;
  herr_t status;
  char buff[ADF_FILENAME_LENGTH+1];

  dim = max_size + 1;
  sid = H5Screate_simple(1, &dim, NULL);
  if (sid < 0) {
    set_error(ADFH_ERR_SCREATE_SIMPLE, err);
    return 1;
  }

  aid = H5Acreate2(id, name, H5T_NATIVE_CHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
  if (aid < 0) {
    H5Sclose(sid);
    set_error(ADFH_ERR_ACREATE, err);
    return 1;
  }

  memset(buff, 0, ADF_FILENAME_LENGTH+1);
  strcpy(buff, value);
  status = H5Awrite(aid, H5T_NATIVE_CHAR, buff);

  H5Aclose(aid);
  H5Sclose(sid);

  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  set_error(NO_ERROR, err);
  return 0;
#endif
}

/* ----------------------------------------------------------------- */
static int get_str_att(hid_t id, const char *name, char *value, int *err)
{
#ifdef ADFH_USE_STRINGS
  hid_t tid, att_id;
  herr_t status;

  ADFH_DEBUG((">ADFH get_str_att [%s]",name));
  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
#if 0
  status = H5Aread(att_id, H5T_NATIVE_CHAR, value);
#else
  tid = H5Aget_type(att_id);
  if (tid < 0) {
    H5Aclose(att_id);
    set_error(ADFH_ERR_AGET_TYPE, err);
    return 1;
  }
  status = H5Aread(att_id, tid, value);
  H5Tclose(tid);
#endif
  H5Aclose(att_id);
  ADFH_DEBUG(("<ADFH get_str_att [%s][%s]",name,value));
  if (status < 0) {
    set_error(ADFH_ERR_AREAD, err);
    return 1;
  }
  return 0;
#else
  /* CAUTION: unly use this for strings <= ADF_FILENAME_LENGTH */
  hid_t att_id;
  herr_t status;
  char buff[ADF_FILENAME_LENGTH+1];

  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
  status = H5Aread(att_id, H5T_NATIVE_CHAR, buff);
  H5Aclose(att_id);
  strcpy(value, buff);
  if (status < 0) {
    set_error(ADFH_ERR_AREAD, err);
    return 1;
  }
  return 0;
#endif
}

/* ----------------------------------------------------------------- */

static int set_str_att(hid_t id, const char *name, const char *value, int *err)
{
#ifdef ADFH_USE_STRINGS
  hid_t tid, att_id;
  herr_t status;

  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
  tid = H5Aget_type(att_id);
  if (tid < 0) {
    H5Aclose(att_id);
    set_error(ADFH_ERR_AGET_TYPE, err);
    return 1;
  }
  status = H5Awrite(att_id, tid, value);
  H5Tclose(tid);
  H5Aclose(att_id);
  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  return 0;
#else
  /* CAUTION: unly use this for strings <= ADF_FILENAME_LENGTH */
  hid_t att_id;
  herr_t status;
  char buff[ADF_FILENAME_LENGTH+1];

  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
  memset(buff, 0, ADF_FILENAME_LENGTH+1);
  strcpy(buff, value);
  status = H5Awrite(att_id, H5T_NATIVE_CHAR, buff);
  H5Aclose(att_id);
  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  return 0;
#endif
}

/* ----------------------------------------------------------------- */

static int new_int_att(hid_t id, const char *name, int value, int *err)
{
  hid_t sid, aid;
  hsize_t dim;
  herr_t status;
  int buff = value;

  dim = 1;
  sid = H5Screate_simple(1, &dim, NULL);
  if (sid < 0) {
    set_error(ADFH_ERR_SCREATE_SIMPLE, err);
    return 1;
  }

  aid = H5Acreate2(id, name, H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT);
  if (aid < 0) {
    H5Sclose(sid);
    set_error(ADFH_ERR_ACREATE, err);
    return 1;
  }

  status = H5Awrite(aid, H5T_NATIVE_INT, &buff);

  H5Aclose(aid);
  H5Sclose(sid);

  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  set_error(NO_ERROR, err);
  return 0;
}

#ifndef ADFH_NO_ORDER
/* ----------------------------------------------------------------- */

static int get_int_att(hid_t id, char *name, int *value, int *err)
{
  hid_t att_id;
  herr_t status;

  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
  status = H5Aread(att_id, H5T_NATIVE_INT, value);
  H5Aclose(att_id);
  if (status < 0) {
    set_error(ADFH_ERR_AREAD, err);
    return 1;
  }
  return 0;
}

/* ----------------------------------------------------------------- */

static int set_int_att(hid_t id, char *name, int value, int *err)
{
  hid_t att_id;
  herr_t status;
  int buff = value;

  if ((att_id = get_att_id(id, name, err)) < 0) return 1;
  status = H5Awrite(att_id, H5T_NATIVE_INT, &buff);
  H5Aclose(att_id);
  if (status < 0) {
    set_error(ADFH_ERR_AWRITE, err);
    return 1;
  }
  return 0;
}
#endif

/* ----------------------------------------------------------------- */

static int new_str_data(hid_t id, const char *name, const char *value,
                       int size, int *err)
{
  hid_t sid, did;
  hsize_t dim;
  herr_t status;
  hid_t xfer_prp=H5P_DEFAULT;
  hid_t dcpl_id=H5P_DEFAULT;

  dim = size+1;
  sid = H5Screate_simple(1, &dim, NULL);
  if (sid < 0) {
    set_error(ADFH_ERR_SCREATE_SIMPLE, err);
    return 1;
  }

  dcpl_id = H5Pcreate(H5P_DATASET_CREATE);

  /* compact storage */
  if(size+1 < CGNS_64KB)
    H5Pset_layout(dcpl_id, H5D_COMPACT);
  else {
    H5Pset_layout(dcpl_id, H5D_CONTIGUOUS);
    H5Pset_alloc_time(dcpl_id, H5D_ALLOC_TIME_EARLY);
    H5Pset_fill_time(dcpl_id, H5D_FILL_TIME_NEVER); 
  }

  did = H5Dcreate2(id, name, H5T_NATIVE_CHAR, sid, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
  if (did < 0) {
    H5Sclose(sid);
    H5Pclose(dcpl_id);
    set_error(ADFH_ERR_DCREATE, err);
    return 1;
  }

#if CG_BUILD_PARALLEL
  if (pcg_mpi_initialized) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
  }
#endif

  status = H5Dwrite(did, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, xfer_prp, value);

#if CG_BUILD_PARALLEL
  if (pcg_mpi_initialized) {
    H5Pclose(xfer_prp);
  }
#endif

  H5Dclose(did);
  H5Sclose(sid);
  H5Pclose(dcpl_id);

  if (status < 0) {
    set_error(ADFH_ERR_DWRITE, err);
    return 1;
  }
  set_error(NO_ERROR, err);
  return 0;
}

/* ----------------------------------------------------------------- */
/*  translate an ADF data type into an HDF one */

static hid_t to_HDF_data_type(const char *tp)
{
  /* H5Tclose performed elsewhere */
  if (0 == strcmp(tp, ADFH_B1))
    return H5Tcopy(H5T_NATIVE_UCHAR);
  if (0 == strcmp(tp, ADFH_C1))
    return H5Tcopy(H5T_NATIVE_CHAR);
  if (0 == strcmp(tp, ADFH_I4))
    return H5Tcopy(H5T_NATIVE_INT32);
  if (0 == strcmp(tp, ADFH_I8))
    return H5Tcopy(H5T_NATIVE_INT64);
  if (0 == strcmp(tp, ADFH_U4))
    return H5Tcopy(H5T_NATIVE_UINT32);
  if (0 == strcmp(tp, ADFH_U8))
    return H5Tcopy(H5T_NATIVE_UINT64);
  if (0 == strcmp(tp, ADFH_R4)) {
    hid_t tid = H5Tcopy(H5T_NATIVE_FLOAT);
    H5Tset_precision(tid, 32);
    return tid;
  }
  if (0 == strcmp(tp, ADFH_R8)) {
    hid_t tid = H5Tcopy(H5T_NATIVE_DOUBLE);
    H5Tset_precision(tid, 64);
    return tid;
  }
  return 0;
}

/* ----------------------------------------------------------------- */

static int check_data_type(const char *tp, int *err)
{
  if (strcmp(tp, ADFH_B1) &&
      strcmp(tp, ADFH_C1) &&
      strcmp(tp, ADFH_I4) &&
      strcmp(tp, ADFH_I8) &&
      strcmp(tp, ADFH_U4) &&
      strcmp(tp, ADFH_U8) &&
      strcmp(tp, ADFH_R4) &&
      strcmp(tp, ADFH_R8)) {
    set_error(INVALID_DATA_TYPE, err);
    return 1;
  }
  set_error(NO_ERROR, err);
  return 0;
}

/* =================================================================
 * callback routines for H5Giterate and H5Aiterate
 * ================================================================= */

/* ----------------------------------------------------------------- */

static herr_t gfind_by_name(hid_t id, const char *name, void *dsname)
{
    if (0 == strcmp (name, (char *)dsname)) return 1;
    return 0;
}

/* ----------------------------------------------------------------- */

static herr_t find_by_name(hid_t id, const char *name, const H5A_info_t* ainfo, void *dsname)
{
    ADFH_CHECK_HID(id);
    if (0 == strcmp (name, (char *)dsname)) return 1;
    return 0;
}

#if 0
/* ----------------------------------------------------------------- */

static herr_t gprint_name(hid_t id, const char *name, void *dsname)
{
  return 0;
}

/* ----------------------------------------------------------------- */

static herr_t print_name(hid_t id, const char *name, const H5A_info_t* ainfo, void *dsname)
{
  printf("[%s]:[%s]\n",(char*)dsname,name);
  return 0;
}
#endif

/* ----------------------------------------------------------------- */

static herr_t count_children(hid_t id, const char *name, void *number)
{
  ADFH_CHECK_HID(id);
  ADFH_DEBUG(("count_children [%s][%d]",name,(*((int *)number))));

  if (*name != D_PREFIX)
    (*((int *)number))++;
  return 0;
}

/* ----------------------------------------------------------------- */

static herr_t children_names(hid_t id, const char *name,
                             const H5L_info_t *linfo, void *namelist)
{
#ifndef ADFH_NO_ORDER
  hid_t gid;
#endif
  int order, err;
  char *p;

  ROOT_OR_DIE_ERR(&err);
  if (*name == D_PREFIX) return 0;
#ifdef ADFH_NO_ORDER
  order = ++mta_root->i_count - mta_root->i_start;
  if (order >= 0 && order < mta_root->i_len) {
    p = (char *)namelist + order * mta_root->n_length;
    strncpy(p, name, mta_root->n_length-1);
    p[mta_root->n_length-1] = 0;
    mta_root->n_names++;
  }
#else
  if ((gid = H5Gopen2(id, name, H5P_DEFAULT)) < 0) return 1;
  ADFH_DEBUG(("ADFH children_names [%d]",gid));
  if (get_int_att(gid, A_ORDER, &order, &err)) {
    H5Gclose(gid);
    return 1;
  }
  order -= mta_root->i_start;
  if (order >= 0 && order < mta_root->i_len) {
    p = (char *)namelist + order * mta_root->n_length;
    strncpy(p, name, mta_root->n_length-1);
    p[mta_root->n_length-1] = 0;
    mta_root->n_names++;
  }
  H5Gclose(gid);
#endif
  return 0;
}

/* ----------------------------------------------------------------- */
static herr_t children_ids(hid_t id, const char *name,
                           const H5L_info_t *linfo, void *idlist)

{
  hid_t gid;
  int order, err;

  ADFH_DEBUG((">ADFH children_ids [%s]",name));
  ROOT_OR_DIE_ERR(&err);
  if (*name == D_PREFIX) return 0;
  if ((gid = H5Gopen2(id, name, H5P_DEFAULT)) < 0) return 1;
#ifdef ADFH_NO_ORDER
  order = ++mta_root->i_count - mta_root->i_start;
  ADFH_DEBUG((">ADFH children_ids [%s] order is [%d]",name,order));
  if (order >= 0 && order < mta_root->i_len) {
    to_ADF_ID(gid,((double *)idlist)[order]);
      mta_root->n_names++;
#ifndef ADFH_FORCE_ID_CLOSE
      track_id(id,gid);
#endif
  }
  else
      H5Gclose(gid);
#else
  if (get_int_att(gid, A_ORDER, &order, &err)) {
    H5Gclose(gid);
    return 1;
  }
  order -= mta_root->i_start;
  if (order >= 0 && order < mta_root->i_len) {
    to_ADF_ID(gid,((double *)idlist)[order]);
      mta_root->n_names++;
#ifndef ADFH_FORCE_ID_CLOSE
      track_id(id,gid);
#endif
  }
  else
      H5Gclose(gid);
#endif
  return 0;
}

#ifndef ADFH_NO_ORDER
/* -----------------------------------------------------------------
  called via H5Giterate in Move_Child & Delete functions.
  removes gaps in _order index attributes */

static herr_t fix_order(hid_t id, const char *name, void *data)
{
  int start, order, err, ret = 0;
  hid_t gid, aid;

  if (mta_root == NULL) return ADFH_ERR_ROOTNULL;
  if (*name == D_PREFIX) return 0;
  if ((gid = H5Gopen2(id, name, H5P_DEFAULT)) < 0)
    return ADFH_ERR_GOPEN;
  if ((aid = get_att_id(gid, A_ORDER, &err)) < 0) {
    H5Gclose(gid);
    return err;
  }
  if (H5Aread(aid, H5T_NATIVE_INT, &order) < 0)
    ret = ADFH_ERR_AREAD;
  else {
    start = *((int *)data);
    if (order > start) {
      order--;
      if (H5Awrite(aid, H5T_NATIVE_INT, &order) < 0)
        ret = ADFH_ERR_AWRITE;
    }
  }
  ADFH_DEBUG(("ADFH fix_order [%d]",gid));
  H5Aclose(aid);
  H5Gclose(gid);
  return ret;
}
#endif

/* ----------------------------------------------------------------- */

static herr_t compare_children(hid_t id, const char *name, void *data)
{
  H5G_stat_t stat, *pstat;

  if (*name != D_PREFIX) {
    pstat = (H5G_stat_t *)data;
    if (H5Gget_objinfo(id, name, 0, &stat) >= 0)
      return CMP_OSTAT(&stat, pstat);
  }
  return 0;
}

#if 0
/* ----------------------------------------------------------------- */

static herr_t print_children(hid_t id, const char *name, void *data)
{
  if (*name != D_PREFIX)
    printf(" %s", name);
  return 0;
}
#endif

/* ================================================================
 * routines for dealing with links
 * ================================================================ */
static hid_t open_link(hid_t id, int *err)
{
  hid_t lid;
  herr_t herr;

  const char  *file;
  const char  *path;
  H5G_stat_t  sb; /* Object information */

  char  querybuff[512];

#ifdef ADFH_DEBUG_ON
  H5O_info_t oinfo;/* debug purpose only */
  char buffname[ADF_NAME_LENGTH+1];
  get_str_att(id, A_NAME, buffname, err);

  ADFH_DEBUG((">ADFH open_link [%s]",buffname));
#endif

  if (H5Lis_registered(H5L_TYPE_EXTERNAL) != 1)
  {
    set_error(ADFH_ERR_LIBREG, err);
    return -1;
  }
  herr=H5Gget_objinfo(id, D_LINK, (hbool_t)0, &sb);

  if (herr<0)
  {
    ADFH_DEBUG((">ADFH open_link type [%d][%d]",herr,sb.type));
    set_error(ADFH_ERR_OBJINFO_FAILED, err);
    return -1;
  }

  /* Soft link                -> link to our current file */
  /* Hard link (User defined) -> link to an external file */

  if (H5G_LINK != sb.type)
  {
    if (H5G_UDLINK != sb.type)
    {
      set_error(ADFH_ERR_NOTXLINK, err);
      return -1;
    }

    if (H5Lget_val(id,D_LINK,querybuff,sizeof(querybuff),H5P_DEFAULT)<0)
    {
      set_error(ADFH_ERR_XLINK_NOVAL, err);
      return -1;
    }

    if (H5Lunpack_elink_val(querybuff,sb.linklen,NULL,&file,&path)<0)
    {
      set_error(ADFH_ERR_XLINK_UNPACK, err);
      return -1;
    }
    /* open the actual link >> IN THE LINK GROUP << */
    ADFH_DEBUG((">ADFH open_link (external)"));
    if ((lid = H5Gopen2(id, D_LINK, H5P_DEFAULT)) < 0)
      {
        set_error(LINK_TARGET_NOT_THERE, err);
        return lid;
      }
  }
  else
  {
    ADFH_DEBUG((">ADFH open_link (symbolic)"));
    if ((lid = H5Gopen2(id, D_LINK, H5P_DEFAULT)) < 0)
      {
        set_error(LINK_TARGET_NOT_THERE, err);
        return lid;
      }
  }
#ifdef ADFH_DEBUG_ON
  H5Oget_info(lid, &oinfo);
  ADFH_DEBUG(("<ADFH open_link [%d]->[%d]:%d",id,lid,oinfo.rc));
#endif
  return lid;
}

/* ----------------------------------------------------------------- */

static int is_link(hid_t id)
{
  char type[3];
  int err;

  if ((!get_str_att(id, A_TYPE, type, &err) && (0 == strcmp(ADFH_LK, type))))
  {
    return 1;
  }
  return 0;
}

/* ----------------------------------------------------------------- */

static hid_t open_node(double id, int *err)
{
  hid_t hid, gid, lid;

  ADFH_DEBUG((">ADFH open_node"));
  to_HDF_ID(id,hid);
  set_error(NO_ERROR, err);
  if (is_link(hid))
  {
    lid=open_link(hid, err); /* bad id trapped in the function */
    ADFH_DEBUG(("<ADFH open_node link"));
    return lid;
  }
  else
  {
    if ((gid = H5Gopen2(hid, ".", H5P_DEFAULT)) < 0)
    {
      set_error(ADFH_ERR_GOPEN, err);
    }
    else
    {
      /* H5G_stat_t sb; */
      ADFH_DEBUG(("<ADFH open_node group [%d]",gid));
      /* H5Gget_objinfo(gid,".",0,&sb); */
      return gid;
    }
  }
  return -1;
}

/* ----------------------------------------------------------------- */

static hid_t parse_path(hid_t pid, char *path, int *err)
{
  hid_t id, nid;
  char *p;

  if ((p = strchr(path, '/')) != NULL) *p++ = 0;
  if ((id = H5Gopen2(pid, path, H5P_DEFAULT)) < 0) {
    set_error(ADFH_ERR_GOPEN, err);
    return id;
  }
  if (p == NULL || !*p) return id;
  if (is_link(id)) {
    nid = open_link(id, err);
    if (H5Gclose(id)<0) ADFH_DEBUG((">ADFH H5Gclose failed"));
    if (nid < 0) return nid;
    id = nid;
  }
  nid = parse_path(id, p, err);
  if (H5Gclose(id)<0) ADFH_DEBUG((">ADFH H5Gclose failed"));
  ADFH_DEBUG(("ADFH parse_path [%d]",nid));
  return nid;
}

/* -----------------------------------------------------------------
 * deletion routines
 * ----------------------------------------------------------------- */

static void delete_node(hid_t pid, const char *name)
{
  H5Gunlink(pid, name); /* do we care about link ? no ? */
}

/* ----------------------------------------------------------------- */

static herr_t delete_children(hid_t id, const char *name, void *data)
{
  if (*name == D_PREFIX)
  {
    ADFH_DEBUG(("delete_children single"));
    if (! is_link(id))
    {
      ADFH_DEBUG(("delete_children is not link [%s]",name));
      H5Gunlink(id, name);
    }
    else
    {
      ADFH_DEBUG(("delete_children is link"));
    }
    ADFH_DEBUG(("delete_children single done"));
  }
  else {
    ADFH_DEBUG(("delete_children loop"));
    if (! is_link(id)) H5Giterate(id, name, NULL, delete_children, data);
    delete_node(id, name);
  }
  return 0;
}

/* -----------------------------------------------------------------
 * check for valid node name
 * The return points to static storage - make a copy if needed
 * ----------------------------------------------------------------- */

static char *check_name(const char *new_name, int *err)
{
  char *p;
  static char name[ADF_NAME_LENGTH+1];

  if (new_name == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return NULL;
  }

  /* skip leading space */

  for (p = (char *)new_name; *p && isspace(*p); p++)
    ;
  if (!*p) {
    set_error(STRING_LENGTH_ZERO, err);
    return NULL;
  }

  if (strlen(p) > ADF_NAME_LENGTH) {
    set_error(STRING_LENGTH_TOO_BIG, err);
    return NULL;
  }
  strcpy(name, p);

  /* remove trailing space */

  for (p = name+strlen(name)-1; p >= name && isspace(*p); p--)
    ;
  *++p = 0;
  if (!*name) {
    set_error(STRING_LENGTH_ZERO, err);
    return NULL;
  }

  /* these may cause problems with HDF5 */

  if (NULL != strchr(name, '/') || 0 == strcmp (name, ".")) {
    set_error(INVALID_NODE_NAME, err);
    return NULL;
  }

  set_error(NO_ERROR, err);
  return name;
}

#ifdef ADFH_FORTRAN_INDEXING

/*------------------------------------------------------------------
 * transpose multi-dimensional indices recursively
 *------------------------------------------------------------------*/

static int swap_dimensions (hid_t gid)
{
  char verstr[ADF_NAME_LENGTH+1];

  sprintf(verstr, "/%s", D_OLDVERS);
  if (H5Lexists(gid, verstr, H5P_DEFAULT)) return 0;
  return 1;
}

static void transpose_dimensions (hid_t hid, const char *name)
{
#if 0
  hid_t did, sid, tid, mid;
  int i, j, temp, ndims, diffs;
  hsize_t bytes, dims[ADF_MAX_DIMENSIONS];
  void *data = NULL;
#else
  hid_t did, sid;
  int i, j, ndims, diffs;
  hsize_t temp, dims[ADF_MAX_DIMENSIONS];
#endif

  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0) return;

  /* get dimensions and size */
  sid = H5Dget_space(did);
  ndims = H5Sget_simple_extent_dims(sid, dims, NULL);
  H5Sclose(sid);
  if (ndims < 2) {
    H5Dclose(did);
    return;
  }
#if 0
  printf("%s:%d [%d", name, bytes, dims[0]);
  for (i = 1; i < ndims; i++) printf(",%d", dims[i]);
  printf("]\n");
  fflush(stdout);
#endif

  /* transpose dimensions */
  diffs = 0;
  for (i = 0, j = ndims-1; i < j; i++, j--) {
    if (dims[i] != dims[j]) {
      temp = dims[i];
      dims[i] = dims[j];
      dims[j] = temp;
      diffs++;
    }
  }
  if (0 == diffs) {
    H5Dclose(did);
    return;
  }
#if 0
  /* read the data, then delete data set */
  bytes = H5Dget_storage_size(did);
  tid = H5Dget_type(did);
  mid = H5Tget_native_type(tid, H5T_DIR_ASCEND);
  if (bytes > 0) {
    data = malloc ((unsigned)bytes);
    H5Dread(did, mid, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  }
  H5Dclose(did);
  H5Gunlink(hid, D_DATA);

  /* rewrite data with new dimensions */
  sid = H5Screate_simple(ndims, dims, NULL);
  did = H5Dcreate2(hid, D_DATA, tid, sid, H5P_DEFAULT, mta_root->g_propdataset, H5P_DEFAULT);
  if (data != NULL) {
    H5Dwrite(did, mid, H5S_ALL, sid, H5P_DEFAULT, data);
    free(data);
  }
  H5Tclose(mid);
  H5Tclose(tid);
  H5Sclose(sid);
#else
  if (H5Dset_extent(did, dims) < 0) {
      fprintf(stderr, "H5Dset_extent failed\n");
  }
#endif
  H5Dclose(did);
}

/* ----------------------------------------------------------------- */

static herr_t fix_dimensions(hid_t id, const char *name, void *data)
{
  hid_t gid;
  int err;
  char type[ADF_DATA_TYPE_LENGTH+1];

  if (*name != D_PREFIX && (gid = H5Gopen2(id, name, H5P_DEFAULT)) >= 0 &&
     !get_str_att(gid, A_TYPE, type, &err) && strcmp(type, ADFH_LK)) {
    H5Giterate(gid, ".", NULL, fix_dimensions, NULL);
    transpose_dimensions(gid,name);
    H5Gclose(gid);
  }
  return 0;
}

#endif

/* ================================================================= */
/* 1 to 1 mapping of ADF functions to HDF mimic functions            */
/* ================================================================= */

void ADFH_Configure(const int option, const void *value, int *err)
{
    if (option == ADFH_CONFIG_COMPRESS) {
        int compress = (int)((size_t)value);
        if (compress < 0)
            CompressData = 6;
        else if (compress > 9)
            CompressData = 9;
        else
            CompressData = compress;
        set_error(NO_ERROR, err);
    }
#if CG_BUILD_PARALLEL
    else if (option == ADFH_CONFIG_MPI_COMM) {
      MPI_Comm* comm = (MPI_Comm*)value;
      if (!comm) {
        set_error(ADFH_ERR_INVALID_USER_DATA, err);
      }
      else {
        ParallelMPICommunicator = (MPI_Comm)*comm;
        set_error(NO_ERROR, err);
      }
    }
#endif
    else {
        set_error(ADFH_ERR_INVALID_OPTION, err);
    }
}

/* ----------------------------------------------------------------- */
/* move a node                                                       */

void ADFH_Move_Child(const double  pid,
                      const double  id,
                      const double  npid,
                      int          *err)
{
  hid_t hpid;
  hid_t hid;
  hid_t hnpid;
  ssize_t len;
  int namelen;
#ifndef ADFH_NO_ORDER
  int old_order, new_order;
#endif
  char buff[2];
  char nodename[ADF_NAME_LENGTH+1];
  char *newpath;
  herr_t status;
  H5G_stat_t stat;

  to_HDF_ID(pid,hpid);
  to_HDF_ID(id,hid);
  to_HDF_ID(npid,hnpid);

  ADFH_DEBUG(("ADFH_Move_Child"));

  if (is_link(hpid) || is_link(hnpid)) {
    set_error(ADFH_ERR_LINK_MOVE, err);
    return;
  }

  /* check that node is actually child of the parent */

  if (H5Gget_objinfo(hid, ".", 0, &stat) < 0 ||
    !H5Giterate(hpid, ".", NULL, compare_children, (void *)&stat)) {
    set_error(CHILD_NOT_OF_GIVEN_PARENT, err);
    return;
  }

  /* get node name */

  if (get_str_att(hid, A_NAME, nodename, err)) return;
  namelen = (int)strlen(nodename);

  /* get new node path */

  len = H5Iget_name(hnpid, buff, 2);
  if (len <= 0) {
    set_error(ADFH_ERR_IGET_NAME, err);
    return;
  }
  newpath = (char *) malloc (len+namelen+2);
  if (newpath == NULL) {
    set_error(MEMORY_ALLOCATION_FAILED, err);
    return;
  }
  H5Iget_name(hnpid, newpath, len+1);
  newpath[len++] = '/';
  strcpy(&newpath[len], nodename);

#ifdef ADFH_DEBUG_ON
  printf("%s move [%s]\n",ADFH_PREFIX,nodename);
  printf("%s to   [%s]\n",ADFH_PREFIX,newpath);
#endif

  status = H5Gmove(hpid, nodename, newpath);
  free(newpath);
  if (status < 0) {
    set_error(ADFH_ERR_GMOVE, err);
    return;
  }

#ifdef ADFH_NO_ORDER
  set_error(NO_ERROR, err);
#else
  /*update _order attribute for node we just moved*/
  ADFH_Number_of_Children(npid, &new_order, err);
  if (*err != NO_ERROR) return;

  /*read/write _order attr*/
  if (get_int_att(hid, A_ORDER, &old_order, err) ||
      set_int_att(hid, A_ORDER, new_order, err)) return;

  /*see if we need to decrement any node _orders under the old parent*/
  *err = H5Giterate(hpid, ".", NULL, fix_order, (void *)&old_order);
  if (!*err)
    set_error(NO_ERROR, err);
#endif
}

/* ----------------------------------------------------------------- */
/* Change the label attribute value                                  */

void ADFH_Set_Label(const double  id,
                    const char   *label,
                    int          *err)
{
  hid_t hid;

  to_HDF_ID(id, hid);

  ADFH_DEBUG(("ADFH_Set_Label [%s]",label));

  if (label == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  if (strlen(label) > ADF_NAME_LENGTH) {
    set_error(STRING_LENGTH_TOO_BIG, err);
    return;
  }
  if (is_link(hid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  set_str_att(hid, A_LABEL, label, err);
}

/* ----------------------------------------------------------------- */
/* Change attribute name and move the group name to new name         */

void ADFH_Put_Name(const double  pid,
                   const double  id,
                   const char   *name,
                   int          *err)
{
  hid_t hpid;
  hid_t hid;
  char *nname, oname[ADF_NAME_LENGTH+1];

  to_HDF_ID(pid,hpid);
  to_HDF_ID(id,hid);

  ADFH_DEBUG(("ADFH_Put_Name [%s]",name));

  if ((nname = check_name(name, err)) == NULL) return;
  if (is_link(hpid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  if (has_child(hpid, nname)) {
    set_error(DUPLICATE_CHILD_NAME, err);
    return;
  }
  if (!get_str_att(hid, A_NAME, oname, err)) {
#ifdef ADFH_DEBUG_ON
    printf("%s change [%s] to [%s]\n",ADFH_PREFIX,oname,nname);
#endif
    if (H5Gmove(hpid, oname, nname) < 0)
      set_error(ADFH_ERR_GMOVE, err);
    else
      set_str_att(hid, A_NAME, nname, err);
  }
}

/* ----------------------------------------------------------------- */
/* Retrieve the name attribute value (same as group name)            */

void ADFH_Get_Name(const double  id,
                   char         *name,
                   int          *err)
{
  hid_t hid;
  char buffname[ADF_NAME_LENGTH+1];

  to_HDF_ID(id,hid);

  ADFH_DEBUG((">ADFH_Get_Name"));

  if (name == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  get_str_att(hid, A_NAME, buffname, err);
  strcpy(name,buffname);

  ADFH_DEBUG(("<ADFH_Get_Name [%s]",name));

}

/* ----------------------------------------------------------------- */
/* Retrieve the label attribute value                                */

void ADFH_Get_Label(const double  id,
                    char         *label,
                    int          *err)
{
  hid_t hid;
  char bufflabel[ADF_LABEL_LENGTH+1] = "";
  ADFH_DEBUG((">ADFH_Get_Label [%d]",id));

  if (label == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  if ((hid = open_node(id, err)) >= 0)
  {
    get_str_att(hid, A_LABEL, bufflabel, err);
    if (H5Gclose(hid)<0)
    {
      ADFH_DEBUG((">ADFH H5Gclose failed (G)"));
    }
  }
  strcpy(label,bufflabel);
  ADFH_DEBUG(("<ADFH_Get_Label"));
}

/* ----------------------------------------------------------------- */
/* Create a new group, set its name in the name attribute
   - I may not need the name attribute, it's redundant and then
     dangerous. Anyway, now it's there, so let us use it
   - Update ref table                                                */

void ADFH_Create(const double  pid,
                 const char   *name,
                 double       *id,
                 int          *err)
{
  hid_t hpid;
  hid_t gid;
  char *pname;
#ifdef ADFH_DEBUG_ON
  H5L_info_t lkbuff;
#endif

  to_HDF_ID(pid, hpid);

  ADFH_DEBUG((">ADFH_Create [%s][%d]",name,hpid));

  if ((pname = check_name(name, err)) == NULL) return;
  if (id == NULL) {
    set_error(NULL_NODEID_POINTER, err);
    return;
  }
  /*
  if (is_link(hpid)) {
    set_error(ADFH_ERR_LINK_NODE, err);
    return;
  }
  */
  if (has_child(hpid, pname)) {
    set_error(DUPLICATE_CHILD_NAME, err);
    return;
  }

  *id = 0;
  gid = H5Gcreate2(hpid, pname,
                   H5P_DEFAULT, mta_root->g_propgroupcreate, H5P_DEFAULT);
#ifdef ADFH_DEBUG_ON
  H5Lget_info(hpid,pname,&lkbuff,H5P_DEFAULT);
  ADFH_DEBUG((">ADFH_Create [%s] index [%d]",pname,lkbuff.corder));
#endif

  if (gid < 0)
    set_error(ADFH_ERR_GCREATE, err);
  else {
#ifdef ADFH_NO_ORDER
    if (new_str_att(gid, A_NAME, pname, ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_LABEL, "", ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_TYPE, ADFH_MT, 2, err) ||
        new_int_att(gid, A_FLAGS, mta_root->g_flags, err)) return;
#else
    int order = 0;
    H5Giterate(hpid, ".", NULL, count_children, (void *)&order);
    if (new_str_att(gid, A_NAME, pname, ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_LABEL, "", ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_TYPE, ADFH_MT, 2, err) ||
        new_int_att(gid, A_ORDER, order, err) ||
        new_int_att(gid, A_FLAGS, mta_root->g_flags, err)) return;
#endif
    to_ADF_ID(gid,*id);
#ifndef ADFH_FORCE_ID_CLOSE
    track_id(hpid,gid);
#endif
    ADFH_DEBUG(("<ADFH_Create [%s][%d][%d]",name,hpid,gid));
  }
}

/* ----------------------------------------------------------------- */
/* delete a node and all children recursively                        */

void ADFH_Delete(const double  pid,
                 const double  id,
                 int    *err)
{
  hid_t hpid;
  hid_t hid;
  char old_name[ADF_NAME_LENGTH+1];
#ifndef ADFH_NO_ORDER
  int old_order;
#endif
  H5G_stat_t stat;

  to_HDF_ID(pid, hpid);
  to_HDF_ID(id, hid);

  ADFH_DEBUG(("ADFH_Delete [%d][%d]",hpid,hid));

  if (is_link(hpid)) {
    set_error(ADFH_ERR_LINK_DELETE, err);
    return;
  }

  /* check that node is actually child of the parent */

  if (H5Gget_objinfo(hid, ".", 0, &stat) < 0 ||
    !H5Giterate(hpid, ".", NULL, compare_children, (void *)&stat)) {
    set_error(CHILD_NOT_OF_GIVEN_PARENT, err);
    return;
  }

  /* get name and order */

#ifdef ADFH_NO_ORDER
  if (get_str_att(hid, A_NAME, old_name, err)) return;
#else
  if (get_str_att(hid, A_NAME, old_name, err) ||
      get_int_att(hid, A_ORDER, &old_order, err)) return;
#endif

  /* delete children nodes recursively */

  if (! is_link(hid))
  {
    H5Giterate(hid, ".", NULL, delete_children, NULL);
  }

  /* delete current node */

  H5Gclose(hid);
  delete_node(hpid, old_name);

  /* decrement node orders */

#ifndef ADFH_NO_ORDER
  *err = H5Giterate(hpid, ".", NULL, fix_order, (void *)&old_order);
  if (!*err)
#endif
    set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Number_of_Children(const double  id,
                             int    *number,
                             int    *err)
{
  hid_t hid;
  int nn,gskip=0;

  ADFH_DEBUG((">ADFH_Number_of_Children"));

  if (number == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }

  *number = 0;
  if ((hid = open_node(id, err)) >= 0) {
    H5Giterate(hid, ".", &gskip, count_children, (void*)number);
    H5Gclose(hid);
  }
  nn=*number;
  (void)nn;  /* avoid unused variable warning */
  ADFH_DEBUG(("<ADFH_Number_of_Children [%d]",nn));
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Node_ID(const double  pid,
                      const char   *name,
                      double       *id,
                      int          *err)
{
  hid_t sid, hpid;
  to_HDF_ID(pid,hpid);

  ADFH_DEBUG((">ADFH_Get_Node_ID [%s][%d]",name,hpid));

  if (name == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  if (id == NULL) {
    set_error(NULL_NODEID_POINTER, err);
    return;
  }

  *id = 0;
  set_error(NO_ERROR, err);
  if (*name == '/') {
    hid_t rid;
    char *path = (char *) malloc (strlen(name)+1);
    if (path == NULL) {
      set_error(MEMORY_ALLOCATION_FAILED, err);
      return;
    }
    strcpy(path, &name[1]);
    rid = H5Gopen2(hpid, "/", H5P_DEFAULT);
    sid = parse_path(rid, path, err);
    H5Gclose(rid);
    free(path);
  }
  else if (is_link(hpid)) {
    hid_t lid = open_link(hpid, err);
    ADFH_DEBUG(("<ADFH_Get_Node_ID open_link [%d]",lid));
    if (lid < 0) return;
    sid = H5Gopen2(lid, name, H5P_DEFAULT);
    ADFH_CHECK_HID(sid);
    H5Gclose(lid);
    if(sid < 0)
    {
      set_error(ADFH_ERR_GOPEN, err);
    }
  }
  else {
    sid = H5Gopen2(hpid, name, H5P_DEFAULT);
    if(sid < 0)
    {
      set_error(ADFH_ERR_GOPEN, err);
    }
  }
#ifndef ADFH_FORCE_ID_CLOSE
  track_id(hpid,sid);
#endif
  to_ADF_ID(sid, *id);
  ADFH_DEBUG(("<ADFH_Get_Node_ID [%s][%d] return [%d]",name,hpid,sid));
}

/* ----------------------------------------------------------------- */

void ADFH_Children_Names(const double pid,
                         const int    istart,
                         const int    ilen,
                         const int    name_length,
                         int   *ilen_ret,
                         char  *names,
                         int   *err)
{
  hid_t hpid;

  ADFH_DEBUG(("ADFH_Children_Names"));

  ROOT_OR_DIE(err);
  if (ilen_ret == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  if (names == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }

  mta_root->i_start = istart;
  mta_root->i_len = ilen;
  mta_root->n_length = name_length;
  mta_root->n_names = 0;
#ifdef ADFH_NO_ORDER
  mta_root->i_count = 0;
#endif

  /*initialize names to null*/
  memset(names, 0, ilen*name_length);
  if ((hpid = open_node(pid, err)) >= 0) {
    H5Literate(hpid,H5_INDEX_CRT_ORDER,H5_ITER_INC,
               NULL,children_names,(void *)names);
    if (names[0]==0)
    {
      H5Literate(hpid,H5_INDEX_NAME,H5_ITER_INC,
                 NULL,children_names,(void *)names);
    }
    H5Gclose(hpid);
  }
  *ilen_ret = mta_root->n_names;
}

/* ----------------------------------------------------------------- */

void ADFH_Children_IDs(const double pid,
                         const int    istart,
                         const int    icount,
                         int   *icount_ret,
                         double  *IDs,
                         int   *err)
{
  hid_t hpid;
  ADFH_DEBUG(("ADFH_Children_IDs"));
  ROOT_OR_DIE(err);
  if (icount_ret == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  if (IDs == NULL) {
    set_error(NULL_NODEID_POINTER, err);
    return;
  }
  IDs[0]=-1;
  mta_root->i_start = istart;
  mta_root->i_len = icount;
  mta_root->n_names = 0;
#ifdef ADFH_NO_ORDER
  mta_root->i_count = 0;
#endif
  if ((hpid = open_node(pid, err)) >= 0) {
    H5Literate(hpid,H5_INDEX_CRT_ORDER,H5_ITER_INC,
               NULL,children_ids,(void *)IDs);
    if (IDs[0]==-1)
    {
      H5Literate(hpid,H5_INDEX_NAME,H5_ITER_INC,
                 NULL,children_ids,(void *)IDs);
    }
    H5Gclose(hpid);
  }
  *icount_ret = mta_root->n_names;
  return;
}

/* ----------------------------------------------------------------- */

void ADFH_Release_ID(const double ID)
{
  hid_t hid;
  ADFH_DEBUG(("ADFH_Release_ID"));

  to_HDF_ID(ID, hid);
  H5Gclose(hid);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Open(const char   *name,
                        const char   *stat,
                        const char   *fmt,
                        double       *root,
                        int          *err)
{
  hid_t fid, gid;
  char *format, buff[ADF_VERSION_LENGTH+1];
  int i, pos, mode;
  hid_t g_propfileopen;

  ADFH_DEBUG(("ADFH_Database_Open [%s]",name));

  /* to be thread safe, should have critical section here */
  if (mta_root==NULL)
  {
    mta_root=(ADFH_MTA*)malloc(sizeof(struct _ADFH_MTA));
    mta_root->g_init = 0;
  }
  mta_root->g_error_state = 0;
  /* flags is int seen as bitfield, fortran flag is first 0x0001
     it is found set to 1 in *all* MLL-based HDF5 files
  */
  mta_root->g_flags = 1;

#ifndef ADFH_DEBUG_ON
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
#endif
  if (!mta_root->g_init) {
#ifndef ADFH_DEBUG_ON
    H5Eset_auto2(H5E_DEFAULT, walk_H5_error, NULL);
#endif
    for (i = 0; i < ADFH_MAXIMUM_FILES; i++) mta_root->g_files[i] = 0;
    mta_root->g_init = 1;

    /* create properties - these are persistent across all open files.
       When all files are closed, then delete properties */
    /* H5Pclose performed at file close time */
    mta_root->g_proplink=H5Pcreate(H5P_LINK_ACCESS);
    H5Pset_nlinks(mta_root->g_proplink, ADF_MAXIMUM_LINK_DEPTH);
    mta_root->g_propgroupcreate=H5Pcreate(H5P_GROUP_CREATE);
    H5Pset_link_creation_order(mta_root->g_propgroupcreate,
                               H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
    mta_root->g_propdataset=H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_alloc_time(mta_root->g_propdataset, H5D_ALLOC_TIME_EARLY);
    H5Pset_fill_time(mta_root->g_propdataset, H5D_FILL_TIME_NEVER); 
  }

  if (name == NULL || stat == NULL || fmt == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }

  /* get open mode */

  strncpy(buff, stat, 9);
  buff[9] = 0;
  for (i = 0; buff[i]; i++)
    buff[i] = TO_UPPER(buff[i]);

  if (0 == strcmp(buff, "UNKNOWN")) {
    if (ACCESS(name, 0))
      mode = ADFH_MODE_NEW;
    else if (ACCESS(name, 2))
      mode = ADFH_MODE_RDO;
    else
      mode = ADFH_MODE_OLD;
  }
  else if (0 == strcmp(buff, "NEW")) {
    if (!ACCESS(name, 0)) {
      set_error(REQUESTED_NEW_FILE_EXISTS, err);
      return;
    }
    mode = ADFH_MODE_NEW;
  }
  else if (0 == strcmp(buff, "READ_ONLY")) {
    if (ACCESS(name, 0)) {
      set_error(REQUESTED_OLD_FILE_NOT_FOUND, err);
      return;
    }
    mode = ADFH_MODE_RDO;
  }
  else if (0 == strcmp(buff, "OLD")) {
    if (ACCESS(name, 0)) {
      set_error(REQUESTED_OLD_FILE_NOT_FOUND, err);
      return;
    }
    mode = ADFH_MODE_OLD;
  }
  else {
    set_error(ADF_FILE_STATUS_NOT_RECOGNIZED, err);
    return;
  }

  /* get format */

#if 0
  if (mode == ADFH_MODE_NEW) {
    strncpy(buff, fmt, 11);
    buff[11] = 0;
    for (i = 0; buff[i]; i++)
      buff[i] = TO_UPPER(buff[i]);

    if (strcmp(buff, "NATIVE") &&
        strncmp(buff, "IEEE_BIG", 8) &&
        strcmp(buff, "IEEE_LITTLE", 11) &&
        strcmp(buff, "CRAY")) {
      set_error(ADF_FILE_FORMAT_NOT_RECOGNIZED, err);
      return;
    }
  }
#endif

  /* get unused slot */

  for (pos = 0; pos < ADFH_MAXIMUM_FILES; pos++) {
    if (mta_root->g_files[pos] == 0) break;
  }
  if (pos == ADFH_MAXIMUM_FILES) {
    set_error(TOO_MANY_ADF_FILES_OPENED, err);
    return;
  }

  /* Patch from Manuel Gageik on IBM BLUEgene/Q systems for better cgp_open performance. */
#ifdef JFC_PATCH_2015_2

  /* http://www.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_meta_block_size.htm
   * default setting is 2048 bytes
   */
  H5Pset_meta_block_size(g_propfileopen, 4096);  /* 1024*1024 */

  /* http://hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_alignment.htm
   * attention: this can increase filesize dramatically if lots of small datasets
   */
  H5Pset_alignment(g_propfileopen, 4096, 4096);

  /* http://www.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_buffer.htm
   * 1 MByte is default of hdf5
   */
  void *tconv; void *bkg;
  H5Pset_buffer(g_propfileopen, 10*1024*1024,tconv, bkg);

  /* http://hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetSieveBufSize
   * '..  used by file drivers that are capable of using data sieving'
   *  1 MByte is default of hdf5
   */
  H5Pset_sieve_buf_size(g_propfileopen, 4*1024*1024);

#endif

  g_propfileopen = H5Pcreate(H5P_FILE_ACCESS);
#ifdef ADFH_H5F_CLOSE_STRONG
  /* set access property to close all open accesses when file closed */
  H5Pset_fclose_degree(g_propfileopen, H5F_CLOSE_STRONG);
#endif

  /*  H5Pset_latest_format(fapl, 1); */
  /* Performance patch applied by KSH on 2009.05.18 */
  H5Pset_libver_bounds(g_propfileopen,
#if H5_VERSION_GE(1,10,3)
		       H5F_LIBVER_V18, H5F_LIBVER_V18);
#else
		       H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
#endif
  /* open the file */

#if CG_BUILD_PARALLEL
  int flag = 0;
  /* check if we are actually running a parallel program */
  MPI_Initialized(&flag);
  if(flag) {
    /* Set the access property list to use MPI */
    if (0 == strcmp(fmt, "PARALLEL")) {

      if(!pcg_mpi_info) pcg_mpi_info = MPI_INFO_NULL;
#if HDF5_HAVE_COLL_METADATA  
      H5Pset_coll_metadata_write(g_propfileopen, 1);
#endif /*HDF5_HAVE_COLL_METADATA*/

      H5Pset_fapl_mpio(g_propfileopen, ParallelMPICommunicator, pcg_mpi_info);
    }
  }
#endif

  set_error(NO_ERROR, err);

  if (mode == ADFH_MODE_NEW) {
    hid_t g_propfilecreate = H5Pcreate(H5P_FILE_CREATE);

#ifdef JFC_PATCH_2015_2

  /* http://www.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_meta_block_size.htm
   * default setting is 2048 bytes
   */
  H5Pset_meta_block_size(g_propfilecreate, 4096);  // 1024*1024

  /* http://hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_alignment.htm
   * attention: this can increase filesize dramatically if lots of small datasets
   */
  H5Pset_alignment(g_propfilecreate, 4096, 4096);

  /* http://www.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_buffer.htm
   * 1 MByte is default of hdf5
   */
  void *tconv; void *bkg;
  H5Pset_buffer(g_propfilecreate, 10*1024*1024,tconv, bkg);

  /* http://hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetSieveBufSize
   * '..  used by file drivers that are capable of using data sieving'
   * 1 MByte is default of hdf5
   */
  H5Pset_sieve_buf_size(g_propfilecreate, 4*1024*1024);

#endif

#if 0 /* MSB -- DISABLED as it is not compatible with HDF5 1.8 file format, need to resolve this CGNS-166 */
#if HDF5_HAVE_FILE_SPACE_STRATEGY
    H5Pset_file_space_strategy(g_propfilecreate, H5F_FSPACE_STRATEGY_FSM_AGGR, 1, (hsize_t)1);
#endif
#endif

    /* add creation time for groups (used by iterators)
      (prop set to file creation )*/
    H5Pset_link_creation_order(g_propfilecreate,
                               H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
    fid = H5Fcreate(name, H5F_ACC_TRUNC, g_propfilecreate, g_propfileopen);
    H5Pclose(g_propfilecreate);
    H5Pclose(g_propfileopen);
    if (fid < 0) {
      set_error(FILE_OPEN_ERROR, err);
      return;
    }
    gid = H5Gopen2(fid, "/", H5P_DEFAULT);
    memset(buff, 0, ADF_VERSION_LENGTH+1);
    ADFH_Library_Version(buff, err);
    format = native_format();
    if (new_str_att(gid, A_NAME, "HDF5 MotherNode", ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_LABEL, "Root Node of HDF5 File", ADF_NAME_LENGTH, err) ||
        new_str_att(gid, A_TYPE, ADFH_MT, 2, err) ||
        new_str_data(gid, D_FORMAT, format, (int)strlen(format), err) ||
        new_str_data(gid, D_VERSION, buff, ADF_VERSION_LENGTH, err)) {
      H5Gclose(gid);
      return;
    }
  }
  else {
    if (H5Fis_hdf5(name) <= 0) {
      H5Pclose(g_propfileopen);
      set_error(ADFH_ERR_NOT_HDF5_FILE, err);
      return;
    }
#if CG_BUILD_PARALLEL
#if HDF5_HAVE_COLL_METADATA
    H5Pset_all_coll_metadata_ops( g_propfileopen, 1 );
#endif
#endif
    if (mode == ADFH_MODE_RDO) {
      fid = H5Fopen(name, H5F_ACC_RDONLY, g_propfileopen);
    }
    else {
      fid = H5Fopen(name, H5F_ACC_RDWR, g_propfileopen);
    }
    H5Pclose(g_propfileopen);
    if (fid < 0) {
      set_error(FILE_OPEN_ERROR, err);
      return;
    }
    gid = H5Gopen2(fid, "/", H5P_DEFAULT);
#ifdef ADFH_FORTRAN_INDEXING
    if (mode != ADFH_MODE_RDO && has_child(gid, D_OLDVERS)) {
      H5Giterate(gid, ".", NULL, fix_dimensions, NULL);
      H5Gmove(gid, D_OLDVERS, D_VERSION);
    }
#endif
  }
  mta_root->g_files[pos] = fid;
#ifndef ADFH_FORCE_ID_CLOSE
  mta_root->g_extids[pos]=NULL;
  mta_root->n_extids[pos]=0;
  mta_root->x_extids[pos]=-1;

  track_id(fid,gid);
#endif
  to_ADF_ID(gid, *root);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Valid(const char   *name,
                        int          *err)
{
    if (NULL == name || 0 == *name)
        *err = NULL_STRING_POINTER;
    else
        *err = H5Fis_hdf5(name);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Get_Format(const double  rootid,
                              char         *format,
                              int          *err)
{
  char node[ADF_NAME_LENGTH+1];
  hid_t did, hid;
  herr_t status;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Database_Get_Format"));

  if (format == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  *format = 0;
  set_error(NO_ERROR, err);

  sprintf(node, "/%s", D_FORMAT);
  to_HDF_ID(rootid,hid);
  if ((did = H5Dopen2(hid, node, H5P_DEFAULT)) < 0) {
    set_error(ADFH_ERR_DOPEN, err);
    return;
  }

#if CG_BUILD_PARALLEL
  hid_t fid = get_file_id(hid);
  hid_t fapl=H5Fget_access_plist(fid);
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    ADFH_CHECK_HID(xfer_prp);
    H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
  }
#endif

  status = H5Dread(did, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, xfer_prp, format);

#if CG_BUILD_PARALLEL
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    H5Pclose(xfer_prp);
  }
  H5Pclose(fapl); /* close the property list */
#endif
  H5Dclose(did);

  if (status < 0)
    set_error(ADFH_ERR_DREAD, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Set_Format(const double  rootid,
                              const char   *format,
                              int          *err)
{
  ADFH_DEBUG(("ADFH_Database_Set_Format"));
  set_error(ADFH_ERR_NOT_IMPLEMENTED, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Delete(const char *name,
                          int        *err)
{
  ADFH_DEBUG(("ADFH_Database_Delete [%s]",name));

  if (H5Fis_hdf5(name) <= 0)
    set_error(ADFH_ERR_NOT_HDF5_FILE, err);
  else if (UNLINK(name))
    set_error(ADFH_ERR_FILE_DELETE, err);
  else
    set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Close(const double  root,
                         int          *status)
{
  int fn,idx;
  hid_t hid,fid;
  ssize_t nobj,n;
#ifdef ADFH_FORCE_ID_CLOSE
  hid_t *objs;
#endif

  ADFH_DEBUG(("ADFH_Database_Close"));
  if (mta_root == NULL)
  {
    ADFH_DEBUG(("ADFH_Database_Close [mta is null]"));
    return;
  }
  ADFH_DEBUG(("ADFH_Database_Close 4"));
  to_HDF_ID(root,hid);
  if ((fn = get_file_number(hid, status)) < 0) return;
  fid = mta_root->g_files[fn];
  mta_root->g_files[fn] = 0;

#if !defined(ADFH_H5F_CLOSE_STRONG) && !defined(ADFH_FORCE_ID_CLOSE)
  /* this gets the file data to disk even if there are open objects */
  H5Fflush(fid, H5F_SCOPE_LOCAL);
#endif

  /* free up all open accesses */
#ifndef ADFH_FORCE_ID_CLOSE
  for (nobj=0;nobj<mta_root->n_extids[fn];nobj++)
  {
    ADFH_DEBUG(("ADFH_Database_Close 3 [%.6d/%.6d]:[%d]",\
                nobj,mta_root->n_extids[fn],mta_root->g_extids[fn][nobj]));
    if (H5Iis_valid(mta_root->g_extids[fn][nobj]))
    {
      H5Oclose(mta_root->g_extids[fn][nobj]);
    }
  }
#else
  ADFH_DEBUG(("ADFH_Database_Close 3"));
  nobj = H5Fget_obj_count(fid, H5F_OBJ_ALL|H5F_OBJ_LOCAL);
  if (nobj) {
    objs = (hid_t *) malloc (nobj * sizeof(hid_t));

    /* close datatypes */

    nobj = H5Fget_obj_count(fid, H5F_OBJ_DATATYPE|H5F_OBJ_LOCAL);
#ifdef ADFH_DEBUG_ON
    printf("%s close DataType [%d] HIDs\n",ADFH_PREFIX,nobj);
#endif
    if (nobj) {
      H5Fget_obj_ids(fid, H5F_OBJ_DATATYPE|H5F_OBJ_LOCAL, -1, objs);
      for (n = 0; n < nobj; n++)
        H5Tclose(objs[n]);
    }

    /* close datasets */

    nobj = H5Fget_obj_count(fid, H5F_OBJ_DATASET|H5F_OBJ_LOCAL);
#ifdef ADFH_DEBUG_ON
    printf("%s close DataSet [%d] HIDs\n",ADFH_PREFIX,nobj);
#endif
    if (nobj) {
      H5Fget_obj_ids(fid, H5F_OBJ_DATASET|H5F_OBJ_LOCAL, -1, objs);
      for (n = 0; n < nobj; n++)
        H5Dclose(objs[n]);
    }

    /* close attributes */

    nobj = H5Fget_obj_count(fid, H5F_OBJ_ATTR|H5F_OBJ_LOCAL);
#ifdef ADFH_DEBUG_ON
    printf("%s close Attr [%d] HIDs\n",ADFH_PREFIX,nobj);
#endif
    if (nobj) {
      H5Fget_obj_ids(fid, H5F_OBJ_ATTR|H5F_OBJ_LOCAL, -1, objs);
      for (n = 0; n < nobj; n++)
        H5Aclose(objs[n]);
    }

    /* close groups */

    nobj = H5Fget_obj_count(fid, H5F_OBJ_GROUP|H5F_OBJ_LOCAL);
#ifdef ADFH_DEBUG_ON
    printf("%s close Group [%d] HIDs\n",ADFH_PREFIX,nobj);
#endif
    if (nobj) {
      H5Fget_obj_ids(fid, H5F_OBJ_GROUP|H5F_OBJ_LOCAL, -1, objs);
      for (n = 0; n < nobj; n++)
        H5Gclose(objs[n]);
    }

#if 0
    /* close file accesses except for current */

    nobj = H5Fget_obj_count(fid, H5F_OBJ_FILE|H5F_OBJ_LOCAL);
    if (nobj) {
      H5Fget_obj_ids(fid, H5F_OBJ_FILE|H5F_OBJ_LOCAL, -1, objs);
      for (n = 0; n < nobj; n++) {
        if (objs[n] != fid)
          H5Fclose(objs[n]);
      }
    }
#endif

    free (objs);
  }
#endif

  ADFH_DEBUG(("ADFH_Database_Close 2"));

  /* close file */
  if (H5Fclose(fid) < 0)
    set_error(FILE_CLOSE_ERROR, status);
  else
    set_error(NO_ERROR, status);

#ifndef ADFH_FORCE_ID_CLOSE
  free(mta_root->g_extids[fn]);
#endif

  ADFH_DEBUG(("ADFH_Database_Close 1"));
  idx=0;
  for (n = 0; n < ADFH_MAXIMUM_FILES; n++) {
    if (mta_root->g_files[n]) {
      idx++;
    }
  }
  /* if no more files open, close properties and free MTA */
  if (idx == 0) {
    H5Pclose(mta_root->g_proplink);
    H5Pclose(mta_root->g_propgroupcreate);
    H5Pclose(mta_root->g_propdataset);
    free(mta_root);
    mta_root=NULL;
  }
  ADFH_DEBUG(("ADFH_Database_Close 0"));
}

/* ----------------------------------------------------------------- */

void ADFH_Is_Link(const double  id,
                  int          *link_path_length,
                  int          *err)
{
  hid_t hid;
  to_HDF_ID(id,hid);

  ADFH_DEBUG(("ADFH_Is_Link"));

  if (is_link(hid)) {
    hid_t did, sid;
    hsize_t size;

    did = H5Dopen2(hid, D_PATH, H5P_DEFAULT);
    ADFH_CHECK_HID(did);
    sid = H5Dget_space(did);
    ADFH_CHECK_HID(sid);
    size = H5Sget_simple_extent_npoints(sid);
    H5Sclose(sid);
    H5Dclose(did);
    *link_path_length = (int)size;

    if (has_child(hid, D_FILE)) {
      did = H5Dopen2(hid, D_FILE, H5P_DEFAULT);
      ADFH_CHECK_HID(did);
      sid = H5Dget_space(did);
      ADFH_CHECK_HID(sid);
      size = H5Sget_simple_extent_npoints(sid);
      H5Sclose(sid);
      H5Dclose(did);
      *link_path_length += (int)size;
    }
  }
  else
    *link_path_length = 0;

  set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Link_Size(const double  id,
                  int          *file_len,
                  int          *name_len,
                  int          *err)
{
  hid_t hid;
  to_HDF_ID(id,hid);
  ADFH_DEBUG(("ADFH_Link_Size"));

  *name_len = *file_len = 0;
  if (is_link(hid)) {
    hid_t did, sid;
    hsize_t size;

    did = H5Dopen2(hid, D_PATH, H5P_DEFAULT);
    sid = H5Dget_space(did);
    size = H5Sget_simple_extent_npoints(sid);
    H5Sclose(sid);
    H5Dclose(did);
    *name_len = (int)size;

    if (has_child(hid, D_FILE)) {
      did = H5Dopen2(hid, D_FILE, H5P_DEFAULT);
      sid = H5Dget_space(did);
      size = H5Sget_simple_extent_npoints(sid);
      H5Sclose(sid);
      H5Dclose(did);
      *file_len = (int)size;
    }
  }

  set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Root_ID(const double  id,
                      double       *root_id,
                      int          *err)
{
  hid_t rid, hid;

  ADFH_DEBUG(("ADFH_Get_Root_ID"));

  to_HDF_ID(id, hid);

  rid = H5Gopen2(hid, "/", H5P_DEFAULT);
  if (rid < 0)
    set_error(ADFH_ERR_GOPEN, err);
  else {
    to_ADF_ID(rid, *root_id);
    set_error(NO_ERROR, err);
  }
#ifndef ADFH_FORCE_ID_CLOSE
  track_id(hid,rid);
#endif
  ADFH_DEBUG(("ADFH_Get_Root_ID root id [%d]",rid));
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Data_Type(const double  id,
                        char         *data_type,
                        int          *err)
{
  hid_t hid;
  char buffdata_type[3];

  ADFH_DEBUG(("ADFH_Get_Data_Type"));

  if ((hid = open_node(id, err)) >= 0) {
    get_str_att(hid, A_TYPE, buffdata_type, err);
    H5Gclose(hid);
    strcpy(data_type,buffdata_type);
  }
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Number_of_Dimensions(const double  id,
                                   int          *num_dims,
                                   int          *err)
{
  hid_t hid, did, sid;
  char type[3];

  ADFH_DEBUG(("ADFH_Get_Number_of_Dimensions"));

  *num_dims = 0;
  if ((hid = open_node(id, err)) < 0) return;
  if (get_str_att(hid, A_TYPE, type, err) ||
    0 == strcmp(type, ADFH_MT) || 0 == strcmp(type, ADFH_LK)) {
    H5Gclose(hid);
    return;
  }

  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0)
    set_error(NO_DATA, err);
  else {
    if ((sid = H5Dget_space(did)) < 0)
      set_error(ADFH_ERR_DGET_SPACE, err);
    else {
      *num_dims = H5Sget_simple_extent_ndims(sid);
      H5Sclose(sid);
    }
    H5Dclose(did);
  }
  H5Gclose(hid);
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Dimension_Values(const double  id,
                               cgsize_t      dim_vals[],
                               int           *err)
{
  int i, ndims, swap = 0;
  hid_t hid, did, sid;
  hsize_t temp_vals[ADF_MAX_DIMENSIONS];

  ADFH_DEBUG(("ADFH_Get_Dimension_Values"));

  ndims=0;
  dim_vals[0]=0;
  if ((hid = open_node(id, err)) < 0) return;
  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0)
    set_error(NO_DATA, err);
  else {
    if ((sid = H5Dget_space(did)) < 0)
      set_error(ADFH_ERR_DGET_SPACE, err);
    else {
      ndims = H5Sget_simple_extent_ndims(sid);
      if (ndims > 0) {
        H5Sget_simple_extent_dims(sid, temp_vals, NULL);
#if CG_SIZEOF_SIZE == 32
        for (i = 0; i < ndims; i++) {
          if (temp_vals[i] > CG_MAX_INT32) {
            set_error(MAX_INT32_SIZE_EXCEEDED, err);
            break;
          }
        }
#endif
#ifdef ADFH_FORTRAN_INDEXING
        if (ndims > 1) swap = swap_dimensions(hid);
#endif
        if (swap) {
          for (i = 0; i < ndims; i++) {
            dim_vals[i] = (cgsize_t)temp_vals[ndims-1-i];
          }
        }
        else {
          for (i = 0; i < ndims; i++) {
            dim_vals[i] = (cgsize_t)temp_vals[i];
          }
        }
      }
      H5Sclose(sid);
    }
    H5Dclose(did);
  }
  H5Gclose(hid);
}

/* ----------------------------------------------------------------- */

void ADFH_Put_Dimension_Information(const double   id,
                                    const char    *data_type,
                                    const int      dims,
                                    const cgsize_t dim_vals[],
                                    const int      HDF5storage_type,
                                    int           *err)
{
  hid_t hid;
  hid_t did, tid, sid;
  int i, swap = 0;
  hsize_t new_dims[ADF_MAX_DIMENSIONS];
  char new_type[3];

  to_HDF_ID(id,hid);

  ADFH_DEBUG(("ADFH_Put_Dimension_Information [%d]",hid));

  if (is_link(hid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  for (i = 0; i < 2; i++)
    new_type[i] = TO_UPPER(data_type[i]);
  new_type[2] = 0;

  if (0 == strcmp(new_type, ADFH_MT)) {
    if (has_data(hid))
      H5Gunlink(hid, D_DATA);
    set_str_att(hid, A_TYPE, new_type, err);
    return;
  }

  if (check_data_type(new_type, err)) return;
  if (dims < 1 || dims > ADF_MAX_DIMENSIONS) {
    set_error(BAD_NUMBER_OF_DIMENSIONS, err);
    return;
  }
  for (i = 0; i < dims; i++) {
    if (dim_vals[i] < 1) {
      set_error(BAD_DIMENSION_VALUE, err);
      return;
    }
  }

  /*
   * The ADF documentation allows the dimension values to be changed
   * without affecting the data, so long as the data type and number
   * of dimensions are the same. With HDF5, we could emulate that by
   * using extendable data spaces (with chunking). However this only
   * allows the data size to increase, not decrease, and coming up
   * with a good value for chunking is difficult. Since changing the
   * dimension values without rewriting the data is not a common
   * operation, I decided to use fixed sizes, then buffer the data
   * in these rare cases.
   */

  if(has_data(hid)) {
    ADFH_DEBUG(("ADFH_Put_Dimension_Information unlink [%d]",hid));
    H5Gunlink(hid, D_DATA);
  }

  if (set_str_att(hid, A_TYPE, new_type, err)) {
    return;
  }

  /* recreate the data space with the new values */

#ifdef ADFH_FORTRAN_INDEXING
  if (dims > 1) swap = swap_dimensions(hid);
#endif
  if (swap) {
    for (i = 0; i < dims; i++)
      new_dims[i] = (hsize_t)dim_vals[dims-1-i];
  }
  else {
    for (i = 0; i < dims; i++)
      new_dims[i] = (hsize_t)dim_vals[i];
  }

  tid = to_HDF_data_type(new_type);
  ADFH_CHECK_HID(tid);
  sid = H5Screate_simple(dims, new_dims, NULL);
  /* better idea? how to guess the right size? */
  if (CompressData >= 0)
  {
    H5Pset_deflate(mta_root->g_propdataset, CompressData);
  }
#if 0
  this causes a problem with memory allocation. For example,
  writing an unstructured coordinate array of 5 billion values
  will result in the HDF5 library trying to allocation 20Gb
  of memory for the chunk, since the first dimension is 5 billion.
  We really need to try to do something more intelligent here

  H5Pset_chunk(mta_root->g_propdataset, dims, new_dims);
#endif

  hssize_t dset_size = H5Sget_select_npoints(sid);
  size_t dtype_size = H5Tget_size(tid); 

  /* Chunked datasets are currently not supported */

  /* Compact storage has a dataset size limit of 64 KiB */
  if(HDF5storage_type == CGIO_COMPACT && dset_size*(hssize_t)dtype_size  < (hssize_t)CGNS_64KB)
    H5Pset_layout(mta_root->g_propdataset, H5D_COMPACT);
  else{
    H5Pset_layout(mta_root->g_propdataset, H5D_CONTIGUOUS);
    H5Pset_alloc_time(mta_root->g_propdataset, H5D_ALLOC_TIME_EARLY);
    H5Pset_fill_time(mta_root->g_propdataset, H5D_FILL_TIME_NEVER); 
  }

  ADFH_CHECK_HID(sid);
  did = H5Dcreate2(hid, D_DATA, tid, sid,
                   H5P_DEFAULT, mta_root->g_propdataset, H5P_DEFAULT);
/*  H5Eprint1(stdout);*/
  ADFH_CHECK_HID(did);

  H5Sclose(sid);
  H5Tclose(tid);

  if (did < 0)
    set_error(ADFH_ERR_DCREATE, err);
  else {
    H5Dclose(did);
    set_error(NO_ERROR, err);
  }
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Link_Path(const double  id,
                        char   *filename,
                        char   *link_path,
                        int    *err)
{
  hid_t hid, did;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Get_Link_Path"));
  to_HDF_ID(id,hid);
  ADFH_CHECK_HID(hid);
  if (!is_link(hid)) {
    set_error(NODE_IS_NOT_A_LINK, err);
    return;
  }

#if CG_BUILD_PARALLEL
  if (pcg_mpi_initialized) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    ADFH_CHECK_HID(xfer_prp);
    H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
  }
#endif

  did = H5Dopen2(hid, D_PATH, H5P_DEFAULT);
  ADFH_CHECK_HID(did);
  H5Dread(did, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, xfer_prp, link_path);
  H5Dclose(did);

  if (has_child(hid, D_FILE)) {
    did = H5Dopen2(hid, D_FILE, H5P_DEFAULT);
    ADFH_CHECK_HID(did);
    H5Dread(did, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, xfer_prp, filename);
    H5Dclose(did);
  }
  else
  {
    *filename = 0;
  }

#if CG_BUILD_PARALLEL
  if (pcg_mpi_initialized) {
    H5Pclose(xfer_prp);
  }
#endif

  set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Link(const double  pid,
               const char   *name,
               const char   *file,
               const char   *name_in_file,
               double       *id,
               int          *err)
{
  char *target;
  herr_t status;
  hid_t lid, hid;
  (void)hid;  /* avoid unused variable warning */

  ADFH_DEBUG(("ADFH_Link [%s][%s][%s]",name,file,name_in_file));

  ROOT_OR_DIE(err);
  ADFH_Create(pid, name, id, err);

  if (*err != NO_ERROR) return;

  to_HDF_ID(*id,lid);
  ADFH_CHECK_HID(lid);
  if (set_str_att(lid, A_TYPE, ADFH_LK, err)) return;

  /*
   * If this is a link to a file, then need to create external link
   * Otherwise, create a soft link
   */
  if (*file) {

  /* actual link is in the D_LINK group to avoid collision with "real" node
     because we cannot have a node ID  and a link on this ID (can we?)
     set actual link to D_LINK name (not the current node) */
  H5Lcreate_external(file,
                     name_in_file,
                     lid,
                     D_LINK,
                     H5P_DEFAULT,
                     mta_root->g_proplink);
  }
  else
  {
    target = (char *) malloc (strlen(name_in_file)+2);
    if (target == NULL) {
      set_error(MEMORY_ALLOCATION_FAILED, err);
      return;
    }
    if (*name_in_file == '/')
      strcpy(target, name_in_file);
    else
      sprintf(target, "/%s", name_in_file);

    /* create a soft link */

    status = H5Glink(lid, H5G_LINK_SOFT, target, D_LINK);
    free(target);
    if (status < 0) {
      set_error(ADFH_ERR_GLINK, err);
      return;
    }
  }

  /* save link path and file */
  if (new_str_data(lid,D_PATH,name_in_file,(int)strlen(name_in_file),err)) return;
  if (*file && new_str_data(lid,D_FILE,file,(int)strlen(file),err))        return;
#ifndef ADFH_FORCE_ID_CLOSE
  hid_t hid;
  to_HDF_ID(pid,hid);
  track_id(hid,lid);
#endif
  set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Flush_to_Disk(const double  id,
                        int          *err)
{
  hid_t hid;
  ADFH_DEBUG(("ADFH_Flush_to_Disk"));

  to_HDF_ID(id,hid);
  if(H5Fflush(hid, H5F_SCOPE_LOCAL) >=0 )
    set_error(NO_ERROR, err);
  else
    set_error(FFLUSH_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Garbage_Collection(const double  id,
                                      int          *err)
{
  ADFH_DEBUG(("ADFH_Database_Garbage_Collection"));

  if(H5garbage_collect() >= 0)
    set_error(NO_ERROR, err);
  else
    set_error(NO_DATA, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Database_Version(const double  root_id,
                           char         *version,
                           char         *creation_date,
                           char         *modification_date,
                           int          *err)
{
  char buff[ADF_VERSION_LENGTH+1];
  char node[ADF_NAME_LENGTH+1];
  hid_t hid, did;
  herr_t status;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Database_Version"));

  if (version == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  *version = 0;
  if (creation_date != NULL) *creation_date = 0;
  if (modification_date != NULL) *modification_date = 0;
  set_error(NO_ERROR, err);

  sprintf(node, "/%s", D_VERSION);
  to_HDF_ID(root_id,hid);
  if ((did = H5Dopen2(hid, node, H5P_DEFAULT)) < 0) {
#ifdef ADFH_FORTRAN_INDEXING
    sprintf(node, "/%s", D_OLDVERS);
    if ((did = H5Dopen2(hid, node, H5P_DEFAULT)) < 0) {
      set_error(ADFH_ERR_DOPEN, err);
      return;
    }
#else
    set_error(ADFH_ERR_DOPEN, err);
    return;
#endif
  }
#if CG_BUILD_PARALLEL
  hid_t fid = get_file_id(hid);
  hid_t fapl=H5Fget_access_plist(fid);
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
  }
#endif
  status = H5Dread(did, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, xfer_prp, buff);
  H5Dclose(did);
#if CG_BUILD_PARALLEL

  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    H5Pclose(xfer_prp);
  }
#endif
  if (status < 0)
    set_error(ADFH_ERR_DREAD, err);
  else
    strcpy(version, buff);
}

/* ----------------------------------------------------------------- */

void ADFH_Library_Version(char *version,
                          int  *err)
{
  unsigned maj, min, rel;

  ADFH_DEBUG(("ADFH_Library_Version"));

  if (version == NULL) {
    set_error(NULL_STRING_POINTER, err);
    return;
  }
  H5get_libversion(&maj, &min, &rel);
  sprintf(version, "HDF5 Version %d.%d.%d", maj, min, rel);
  set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Set_Error_State(const int  error_state,
                          int       *error_return)
{
  ADFH_DEBUG(("ADFH_Set_Error_State"));

  ROOT_OR_DIE(error_return);
  mta_root->g_error_state = error_state;
  set_error(NO_ERROR, error_return);
}

/* ----------------------------------------------------------------- */

void ADFH_Error_Message(const int  error_return_input,
                        char      *error_string )
{
  int i;

  ADFH_DEBUG(("ADFH_Error_Message"));

  if (error_string == NULL) return;

  for (i = 0; i < NUM_ERRORS; i++) {
    if (ErrorList[i].errcode == error_return_input) {
      strcpy(error_string, ErrorList[i].errmsg);
      return;
    }
  }
  sprintf(error_string, "error number %d", error_return_input);
}

/* ----------------------------------------------------------------- */

void ADFH_Get_Error_State(int *error_state,
                          int *error_return)
{
  ADFH_DEBUG(("ADFH_Get_Error_State"));
  ROOT_OR_DIE(error_return);
  *error_state = mta_root->g_error_state;
  set_error(NO_ERROR, error_return);
}

/* ----------------------------------------------------------------- */

void ADFH_Read_Block_Data(const double ID,
                      const cgsize_t b_start,
                      const cgsize_t b_end,
                      const char *m_data_type,
                      void *data,
                      int *err )
{
  hid_t hid, did, mid, dspace;
  size_t size, count, offset;
  char *buff;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Read_Block_Data"));

  if (data == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  if (b_start > b_end) {
    set_error(MINIMUM_GT_MAXIMUM, err);
    return;
  }
  if (b_start < 1) {
    set_error(START_OUT_OF_DEFINED_RANGE, err);
    return;
  }
  if ((hid = open_node(ID, err)) < 0) return;

  if (!has_data(hid)) {
    H5Gclose(hid);
    set_error(NO_DATA, err);
    return;
  }
  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0) {
    H5Gclose(hid);
    set_error(ADFH_ERR_DOPEN, err);
    return;
  }

  dspace = H5Dget_space(did);
  ADFH_CHECK_HID(dspace);
  count = (size_t)H5Sget_simple_extent_npoints(dspace);
  H5Sclose(dspace);

  if ((size_t)b_end > count) {
    H5Dclose(did);
    H5Gclose(hid);
    set_error(END_OUT_OF_DEFINED_RANGE, err);
    return;
  }

  /* instead of trying to compute data space extents from
   * b_start and b_end, just read all the data into a
   * 1-d array and copy the range we want */

  if (m_data_type) {
    mid = to_HDF_data_type(m_data_type);
  }
  else {
    set_error(INVALID_DATA_TYPE, err);
    return;
  }
  ADFH_CHECK_HID(mid);
  size = H5Tget_size(mid);

  if ((buff = (char *) malloc (size * count)) == NULL) {
    H5Tclose(mid);
    H5Dclose(did);
    H5Gclose(hid);
    set_error(MEMORY_ALLOCATION_FAILED, err);
    return;
  }

#if CG_BUILD_PARALLEL
  hid_t fid = get_file_id(hid);
  hid_t fapl=H5Fget_access_plist(fid);
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    ADFH_CHECK_HID(xfer_prp);
    H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
  }
#endif

  if (H5Dread(did, mid, H5S_ALL, H5S_ALL, xfer_prp, buff) < 0)
    set_error(ADFH_ERR_DREAD, err);
  else {
    offset = size * (size_t)(b_start - 1);
    count = size * (size_t)(b_end - b_start + 1);
    memcpy(data, &buff[offset], count);
    set_error(NO_ERROR, err);
  }

  free (buff);
#if CG_BUILD_PARALLEL
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    H5Pclose(xfer_prp);
  }
#endif
  H5Tclose(mid);
  H5Dclose(did);
  H5Gclose(hid);
}

/* ----------------------------------------------------------------- */

void ADFH_Read_Data(const double ID,
                    const cgsize_t s_start[],
                    const cgsize_t s_end[],
                    const cgsize_t s_stride[],
                    const int m_num_dims,
                    const cgsize_t m_dims[],
                    const cgsize_t m_start[],
                    const cgsize_t m_end[],
                    const cgsize_t m_stride[],
                    const char *m_data_type,
                    char *data,
                    int *err )
{
  int n, ndim;
  hid_t hid, did, mid, dspace, mspace;
  hsize_t dims[ADF_MAX_DIMENSIONS];
  hsize_t start[ADF_MAX_DIMENSIONS];
  hsize_t stride[ADF_MAX_DIMENSIONS];
  hsize_t count[ADF_MAX_DIMENSIONS];
  herr_t status;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Read_Data"));

  if ((hid = open_node(ID, err)) < 0) return;

  if (!has_data(hid)) {
    H5Gclose(hid);
    set_error(NO_DATA, err);
    return;
  }
  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0) {
    H5Gclose(hid);
    set_error(ADFH_ERR_DOPEN, err);
    return;
  }

  /* get data space extents */

  dspace = H5Dget_space(did);
  ADFH_CHECK_HID(dspace);
  ndim = H5Sget_simple_extent_ndims(dspace);
  H5Sget_simple_extent_dims(dspace, dims, NULL);

  /* create data hyperslab */

#ifdef ADFH_FORTRAN_INDEXING
  if (ndim > 1 && !swap_dimensions(hid)) {
      H5Sclose(dspace);
      H5Dclose(did);
      H5Gclose(hid);
      set_error(ADFH_ERR_NEED_TRANSPOSE, err);
      return;
  }
#endif
  for (n = 0; n < ndim; n++) {
    if (s_start[n] < 1)
      set_error(START_OUT_OF_DEFINED_RANGE, err);
#ifdef ADFH_FORTRAN_INDEXING
    else if ((hsize_t)s_end[n] > dims[ndim-1-n])
#else
    else if ((hsize_t)s_end[n] > dims[n])
#endif
      set_error(END_OUT_OF_DEFINED_RANGE, err);
    else if (s_start[n] > s_end[n])
      set_error(MINIMUM_GT_MAXIMUM, err);
    else if (s_stride[n] < 1 ||
      s_stride[n] > (s_end[n] - s_start[n] + 1))
      set_error(BAD_STRIDE_VALUE, err);
    else
      set_error(NO_ERROR, err);
    if (*err != NO_ERROR) {
      H5Sclose(dspace);
      H5Dclose(did);
      H5Gclose(hid);
      return;
    }
#ifdef ADFH_FORTRAN_INDEXING
    start[ndim-1-n] = s_start[n] - 1;
    stride[ndim-1-n] = s_stride[n];
    count[ndim-1-n] = (s_end[n] - s_start[n] + 1) / s_stride[n];
#else
    start[n] = s_start[n] - 1;
    stride[n] = s_stride[n];
    count[n] = (s_end[n] - s_start[n] + 1) / s_stride[n];
#endif
  }

  H5Sselect_hyperslab(dspace, H5S_SELECT_SET, start, stride, count, NULL);

  /* create memory hyperslab */

  for (n = 0; n < m_num_dims; n++) {
    if (m_start[n] < 1)
      set_error(START_OUT_OF_DEFINED_RANGE, err);
    else if (m_end[n] > m_dims[n])
      set_error(END_OUT_OF_DEFINED_RANGE, err);
    else if (m_start[n] > m_end[n])
      set_error(MINIMUM_GT_MAXIMUM, err);
    else if (m_stride[n] < 1 ||
      m_stride[n] > (m_end[n] - m_start[n] + 1))
      set_error(BAD_STRIDE_VALUE, err);
    else
      set_error(NO_ERROR, err);
    if (*err != NO_ERROR) {
      H5Sclose(dspace);
      H5Dclose(did);
      H5Gclose(hid);
      return;
    }
#ifdef ADFH_FORTRAN_INDEXING
    dims[m_num_dims-1-n] = m_dims[n];
    start[m_num_dims-1-n] = m_start[n] - 1;
    stride[m_num_dims-1-n] = m_stride[n];
    count[m_num_dims-1-n] = (m_end[n] - m_start[n] + 1) / m_stride[n];
#else
    dims[n] = m_dims[n];
    start[n] = m_start[n] - 1;
    stride[n] = m_stride[n];
    count[n] = (m_end[n] - m_start[n] + 1) / m_stride[n];
#endif
  }

  mspace = H5Screate_simple(m_num_dims, dims, NULL);
  ADFH_CHECK_HID(mspace);

  H5Sselect_hyperslab(mspace, H5S_SELECT_SET, start, stride, count, NULL);

  if (H5Sget_select_npoints(mspace) != H5Sget_select_npoints(dspace)) {
      H5Sclose(mspace);
      H5Sclose(dspace);
      H5Dclose(did);
      H5Gclose(hid);
      set_error(UNEQUAL_MEMORY_AND_DISK_DIMS, err);
      return;
  }

  /* read the data */

  if (m_data_type) {
    mid = to_HDF_data_type(m_data_type);
  }
  else {
    set_error(INVALID_DATA_TYPE, err);
    return;
  }
  ADFH_CHECK_HID(mid);

#if CG_BUILD_PARALLEL
  hid_t fid = get_file_id(hid);
  hid_t fapl=H5Fget_access_plist(fid);
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    ADFH_CHECK_HID(xfer_prp);
    H5Pset_dxpl_mpio(xfer_prp, default_pio_mode);
  }
#endif

  status = H5Dread(did, mid, mspace, dspace, xfer_prp, data);

#if CG_BUILD_PARALLEL
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    H5Pclose(xfer_prp);
  }
#endif
  H5Sclose(mspace);
  H5Sclose(dspace);
  H5Tclose(mid);
  H5Dclose(did);
  H5Gclose(hid);

  if (status < 0)
    set_error(ADFH_ERR_DREAD, err);
  else
    set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Read_All_Data(const double  id,
                        const char   *m_data_type,
                        char         *data,
                        int          *err)
{
  hid_t hid, did, mid;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Read_All_Data"));

  if ((hid = open_node(id, err)) < 0) return;

  if (has_data(hid)) {
    did = H5Dopen2(hid, D_DATA, H5P_DEFAULT);
    ADFH_CHECK_HID(did);
    if (m_data_type) {
      mid = to_HDF_data_type(m_data_type);
    }
    else {
      set_error(INVALID_DATA_TYPE, err);
      return;
    }
    ADFH_CHECK_HID(mid);
#if CG_BUILD_PARALLEL
    if (pcg_mpi_initialized) {
      xfer_prp = H5Pcreate(H5P_DATASET_XFER);
      ADFH_CHECK_HID(xfer_prp);
      H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
    }
#endif
    if (H5Dread(did, mid, H5S_ALL, H5S_ALL, xfer_prp, data) < 0)
      set_error(ADFH_ERR_DREAD, err);
    else
      set_error(NO_ERROR, err);

#if CG_BUILD_PARALLEL
    if (pcg_mpi_initialized) {
      H5Pclose(xfer_prp);
    }
#endif
    H5Tclose(mid);
    H5Dclose(did);
  }
  else
    set_error(NO_DATA, err);
  H5Gclose(hid);
}

/* ----------------------------------------------------------------- */

void ADFH_Write_Block_Data(const double ID,
                            const cgsize_t b_start,
                            const cgsize_t b_end,
                            char *data,
                            int *err )
{
  hid_t hid, did, mid, tid, dspace;
  size_t size, count, offset;
  char *buff;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Write_Block_Data"));

  if (data == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  if (b_start > b_end) {
    set_error(MINIMUM_GT_MAXIMUM, err);
    return;
  }
  if (b_start < 1) {
    set_error(START_OUT_OF_DEFINED_RANGE, err);
    return;
  }

  to_HDF_ID(ID,hid);
  if (is_link(hid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  if (!has_data(hid)) {
    set_error(NO_DATA, err);
    return;
  }
  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0) {
    set_error(ADFH_ERR_DOPEN, err);
    return;
  }

  dspace = H5Dget_space(did);
  ADFH_CHECK_HID(dspace);
  count = (size_t)H5Sget_simple_extent_npoints(dspace);
  H5Sclose(dspace);

  if ((size_t)b_end > count) {
    H5Dclose(did);
    set_error(END_OUT_OF_DEFINED_RANGE, err);
    return;
  }

  /* instead of trying to compute data space extents from
   * b_start and b_end, just read all the data into a
   * 1-d array, copy the range we want and rewrite the data */

  ADFH_CHECK_HID(did);
  tid = H5Dget_type(did);
  ADFH_CHECK_HID(tid);
  mid = H5Tget_native_type(tid, H5T_DIR_ASCEND);
  ADFH_CHECK_HID(mid);
  size = H5Tget_size(mid);

  if ((buff = (char *) malloc (size * count)) == NULL) {
    H5Tclose(mid);
    H5Tclose(tid);
    H5Dclose(did);
    set_error(MEMORY_ALLOCATION_FAILED, err);
    return;
  }

#if CG_BUILD_PARALLEL
    hid_t fid = get_file_id(hid);
    hid_t fapl=H5Fget_access_plist(fid);
    if (H5Pget_driver(fapl) == H5FD_MPIO) {
      xfer_prp = H5Pcreate(H5P_DATASET_XFER);
      ADFH_CHECK_HID(xfer_prp);
      H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE);
    }
#endif

  if (H5Dread(did, mid, H5S_ALL, H5S_ALL, xfer_prp, buff) < 0)
    set_error(ADFH_ERR_DREAD, err);
  else {
    offset = size * (size_t)(b_start - 1);
    count = size * (size_t)(b_end - b_start + 1);
    memcpy(&buff[offset], data, count);
    if (H5Dwrite(did, mid, H5S_ALL, H5S_ALL, xfer_prp, buff) < 0)
      set_error(ADFH_ERR_DWRITE, err);
    else
      set_error(NO_ERROR, err);
  }

  free (buff);
#if CG_BUILD_PARALLEL
    if (H5Pget_driver(fapl) == H5FD_MPIO) {
      H5Pclose(xfer_prp);
    }
#endif
  H5Tclose(mid);
  H5Tclose(tid);
  H5Dclose(did);
}

/* ----------------------------------------------------------------- */

void ADFH_Write_Data(const double ID,
                     const cgsize_t s_start[],
                     const cgsize_t s_end[],
                     const cgsize_t s_stride[],
                     const char *m_data_type,
                     const int m_num_dims,
                     const cgsize_t m_dims[],
                     const cgsize_t m_start[],
                     const cgsize_t m_end[],
                     const cgsize_t m_stride[],
                     const char *data,
                     int *err )
{
  int n, ndim;
  hid_t hid, did, mid, tid, dspace, mspace;
  hsize_t dims[ADF_MAX_DIMENSIONS];
  hsize_t start[ADF_MAX_DIMENSIONS];
  hsize_t stride[ADF_MAX_DIMENSIONS];
  hsize_t count[ADF_MAX_DIMENSIONS];
  herr_t status;
  hid_t xfer_prp = H5P_DEFAULT;

  ADFH_DEBUG(("ADFH_Write_Data"));

  if (data == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  to_HDF_ID(ID,hid);
  if (is_link(hid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  if (!has_data(hid)) {
    set_error(NO_DATA, err);
    return;
  }
  if ((did = H5Dopen2(hid, D_DATA, H5P_DEFAULT)) < 0) {
    set_error(ADFH_ERR_DOPEN, err);
    return;
  }

  /* get data space extents */

  dspace = H5Dget_space(did);
  ADFH_CHECK_HID(dspace);
  ndim = H5Sget_simple_extent_ndims(dspace);
  H5Sget_simple_extent_dims(dspace, dims, NULL);

  /* create data hyperslab */

#ifdef ADFH_FORTRAN_INDEXING
  if (ndim > 1 && !swap_dimensions(hid)) {
      H5Sclose(dspace);
      H5Dclose(did);
      set_error(ADFH_ERR_NEED_TRANSPOSE, err);
      return;
  }
#endif
  for (n = 0; n < ndim; n++) {
    if (s_start[n] < 1)
      set_error(START_OUT_OF_DEFINED_RANGE, err);
#ifdef ADFH_FORTRAN_INDEXING
    else if ((hsize_t)s_end[n] > dims[ndim-1-n])
#else
    else if ((hsize_t)s_end[n] > dims[n])
#endif
      set_error(END_OUT_OF_DEFINED_RANGE, err);
    else if (s_start[n] > s_end[n])
      set_error(MINIMUM_GT_MAXIMUM, err);
    else if (s_stride[n] < 1 ||
      s_stride[n] > (s_end[n] - s_start[n] + 1))
      set_error(BAD_STRIDE_VALUE, err);
    else
      set_error(NO_ERROR, err);
    if (*err != NO_ERROR) {
      H5Sclose(dspace);
      H5Dclose(did);
      return;
    }
#ifdef ADFH_FORTRAN_INDEXING
    start[ndim-1-n] = s_start[n] - 1;
    stride[ndim-1-n] = s_stride[n];
    count[ndim-1-n] = (s_end[n] - s_start[n] + 1) / s_stride[n];
#else
    start[n] = s_start[n] - 1;
    stride[n] = s_stride[n];
    count[n] = (s_end[n] - s_start[n] + 1) / s_stride[n];
#endif
  }

  H5Sselect_hyperslab(dspace, H5S_SELECT_SET, start, stride, count, NULL);

  /* create memory hyperslab */

  for (n = 0; n < m_num_dims; n++) {
    if (m_start[n] < 1)
      set_error(START_OUT_OF_DEFINED_RANGE, err);
    else if (m_end[n] > m_dims[n])
      set_error(END_OUT_OF_DEFINED_RANGE, err);
    else if (m_start[n] > m_end[n])
      set_error(MINIMUM_GT_MAXIMUM, err);
    else if (m_stride[n] < 1 ||
      m_stride[n] > (m_end[n] - m_start[n] + 1))
      set_error(BAD_STRIDE_VALUE, err);
    else
      set_error(NO_ERROR, err);
    if (*err != NO_ERROR) {
      H5Sclose(dspace);
      H5Dclose(did);
      return;
    }
#ifdef ADFH_FORTRAN_INDEXING
    dims[m_num_dims-1-n] = m_dims[n];
    start[m_num_dims-1-n] = m_start[n] - 1;
    stride[m_num_dims-1-n] = m_stride[n];
    count[m_num_dims-1-n] = (m_end[n] - m_start[n] + 1) / m_stride[n];
#else
    dims[n] = m_dims[n];
    start[n] = m_start[n] - 1;
    stride[n] = m_stride[n];
    count[n] = (m_end[n] - m_start[n] + 1) / m_stride[n];
#endif
  }

  mspace = H5Screate_simple(m_num_dims, dims, NULL);
  ADFH_CHECK_HID(mspace);

  H5Sselect_hyperslab(mspace, H5S_SELECT_SET, start, stride, count, NULL);

  if (H5Sget_select_npoints(mspace) != H5Sget_select_npoints(dspace)) {
      H5Sclose(mspace);
      H5Sclose(dspace);
      H5Dclose(did);
      set_error(UNEQUAL_MEMORY_AND_DISK_DIMS, err);
      return;
  }

  /* write the data */

  ADFH_CHECK_HID(did);
  tid = H5Dget_type(did);
  ADFH_CHECK_HID(tid);
  if (m_data_type) {
    mid = to_HDF_data_type(m_data_type);
  }
  else {
    mid = H5Tget_native_type(tid, H5T_DIR_ASCEND);
  }
  ADFH_CHECK_HID(mid);

#if CG_BUILD_PARALLEL
  hid_t fid = get_file_id(hid);
  hid_t fapl=H5Fget_access_plist(fid);
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    xfer_prp = H5Pcreate(H5P_DATASET_XFER);
    ADFH_CHECK_HID(xfer_prp);
    H5Pset_dxpl_mpio(xfer_prp, default_pio_mode);
  }
#endif

  status = H5Dwrite(did, mid, mspace, dspace, xfer_prp, data);

#if CG_BUILD_PARALLEL
  if (H5Pget_driver(fapl) == H5FD_MPIO) {
    H5Pclose(xfer_prp);
  }
#endif
  H5Sclose(mspace);
  H5Sclose(dspace);
  H5Tclose(mid);
  H5Tclose(tid);
  H5Dclose(did);

  if (status < 0)
    set_error(ADFH_ERR_DWRITE, err);
  else
    set_error(NO_ERROR, err);
}

/* ----------------------------------------------------------------- */

void ADFH_Write_All_Data(const double  id,
                         const char   *m_data_type,
                         const char   *data,
                         int          *err)
{
  hid_t hid;
  hid_t did, tid, mid;
  hid_t xfer_prp = H5P_DEFAULT;

  to_HDF_ID(id, hid);

  ADFH_DEBUG(("ADFH_Write_All_Data"));

  if (data == NULL) {
    set_error(NULL_POINTER, err);
    return;
  }
  if (is_link(hid)) {
    set_error(ADFH_ERR_LINK_DATA, err);
    return;
  }
  if (has_data(hid)) {
    ADFH_CHECK_HID(hid);
    did = H5Dopen2(hid, D_DATA, H5P_DEFAULT);
    ADFH_CHECK_HID(did);
    tid = H5Dget_type(did);
    ADFH_CHECK_HID(tid);
    if (m_data_type) {
      mid = to_HDF_data_type(m_data_type);
    }
    else {
      mid = H5Tget_native_type(tid, H5T_DIR_ASCEND);
    }
    ADFH_CHECK_HID(mid);
#if CG_BUILD_PARALLEL
    if (pcg_mpi_initialized) {
      xfer_prp = H5Pcreate(H5P_DATASET_XFER);
      ADFH_CHECK_HID(xfer_prp);
      if (H5Pset_dxpl_mpio(xfer_prp, H5FD_MPIO_COLLECTIVE) < 0)
        set_error(ADFH_ERR_DWRITE, err);
    }
#endif

    if (H5Dwrite(did, mid, H5S_ALL, H5S_ALL, xfer_prp, data) < 0)
      set_error(ADFH_ERR_DWRITE, err);
    else
      set_error(NO_ERROR, err);
#if CG_BUILD_PARALLEL
    if (pcg_mpi_initialized) {
      H5Pclose(xfer_prp);
    }
#endif
    H5Tclose(mid);
    H5Tclose(tid);
    H5Dclose(did);
  }
  else
    set_error(NO_DATA, err);
}

