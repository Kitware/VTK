/* ----------------------------------------------------------------------
   MR-MPI = MapReduce-MPI library
   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2009) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level MapReduce directory.
------------------------------------------------------------------------- */

#ifndef MPI_STUBS
#define MPI_STUBS

#include "vtkConfigure.h"

#if defined(WIN32) && defined(VTK_BUILD_SHARED_LIBS)
 #if defined(mpistubs_EXPORTS)
  #define MPI_EXPORT __declspec( dllexport ) 
 #else
  #define MPI_EXPORT __declspec( dllimport ) 
 #endif
#else
 #define MPI_EXPORT
#endif

/* dummy defs for MPI stubs */

#define MPI_COMM_WORLD 0

#define MPI_INT 1
#define MPI_FLOAT 2
#define MPI_DOUBLE 3
#define MPI_CHAR 4
#define MPI_BYTE 5
#define MPI_DOUBLE_INT 6

#define MPI_SUM 1
#define MPI_MAX 2
#define MPI_MIN 3
#define MPI_MAXLOC 4
#define MPI_MINLOC 5

#define MPI_ANY_SOURCE -1

#define MPI_Comm int
#define MPI_Request int
#define MPI_Datatype int
#define MPI_Op int

#ifdef __cplusplus
extern "C" {
#endif

/* MPI data structs */

MPI_EXPORT typedef struct MPI_Status_Struct {
  int MPI_SOURCE;
} MPI_Status;

/* Function prototypes for MPI stubs */

MPI_EXPORT void MPI_Init(int *argc, char ***argv);
MPI_EXPORT void MPI_Initialized(int *flag);
MPI_EXPORT void MPI_Finalize();

MPI_EXPORT void MPI_Comm_rank(MPI_Comm comm, int *me);
MPI_EXPORT void MPI_Comm_size(MPI_Comm comm, int *nprocs);
MPI_EXPORT void MPI_Abort(MPI_Comm comm, int errorcode);
MPI_EXPORT double MPI_Wtime();

MPI_EXPORT void MPI_Send(void *buf, int count, MPI_Datatype datatype,
        int dest, int tag, MPI_Comm comm);
MPI_EXPORT void MPI_Rsend(void *buf, int count, MPI_Datatype datatype,
         int dest, int tag, MPI_Comm comm);
MPI_EXPORT void MPI_Recv(void *buf, int count, MPI_Datatype datatype,
        int source, int tag, MPI_Comm comm, MPI_Status *status);
MPI_EXPORT void MPI_Irecv(void *buf, int count, MPI_Datatype datatype,
         int source, int tag, MPI_Comm comm, MPI_Request *request);
MPI_EXPORT void MPI_Wait(MPI_Request *request, MPI_Status *status);
MPI_EXPORT void MPI_Waitall(int n, MPI_Request *request, MPI_Status *status);
MPI_EXPORT void MPI_Waitany(int count, MPI_Request *request, int *index, 
     MPI_Status *status);
MPI_EXPORT void MPI_Sendrecv(void *sbuf, int scount, MPI_Datatype sdatatype,
      int dest, int stag, void *rbuf, int rcount,
      MPI_Datatype rdatatype, int source, int rtag,
      MPI_Comm comm, MPI_Status *status);
MPI_EXPORT void MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count);

MPI_EXPORT void MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *comm_out);
MPI_EXPORT void MPI_Comm_dup(MPI_Comm comm, MPI_Comm *comm_out);
MPI_EXPORT void MPI_Comm_free(MPI_Comm *comm);

MPI_EXPORT void MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods,
         int reorder, MPI_Comm *comm_cart);
MPI_EXPORT void MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods,
      int *coords);
MPI_EXPORT void MPI_Cart_shift(MPI_Comm comm, int direction, int displ,
        int *source, int *dest);
MPI_EXPORT void MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank);

MPI_EXPORT void MPI_Barrier(MPI_Comm comm);
MPI_EXPORT void MPI_Bcast(void *buf, int count, MPI_Datatype datatype,
         int root, MPI_Comm comm);
MPI_EXPORT void MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
MPI_EXPORT void MPI_Scan(void *sendbuf, void *recvbuf, int count,
        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
MPI_EXPORT void MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
       void *recvbuf, int recvcount, MPI_Datatype recvtype,
       MPI_Comm comm);
MPI_EXPORT void MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
        void *recvbuf, int *recvcounts, int *displs,
        MPI_Datatype recvtype, MPI_Comm comm);
MPI_EXPORT void MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts,
      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
MPI_EXPORT void MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    int root, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif

