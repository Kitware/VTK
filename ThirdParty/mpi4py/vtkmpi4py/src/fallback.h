#ifndef PyMPI_FALLBACK_H
#define PyMPI_FALLBACK_H

/* ---------------------------------------------------------------- */

#ifdef Py_PYTHON_H

#ifndef PyMPI_MALLOC
#define PyMPI_MALLOC PyMem_Malloc
#endif
#ifndef PyMPI_FREE
#define PyMPI_FREE PyMem_Free
#endif

#else

#include <stdlib.h>

#ifndef PyMPI_MALLOC
#define PyMPI_MALLOC malloc
#endif
#ifndef PyMPI_Free
#define PyMPI_Free free
#endif

#endif

/* ---------------------------------------------------------------- */

/* Version Number */

#ifdef PyMPI_MISSING_MPI_VERSION
#if !defined(MPI_VERSION)
#define MPI_VERSION 1
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SUBVERSION
#if !defined(MPI_SUBVERSION)
#define MPI_SUBVERSION 0
#endif
#endif

#ifdef PyMPI_MISSING_MPI_Get_version
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

/* ---------------------------------------------------------------- */

/* Threading Support */

#ifdef PyMPI_MISSING_MPI_Init_thread
static int PyMPI_Init_thread(int *argc, char ***argv,
                             int required, int *provided)
{
  int ierr = MPI_SUCCESS;
  if (!provided) return MPI_ERR_ARG;
  ierr = MPI_Init(argc, argv);
  if (ierr != MPI_SUCCESS) return ierr;
  *provided = MPI_THREAD_SINGLE;
  return MPI_SUCCESS;
}
#undef  MPI_Init_thread
#define MPI_Init_thread PyMPI_Init_thread
#endif

#ifdef PyMPI_MISSING_MPI_Query_thread
static int PyMPI_Query_thread(int *provided)
{
  if (!provided) return MPI_ERR_ARG;
  provided = MPI_THREAD_SINGLE;
  return MPI_SUCCESS;
}
#undef  MPI_Query_thread
#define MPI_Query_thread PyMPI_Query_thread
#endif

#ifdef PyMPI_MISSING_MPI_Is_thread_main
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

#ifdef PyMPI_MISSING_MPI_STATUS_IGNORE
static MPI_Status PyMPI_STATUS_IGNORE;
#undef MPI_STATUS_IGNORE
#define MPI_STATUS_IGNORE ((MPI_Status*)(&PyMPI_STATUS_IGNORE))
#endif

#ifdef PyMPI_MISSING_MPI_STATUSES_IGNORE
#ifndef PyMPI_MPI_STATUSES_IGNORE_SIZE
#if defined(__GNUC__) || defined(__ICC) || defined(__INTEL_COMPILER)
#warning MPI_STATUSES_IGNORE will use static storage of size 4096
#warning Buffer overruns may occur. You were warned !!!
#endif
#define PyMPI_MPI_STATUSES_IGNORE_SIZE 4096
#endif
static MPI_Status PyMPI_STATUSES_IGNORE[PyMPI_MPI_STATUSES_IGNORE_SIZE];
#undef  MPI_STATUSES_IGNORE
#define MPI_STATUSES_IGNORE ((MPI_Status*)(PyMPI_STATUSES_IGNORE))
#endif

/* ---------------------------------------------------------------- */

/* Datatypes */

#ifdef PyMPI_MISSING_MPI_LONG_LONG
#undef  MPI_LONG_LONG
#define MPI_LONG_LONG MPI_LONG_LONG_INT
#endif

#ifdef PyMPI_MISSING_MPI_Type_get_extent
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

#ifdef PyMPI_MISSING_MPI_Type_dup
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

#ifdef PyMPI_MISSING_MPI_Type_create_indexed_block
static int PyMPI_Type_create_indexed_block(int count,
                                           int blocklength,
                                           int displacements[],
                                           MPI_Datatype oldtype,
                                           MPI_Datatype *newtype)
{
  int i, *blocklengths = 0;
  int ierr = MPI_SUCCESS;
  if (count > 0) {
    blocklengths = (int *) PyMPI_MALLOC(count*sizeof(int));
    if (!blocklengths) return MPI_ERR_INTERN;
  }
  for (i=0; i<count; i++) blocklengths[i] = blocklength;
  ierr = MPI_Type_indexed(count,blocklengths,displacements,oldtype,newtype);
  if (blocklengths) PyMPI_FREE(blocklengths);
  return ierr;
}
#undef  MPI_Type_create_indexed_block
#define MPI_Type_create_indexed_block PyMPI_Type_create_indexed_block
#endif

/*
 * Adapted from the implementation in MPICH2 sources,
 * mpich2-1.0.7/src/mpi/datatype/type_create_subarray.c
 *
 */
#ifdef PyMPI_MISSING_MPI_Type_create_subarray

#undef  PyMPI_CHKARG
#define PyMPI_CHKARG(EXPR) if (!(EXPR)) return MPI_ERR_ARG
#undef  PyMPI_CHKERR
#define PyMPI_CHKERR(IERR) if (IERR) return IERR
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

  PyMPI_CHKARG( ndims > 0 );
  PyMPI_CHKARG( sizes );
  PyMPI_CHKARG( subsizes );
  PyMPI_CHKARG( starts );
  PyMPI_CHKARG( newtype );
  for (i=0; i<ndims; i++) {
    PyMPI_CHKARG( sizes[i] >  0 );
    PyMPI_CHKARG( subsizes[i] >  0 );
    PyMPI_CHKARG( starts[i] >= 0 );
    PyMPI_CHKARG( sizes[i] >= subsizes[i] );
    PyMPI_CHKARG( starts[i] <= (sizes[i] - subsizes[i]) );
  }
  PyMPI_CHKARG( (order==MPI_ORDER_C) || (order==MPI_ORDER_FORTRAN) );

  ierr = MPI_Type_extent(oldtype, &extent); PyMPI_CHKERR(ierr);

  if (order == MPI_ORDER_FORTRAN) {
    if (ndims == 1)
      ierr = MPI_Type_contiguous(subsizes[0],
                                 oldtype,
                                 &tmp1); PyMPI_CHKERR(ierr);
    else {
      ierr = MPI_Type_vector(subsizes[1], subsizes[0], sizes[0],
                             oldtype, &tmp1); PyMPI_CHKERR(ierr);
      size = sizes[0]*extent;
      for (i=2; i<ndims; i++) {
        size *= sizes[i-1];
        ierr = MPI_Type_hvector(subsizes[i], 1, size,
                                tmp1, &tmp2); PyMPI_CHKERR(ierr);
        ierr = MPI_Type_free(&tmp1); PyMPI_CHKERR(ierr);
        tmp1 = tmp2;
      }
    }
    /* add displacement and UB */
    disps[1] = starts[0];
    size = 1;
    for (i=1; i<ndims; i++) {
      size *= sizes[i-1];
      disps[1] += size*starts[i];
    }
  } else /* MPI_ORDER_C */ {
    /* dimension ndims-1 changes fastest */
    if (ndims == 1) {
      ierr = MPI_Type_contiguous(subsizes[0],
                                 oldtype, &tmp1); PyMPI_CHKERR(ierr);
    } else {
      ierr = MPI_Type_vector(subsizes[ndims-2], subsizes[ndims-1],
                             sizes[ndims-1],
                             oldtype, &tmp1); PyMPI_CHKERR(ierr);
      size = sizes[ndims-1]*extent;
      for (i=ndims-3; i>=0; i--) {
        size *= sizes[i+1];
        ierr = MPI_Type_hvector(subsizes[i], 1, size,
                                tmp1, &tmp2); PyMPI_CHKERR(ierr);
        ierr = MPI_Type_free(&tmp1); PyMPI_CHKERR(ierr);
        tmp1 = tmp2;
      }
    }
    /* add displacement and UB */
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

  ierr = MPI_Type_struct(3, blklens, disps, types,
                         newtype); PyMPI_CHKERR(ierr);
  ierr = MPI_Type_free(&tmp1); PyMPI_CHKERR(ierr);

  return MPI_SUCCESS;
}
#undef PyMPI_CHKARG
#undef PyMPI_CHKERR

#undef  MPI_Type_create_subarray
#define MPI_Type_create_subarray PyMPI_Type_create_subarray
#endif

/*
 * Adapted from the implementation in MPICH2 sources,
 * mpich2-1.0.7/src/mpi/datatype/type_create_darray.c
 *
 */
#ifdef PyMPI_MISSING_MPI_Type_create_darray

#undef  PyMPI_MIN
#define PyMPI_MIN(__a, __b) (((__a) < (__b)) ? (__a) : (__b))
#undef  PyMPI_CHKARG
#define PyMPI_CHKARG(EXPR) if (!(EXPR)) return MPI_ERR_ARG
#undef  PyMPI_CHKERR
#define PyMPI_CHKERR(IERR) if (IERR) goto fn_exit
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
  mysize = PyMPI_MIN(blksize, j);
  if (mysize < 0) mysize = 0;
  stride = orig_extent;
  if (order == MPI_ORDER_FORTRAN) {
    if (dim == 0) {
      ierr = MPI_Type_contiguous(mysize,
                                 type_old, type_new); PyMPI_CHKERR(ierr);
    } else {
      for (i=0; i<dim; i++) stride *= gsizes[i];
      ierr = MPI_Type_hvector(mysize, 1, stride,
                              type_old, type_new); PyMPI_CHKERR(ierr);
    }
  } else { /* order == MPI_ORDER_C */
    if (dim == ndims-1) {
      ierr = MPI_Type_contiguous(mysize,
                                 type_old, type_new); PyMPI_CHKERR(ierr);
    } else {
      for (i=ndims-1; i>dim; i--) stride *= gsizes[i];
      ierr = MPI_Type_hvector(mysize, 1, stride,
                              type_old, type_new); PyMPI_CHKERR(ierr);
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
    local_size += PyMPI_MIN(rem, blksize);
  }

  count = local_size/blksize;
  rem = local_size % blksize;

  stride = nprocs*blksize*orig_extent;
  if (order == MPI_ORDER_FORTRAN)
    for (i=0; i<dim; i++) stride *= gsizes[i];
  else
    for (i=ndims-1; i>dim; i--) stride *= gsizes[i];

  ierr = MPI_Type_hvector(count, blksize, stride,
                          type_old, type_new);PyMPI_CHKERR(ierr);
  /* if the last block is of size less than blksize,
     include it separately using MPI_Type_struct */
  if (rem) {
    types[0] = *type_new;
    types[1] = type_old;
    disps[0] = 0;
    disps[1] = count*stride;
    blklens[0] = 1;
    blklens[1] = rem;
    ierr = MPI_Type_struct(2, blklens, disps, types,
                           &type_tmp); PyMPI_CHKERR(ierr);
    ierr = MPI_Type_free(type_new); PyMPI_CHKERR(ierr);
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
      ierr = MPI_Type_struct(3, blklens, disps, types,
                             &type_tmp); PyMPI_CHKERR(ierr);
      ierr = MPI_Type_free(type_new); PyMPI_CHKERR(ierr);
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

  int      *coords  = 0;
  MPI_Aint *offsets = 0;

  orig_extent=0;
  type_old = type_new = MPI_DATATYPE_NULL;;
  types[0] = types[1] = types[2] = MPI_DATATYPE_NULL;

  ierr = MPI_Type_extent(oldtype, &orig_extent); PyMPI_CHKERR(ierr);

  PyMPI_CHKARG(rank     >= 0);
  PyMPI_CHKARG(size     >  0);
  PyMPI_CHKARG(ndims    >  0);
  PyMPI_CHKARG(gsizes   != 0);
  PyMPI_CHKARG(distribs != 0);
  PyMPI_CHKARG(dargs    != 0);
  PyMPI_CHKARG(psizes   != 0);
  PyMPI_CHKARG((order == MPI_ORDER_C) ||
               (order == MPI_ORDER_FORTRAN));
  for (i=0; ierr == MPI_SUCCESS && i < ndims; i++) {
    PyMPI_CHKARG(gsizes[1] >  0);
    PyMPI_CHKARG(psizes[1] >  0);
    PyMPI_CHKARG((distribs[i] == MPI_DISTRIBUTE_NONE)  ||
                 (distribs[i] == MPI_DISTRIBUTE_BLOCK) ||
                 (distribs[i] == MPI_DISTRIBUTE_CYCLIC));
    PyMPI_CHKARG((dargs[i] == MPI_DISTRIBUTE_DFLT_DARG) ||
                 (dargs[i] > 0));
    PyMPI_CHKARG (!((distribs[i] == MPI_DISTRIBUTE_NONE) &&
                    (psizes[i] != 1)));
  }

  /* calculate position in Cartesian grid
     as MPI would (row-major ordering) */
  coords  = (int *) PyMPI_MALLOC(ndims*sizeof(int));
  if (coords  == 0) { ierr = MPI_ERR_INTERN; PyMPI_CHKERR(ierr); }
  offsets = (MPI_Aint *) PyMPI_MALLOC(ndims*sizeof(MPI_Aint));
  if (offsets == 0) { ierr = MPI_ERR_INTERN; PyMPI_CHKERR(ierr); }

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
                                offsets+i); PyMPI_CHKERR(ierr);
      } else if (distribs[i] == MPI_DISTRIBUTE_CYCLIC) {
        ierr = PyMPI_Type_cyclic(gsizes, i, ndims,
                                 psizes[i], coords[i], dargs[i],
                                 order, orig_extent,
                                 type_old, &type_new,
                                 offsets+i); PyMPI_CHKERR(ierr);
      } else if (distribs[i] == MPI_DISTRIBUTE_NONE) {
        /* treat it as a block distribution on 1 process */
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                1, 0,  MPI_DISTRIBUTE_DFLT_DARG,
                                order, orig_extent,
                                type_old, &type_new,
                                offsets+i); PyMPI_CHKERR(ierr);
      }
      if (i != 0) {
        ierr = MPI_Type_free(&type_old); PyMPI_CHKERR(ierr);
      }
      type_old = type_new;
    }
    /* add displacement and UB */
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
                                offsets+i); PyMPI_CHKERR(ierr);
      } else if (distribs[i] == MPI_DISTRIBUTE_CYCLIC) {
        ierr = PyMPI_Type_cyclic(gsizes, i, ndims,
                                 psizes[i], coords[i], dargs[i],
                                 order,  orig_extent,
                                 type_old, &type_new,
                                 offsets+i); PyMPI_CHKERR(ierr);
      } else if (distribs[i] == MPI_DISTRIBUTE_NONE) {
        /* treat it as a block distribution on 1 process */
        ierr = PyMPI_Type_block(gsizes, i, ndims,
                                psizes[i], coords[i], MPI_DISTRIBUTE_DFLT_DARG,
                                order, orig_extent,
                                type_old, &type_new,
                                offsets+i); PyMPI_CHKERR(ierr);
      }
      if (i != ndims-1) {
        ierr = MPI_Type_free(&type_old); PyMPI_CHKERR(ierr);
      }
      type_old = type_new;
    }
    /* add displacement and UB */
    disps[1] = offsets[ndims-1];
    tmp_size = 1;
    for (i=ndims-2; i>=0; i--) {
      tmp_size *= gsizes[i+1];
      disps[1] += tmp_size*offsets[i];
    }
    /* rest done below for both Fortran and C order */
  }

  disps[1] *= orig_extent;
  disps[2] = orig_extent;
  for (i=0; i<ndims; i++) disps[2] *= gsizes[i];
  disps[0] = 0;
  blklens[0] = blklens[1] = blklens[2] = 1;
  types[0] = MPI_LB;
  types[1] = type_new;
  types[2] = MPI_UB;
  ierr = MPI_Type_struct(3, blklens, disps, types,
                         newtype);PyMPI_CHKERR(ierr);
  ierr = MPI_Type_free(&type_new);PyMPI_CHKERR(ierr);

  ierr = MPI_SUCCESS;
 fn_exit:
  if (coords  != 0) PyMPI_FREE(coords);
  if (offsets != 0) PyMPI_FREE(offsets);
  return ierr;
}
#undef PyMPI_MIN
#undef PyMPI_CHKARG
#undef PyMPI_CHKERR

#undef  MPI_Type_create_darray
#define MPI_Type_create_darray PyMPI_Type_create_darray
#endif

/* ---------------------------------------------------------------- */

#ifndef PyMPI_MISSING_MPI_Request_get_status
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
    #if defined(PyMPI_MISSING_MPI_Status_set_cancelled) ||  \
        defined(PyMPI_MISSING_MPI_Status_set_elements)
    int n = (int) sizeof(MPI_Status);
    unsigned char *p = (unsigned char *)status;
    while (n-- > 0) p[n] = 0;
    #endif
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG    = MPI_ANY_TAG;
    status->MPI_ERROR  = MPI_SUCCESS;
    #ifndef PyMPI_MISSING_MPI_Status_set_elements
    MPI_Status_set_elements(status, MPI_BYTE, 0);
    #endif
    #ifndef PyMPI_MISSING_MPI_Status_set_cancelled
    MPI_Status_set_cancelled(status, 0);
    #endif
  }
  return MPI_SUCCESS;
}
#undef  MPI_Request_get_status
#define MPI_Request_get_status PyMPI_Request_get_status
#endif
#endif

/* ---------------------------------------------------------------- */

#ifdef PyMPI_MISSING_MPI_Reduce_scatter_block
static int PyMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf,
                                      int recvcount, MPI_Datatype datatype,
                                      MPI_Op op, MPI_Comm comm)
{
  int ierr = MPI_SUCCESS;
  int n = 1, *recvcounts = 0;
  ierr = MPI_Comm_size(comm, &n);
  if (ierr != MPI_SUCCESS) return ierr;
  recvcounts = (int *) PyMPI_MALLOC(n*sizeof(int));
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

/* Memory Allocation */

#if (defined(PyMPI_MISSING_MPI_Alloc_mem) || \
     defined(PyMPI_MISSING_MPI_Free_mem))

static int PyMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  char *buf = 0, **basebuf = 0;
  if (size < 0) return MPI_ERR_ARG;
  if (!baseptr) return MPI_ERR_ARG;
  if (size == 0) size = 1;
  buf = (char *) PyMPI_MALLOC(size);
  if (!buf) return MPI_ERR_NO_MEM;
  basebuf = (char **) baseptr;
  *basebuf = buf;
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

#endif /* !PyMPI_FALLBACK_H */

/*
  Local variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/
