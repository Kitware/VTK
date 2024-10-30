#ifndef PyMPI_FALLBACK_H
#define PyMPI_FALLBACK_H

/* ---------------------------------------------------------------- */

#include <stdlib.h>
#ifndef PyMPI_MALLOC
  #define PyMPI_MALLOC malloc
#endif
#ifndef PyMPI_FREE
  #define PyMPI_FREE free
#endif

/* ---------------------------------------------------------------- */

/* Version Number */

#ifndef PyMPI_HAVE_MPI_VERSION
#if !defined(MPI_VERSION)
#define MPI_VERSION 1
#endif
#endif

#ifndef PyMPI_HAVE_MPI_SUBVERSION
#if !defined(MPI_SUBVERSION)
#define MPI_SUBVERSION 0
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Get_version
static int PyMPI_Get_version(int *version, int* subversion)
{
  if (!version)    return MPI_ERR_ARG;
  if (!subversion) return MPI_ERR_ARG;
  *version    = MPI_VERSION;
  *subversion = MPI_SUBVERSION;
  return MPI_SUCCESS;
}
#undef  MPI_Get_version
#define MPI_Get_version PyMPI_Get_version
#endif

#ifndef PyMPI_HAVE_MPI_Get_library_version
#define PyMPI_MAX_LIBRARY_VERSION_STRING 8
static int PyMPI_Get_library_version(char version[], int *rlen)
{
  if (!version) return MPI_ERR_ARG;
  if (!rlen)    return MPI_ERR_ARG;
  version[0] = 'M';
  version[1] = 'P';
  version[2] = 'I';
  version[3] = ' ';
  version[4] = '0' + (char) MPI_VERSION;
  version[5] = '.';
  version[6] = '0' + (char) MPI_SUBVERSION;
  version[7] = 0;
  *rlen = 7;
  return MPI_SUCCESS;
}
#undef  MPI_MAX_LIBRARY_VERSION_STRING
#define MPI_MAX_LIBRARY_VERSION_STRING \
        PyMPI_MAX_LIBRARY_VERSION_STRING
#undef  MPI_Get_library_version
#define MPI_Get_library_version \
        PyMPI_Get_library_version
#endif

/* ---------------------------------------------------------------- */

/* Threading Support */

#ifndef PyMPI_HAVE_MPI_Init_thread
static int PyMPI_Init_thread(int *argc, char ***argv,
                             int required, int *provided)
{
  int ierr = MPI_SUCCESS;
  if (!provided) return MPI_ERR_ARG;
  ierr = MPI_Init(argc, argv);
  if (ierr != MPI_SUCCESS) return ierr;
  (void)required;
  *provided = MPI_THREAD_SINGLE;
  return MPI_SUCCESS;
}
#undef  MPI_Init_thread
#define MPI_Init_thread PyMPI_Init_thread
#endif

#ifndef PyMPI_HAVE_MPI_Query_thread
static int PyMPI_Query_thread(int *provided)
{
  if (!provided) return MPI_ERR_ARG;
  *provided = MPI_THREAD_SINGLE;
  return MPI_SUCCESS;
}
#undef  MPI_Query_thread
#define MPI_Query_thread PyMPI_Query_thread
#endif

#ifndef PyMPI_HAVE_MPI_Is_thread_main
static int PyMPI_Is_thread_main(int *flag)
{
  if (!flag) return MPI_ERR_ARG;
  *flag = 1; /* XXX this is completely broken !! */
  return MPI_SUCCESS;
}
#undef  MPI_Is_thread_main
#define MPI_Is_thread_main PyMPI_Is_thread_main
#endif

/* ---------------------------------------------------------------- */

/* Status */

#ifndef PyMPI_HAVE_MPI_STATUS_IGNORE
static MPI_Status PyMPI_STATUS_IGNORE;
#undef  MPI_STATUS_IGNORE
#define MPI_STATUS_IGNORE ((MPI_Status*)(&PyMPI_STATUS_IGNORE))
#endif

#ifndef PyMPI_HAVE_MPI_STATUSES_IGNORE
#ifndef PyMPI_MPI_STATUSES_IGNORE_SIZE
#define PyMPI_MPI_STATUSES_IGNORE_SIZE 4096
#endif
static MPI_Status PyMPI_STATUSES_IGNORE[PyMPI_MPI_STATUSES_IGNORE_SIZE];
#undef  MPI_STATUSES_IGNORE
#define MPI_STATUSES_IGNORE ((MPI_Status*)(PyMPI_STATUSES_IGNORE))
#endif

#define PyMPI_Status_GET_ATTR(name, NAME) \
static int PyMPI_Status_get_##name(MPI_Status *s, int *i) \
{ if (s && i) { *i = s->MPI_##NAME; } return MPI_SUCCESS; }

#define PyMPI_Status_SET_ATTR(name, NAME) \
static int PyMPI_Status_set_##name(MPI_Status *s, int i) \
{ if (s) { s->MPI_##NAME = i; } return MPI_SUCCESS; }

#ifndef PyMPI_HAVE_MPI_Status_get_source
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_get_source MPIX_Status_get_source
#else
PyMPI_Status_GET_ATTR(source, SOURCE)
#endif
#undef  MPI_Status_get_source
#define MPI_Status_get_source PyMPI_Status_get_source
#endif

#ifndef PyMPI_HAVE_MPI_Status_set_source
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_set_source MPIX_Status_set_source
#else
PyMPI_Status_SET_ATTR(source, SOURCE)
#endif
#undef  MPI_Status_set_source
#define MPI_Status_set_source PyMPI_Status_set_source
#endif

#ifndef PyMPI_HAVE_MPI_Status_get_tag
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_get_tag MPIX_Status_get_tag
#else
PyMPI_Status_GET_ATTR(tag, TAG)
#endif
#undef  MPI_Status_get_tag
#define MPI_Status_get_tag PyMPI_Status_get_tag
#endif

#ifndef PyMPI_HAVE_MPI_Status_set_tag
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_set_tag MPIX_Status_set_tag
#else
PyMPI_Status_SET_ATTR(tag, TAG)
#endif
#undef  MPI_Status_set_tag
#define MPI_Status_set_tag PyMPI_Status_set_tag
#endif

#ifndef PyMPI_HAVE_MPI_Status_get_error
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_get_error MPIX_Status_get_error
#else
PyMPI_Status_GET_ATTR(error, ERROR)
#endif
#undef  MPI_Status_get_error
#define MPI_Status_get_error PyMPI_Status_get_error
#endif

#ifndef PyMPI_HAVE_MPI_Status_set_error
#if defined(MPIX_HAVE_MPI_STATUS_GETSET)
#define PyMPI_Status_set_error MPIX_Status_set_error
#else
PyMPI_Status_SET_ATTR(error, ERROR)
#endif
#undef  MPI_Status_set_error
#define MPI_Status_set_error PyMPI_Status_set_error
#endif

#ifdef PyMPI_Status_GET_ATTR
#undef PyMPI_Status_GET_ATTR
#endif

#ifdef PyMPI_Status_SET_ATTR
#undef PyMPI_Status_SET_ATTR
#endif

/* ---------------------------------------------------------------- */

/* Datatypes */

#ifndef PyMPI_HAVE_MPI_LONG_LONG
#undef  MPI_LONG_LONG
#define MPI_LONG_LONG MPI_LONG_LONG_INT
#endif

#ifndef PyMPI_HAVE_MPI_Type_get_extent
static int PyMPI_Type_get_extent(MPI_Datatype datatype,
                                 MPI_Aint *lb, MPI_Aint *extent)
{
  int ierr = MPI_SUCCESS;
  ierr = MPI_Type_lb(datatype, lb);
  if (ierr != MPI_SUCCESS) return ierr;
  ierr = MPI_Type_extent(datatype, extent);
  if (ierr != MPI_SUCCESS) return ierr;
  return MPI_SUCCESS;
}
#undef  MPI_Type_get_extent
#define MPI_Type_get_extent PyMPI_Type_get_extent
#endif

#ifndef PyMPI_HAVE_MPI_Type_dup
static int PyMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype)
{
  int ierr = MPI_SUCCESS;
  ierr = MPI_Type_contiguous(1, datatype, newtype);
  if (ierr != MPI_SUCCESS) return ierr;
  ierr = MPI_Type_commit(newtype); /* the safe way  ... */
  if (ierr != MPI_SUCCESS) return ierr;
  return MPI_SUCCESS;
}
#undef  MPI_Type_dup
#define MPI_Type_dup PyMPI_Type_dup
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_indexed_block
static int PyMPI_Type_create_indexed_block(int count,
                                           int blocklength,
                                           int displacements[],
                                           MPI_Datatype oldtype,
                                           MPI_Datatype *newtype)
{
  int i, *blocklengths = NULL, ierr = MPI_SUCCESS;
  if (count > 0) {
    blocklengths = (int *) PyMPI_MALLOC((size_t)count*sizeof(int));
    if (!blocklengths) return MPI_ERR_INTERN;
  }
  for (i=0; i<count; i++) blocklengths[i] = blocklength;
  ierr = MPI_Type_indexed(count,blocklengths,displacements,oldtype,newtype);
  if (blocklengths) PyMPI_FREE(blocklengths);
  return ierr;
}
#undef  MPI_Type_create_indexed_block
#define MPI_Type_create_indexed_block PyMPI_Type_create_indexed_block
#undef  MPI_COMBINER_INDEXED_BLOCK
#define MPI_COMBINER_INDEXED_BLOCK MPI_COMBINER_INDEXED
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_hindexed_block
static int PyMPI_Type_create_hindexed_block(int count,
                                            int blocklength,
                                            MPI_Aint displacements[],
                                            MPI_Datatype oldtype,
                                            MPI_Datatype *newtype)
{
  int i, *blocklengths = NULL, ierr = MPI_SUCCESS;
  if (count > 0) {
    blocklengths = (int *) PyMPI_MALLOC((size_t)count*sizeof(int));
    if (!blocklengths) return MPI_ERR_INTERN;
  }
  for (i=0; i<count; i++) blocklengths[i] = blocklength;
  ierr = MPI_Type_create_hindexed(count,blocklengths,displacements,oldtype,newtype);
  if (blocklengths) PyMPI_FREE(blocklengths);
  return ierr;
}
#undef  MPI_Type_create_hindexed_block
#define MPI_Type_create_hindexed_block PyMPI_Type_create_hindexed_block
#undef  MPI_COMBINER_HINDEXED_BLOCK
#define MPI_COMBINER_HINDEXED_BLOCK MPI_COMBINER_HINDEXED
#endif

/*
 * Adapted from the implementation in MPICH2 sources,
 * mpich2-1.0.7/src/mpi/datatype/type_create_subarray.c
 *
 */
#ifndef PyMPI_HAVE_MPI_Type_create_subarray

#undef  PyMPI_CHKARG
#define PyMPI_CHKARG(expr) if (!(expr)) return MPI_ERR_ARG

static int PyMPI_Type_create_subarray(int ndims,
                                      int sizes[],
                                      int subsizes[],
                                      int starts[],
                                      int order,
                                      MPI_Datatype oldtype,
                                      MPI_Datatype *newtype)
{
  int i = 0;
  MPI_Aint size = 0, extent = 0, disps[3];
  int blklens[3];
  MPI_Datatype tmp1, tmp2, types[3];
  int ierr = MPI_SUCCESS;
  tmp1 = tmp2 = types[0] = types[1] = types[2] = MPI_DATATYPE_NULL;

  PyMPI_CHKARG(ndims > 0);
  PyMPI_CHKARG(sizes);
  PyMPI_CHKARG(subsizes);
  PyMPI_CHKARG(starts);
  PyMPI_CHKARG(newtype);
  for (i=0; i<ndims; i++) {
    PyMPI_CHKARG(sizes[i] >  0);
    PyMPI_CHKARG(subsizes[i] >  0);
    PyMPI_CHKARG(starts[i] >= 0);
    PyMPI_CHKARG(sizes[i] >= subsizes[i]);
    PyMPI_CHKARG(starts[i] <= (sizes[i] - subsizes[i]));
  }
  PyMPI_CHKARG((order==MPI_ORDER_C) || (order==MPI_ORDER_FORTRAN));

  ierr = MPI_Type_extent(oldtype, &extent);
  if (ierr != MPI_SUCCESS) return ierr;

  if (order == MPI_ORDER_FORTRAN) {
    if (ndims == 1) {
      ierr = MPI_Type_contiguous(subsizes[0], oldtype, &tmp1);
      if (ierr != MPI_SUCCESS) return ierr;
    } else {
      ierr = MPI_Type_vector(subsizes[1], subsizes[0],
                             sizes[0], oldtype, &tmp1);
      if (ierr != MPI_SUCCESS) return ierr;
      size = sizes[0]*extent;
      for (i=2; i<ndims; i++) {
        size *= sizes[i-1];
        ierr = MPI_Type_hvector(subsizes[i], 1, size, tmp1, &tmp2);
        if (ierr != MPI_SUCCESS) return ierr;
        ierr = MPI_Type_free(&tmp1);
        if (ierr != MPI_SUCCESS) return ierr;
        tmp1 = tmp2;
      }
    }
    /* add displacement and upper bound */
    disps[1] = starts[0];
    size = 1;
    for (i=1; i<ndims; i++) {
      size *= sizes[i-1];
      disps[1] += size*starts[i];
    }
  } else /* MPI_ORDER_C */ {
    /* dimension ndims-1 changes fastest */
    if (ndims == 1) {
      ierr = MPI_Type_contiguous(subsizes[0], oldtype, &tmp1);
      if (ierr != MPI_SUCCESS) return ierr;
    } else {
      ierr = MPI_Type_vector(subsizes[ndims-2], subsizes[ndims-1],
                             sizes[ndims-1], oldtype, &tmp1);
      if (ierr != MPI_SUCCESS) return ierr;
      size = sizes[ndims-1]*extent;
      for (i=ndims-3; i>=0; i--) {
        size *= sizes[i+1];
        ierr = MPI_Type_hvector(subsizes[i], 1, size, tmp1, &tmp2);
        if (ierr != MPI_SUCCESS) return ierr;
        ierr = MPI_Type_free(&tmp1);
        if (ierr != MPI_SUCCESS) return ierr;
        tmp1 = tmp2;
      }
    }
    /* add displacement and upper bound */
    disps[1] = starts[ndims-1];
    size = 1;
    for (i=ndims-2; i>=0; i--) {
      size *= sizes[i+1];
      disps[1] += size*starts[i];
    }
  }

  disps[1] *= extent;
  disps[2] = extent;
  for (i=0; i<ndims; i++) disps[2] *= sizes[i];
  disps[0] = 0;
  blklens[0] = blklens[1] = blklens[2] = 1;
  types[0] = MPI_LB;
  types[1] = tmp1;
  types[2] = MPI_UB;

  ierr = MPI_Type_struct(3, blklens, disps, types, newtype);
  if (ierr != MPI_SUCCESS) return ierr;
  ierr = MPI_Type_free(&tmp1);
  if (ierr != MPI_SUCCESS) return ierr;

  return MPI_SUCCESS;
}

#undef PyMPI_CHKARG

#undef  MPI_Type_create_subarray
#define MPI_Type_create_subarray PyMPI_Type_create_subarray
#endif

/*
 * Adapted from the implementation in MPICH2 sources,
 * mpich2-1.0.7/src/mpi/datatype/type_create_darray.c
 *
 */
#ifndef PyMPI_HAVE_MPI_Type_create_darray

#undef  PyMPI_CHKARG
#define PyMPI_CHKARG(expr) if (!(expr)) return MPI_ERR_ARG

static int PyMPI_Type_block(int *gsizes,
                            int dim,
                            int ndims,
                            int nprocs,
                            int rank,
                            int darg,
                            int order,
                            MPI_Aint orig_extent,
                            MPI_Datatype type_old,
                            MPI_Datatype *type_new,
                            MPI_Aint *offset)
{
  int ierr, blksize, global_size, mysize, i, j;
  MPI_Aint stride;

  global_size = gsizes[dim];

  if (darg == MPI_DISTRIBUTE_DFLT_DARG)
    blksize = (global_size + nprocs - 1)/nprocs;
  else {
    blksize = darg;
    PyMPI_CHKARG(blksize > 0);
    PyMPI_CHKARG(blksize * nprocs >= global_size);
  }

  j = global_size - blksize*rank;
  mysize = (blksize < j) ? blksize : j;
  if (mysize < 0) mysize = 0;
  stride = orig_extent;
  if (order == MPI_ORDER_FORTRAN) {
    if (dim == 0) {
      ierr = MPI_Type_contiguous(mysize, type_old, type_new);
      if (ierr != MPI_SUCCESS) goto fn_exit;
    } else {
      for (i=0; i<dim; i++) stride *= gsizes[i];
      ierr = MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
      if (ierr != MPI_SUCCESS) goto fn_exit;
    }
  } else { /* order == MPI_ORDER_C */
    if (dim == ndims-1) {
      ierr = MPI_Type_contiguous(mysize, type_old, type_new);
      if (ierr != MPI_SUCCESS) goto fn_exit;
    } else {
      for (i=ndims-1; i>dim; i--) stride *= gsizes[i];
      ierr = MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
      if (ierr != MPI_SUCCESS) goto fn_exit;
    }
  }

  *offset = blksize * rank;
  if (mysize == 0) *offset = 0;

  ierr = MPI_SUCCESS;
 fn_exit:
  return ierr;
}
static int PyMPI_Type_cyclic(int *gsizes,
                             int dim,
                             int ndims,
                             int nprocs,
                             int rank,
                             int darg,
                             int order,
                             MPI_Aint orig_extent,
                             MPI_Datatype type_old,
                             MPI_Datatype *type_new,
                             MPI_Aint *offset)
{
  int ierr, blksize, i, blklens[3], st_index, end_index,
    local_size, rem, count;
  MPI_Aint stride, disps[3];
  MPI_Datatype type_tmp, types[3];

  type_tmp = MPI_DATATYPE_NULL;
  types[0] = types[1] = types[2] = MPI_DATATYPE_NULL;

  if (darg == MPI_DISTRIBUTE_DFLT_DARG)
    blksize = 1;
  else
    blksize = darg;
  PyMPI_CHKARG(blksize > 0);

  st_index = rank*blksize;
  end_index = gsizes[dim] - 1;

  if (end_index < st_index)
    local_size = 0;
  else {
    local_size = ((end_index - st_index + 1)/(nprocs*blksize))*blksize;
    rem = (end_index - st_index + 1) % (nprocs*blksize);
    local_size += (rem < blksize) ? rem : blksize;
  }

  count = local_size/blksize;
  rem = local_size % blksize;

  stride = nprocs*blksize*orig_extent;
  if (order == MPI_ORDER_FORTRAN)
    for (i=0; i<dim; i++) stride *= gsizes[i];
  else
    for (i=ndims-1; i>dim; i--) stride *= gsizes[i];

  ierr = MPI_Type_hvector(count, blksize, stride, type_old, type_new);
  if (ierr != MPI_SUCCESS) goto fn_exit;
  /* if the last block is of size less than blksize,
     include it separately using MPI_Type_struct */
  if (rem) {
    types[0] = *type_new;
    types[1] = type_old;
    disps[0] = 0;
    disps[1] = count*stride;
    blklens[0] = 1;
    blklens[1] = rem;
    ierr = MPI_Type_struct(2, blklens, disps, types, &type_tmp);
    if (ierr != MPI_SUCCESS) goto fn_exit;
    ierr = MPI_Type_free(type_new);
    if (ierr != MPI_SUCCESS) goto fn_exit;
    *type_new = type_tmp;
  }
  /* In the first iteration, we need to set the
     displacement in that dimension correctly. */
  if ( ((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
       ((order == MPI_ORDER_C) && (dim == ndims-1)) )
    {
      types[0] = MPI_LB;
      disps[0] = 0;
      types[1] = *type_new;
      disps[1] = rank * blksize * orig_extent;
      types[2] = MPI_UB;
      disps[2] = orig_extent * gsizes[dim];
      blklens[0] = blklens[1] = blklens[2] = 1;
      ierr = MPI_Type_struct(3, blklens, disps, types, &type_tmp);
      if (ierr != MPI_SUCCESS) goto fn_exit;
      ierr = MPI_Type_free(type_new);
      if (ierr != MPI_SUCCESS) goto fn_exit;
      *type_new = type_tmp;
      *offset = 0;
    } else {
    *offset = rank * blksize;
  }

  if (local_size == 0) *offset = 0;

  ierr = MPI_SUCCESS;
 fn_exit:
  return ierr;
}
static int PyMPI_Type_create_darray(int size,
                                    int rank,
                                    int ndims,
                                    int gsizes[],
                                    int distribs[],
                                    int dargs[],
                                    int psizes[],
                                    int order,
                                    MPI_Datatype oldtype,
                                    MPI_Datatype *newtype)
{
  int ierr = MPI_SUCCESS, i;
  int procs, tmp_rank, tmp_size, blklens[3];
  MPI_Aint orig_extent, disps[3];
  MPI_Datatype type_old, type_new, types[3];

  int      *coords  = NULL;
  MPI_Aint *offsets = NULL;

  orig_extent=0;
  type_old = type_new = MPI_DATATYPE_NULL;
  types[0] = types[1] = types[2] = MPI_DATATYPE_NULL;

  ierr = MPI_Type_extent(oldtype, &orig_extent);
  if (ierr != MPI_SUCCESS) goto fn_exit;

  PyMPI_CHKARG(rank >= 0);
  PyMPI_CHKARG(size > 0);
  PyMPI_CHKARG(ndims > 0);
  PyMPI_CHKARG(gsizes);
  PyMPI_CHKARG(distribs);
  PyMPI_CHKARG(dargs);
  PyMPI_CHKARG(psizes);
  PyMPI_CHKARG((order==MPI_ORDER_C) ||
               (order==MPI_ORDER_FORTRAN) );
  for (i=0; i < ndims; i++) {
    PyMPI_CHKARG(gsizes[1] > 0);
    PyMPI_CHKARG(psizes[1] > 0);
    PyMPI_CHKARG((distribs[i] == MPI_DISTRIBUTE_NONE)  ||
                 (distribs[i] == MPI_DISTRIBUTE_BLOCK) ||
                 (distribs[i] == MPI_DISTRIBUTE_CYCLIC));
    PyMPI_CHKARG((dargs[i] == MPI_DISTRIBUTE_DFLT_DARG) ||
                 (dargs[i] > 0));
    PyMPI_CHKARG(!((distribs[i] == MPI_DISTRIBUTE_NONE) &&
                   (psizes[i] != 1)));
  }

  /* calculate position in Cartesian grid
     as MPI would (row-major ordering) */
  coords  = (int *) PyMPI_MALLOC((size_t)ndims*sizeof(int));
  if (!coords)  { ierr = MPI_ERR_INTERN; goto fn_exit; }
  offsets = (MPI_Aint *) PyMPI_MALLOC((size_t)ndims*sizeof(MPI_Aint));
  if (!offsets) { ierr = MPI_ERR_INTERN; goto fn_exit; }

  procs = size;
  tmp_rank = rank;
  for (i=0; i<ndims; i++) {
    procs = procs/psizes[i];
    coords[i] = tmp_rank/procs;
    tmp_rank = tmp_rank % procs;
  }

  type_old = oldtype;

  if (order == MPI_ORDER_FORTRAN) {
    /* dimension 0 changes fastest */
    for (i=0; i<ndims; i++) {
      if (distribs[i] == MPI_DISTRIBUTE_BLOCK) {
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                psizes[i], coords[i], dargs[i],
                                order, orig_extent,
                                type_old,  &type_new,
                                offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      } else if (distribs[i] == MPI_DISTRIBUTE_CYCLIC) {
        ierr = PyMPI_Type_cyclic(gsizes, i, ndims,
                                 psizes[i], coords[i], dargs[i],
                                 order, orig_extent,
                                 type_old, &type_new,
                                 offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      } else if (distribs[i] == MPI_DISTRIBUTE_NONE) {
        /* treat it as a block distribution on 1 process */
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                1, 0, MPI_DISTRIBUTE_DFLT_DARG,
                                order, orig_extent,
                                type_old, &type_new,
                                offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      }
      if (i != 0) {
        ierr = MPI_Type_free(&type_old);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      }
      type_old = type_new;
    }
    /* add displacement and upper bound */
    disps[1] = offsets[0];
    tmp_size = 1;
    for (i=1; i<ndims; i++) {
      tmp_size *= gsizes[i-1];
      disps[1] += tmp_size*offsets[i];
    }
    /* rest done below for both Fortran and C order */
  } else /* order == MPI_ORDER_C */ {
    /* dimension ndims-1 changes fastest */
    for (i=ndims-1; i>=0; i--) {
      if (distribs[i] == MPI_DISTRIBUTE_BLOCK) {
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                psizes[i], coords[i], dargs[i],
                                order, orig_extent,
                                type_old, &type_new,
                                offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      } else if (distribs[i] == MPI_DISTRIBUTE_CYCLIC) {
        ierr = PyMPI_Type_cyclic(gsizes, i, ndims,
                                 psizes[i], coords[i], dargs[i],
                                 order,  orig_extent,
                                 type_old, &type_new,
                                 offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      } else if (distribs[i] == MPI_DISTRIBUTE_NONE) {
        /* treat it as a block distribution on 1 process */
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                psizes[i], coords[i],
                                MPI_DISTRIBUTE_DFLT_DARG,
                                order, orig_extent,
                                type_old, &type_new,
                                offsets+i);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      }
      if (i != ndims-1) {
        ierr = MPI_Type_free(&type_old);
        if (ierr != MPI_SUCCESS) goto fn_exit;
      }
      type_old = type_new;
    }
    /* add displacement and upper bound */
    disps[1] = offsets[ndims-1];
    tmp_size = 1;
    for (i=ndims-2; i>=0; i--) {
      tmp_size *= gsizes[i+1];
      disps[1] += tmp_size*offsets[i];
    }
    /* rest done below for both Fortran and C order */
  }

  disps[0] = 0;
  disps[1] *= orig_extent;
  disps[2] = orig_extent;
  for (i=0; i<ndims; i++) disps[2] *= gsizes[i];
  blklens[0] = blklens[1] = blklens[2] = 1;
  types[0] = MPI_LB;
  types[1] = type_new;
  types[2] = MPI_UB;
  ierr = MPI_Type_struct(3, blklens, disps, types, newtype);
  if (ierr != MPI_SUCCESS) goto fn_exit;
  ierr = MPI_Type_free(&type_new);
  if (ierr != MPI_SUCCESS) goto fn_exit;

  ierr = MPI_SUCCESS;
 fn_exit:
  if (coords)  PyMPI_FREE(coords);
  if (offsets) PyMPI_FREE(offsets);
  return ierr;
}

#undef PyMPI_CHKARG

#undef  MPI_Type_create_darray
#define MPI_Type_create_darray PyMPI_Type_create_darray
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_resized
int PyMPI_Type_create_resized(MPI_Datatype oldtype,
                              MPI_Aint lb,
                              MPI_Aint extent,
                              MPI_Datatype *newtype)
{
  int blens[3];
  MPI_Aint disps[3];
  MPI_Datatype types[3];
  MPI_Aint ub = extent - lb;
  blens[0] = 1; disps[0] = lb; types[0] = MPI_LB;
  blens[1] = 1; disps[1] = 0;  types[1] = oldtype;
  blens[2] = 1; disps[2] = ub; types[2] = MPI_UB;
  return MPI_Type_struct(3, blens, disps, types, newtype);
}
#undef  MPI_Type_create_resized
#define MPI_Type_create_resized PyMPI_Type_create_resized
#endif


#ifndef PyMPI_HAVE_MPI_Type_size_x
static int PyMPI_Type_size_x(MPI_Datatype datatype,
                             MPI_Count *size)
{
  int ierr = MPI_SUCCESS;
  int size_ = MPI_UNDEFINED;
  ierr = MPI_Type_size(datatype, &size_);
  if (ierr != MPI_SUCCESS) return ierr;
  *size = (MPI_Count) size_;
  return MPI_SUCCESS;
}
#undef  MPI_Type_size_x
#define MPI_Type_size_x PyMPI_Type_size_x
#endif

#ifndef PyMPI_HAVE_MPI_Type_get_extent_x
static int PyMPI_Type_get_extent_x(MPI_Datatype datatype,
                                   MPI_Count *lb,
                                   MPI_Count *extent)
{
  int ierr = MPI_SUCCESS;
  MPI_Aint lb_ = MPI_UNDEFINED, extent_ = MPI_UNDEFINED;
  ierr = MPI_Type_get_extent(datatype, &lb_, &extent_);
  if (ierr != MPI_SUCCESS) return ierr;
  *lb     = (MPI_Count) lb_;
  *extent = (MPI_Count) extent_;
  return MPI_SUCCESS;
}
#undef  MPI_Type_get_extent_x
#define MPI_Type_get_extent_x PyMPI_Type_get_extent_x
#endif

#ifndef PyMPI_HAVE_MPI_Type_get_true_extent_x
static int PyMPI_Type_get_true_extent_x(MPI_Datatype datatype,
                                        MPI_Count *lb,
                                        MPI_Count *extent)
{
  int ierr = MPI_SUCCESS;
  MPI_Aint lb_ = MPI_UNDEFINED, extent_ = MPI_UNDEFINED;
  ierr = MPI_Type_get_true_extent(datatype, &lb_, &extent_);
  if (ierr != MPI_SUCCESS) return ierr;
  *lb     = (MPI_Count) lb_;
  *extent = (MPI_Count) extent_;
  return MPI_SUCCESS;
}
#undef  MPI_Type_get_true_extent_x
#define MPI_Type_get_true_extent_x PyMPI_Type_get_true_extent_x
#endif

#ifndef PyMPI_HAVE_MPI_Get_elements_x
static int PyMPI_Get_elements_x(MPI_Status *status,
                                MPI_Datatype datatype,
                                MPI_Count *elements)
{
  int ierr = MPI_SUCCESS;
  int elements_ = MPI_UNDEFINED;
  ierr = MPI_Get_elements(status, datatype, &elements_);
  if (ierr != MPI_SUCCESS) return ierr;
  *elements = (MPI_Count) elements_;
  return MPI_SUCCESS;
}
#undef  MPI_Get_elements_x
#define MPI_Get_elements_x PyMPI_Get_elements_x
#endif

#ifndef PyMPI_HAVE_MPI_Status_set_elements_x
static int PyMPI_Status_set_elements_x(MPI_Status *status,
                                       MPI_Datatype datatype,
                                       MPI_Count elements)
{
  int elements_ = (int) elements;
  if (elements != (MPI_Count) elements_) return MPI_ERR_ARG; /* XXX */
  return MPI_Status_set_elements(status, datatype, elements_);
}
#undef  MPI_Status_set_elements_x
#define MPI_Status_set_elements_x PyMPI_Status_set_elements_x
#endif

#ifndef PyMPI_HAVE_MPI_Aint_add
static MPI_Aint PyMPI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
  return (MPI_Aint) ((char*)base + disp);
}
#undef  MPI_Aint_add
#define MPI_Aint_add PyMPI_Aint_add
#endif

#ifndef PyMPI_HAVE_MPI_Aint_diff
static MPI_Aint PyMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
  return (MPI_Aint) ((char*)addr1 - (char*)addr2);
}
#undef  MPI_Aint_diff
#define MPI_Aint_diff PyMPI_Aint_diff
#endif

#ifndef PyMPI_HAVE_MPI_Type_get_value_index
static int PyMPI_Type_get_value_index(MPI_Datatype value,
                                      MPI_Datatype index,
                                      MPI_Datatype *pair)
{
  if (!pair) return MPI_ERR_ARG;
  /**/ if (index != MPI_INT)          *pair = MPI_DATATYPE_NULL;
  else if (value == MPI_FLOAT)        *pair = MPI_FLOAT_INT;
  else if (value == MPI_DOUBLE)       *pair = MPI_DOUBLE_INT;
  else if (value == MPI_LONG_DOUBLE)  *pair = MPI_LONG_DOUBLE_INT;
  else if (value == MPI_LONG)         *pair = MPI_LONG_INT;
  else if (value == MPI_INT)          *pair = MPI_2INT;
  else if (value == MPI_SHORT)        *pair = MPI_SHORT_INT;
  else                                *pair = MPI_DATATYPE_NULL;
  return MPI_SUCCESS;
}
#undef  MPI_Type_get_value_index
#define MPI_Type_get_value_index PyMPI_Type_get_value_index
#endif

/* ---------------------------------------------------------------- */

#ifdef PyMPI_HAVE_MPI_Request_get_status
#if ((10 * MPI_VERSION + MPI_SUBVERSION) < 22)
static int PyMPI_Request_get_status(MPI_Request request,
                                    int *flag, MPI_Status *status)
{
  if (request != MPI_REQUEST_NULL || !flag)
    return MPI_Request_get_status(request, flag, status);
  *flag = 1;
  if (status &&
      status != MPI_STATUS_IGNORE &&
      status != MPI_STATUSES_IGNORE) {
    #if !defined(PyMPI_HAVE_MPI_Status_set_cancelled) || \
        !defined(PyMPI_HAVE_MPI_Status_set_elements)
    int n = (int) sizeof(MPI_Status);
    unsigned char *p = (unsigned char *)status;
    while (n-- > 0) p[n] = 0;
    #endif
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG    = MPI_ANY_TAG;
    status->MPI_ERROR  = MPI_SUCCESS;
    #ifdef PyMPI_HAVE_MPI_Status_set_elements
    (void)MPI_Status_set_elements(status, MPI_BYTE, 0);
    #endif
    #ifdef PyMPI_HAVE_MPI_Status_set_cancelled
    (void)MPI_Status_set_cancelled(status, 0);
    #endif
  }
  return MPI_SUCCESS;
}
#undef  MPI_Request_get_status
#define MPI_Request_get_status PyMPI_Request_get_status
#endif
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Reduce_scatter_block
static int PyMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf,
                                      int recvcount, MPI_Datatype datatype,
                                      MPI_Op op, MPI_Comm comm)
{
  int ierr = MPI_SUCCESS;
  int n = 1, *recvcounts = NULL;
  ierr = MPI_Comm_size(comm, &n);
  if (ierr != MPI_SUCCESS) return ierr;
  recvcounts = (int *) PyMPI_MALLOC((size_t)n*sizeof(int));
  if (!recvcounts) return MPI_ERR_INTERN;
  while (n-- > 0) recvcounts[n] = recvcount;
  ierr = MPI_Reduce_scatter(sendbuf, recvbuf,
                            recvcounts, datatype,
                            op, comm);
  PyMPI_FREE(recvcounts);
  return ierr;
}
#undef  MPI_Reduce_scatter_block
#define MPI_Reduce_scatter_block PyMPI_Reduce_scatter_block
#endif

/* ---------------------------------------------------------------- */

/* Communicator Info */

#ifndef PyMPI_HAVE_MPI_Comm_dup_with_info
static int PyMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info,
                                    MPI_Comm *newcomm)
{
  int dummy, ierr;
  if (info != MPI_INFO_NULL) {
    ierr = MPI_Info_get_nkeys(info, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  return MPI_Comm_dup(comm, newcomm);
}
#undef  MPI_Comm_dup_with_info
#define MPI_Comm_dup_with_info PyMPI_Comm_dup_with_info
#endif

#ifndef PyMPI_HAVE_MPI_Comm_idup_with_info
static int PyMPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info,
                                     MPI_Comm *newcomm, MPI_Request *request)
{
  int dummy, ierr;
  if (info != MPI_INFO_NULL) {
    ierr = MPI_Info_get_nkeys(info, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  return MPI_Comm_idup(comm, newcomm, request);
}
#undef  MPI_Comm_idup_with_info
#define MPI_Comm_idup_with_info PyMPI_Comm_idup_with_info
#endif

#ifndef PyMPI_HAVE_MPI_Comm_set_info
static int PyMPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
  int dummy, ierr;
  ierr = MPI_Comm_size(comm, &dummy);
  if (ierr != MPI_SUCCESS) return ierr;
  if (info != MPI_INFO_NULL) {
    ierr = MPI_Info_get_nkeys(info, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  return MPI_SUCCESS;
}
#undef  MPI_Comm_set_info
#define MPI_Comm_set_info PyMPI_Comm_set_info
#endif

#ifndef PyMPI_HAVE_MPI_Comm_get_info
static int PyMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info)
{
  int dummy, ierr;
  ierr = MPI_Comm_size(comm, &dummy);
  if (ierr != MPI_SUCCESS) return ierr;
  return MPI_Info_create(info);
}
#undef  MPI_Comm_get_info
#define MPI_Comm_get_info PyMPI_Comm_get_info
#endif

/* ---------------------------------------------------------------- */

#if !defined(PyMPI_HAVE_MPI_WEIGHTS_EMPTY)
static const int PyMPI_WEIGHTS_EMPTY_ARRAY[1] = {MPI_UNDEFINED};
static int * const PyMPI_WEIGHTS_EMPTY = (int*)PyMPI_WEIGHTS_EMPTY_ARRAY;
#undef  MPI_WEIGHTS_EMPTY
#define MPI_WEIGHTS_EMPTY PyMPI_WEIGHTS_EMPTY
#endif

/* ---------------------------------------------------------------- */

/* Memory Allocation */

#if !defined(PyMPI_HAVE_MPI_Alloc_mem) || \
    !defined(PyMPI_HAVE_MPI_Free_mem)

static int PyMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  char *buf = NULL;
  if (size < 0) return MPI_ERR_ARG;
  if (!baseptr) return MPI_ERR_ARG;
  if (size == 0) size = 1;
  buf = (char *) PyMPI_MALLOC((size_t)size);
  if (!buf) return MPI_ERR_NO_MEM;
  (void)info;
  *(char **)baseptr = buf;
  return MPI_SUCCESS;
}
#undef  MPI_Alloc_mem
#define MPI_Alloc_mem PyMPI_Alloc_mem

static int PyMPI_Free_mem(void *baseptr)
{
  if (!baseptr) return MPI_ERR_ARG;
  PyMPI_FREE(baseptr);
  return MPI_SUCCESS;
}
#undef  MPI_Free_mem
#define MPI_Free_mem PyMPI_Free_mem

#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Win_allocate
#ifdef  PyMPI_HAVE_MPI_Win_create

static int PyMPI_WIN_KEYVAL_MPIMEM = MPI_KEYVAL_INVALID;

static int MPIAPI
PyMPI_win_free_mpimem(MPI_Win win, int k, void *v, void *xs)
{
  (void)win; (void)k; (void)xs; /* unused */
  return MPI_Free_mem(v);
}

static int MPIAPI
PyMPI_win_free_keyval(MPI_Comm comm, int k, void *v, void *xs)
{
  int ierr = MPI_SUCCESS;
  (void)comm; (void)xs; /* unused */
  ierr = MPI_Win_free_keyval((int *)v);
  if (ierr != MPI_SUCCESS) return ierr;
  ierr = MPI_Comm_free_keyval(&k);
  if (ierr != MPI_SUCCESS) return ierr;
  return MPI_SUCCESS;
}

static int PyMPI_Win_allocate(MPI_Aint size, int disp_unit,
                              MPI_Info info, MPI_Comm comm,
                              void *baseptr_, MPI_Win *win_)
{
  int ierr = MPI_SUCCESS;
  void *baseptr = MPI_BOTTOM;
  MPI_Win win = MPI_WIN_NULL;
  if (!baseptr_) return MPI_ERR_ARG;
  if (!win_)     return MPI_ERR_ARG;
  ierr = MPI_Alloc_mem(size?size:1, info, &baseptr);
  if (ierr != MPI_SUCCESS) goto error;
  ierr = MPI_Win_create(baseptr, size, disp_unit, info, comm, &win);
  if (ierr != MPI_SUCCESS) goto error;
#if defined(PyMPI_HAVE_MPI_Win_create_keyval) && \
    defined(PyMPI_HAVE_MPI_Win_set_attr)
  if (PyMPI_WIN_KEYVAL_MPIMEM == MPI_KEYVAL_INVALID) {
    int comm_keyval = MPI_KEYVAL_INVALID;
    ierr = MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN,
                                 PyMPI_win_free_mpimem,
                                 &PyMPI_WIN_KEYVAL_MPIMEM, NULL);
    if (ierr != MPI_SUCCESS) goto error;
    ierr = MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN,
                                  PyMPI_win_free_keyval,
                                  &comm_keyval, NULL);
    if (ierr == MPI_SUCCESS)
      (void)MPI_Comm_set_attr(MPI_COMM_SELF, comm_keyval,
                              &PyMPI_WIN_KEYVAL_MPIMEM);
  }
  ierr = MPI_Win_set_attr(win, PyMPI_WIN_KEYVAL_MPIMEM, baseptr);
  if (ierr != MPI_SUCCESS) goto error;
#endif
  *((void**)baseptr_) = baseptr;
  *win_ = win;
  return MPI_SUCCESS;
 error:
  if (baseptr != MPI_BOTTOM) (void)MPI_Free_mem(baseptr);
  if (win != MPI_WIN_NULL)   (void)MPI_Win_free(&win);
  return ierr;
}
#undef  MPI_Win_allocate
#define MPI_Win_allocate PyMPI_Win_allocate

#endif
#endif

#ifndef PyMPI_HAVE_MPI_Win_set_info
static int PyMPI_Win_set_info(MPI_Win win, MPI_Info info)
{
  int dummy, ierr;
  if (win == MPI_WIN_NULL) return MPI_ERR_WIN;
  if (info != MPI_INFO_NULL) {
    ierr = MPI_Info_get_nkeys(info, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  return MPI_SUCCESS;
}
#undef  MPI_Win_set_info
#define MPI_Win_set_info PyMPI_Win_set_info
#endif

#ifndef PyMPI_HAVE_MPI_Win_get_info
static int PyMPI_Win_get_info(MPI_Win win, MPI_Info *info)
{
  if (win == MPI_WIN_NULL) return MPI_ERR_WIN;
  return MPI_Info_create(info);
}
#undef  MPI_Win_get_info
#define MPI_Win_get_info PyMPI_Win_get_info
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Info_get_string
static int PyMPI_Info_get_string(MPI_Info info,
                                 const char key[],
                                 int *buflen,
                                 char value[],
                                 int *flag)
{
  int ierr, valuelen = buflen ? *buflen : 0;
  if (valuelen) {
    ierr = MPI_Info_get(info, key, valuelen-1, value, flag);
    if (ierr != MPI_SUCCESS) return ierr;
    if (value && flag && *flag) value[valuelen] = 0;
  }
  ierr = MPI_Info_get_valuelen(info, key, &valuelen, flag);
  if (ierr != MPI_SUCCESS) return ierr;
  if (buflen && flag && *flag) *buflen = valuelen + 1;
  return MPI_SUCCESS;
}
#undef  MPI_Info_get_string
#define MPI_Info_get_string PyMPI_Info_get_string
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_F_SOURCE
#define PyMPI_F_SOURCE ((int)(offsetof(MPI_Status,MPI_SOURCE)/sizeof(int)))
#undef  MPI_F_SOURCE
#define MPI_F_SOURCE PyMPI_F_SOURCE
#endif

#ifndef PyMPI_HAVE_MPI_F_TAG
#define PyMPI_F_TAG ((int)(offsetof(MPI_Status,MPI_TAG)/sizeof(int)))
#undef  MPI_F_TAG
#define MPI_F_TAG PyMPI_F_TAG
#endif

#ifndef PyMPI_HAVE_MPI_F_ERROR
#define PyMPI_F_ERROR ((int)(offsetof(MPI_Status,MPI_ERROR)/sizeof(int)))
#undef  MPI_F_ERROR
#define MPI_F_ERROR PyMPI_F_ERROR
#endif

#ifndef PyMPI_HAVE_MPI_F_STATUS_SIZE
#define PyMPI_F_STATUS_SIZE ((int)(sizeof(MPI_Status)/sizeof(int)))
#undef  MPI_F_STATUS_SIZE
#define MPI_F_STATUS_SIZE PyMPI_F_STATUS_SIZE
#endif

/* ---------------------------------------------------------------- */

#include "largecnt.h"

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Type_contiguous_c
static int PyMPI_Type_contiguous_c(MPI_Count count,
                                   MPI_Datatype oldtype,
                                   MPI_Datatype *newtype)
{
  int ierr; int c;
  PyMPICastValue(int, c, MPI_Count, count);
  ierr = MPI_Type_contiguous(c, oldtype, newtype);
 fn_exit:
  return ierr;
}
#undef  MPI_Type_contiguous_c
#define MPI_Type_contiguous_c PyMPI_Type_contiguous_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_vector_c
static int PyMPI_Type_vector_c(MPI_Count count,
                               MPI_Count blocklength,
                               MPI_Count stride,
                               MPI_Datatype oldtype,
                               MPI_Datatype *newtype)
{
  int ierr; int c, b, s;
  PyMPICastValue(int, c, MPI_Count, count);
  PyMPICastValue(int, b, MPI_Count, blocklength);
  PyMPICastValue(int, s, MPI_Count, stride);
  ierr = MPI_Type_vector(c, b, s, oldtype, newtype);
 fn_exit:
  return ierr;
}
#undef  MPI_Type_vector_c
#define MPI_Type_vector_c PyMPI_Type_vector_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_hvector_c
static int PyMPI_Type_create_hvector_c(MPI_Count count,
                                       MPI_Count blocklength,
                                       MPI_Count stride,
                                       MPI_Datatype oldtype,
                                       MPI_Datatype *newtype)
{
  int ierr; int c, b, s;
  PyMPICastValue(int, c, MPI_Count, count);
  PyMPICastValue(int, b, MPI_Count, blocklength);
  PyMPICastValue(int, s, MPI_Count, stride);
  ierr = MPI_Type_create_hvector(c, b, s, oldtype, newtype);
 fn_exit:
  return ierr;
}
#undef  MPI_Type_create_hvector_c
#define MPI_Type_create_hvector_c PyMPI_Type_create_hvector_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_indexed_c
static int PyMPI_Type_indexed_c(MPI_Count count,
                                const MPI_Count blocklengths[],
                                const MPI_Count displacements[],
                                MPI_Datatype oldtype,
                                MPI_Datatype *newtype)
{
  int ierr; int c, *b = NULL, *d = NULL;
  PyMPICastValue(int, c, MPI_Count, count);
  PyMPICastArray(int, b, MPI_Count, blocklengths,  count);
  PyMPICastArray(int, d, MPI_Count, displacements, count);
  ierr = MPI_Type_indexed(c, b, d, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(b);
  PyMPIFreeArray(d);
  return ierr;
}
#undef  MPI_Type_indexed_c
#define MPI_Type_indexed_c PyMPI_Type_indexed_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_hindexed_c
static int PyMPI_Type_create_hindexed_c(MPI_Count count,
                                        const MPI_Count blocklengths[],
                                        const MPI_Count displacements[],
                                        MPI_Datatype oldtype,
                                        MPI_Datatype *newtype)
{
  int ierr; int c, *b = NULL; MPI_Aint *d = NULL;
  PyMPICastValue(int, c, MPI_Count, count);
  PyMPICastArray(int, b, MPI_Count, blocklengths,  count);
  PyMPICastArray(MPI_Aint, d, MPI_Count, displacements, count);
  ierr = MPI_Type_create_hindexed(c, b, d, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(b);
  PyMPIFreeArray(d);
  return ierr;
}
#undef  MPI_Type_create_hindexed_c
#define MPI_Type_create_hindexed_c PyMPI_Type_create_hindexed_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_indexed_block_c
static int PyMPI_Type_create_indexed_block_c(MPI_Count count,
                                             MPI_Count blocklength,
                                             const MPI_Count displacements[],
                                             MPI_Datatype oldtype,
                                             MPI_Datatype *newtype)
{
  int ierr; int c, b, *d = NULL;
  PyMPICastValue(int, c, MPI_Count, count);
  PyMPICastValue(int, b, MPI_Count, blocklength);
  PyMPICastArray(int, d, MPI_Count, displacements, count);
  ierr = MPI_Type_create_indexed_block(c, b, d, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(d);
  return ierr;
}
#undef  MPI_Type_create_indexed_block_c
#define MPI_Type_create_indexed_block_c PyMPI_Type_create_indexed_block_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_hindexed_block_c
static int PyMPI_Type_create_hindexed_block_c(MPI_Count count,
                                              MPI_Count blocklength,
                                              const MPI_Count displacements[],
                                              MPI_Datatype oldtype,
                                              MPI_Datatype *newtype)
{
  int ierr; int c, b; MPI_Aint *d = NULL;
  PyMPICastValue(int,      c, MPI_Count, count);
  PyMPICastValue(int,      b, MPI_Count, blocklength);
  PyMPICastArray(MPI_Aint, d, MPI_Count, displacements, count);
  ierr = MPI_Type_create_hindexed_block(c, b, d, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(d);
  return ierr;
}
#undef  MPI_Type_create_hindexed_block_c
#define MPI_Type_create_hindexed_block_c PyMPI_Type_create_hindexed_block_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_struct_c
static int PyMPI_Type_create_struct_c(MPI_Count count,
                                      const MPI_Count blocklengths[],
                                      const MPI_Count displacements[],
                                      const MPI_Datatype types[],
                                      MPI_Datatype *newtype)
{
  int ierr; int c, *b = NULL; MPI_Aint *d = NULL;
  MPI_Datatype *t = (MPI_Datatype *) types;
  PyMPICastValue(int,      c, MPI_Count, count);
  PyMPICastArray(int,      b, MPI_Count, blocklengths,  count);
  PyMPICastArray(MPI_Aint, d, MPI_Count, displacements, count);
  ierr = MPI_Type_create_struct(c, b, d, t, newtype);
 fn_exit:
  PyMPIFreeArray(b);
  PyMPIFreeArray(d);
  return ierr;
}
#undef  MPI_Type_create_struct_c
#define MPI_Type_create_struct_c PyMPI_Type_create_struct_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_subarray_c
static int PyMPI_Type_create_subarray_c(int ndims,
                                        const MPI_Count sizes[],
                                        const MPI_Count subsizes[],
                                        const MPI_Count starts[],
                                        int order,
                                        MPI_Datatype oldtype,
                                        MPI_Datatype *newtype)

{
  int ierr; int *N = NULL, *n = NULL, *s = NULL;
  PyMPICastArray(int, N, MPI_Count, sizes,    ndims);
  PyMPICastArray(int, n, MPI_Count, subsizes, ndims);
  PyMPICastArray(int, s, MPI_Count, starts,   ndims);
  ierr = MPI_Type_create_subarray(ndims, N, n, s,
                                  order, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(N);
  PyMPIFreeArray(n);
  PyMPIFreeArray(s);
  return ierr;
}
#undef  MPI_Type_create_subarray_c
#define MPI_Type_create_subarray_c PyMPI_Type_create_subarray_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_darray_c
static int PyMPI_Type_create_darray_c(int size, int rank, int ndims,
                                      const MPI_Count gsizes[],
                                      const int distribs[],
                                      const int dargs[],
                                      const int psizes[],
                                      int order,
                                      MPI_Datatype oldtype,
                                      MPI_Datatype *newtype)

{
  int ierr; int *g = NULL;
  PyMPICastArray(int, g, MPI_Count, gsizes, ndims);
  ierr = MPI_Type_create_darray(size, rank, ndims, g,
                                (int *) distribs,
                                (int *) dargs,
                                (int *) psizes,
                                order, oldtype, newtype);
 fn_exit:
  PyMPIFreeArray(g);
  return ierr;
}
#undef  MPI_Type_create_darray_c
#define MPI_Type_create_darray_c PyMPI_Type_create_darray_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_create_resized_c
static int PyMPI_Type_create_resized_c(MPI_Datatype oldtype,
                                       MPI_Count lb,
                                       MPI_Count extent,
                                       MPI_Datatype *newtype)
{
  int ierr; MPI_Aint ilb, iex;
  PyMPICastValue(MPI_Aint, ilb, MPI_Count, lb);
  PyMPICastValue(MPI_Aint, iex, MPI_Count, extent);
  ierr = MPI_Type_create_resized(oldtype, ilb, iex, newtype);
 fn_exit:
  return ierr;
}
#undef  MPI_Type_create_resized_c
#define MPI_Type_create_resized_c PyMPI_Type_create_resized_c
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Type_get_envelope_c
static int PyMPI_Type_get_envelope_c(MPI_Datatype datatype,
                                     MPI_Count *num_integers,
                                     MPI_Count *num_addresses,
                                     MPI_Count *num_large_counts,
                                     MPI_Count *num_datatypes,
                                     int *combiner)
{
  int ierr; int ni = 0, na = 0, nd = 0;
  ierr = MPI_Type_get_envelope(datatype, &ni, &na, &nd, combiner);
  if (ierr != MPI_SUCCESS) return ierr;
  if (num_integers) *num_integers     = ni;
  if (num_addresses) *num_addresses    = na;
  if (num_large_counts) *num_large_counts = 0;
  if (num_datatypes) *num_datatypes    = nd;
  return ierr;
}
#undef  MPI_Type_get_envelope_c
#define MPI_Type_get_envelope_c PyMPI_Type_get_envelope_c
#endif

#ifndef PyMPI_HAVE_MPI_Type_get_contents_c
static int PyMPI_Type_get_contents_c(MPI_Datatype datatype,
                                     MPI_Count max_integers,
                                     MPI_Count max_addresses,
                                     MPI_Count max_large_counts,
                                     MPI_Count max_datatypes,
                                     int integers[],
                                     MPI_Aint addresses[],
                                     MPI_Count large_counts[],
                                     MPI_Datatype datatypes[])
{
  int ierr; int ni, na, nd;
  PyMPICastValue(int, ni, MPI_Count, max_integers);
  PyMPICastValue(int, na, MPI_Count, max_addresses);
  PyMPICastValue(int, nd, MPI_Count, max_datatypes);
  ierr = MPI_Type_get_contents(datatype,
                               ni, na, nd,
                               integers,
                               addresses,
                               datatypes);
  (void)max_large_counts;
  (void)large_counts;
 fn_exit:
  return ierr;
}
#undef  MPI_Type_get_contents_c
#define MPI_Type_get_contents_c PyMPI_Type_get_contents_c
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Pack_c
static int PyMPI_Pack_c(const void *inbuf,
                        MPI_Count incount,
                        MPI_Datatype datatype,
                        void *outbuf,
                        MPI_Count outsize,
                        MPI_Count *position,
                        MPI_Comm comm)
{
  int ierr; int ic, os, pp;
  PyMPICastValue(int, ic, MPI_Count, incount);
  PyMPICastValue(int, os, MPI_Count, outsize);
  PyMPICastValue(int, pp, MPI_Count, *position);
  ierr = MPI_Pack((void*)inbuf, ic, datatype, outbuf, os, &pp, comm);
  if (ierr == MPI_SUCCESS) *position = pp;
 fn_exit:
  return ierr;
}
#undef  MPI_Pack_c
#define MPI_Pack_c PyMPI_Pack_c
#endif

#ifndef PyMPI_HAVE_MPI_Unpack_c
static int PyMPI_Unpack_c(const void *inbuf,
                          MPI_Count insize,
                          MPI_Count *position,
                          void *outbuf,
                          MPI_Count outcount,
                          MPI_Datatype datatype,
                          MPI_Comm comm)
{
  int ierr; int is, pp, oc;
  PyMPICastValue(int, is, MPI_Count, insize);
  PyMPICastValue(int, pp, MPI_Count, *position);
  PyMPICastValue(int, oc, MPI_Count, outcount);
  ierr = MPI_Unpack((void*)inbuf, is, &pp, outbuf, oc, datatype, comm);
  if (ierr == MPI_SUCCESS) *position = pp;
 fn_exit:
  return ierr;
}
#undef  MPI_Unpack_c
#define MPI_Unpack_c PyMPI_Unpack_c
#endif

#ifndef PyMPI_HAVE_MPI_Pack_size_c
static int PyMPI_Pack_size_c(MPI_Count count,
                             MPI_Datatype datatype,
                             MPI_Comm comm,
                             MPI_Count *size)
{
  int ierr; int c, s;
  PyMPICastValue(int, c, MPI_Count, count);
  ierr = MPI_Pack_size(c, datatype, comm, &s);
  if (ierr == MPI_SUCCESS) *size = s;
 fn_exit:
  return ierr;
}
#undef  MPI_Pack_size_c
#define MPI_Pack_size_c PyMPI_Pack_size_c
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Pack_external_c
static int PyMPI_Pack_external_c(const char datarep[],
                                 const void *inbuf,
                                 MPI_Count incount,
                                 MPI_Datatype datatype,
                                 void *outbuf,
                                 MPI_Count outsize,
                                 MPI_Count *position)
{
  int ierr; int ic; MPI_Aint os, pp;
  PyMPICastValue(int,      ic, MPI_Count, incount);
  PyMPICastValue(MPI_Aint, os, MPI_Count, outsize);
  PyMPICastValue(MPI_Aint, pp, MPI_Count, *position);
  ierr = MPI_Pack_external((char*)datarep,
                           (void*)inbuf, ic, datatype,
                           outbuf, os, &pp);
  if (ierr == MPI_SUCCESS) *position = pp;
 fn_exit:
  return ierr;
}
#undef  MPI_Pack_external_c
#define MPI_Pack_external_c PyMPI_Pack_external_c
#endif

#ifndef PyMPI_HAVE_MPI_Unpack_external_c
static int PyMPI_Unpack_external_c(const char datarep[],
                                   const void *inbuf,
                                   MPI_Count insize,
                                   MPI_Count *position,
                                   void *outbuf,
                                   MPI_Count outcount,
                                   MPI_Datatype datatype)
{
  int ierr; MPI_Aint is, pp; int oc;
  PyMPICastValue(MPI_Aint, is, MPI_Count, insize);
  PyMPICastValue(MPI_Aint, pp, MPI_Count, *position);
  PyMPICastValue(int,      oc, MPI_Count, outcount);
  ierr = MPI_Unpack_external((char*)datarep,
                             (void*)inbuf, is, &pp,
                             outbuf, oc, datatype);
  if (ierr == MPI_SUCCESS) *position = pp;
 fn_exit:
  return ierr;
}
#undef  MPI_Unpack_external_c
#define MPI_Unpack_external_c PyMPI_Unpack_external_c
#endif

#ifndef PyMPI_HAVE_MPI_Pack_external_size_c
static int PyMPI_Pack_external_size_c(const char datarep[],
                                      MPI_Count count,
                                      MPI_Datatype datatype,
                                      MPI_Count *size)
{
  int ierr; int c; MPI_Aint s;
  PyMPICastValue(int, c, MPI_Count, count);
  ierr = MPI_Pack_external_size((char*)datarep, c, datatype, &s);
  if (ierr == MPI_SUCCESS) *size = s;
 fn_exit:
  return ierr;
}
#undef  MPI_Pack_external_size_c
#define MPI_Pack_external_size_c PyMPI_Pack_external_size_c
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_HAVE_MPI_Register_datarep_c

typedef struct PyMPI_datarep_s {
  MPI_Datarep_conversion_function_c *read_fn;
  MPI_Datarep_conversion_function_c *write_fn;
  MPI_Datarep_extent_function *extent_fn;
  void *extra_state;
} PyMPI_datarep_t;

static int MPIAPI
PyMPI_datarep_read_fn(void *userbuf,
                      MPI_Datatype datatype,
                      int count,
                      void *filebuf,
                      MPI_Offset position,
                      void *extra_state)
{
  PyMPI_datarep_t *drep = (PyMPI_datarep_t *) extra_state;
  return drep->read_fn(userbuf, datatype, count,
                       filebuf, position,
                       drep->extra_state);
}

static int MPIAPI
PyMPI_datarep_write_fn(void *userbuf,
                       MPI_Datatype datatype,
                       int count,
                       void *filebuf,
                       MPI_Offset position,
                       void *extra_state)
{
  PyMPI_datarep_t *drep = (PyMPI_datarep_t *) extra_state;
  return drep->write_fn(userbuf, datatype, count,
                        filebuf, position,
                        drep->extra_state);
}

static int PyMPI_Register_datarep_c(const char *datarep,
  MPI_Datarep_conversion_function_c *read_conversion_fn,
  MPI_Datarep_conversion_function_c *write_conversion_fn,
  MPI_Datarep_extent_function *dtype_file_extent_fn,
  void *extra_state)
{
  static int n = 0; enum {N=32};
  static PyMPI_datarep_t registry[N];
  PyMPI_datarep_t *drep = (n < N) ? &registry[n++] :
    (PyMPI_datarep_t *) PyMPI_MALLOC(sizeof(PyMPI_datarep_t));

  MPI_Datarep_conversion_function *r_fn = MPI_CONVERSION_FN_NULL;
  MPI_Datarep_conversion_function *w_fn = MPI_CONVERSION_FN_NULL;
  MPI_Datarep_extent_function *e_fn = dtype_file_extent_fn;

  drep->read_fn = read_conversion_fn;
  drep->write_fn = write_conversion_fn;
  drep->extent_fn = dtype_file_extent_fn;
  drep->extra_state = extra_state;

  if (read_conversion_fn != MPI_CONVERSION_FN_NULL_C)
    r_fn = PyMPI_datarep_read_fn;
  if (write_conversion_fn != MPI_CONVERSION_FN_NULL_C)
    w_fn = PyMPI_datarep_write_fn;

  return MPI_Register_datarep(datarep, r_fn, w_fn, e_fn, drep);
}

#undef  MPI_Register_datarep_c
#define MPI_Register_datarep_c PyMPI_Register_datarep_c
#endif

/* ---------------------------------------------------------------- */

#if ((10 * MPI_VERSION + MPI_SUBVERSION) < 41)

#define PyMPI_GET_NAME_NULLOBJ(MPI_HANDLE_NULL) \
  do { if (obj == MPI_HANDLE_NULL && name && rlen) { \
  (void) strncpy(name, #MPI_HANDLE_NULL, MPI_MAX_OBJECT_NAME); \
  name[MPI_MAX_OBJECT_NAME] = 0; *rlen = (int) strlen(name); \
  return MPI_SUCCESS; \
  } } while(0)

static int PyMPI_Type_get_name(MPI_Datatype obj, char *name, int *rlen)
{
  PyMPI_GET_NAME_NULLOBJ(MPI_DATATYPE_NULL);
  return MPI_Type_get_name(obj, name, rlen);
}
#undef  MPI_Type_get_name
#define MPI_Type_get_name PyMPI_Type_get_name

static int PyMPI_Comm_get_name(MPI_Comm obj, char *name, int *rlen)
{
  PyMPI_GET_NAME_NULLOBJ(MPI_COMM_NULL);
  return MPI_Comm_get_name(obj, name, rlen);
}
#undef  MPI_Comm_get_name
#define MPI_Comm_get_name PyMPI_Comm_get_name

static int PyMPI_Win_get_name(MPI_Win obj, char *name, int *rlen)
{
  PyMPI_GET_NAME_NULLOBJ(MPI_WIN_NULL);
  return MPI_Win_get_name(obj, name, rlen);
}
#undef  MPI_Win_get_name
#define MPI_Win_get_name PyMPI_Win_get_name

#undef PyMPI_GET_NAME_NULLOBJ

#endif

/* ---------------------------------------------------------------- */

#include "mpiulfm.h"

#ifndef PyMPI_HAVE_MPI_Comm_revoke
#ifndef PyMPI_HAVE_MPIX_Comm_revoke

#ifndef PyMPI_HAVE_MPI_Comm_is_revoked
#ifndef PyMPI_HAVE_MPIX_Comm_is_revoked
static int PyMPI_Comm_is_revoked(MPI_Comm comm, int *flag)
{
  if (!flag) {
    (void) MPI_Comm_call_errhandler(comm, MPI_ERR_ARG);
    return MPI_ERR_ARG;
  }
  {
    int dummy, ierr;
    ierr = MPI_Comm_test_inter(comm, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  *flag = 0;
  return MPI_SUCCESS;
}
#undef  MPI_Comm_is_revoked
#define MPI_Comm_is_revoked PyMPI_Comm_is_revoked
#endif
#endif

#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_get_failed
#ifndef PyMPI_HAVE_MPIX_Comm_get_failed
static int PyMPI_Comm_get_failed(MPI_Comm comm, MPI_Group *group)
{
  {
    int dummy, ierr;
    ierr = MPI_Comm_test_inter(comm, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  if (!group) {
    (void) MPI_Comm_call_errhandler(comm, MPI_ERR_ARG);
    return MPI_ERR_ARG;
  }
  return MPI_Group_union(MPI_GROUP_EMPTY, MPI_GROUP_EMPTY, group);
}
#undef  MPI_Comm_get_failed
#define MPI_Comm_get_failed PyMPI_Comm_get_failed
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_ack_failed
#ifndef PyMPI_HAVE_MPIX_Comm_ack_failed
static int PyMPI_Comm_ack_failed(MPI_Comm comm,
                                 int num_to_ack,
                                 int *num_acked)
{
  {
    int dummy, ierr;
    ierr = MPI_Comm_test_inter(comm, &dummy);
    if (ierr != MPI_SUCCESS) return ierr;
  }
  if (!num_acked) {
    (void) MPI_Comm_call_errhandler(comm, MPI_ERR_ARG);
    return MPI_ERR_ARG;
  }
  (void)num_to_ack;
  *num_acked = 0;
  return MPI_SUCCESS;
}
#undef  MPI_Comm_ack_failed
#define MPI_Comm_ack_failed PyMPI_Comm_ack_failed
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_agree
#ifndef PyMPI_HAVE_MPIX_Comm_agree
static int PyMPI_Comm_agree(MPI_Comm comm, int *flag)
{
  int ibuf = flag ? *flag : 0;
  return MPI_Allreduce_c(&ibuf, flag, 1, MPI_INT, MPI_BAND, comm);
}
#undef  MPI_Comm_agree
#define MPI_Comm_agree PyMPI_Comm_agree
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_iagree
#ifndef PyMPI_HAVE_MPIX_Comm_iagree
static int MPIAPI PyMPI_iagree_free_fn(MPI_Comm c, int k, void *v, void *xs)
{ return (void) c, (void) xs, PyMPI_FREE(v), MPI_Comm_free_keyval(&k); }
static int PyMPI_Comm_iagree(MPI_Comm comm, int *flag, MPI_Request *request)
{
  int ierr, keyval, *ibuf;
  MPI_Comm_copy_attr_function *copy_fn = MPI_COMM_NULL_COPY_FN;
  MPI_Comm_delete_attr_function *free_fn = PyMPI_iagree_free_fn;
  ierr = MPI_Comm_create_keyval(copy_fn, free_fn, &keyval, NULL);
  if (ierr != MPI_SUCCESS) return ierr;
  ibuf = (int *) PyMPI_MALLOC(sizeof(int));
  ierr = MPI_Comm_set_attr(comm, keyval, ibuf);
  if (ierr != MPI_SUCCESS) return PyMPI_FREE(ibuf), ierr;
  ibuf[0] = flag ? *flag : 0;
  return MPI_Iallreduce_c(ibuf, flag, 1, MPI_INT, MPI_BAND, comm, request);
}
#undef  MPI_Comm_iagree
#define MPI_Comm_iagree PyMPI_Comm_iagree
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_shrink
#ifndef PyMPI_HAVE_MPIX_Comm_shrink
static int PyMPI_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm)
{
  return MPI_Comm_dup(comm, newcomm);
}
#undef  MPI_Comm_shrink
#define MPI_Comm_shrink PyMPI_Comm_shrink
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_ishrink
#ifndef PyMPI_HAVE_MPIX_Comm_ishrink
static int PyMPI_Comm_ishrink(MPI_Comm comm,
                              MPI_Comm *newcomm,
                              MPI_Request *request)
{
  return MPI_Comm_idup(comm, newcomm, request);
}
#undef  MPI_Comm_ishrink
#define MPI_Comm_ishrink PyMPI_Comm_ishrink
#endif
#endif

/* ---------------------------------------------------------------- */

#endif /* !PyMPI_FALLBACK_H */

/*
  Local variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/
