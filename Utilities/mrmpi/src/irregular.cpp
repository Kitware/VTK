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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "irregular.h"
#include "memory.h"
#include "error.h"

using namespace MAPREDUCE_NS;

#ifdef MAX
#  undef MAX
#endif 
#define MAX(A,B) ((A) > (B)) ? (A) : (B)

enum{UNSET,SET};
enum{NONE,SAME,VARYING};

/* ---------------------------------------------------------------------- */

Irregular::Irregular(MPI_Comm caller)
{
  comm = caller;
  MPI_Comm_rank(comm,&me);
  MPI_Comm_size(comm,&nprocs);

  memory = new Memory(comm);
  error = new Error(comm);

  sendproc = sendcount = sendsize = sendindices = NULL;
  sendoffset = NULL;
  sendoffsetflag = 0;
  recvproc = recvcount = recvsize = NULL;
  request = NULL;
  status = NULL;

  patternflag = UNSET;
  sizestyle = NONE;
}


/* ---------------------------------------------------------------------- */

Irregular::~Irregular()
{
  delete memory;
  delete error;

  delete [] sendproc;
  delete [] sendcount;
  delete [] sendsize;
  memory->sfree(sendindices);
  if (sendoffsetflag) memory->sfree(sendoffset);

  delete [] recvproc;
  delete [] recvcount;
  delete [] recvsize;

  delete [] request;
  delete [] status;
}

/* ----------------------------------------------------------------------
  n = # of datums contributed by this proc
  proclist = which proc each datum is to be sent to
------------------------------------------------------------------------- */

void Irregular::pattern(int n, int *proclist)
{
  patternflag = SET;
  sizestyle = NONE;

  ndatumsend = n;

  // list = 1 for procs I send to, including self
  // nrecv = # of messages I receive, not including self
  // self = 0 if no data for self, 1 if there is

  int *list = new int[nprocs];
  int *counts = new int[nprocs];

  for (int i = 0; i < nprocs; i++) {
    list[i] = 0;
    counts[i] = 1;
  }

  for (int i = 0; i < n; i++) list[proclist[i]] = 1;
  MPI_Reduce_scatter(list,&nrecv,counts,MPI_INT,MPI_SUM,comm);

  self = 0;
  if (list[me]) self = 1;
  if (self) nrecv--;

  // storage for recv info, not including self

  recvproc = new int[nrecv];
  recvcount = new int[nrecv];
  recvsize = new int[nrecv];
  request = new MPI_Request[nrecv];
  status = new MPI_Status[nrecv];

  // list = # of datums to send to each proc, including self
  // nsend = # of messages I send, not including self
  
  for (int i = 0; i < nprocs; i++) list[i] = 0;
  for (int i = 0; i < n; i++) list[proclist[i]]++;

  nsend = 0;
  for (int i = 0; i < nprocs; i++) if (list[i] > 0) nsend++;
  if (self) nsend--;

  // storage for send info, including self

  sendproc = new int[nsend+self];
  sendcount = new int[nsend+self];
  sendsize = new int[nsend+self];
  sendindices = (int *) memory->smalloc(n*sizeof(int),"sendindices");

  // setup sendprocs and sendcounts, including self
  // each proc begins with iproc > me, and continues until iproc = me
  // list ends up with pointer to which send that proc is associated with

  int iproc = me;
  int isend = 0;
  for (int i = 0; i < nprocs; i++) {
    iproc++;
    if (iproc == nprocs) iproc = 0;
    if (list[iproc] > 0) {
      sendproc[isend] = iproc;
      sendcount[isend] = list[iproc];
      list[iproc] = isend;
      isend++;
    }
  }

  // post all receives for datum counts

  for (int i = 0; i < nrecv; i++)
    MPI_Irecv(&recvcount[i],1,MPI_INT,MPI_ANY_SOURCE,0,comm,&request[i]);

  // barrier to insure receives are posted

  MPI_Barrier(comm);

  // send each datum count, packing buf with needed datums

  for (int i = 0; i < nsend; i++)
    MPI_Send(&sendcount[i],1,MPI_INT,sendproc[i],0,comm);

  // insure all MPI_ANY_SOURCE messages are received
  // set recvproc

  if (nrecv) MPI_Waitall(nrecv,request,status);
  for (int i = 0; i < nrecv; i++) recvproc[i] = status[i].MPI_SOURCE;

  // ndatumrecv = total datums received, including self

  ndatumrecv = 0;
  for (int i = 0; i < nrecv; i++)
    ndatumrecv += recvcount[i];
  if (self) ndatumrecv += sendcount[nsend];

  // setup sendindices, including self
  // counts = offset into sendindices for each proc I send to
  // let sc0 = sendcount[0], sc1 = sendcount[1], etc
  // sendindices[0:sc0-1] = indices of datums in 1st message
  // sendindices[sc0:sc0+sc1-1] = indices of datums in 2nd message, etc

  counts[0] = 0;
  for (int i = 1; i < nsend+self; i++)
    counts[i] = counts[i-1] + sendcount[i-1];

  for (int i = 0; i < n; i++) {
    isend = list[proclist[i]];
    sendindices[counts[isend]++] = i;
  }

  // clean up

  delete [] counts;
  delete [] list;
}

/* ----------------------------------------------------------------------
  n = size of each received datum
  return total size in bytes of received data on this proc
------------------------------------------------------------------------- */

int Irregular::size(int n)
{
  if (patternflag == UNSET) error->all("Cannot size without pattern");
  sizestyle = SAME;

  nsize = n;

  nsendmax = 0;
  for (int i = 0; i < nsend+self; i++) {
    sendsize[i] = nsize * sendcount[i];
    if (i < nsend) nsendmax = MAX(nsendmax,sendsize[i]);
  }

  for (int i = 0; i < nrecv; i++) recvsize[i] = nsize * recvcount[i];
  nbytesrecv = nsize * ndatumrecv;

  return nbytesrecv;
}

/* ----------------------------------------------------------------------
  slength,rlength = size of each datum to send and recv
  soffset = offset into eventual buffer of send data for each datum
  soffset can be NULL, in which case will build sendoffset from slength
  return total size in bytes of received data on this proc
------------------------------------------------------------------------- */

int Irregular::size(int *slength, int *soffset, int *rlength)
{
  if (patternflag == UNSET) error->all("Cannot size without pattern");
  sizestyle = VARYING;

  // store local copy of pointers to send lengths/offsets
  // if soffset not provided, create local copy from slength

  sendsizedatum = slength;

  if (soffset == NULL) {
    sendoffsetflag = 1;
    sendoffset = (int *) memory->smalloc(ndatumsend*sizeof(int),"sendoffset");

    if (ndatumsend) sendoffset[0] = 0;
    for (int i = 1; i < ndatumsend; i++)
      sendoffset[i] = sendoffset[i-1] + sendsizedatum[i-1];

  } else {
    if (sendoffsetflag) memory->sfree(sendoffset);
    sendoffsetflag = 0;
    sendoffset = soffset;
  }

  nsendmax = 0;
  int m = 0;
  for (int i = 0; i < nsend+self; i++) {
    sendsize[i] = 0;
    for (int j = 0; j < sendcount[i]; j++)
      sendsize[i] += sendsizedatum[sendindices[m++]];
    if (i < nsend) nsendmax = MAX(nsendmax,sendsize[i]);
  }

  nbytesrecv = 0;
  m = 0;
  for (int i = 0; i < nrecv; i++) {
    recvsize[i] = 0;
    for (int j = 0; j < recvcount[i]; j++) recvsize[i] += rlength[m++];
    nbytesrecv += recvsize[i];
  }
  if (self) nbytesrecv += sendsize[nsend];

  return nbytesrecv;
}

/* ----------------------------------------------------------------------
  wrapper on 2 versions of exchange
------------------------------------------------------------------------- */

void Irregular::exchange(char *sendbuf, char *recvbuf)
{
  if (sizestyle == SAME) exchange_same(sendbuf,recvbuf);
  else if (sizestyle == VARYING) exchange_varying(sendbuf,recvbuf);
  else error->all("Irregular size was not set");
}

/* ----------------------------------------------------------------------
  sendbuf = data to send
  recvbuf = buffer to recv all data into
  requires nsize,nsendmax,recvsize,sendsize be setup by size(int)
------------------------------------------------------------------------- */

void Irregular::exchange_same(char *sendbuf, char *recvbuf)
{
  // post all receives

  int recvoffset = 0;
  for (int irecv = 0; irecv < nrecv; irecv++) {
    MPI_Irecv(&recvbuf[recvoffset],recvsize[irecv],MPI_BYTE,
            recvproc[irecv],0,comm,&request[irecv]);
    recvoffset += recvsize[irecv];
  }

  // malloc buf for largest send

  char *buf = (char *) memory->smalloc(nsendmax,"buf");

  // barrier to insure receives are posted

  MPI_Barrier(comm);

  // send each message, packing buf with needed datums

  int m = 0;
  for (int isend = 0; isend < nsend; isend++) {
    int bufoffset = 0;
    for (int i = 0; i < sendcount[isend]; i++) {
      memcpy(&buf[bufoffset],&sendbuf[nsize*sendindices[m++]],nsize);
      bufoffset += nsize;
    }
    MPI_Send(buf,sendsize[isend],MPI_BYTE,sendproc[isend],0,comm);
  }       

  // copy self data directly from sendbuf to recvbuf

  if (self)
    for (int i = 0; i < sendcount[nsend]; i++) {
      memcpy(&recvbuf[recvoffset],&sendbuf[nsize*sendindices[m++]],nsize);
      recvoffset += nsize;
    }

  // free send buffer

  memory->sfree(buf);

  // wait on all incoming messages

  if (nrecv) MPI_Waitall(nrecv,request,status);
}

/* ----------------------------------------------------------------------
  sendbuf = data to send
  recvbuf = buffer to recv all data into
  requires nsendmax,recvsize,sendsize,sendoffset,sendsizedatum
    be setup by size(int *, int *)
------------------------------------------------------------------------- */

void Irregular::exchange_varying(char *sendbuf, char *recvbuf)
{
  // post all receives

  int recvoffset = 0;
  for (int irecv = 0; irecv < nrecv; irecv++) {
    MPI_Irecv(&recvbuf[recvoffset],recvsize[irecv],MPI_BYTE,
            recvproc[irecv],0,comm,&request[irecv]);
    recvoffset += recvsize[irecv];
  }

  // malloc buf for largest send

  char *buf = (char *) memory->smalloc(nsendmax,"buf");

  // barrier to insure receives are posted

  MPI_Barrier(comm);

  // send each message, packing buf with needed datums

  int index;
  int m = 0;
  for (int isend = 0; isend < nsend; isend++) {
    int bufoffset = 0;
    for (int i = 0; i < sendcount[isend]; i++) {
      index = sendindices[m++];
      memcpy(&buf[bufoffset],&sendbuf[sendoffset[index]],sendsizedatum[index]);
      bufoffset += sendsizedatum[index];
    }
    MPI_Send(buf,sendsize[isend],MPI_BYTE,sendproc[isend],0,comm);
  }

  // copy self data directly from sendbuf to recvbuf

  if (self)
    for (int i = 0; i < sendcount[nsend]; i++) {
      index = sendindices[m++];
      memcpy(&recvbuf[recvoffset],&sendbuf[sendoffset[index]],
       sendsizedatum[index]);
      recvoffset += sendsizedatum[index];
    }

  // free send buffer

  memory->sfree(buf);

  // wait on all incoming messages

  if (nrecv) MPI_Waitall(nrecv,request,status);
}
