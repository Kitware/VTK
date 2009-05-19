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

/* Single-processor "stub" versions of MPI routines */

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "vtksys/SystemTools.hxx"

#include "mpi.h"

/* lo-level function prototypes */

void mpi_copy_int(void *, void *, int);
void mpi_copy_float(void *, void *, int);
void mpi_copy_double(void *, void *, int);
void mpi_copy_char(void *, void *, int);
void mpi_copy_byte(void *, void *, int);

/* lo-level data structure */

struct _double_int {
  double value;
  int proc;
} double_int;

/* ---------------------------------------------------------------------- */
/* MPI Functions */
/* ---------------------------------------------------------------------- */

void MPI_Init(int * /* argc */, char *** /* argv */) {}

/* ---------------------------------------------------------------------- */

void MPI_Initialized(int *flag)
{
  *flag = 1;
}

/* ---------------------------------------------------------------------- */

void MPI_Finalize() {}

/* ---------------------------------------------------------------------- */

void MPI_Comm_rank(MPI_Comm /* comm */, int *me)
{
  *me = 0;
}

/* ---------------------------------------------------------------------- */

void MPI_Comm_size(MPI_Comm /* comm */, int *nprocs)
{
  *nprocs = 1;
}

/* ---------------------------------------------------------------------- */

void MPI_Abort(MPI_Comm /* comm */, int /* errorcode */)
{
  exit(1);
}

/* ---------------------------------------------------------------------- */

double MPI_Wtime()
{
  return vtksys::SystemTools::GetTime ();
}

/* ---------------------------------------------------------------------- */

void MPI_Send(void * /* buf */, int  /* count */, MPI_Datatype  /* datatype */,
        int  /* dest */, int  /* tag */, MPI_Comm  /* comm */)
{
  printf("MPI Stub WARNING: Should not send message to self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Rsend(void * /* buf */, int  /* count */, MPI_Datatype  /* datatype */,
         int  /* dest */, int  /* tag */, MPI_Comm  /* comm */)
{
  printf("MPI Stub WARNING: Should not rsend message to self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Recv(void * /* buf */, int  /* count */, MPI_Datatype  /* datatype */,
        int  /* source */, int  /* tag */, MPI_Comm  /* comm */, MPI_Status * /* status */)
{
  printf("MPI Stub WARNING: Should not recv message from self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Irecv(void * /* buf */, int /* count */, MPI_Datatype /* datatype */,
         int /* source */, int /* tag */, MPI_Comm /* comm */, MPI_Request * /* request */)
{
  printf("MPI Stub WARNING: Should not recv message from self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Wait(MPI_Request * /* request */, MPI_Status * /* status */)
{
  printf("MPI Stub WARNING: Should not wait on message from self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Waitall(int /* n */, MPI_Request * /* request */, MPI_Status * /* status */)
{
  printf("MPI Stub WARNING: Should not wait on message from self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Waitany(int /* count */, MPI_Request * /* request */, int * /* index */, 
     MPI_Status * /* status */)
{
  printf("MPI Stub WARNING: Should not wait on message from self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Sendrecv(void * /* sbuf */, int  /* scount */, MPI_Datatype  /* sdatatype */,
      int  /* dest */, int  /* stag */, void * /* rbuf */, int  /* rcount */,
      MPI_Datatype  /* rdatatype */, int  /* source */, int  /* rtag */,
      MPI_Comm  /* comm */, MPI_Status * /* status */)
{
  printf("MPI Stub WARNING: Should not send message to self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Get_count(MPI_Status * /* status */, MPI_Datatype /* datatype */, int * /* count */)
{
  printf("MPI Stub WARNING: Should not get count of message to self\n");
}

/* ---------------------------------------------------------------------- */

void MPI_Comm_split(MPI_Comm comm, int /* color */, int /* key */, MPI_Comm *comm_out)
{
  *comm_out = comm;
}

/* ---------------------------------------------------------------------- */

void MPI_Comm_dup(MPI_Comm comm, MPI_Comm *comm_out)
{
  *comm_out = comm;
}

/* ---------------------------------------------------------------------- */

void MPI_Comm_free(MPI_Comm * /* comm */) { }

/* ---------------------------------------------------------------------- */

void MPI_Cart_create(MPI_Comm comm_old, int /* ndims */, int * /* dims */, int * /*periods */,
         int /* reorder */, MPI_Comm *comm_cart)
{
  *comm_cart = comm_old;
}

/* ---------------------------------------------------------------------- */

void MPI_Cart_get(MPI_Comm /* comm */, int /* maxdims */, int *dims, int *periods,
      int *coords)
{
  dims[0] = dims[1] = dims[2] = 1;
  periods[0] = periods[1] = periods[2] = 1;
  coords[0] = coords[1] = coords[2] = 0;
}

/* ---------------------------------------------------------------------- */

void MPI_Cart_shift(MPI_Comm /* comm */, int /* direction */, int /* displ */,
        int *source, int *dest)
{
  *source = *dest = 0;
}

/* ---------------------------------------------------------------------- */

void MPI_Cart_rank(MPI_Comm /* comm */, int * /* coords */, int *rank)
{
  *rank = 0;
}

/* ---------------------------------------------------------------------- */

void MPI_Barrier(MPI_Comm /* comm */) {}

/* ---------------------------------------------------------------------- */

void MPI_Bcast(void * /* buf */, int /* count */, MPI_Datatype /* datatype */,
         int /* root */, MPI_Comm /* comm */) {}

/* ---------------------------------------------------------------------- */

/* copy values from data1 to data2 */

void MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
       MPI_Datatype datatype, MPI_Op /* op */, MPI_Comm /* comm */)
{
  int n = 0;
  if (datatype == MPI_INT) n = count*sizeof(int);
  else if (datatype == MPI_FLOAT) n = count*sizeof(float);
  else if (datatype == MPI_DOUBLE) n = count*sizeof(double);
  else if (datatype == MPI_CHAR) n = count*sizeof(char);
  else if (datatype == MPI_BYTE) n = count*sizeof(char);
  else if (datatype == MPI_DOUBLE_INT) n = count*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}

/* ---------------------------------------------------------------------- */

void MPI_Scan(void *sendbuf, void *recvbuf, int count,
        MPI_Datatype datatype, MPI_Op /* op */, MPI_Comm /* comm */)
{
  int n = 0;
  if (datatype == MPI_INT) n = count*sizeof(int);
  else if (datatype == MPI_FLOAT) n = count*sizeof(float);
  else if (datatype == MPI_DOUBLE) n = count*sizeof(double);
  else if (datatype == MPI_CHAR) n = count*sizeof(char);
  else if (datatype == MPI_BYTE) n = count*sizeof(char);
  else if (datatype == MPI_DOUBLE_INT) n = count*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}

/* ---------------------------------------------------------------------- */

/* copy values from data1 to data2 */

void MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
       void *recvbuf, int /* recvcount */, MPI_Datatype /* recvtype */,
       MPI_Comm /* comm */)
{
  int n = 0;
  if (sendtype == MPI_INT) n = sendcount*sizeof(int);
  else if (sendtype == MPI_FLOAT) n = sendcount*sizeof(float);
  else if (sendtype == MPI_DOUBLE) n = sendcount*sizeof(double);
  else if (sendtype == MPI_CHAR) n = sendcount*sizeof(char);
  else if (sendtype == MPI_BYTE) n = sendcount*sizeof(char);
  else if (sendtype == MPI_DOUBLE_INT) n = sendcount*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}

/* ---------------------------------------------------------------------- */

/* copy values from data1 to data2 */

void MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
        void *recvbuf, int * /* recvcounts */, int * /* displs */,
        MPI_Datatype /* recvtype */, MPI_Comm /* comm */)
{
  int n = 0;
  if (sendtype == MPI_INT) n = sendcount*sizeof(int);
  else if (sendtype == MPI_FLOAT) n = sendcount*sizeof(float);
  else if (sendtype == MPI_DOUBLE) n = sendcount*sizeof(double);
  else if (sendtype == MPI_CHAR) n = sendcount*sizeof(char);
  else if (sendtype == MPI_BYTE) n = sendcount*sizeof(char);
  else if (sendtype == MPI_DOUBLE_INT) n = sendcount*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}

/* ---------------------------------------------------------------------- */

/* copy values from data1 to data2 */

void MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts,
      MPI_Datatype datatype, MPI_Op /* op */, MPI_Comm /* comm */)
{
  int n = 0;
  if (datatype == MPI_INT) n = *recvcounts*sizeof(int);
  else if (datatype == MPI_FLOAT) n = *recvcounts*sizeof(float);
  else if (datatype == MPI_DOUBLE) n = *recvcounts*sizeof(double);
  else if (datatype == MPI_CHAR) n = *recvcounts*sizeof(char);
  else if (datatype == MPI_BYTE) n = *recvcounts*sizeof(char);
  else if (datatype == MPI_DOUBLE_INT) n = *recvcounts*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}

/* ---------------------------------------------------------------------- */

/* copy values from data1 to data2 */

void MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
       void *recvbuf, int /* recvcount */, MPI_Datatype /* recvtype */,
       int /* root */, MPI_Comm /* comm */)
{
  int n = 0;
  if (sendtype == MPI_INT) n = sendcount*sizeof(int);
  else if (sendtype == MPI_FLOAT) n = sendcount*sizeof(float);
  else if (sendtype == MPI_DOUBLE) n = sendcount*sizeof(double);
  else if (sendtype == MPI_CHAR) n = sendcount*sizeof(char);
  else if (sendtype == MPI_BYTE) n = sendcount*sizeof(char);
  else if (sendtype == MPI_DOUBLE_INT) n = sendcount*sizeof(double_int);

  memcpy(recvbuf,sendbuf,n);
}
