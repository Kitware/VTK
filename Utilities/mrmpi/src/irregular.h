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

#ifndef IRREGULAR_H
#define IRREGULAR_H

#include "mpi.h"
#include "mrmpi_config.h"

namespace MAPREDUCE_NS {

class MRMPI_EXPORT Irregular {
 public:
  Irregular(MPI_Comm);
  ~Irregular();

  void pattern(int, int *);
  int size(int);
  int size(int *, int *, int *);
  void exchange(char *, char *);

 private:
  int me,nprocs;

  int patternflag;           // UNSET,SET
  int sizestyle;             // NONE,SAME,VARYING

  int self;                  // 0 = no data to copy to self, 1 = yes

  int ndatumsend;            // # of datums to send w/ self
  int ndatumrecv;            // # of datums to recv w/ self
  int nbytesrecv;            // total bytes in received data w/ self
  int nsend;                 // # of messages to send w/out self
  int nrecv;                 // # of messages to recv w/out self
  int nsendmax;              // # of bytes in largest send message, w/out self

  int *sendproc;             // list of procs to send to w/out self
  int *sendcount;            // # of datums to send to each proc w/ self
  int *sendsize;             // # of bytes to send to each proc w/ self
  int *sendindices;          // indices of datums to send to each proc w/ self

  int nsize;                 // size of every datum in bytes (SAME)
  int *sendsizedatum;        // bytes in each datum to send w/ self (VARYING)
  int *sendoffset;           // byte offset to where each datum starts w/ self
  int sendoffsetflag;        // 1 if allocated sendoffset, 0 if passed in

  int *recvproc;             // list of procs to recv from w/out self
  int *recvcount;            // # of datums to recv from each proc w/out self
  int *recvsize;             // # of bytes to recv from each proc w/out self

  MPI_Request *request;      // MPI requests for posted recvs
  MPI_Status *status;        // MPI statuses for Waitall
  MPI_Comm comm;             // MPI communicator for all communication

  class Memory *memory;
  class Error *error;

  void exchange_same(char *, char *);
  void exchange_varying(char *, char *);
};

}

#endif
