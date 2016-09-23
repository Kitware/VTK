/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPixelTransfer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPPixelTransfer
 *
 * class to handle inter-process communication of pixel data from
 * non-contiguous regions of a shared index space. For example copying
 * a subset of one image to a subset of another. The class can be used
 * for purely local(no MPI) non-contigious data transfers by setting
 * the source and destination ranks to the same id. In that case
 * memcpy is used.
 *
 * @sa
 * vtkPixelExtent
*/

#ifndef vtkPPixelTransfer_h
#define vtkPPixelTransfer_h

#include "vtkPixelTransfer.h"
#include "vtkRenderingParallelLICModule.h" // for export
#include "vtkSetGet.h" // for macros
#include "vtkPixelExtent.h" // for pixel extent
#include "vtkMPI.h" // for mpi
#include "vtkMPIPixelTT.h" // for type traits
#include "vtkMPIPixelView.h" // for mpi subarrays

// included vtkSystemIncludes.h in the base class.
#include <iostream> // for ostream
#include <vector> // for vector
#include <cstring> // for memcpy

// #define vtkPPixelTransferDEBUG

class VTKRENDERINGPARALLELLIC_EXPORT vtkPPixelTransfer : public vtkPixelTransfer
{
public:
  vtkPPixelTransfer()
      :
    SrcRank(0),
    DestRank(0),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are different.
   */
  vtkPPixelTransfer(
        int srcRank,
        const vtkPixelExtent &srcWholeExt,
        const vtkPixelExtent &srcExt,
        int destRank,
        const vtkPixelExtent &destWholeExt,
        const vtkPixelExtent &destExt,
        int id=0)
        :
    Id(id),
    SrcRank(srcRank),
    SrcWholeExt(srcWholeExt),
    SrcExt(srcExt),
    DestRank(destRank),
    DestWholeExt(destWholeExt),
    DestExt(destExt),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are the same.
   */
  vtkPPixelTransfer(
        int srcRank,
        const vtkPixelExtent &srcWholeExt,
        const vtkPixelExtent &targetExt,
        int destRank,
        const vtkPixelExtent &destWholeExt,
        int id)
        :
    Id(id),
    SrcRank(srcRank),
    SrcWholeExt(srcWholeExt),
    SrcExt(targetExt),
    DestRank(destRank),
    DestWholeExt(destWholeExt),
    DestExt(targetExt),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, both the whole and the subsets are the same.
   */
  vtkPPixelTransfer(
        int srcRank,
        int destRank,
        const vtkPixelExtent &wholeExt,
        const vtkPixelExtent &targetExt,
        int id=0)
        :
    Id(id),
    SrcRank(srcRank),
    SrcWholeExt(wholeExt),
    SrcExt(targetExt),
    DestRank(destRank),
    DestWholeExt(wholeExt),
    DestExt(targetExt),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, both the whole and the subsets are the same.
   */
  vtkPPixelTransfer(
        int srcRank,
        int destRank,
        const vtkPixelExtent &ext,
        int id=0)
        :
    Id(id),
    SrcRank(srcRank),
    SrcWholeExt(ext),
    SrcExt(ext),
    DestRank(destRank),
    DestWholeExt(ext),
    DestExt(ext),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from whole extent of source to whole extent
   * of dest, where source and destination have different whole extents.
   */
  vtkPPixelTransfer(
        int srcRank,
        const vtkPixelExtent &srcWholeExt,
        int destRank,
        const vtkPixelExtent &destWholeExt,
        int id=0)
        :
    Id(id),
    SrcRank(srcRank),
    SrcWholeExt(srcWholeExt),
    SrcExt(srcWholeExt),
    DestRank(destRank),
    DestWholeExt(destWholeExt),
    DestExt(destWholeExt),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are different. This is a local
   * operation there will be no communication.
   */
  vtkPPixelTransfer(
        const vtkPixelExtent &srcWholeExt,
        const vtkPixelExtent &srcExt,
        const vtkPixelExtent &destWholeExt,
        const vtkPixelExtent &destExt)
        :
    Id(0),
    SrcRank(0),
    SrcWholeExt(srcWholeExt),
    SrcExt(srcExt),
    DestRank(0),
    DestWholeExt(destWholeExt),
    DestExt(destExt),
    UseBlockingSend(0),
    UseBlockingRecv(0)
    {}

  ~vtkPPixelTransfer(){}

  /**
   * Set/Get the MPI rank of source and destination
   * processes.
   */
  void SetSourceRank(int rank)
  { this->SrcRank=rank; }

  int GetSourceRank() const
  { return this->SrcRank; }

  void SetDestinationRank(int rank)
  { this->DestRank=rank; }

  int GetDestinationRank() const
  { return this->DestRank; }

  /**
   * Tests to determine a given rank's role in this transaction.
   * If both Sender and Receiver are true then the operation
   * is local and no mpi calls are made.
   */
  bool Sender(int rank) const { return (this->SrcRank == rank); }
  bool Receiver(int rank) const { return (this->DestRank == rank); }
  bool Local(int rank) const
  { return (this->Sender(rank) && this->Receiver(rank)); }

  /**
   * Set/Get the source extent. This is the extent of the
   * array that data will be coppied from.
   */
  void SetSourceWholeExtent(vtkPixelExtent &srcExt)
  { this->SrcWholeExt=srcExt; }

  vtkPixelExtent &GetSourceWholeExtent()
  { return this->SrcWholeExt; }

  const vtkPixelExtent &GetSourceWholeExtent() const
  { return this->SrcWholeExt; }

  /**
   * Set/Get the source extent. This is the subset extent in the
   * array that data will be coppied from.
   */
  void SetSourceExtent(vtkPixelExtent &srcExt)
  { this->SrcExt=srcExt; }

  vtkPixelExtent &GetSourceExtent()
  { return this->SrcExt; }

  const vtkPixelExtent &GetSourceExtent() const
  { return this->SrcExt; }

  /**
   * Set/get the destination extent. This is the extent
   * of array that will recveive the data.
   */
  void SetDestinationWholeExtent(vtkPixelExtent &destExt)
  { this->DestWholeExt=destExt; }

  vtkPixelExtent &GetDestinationWholeExtent()
  { return this->DestWholeExt; }

  const vtkPixelExtent &GetDestinationWholeExtent() const
  { return this->DestWholeExt; }

  /**
   * Set/get the destination extent. This is the subset extent
   * in the array that will recveive the data.
   */
  void SetDestinationExtent(vtkPixelExtent &destExt)
  { this->DestExt=destExt; }

  vtkPixelExtent &GetDestinationExtent()
  { return this->DestExt; }

  const vtkPixelExtent &GetDestinationExtent() const
  { return this->DestExt; }

  /**
   * Set/get the transaction id.
   */
  void SetTransactionId(int id)
  { this->Id=id; }

  int GetTransactionId() const
  { return this->Id; }

  /**
   * Enable/diasable non-blocking communication
   */
  void SetUseBlockingSend(int val)
  { this->UseBlockingSend=val; }

  int GetUseBlockingSend() const
  { return this->UseBlockingSend; }

  void SetUseBlockingRecv(int val)
  { this->UseBlockingRecv=val; }

  int GetUseBlockingRecv() const
  { return this->UseBlockingRecv; }

  /**
   * Transfer data from source to destination.
   */
  template<typename SOURCE_TYPE, typename DEST_TYPE>
  int Execute(
        MPI_Comm comm,
        int rank,
        int nComps,
        SOURCE_TYPE *srcData,
        DEST_TYPE *destData,
        std::vector<MPI_Request> &reqs,
        std::deque<MPI_Datatype> &types,
        int tag);

  /**
   * Transfer data from source to destination. convenience for working
   * with vtk data type enum rather than c types.
   */
  int Execute(
        MPI_Comm comm,
        int rank,
        int nComps,
        int srcType,
        void *srcData,
        int destType,
        void *destData,
        std::vector<MPI_Request> &reqs,
        std::deque<MPI_Datatype> &types,
        int tag);

  /**
   * Block transfer for local memory to memory transfers, without using mpi.
   */
  int Blit(
         int nComps,
         int srcType,
         void *srcData,
         int destType,
         void *destData);

private:
  // distpatch helper for vtk data type enum
  template<typename SOURCE_TYPE>
  int Execute(
        MPI_Comm comm,
        int rank,
        int nComps,
        SOURCE_TYPE *srcData,
        int destType,
        void *destData,
        std::vector<MPI_Request> &reqs,
        std::deque<MPI_Datatype> &types,
        int tag);

private:
  int Id;                      // transaction id
  int SrcRank;                 // rank who owns source memory
  vtkPixelExtent SrcWholeExt;  // source extent
  vtkPixelExtent SrcExt;       // source subset to transfer
  int DestRank;                // rank who owns destination memory
  vtkPixelExtent DestWholeExt; // destination extent
  vtkPixelExtent DestExt;      // destination subset
  int UseBlockingSend;         // controls for non-blocking comm
  int UseBlockingRecv;
};

//-----------------------------------------------------------------------------
template<typename SOURCE_TYPE>
int vtkPPixelTransfer::Execute(
       MPI_Comm comm,
       int rank,
       int nComps,
       SOURCE_TYPE *srcData,
       int destType,
       void *destData,
       std::vector<MPI_Request> &reqs,
       std::deque<MPI_Datatype> &types,
       int tag)
{
  // second layer of dispatch
  switch(destType)
  {
    vtkTemplateMacro(
        return this->Execute(
            comm,
            rank,
            nComps,
            srcData,
            (VTK_TT*)destData,
            reqs,
            types,
            tag));
  }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename SOURCE_TYPE, typename DEST_TYPE>
int vtkPPixelTransfer::Execute(
       MPI_Comm comm,
       int rank,
       int nComps,
       SOURCE_TYPE *srcData,
       DEST_TYPE *destData,
       std::vector<MPI_Request> &reqs,
       std::deque<MPI_Datatype> &types,
       int tag)
{
  int iErr = 0;
  if ((comm == MPI_COMM_NULL) || (this->Local(rank)))
  {
    // transaction is local, bypass mpi in favor of memcpy
    return vtkPixelTransfer::Blit(
            this->SrcWholeExt,
            this->SrcExt,
            this->DestWholeExt,
            this->DestExt,
            nComps,
            srcData,
            nComps,
            destData);
  }

  if (rank == this->DestRank)
  {
    // use mpi to receive the data
    if (destData == NULL)
    {
      return -1;
    }

    MPI_Datatype subarray;
    iErr = vtkMPIPixelViewNew<DEST_TYPE>(
          this->DestWholeExt,
          this->DestExt,
          nComps,
          subarray);
    if (iErr)
    {
      return -4;
    }

    if (this->UseBlockingRecv)
    {
      MPI_Status stat;
      iErr = MPI_Recv(
            destData,
            1,
            subarray,
            this->SrcRank,
            tag,
            comm,
            &stat);
    }
    else
    {
      reqs.push_back(MPI_REQUEST_NULL);
      iErr = MPI_Irecv(
            destData,
            1,
            subarray,
            this->SrcRank,
            tag,
            comm,
            &reqs.back());
    }

    #define HOLD_RECV_TYPES
    #ifdef HOLD_RECV_YPES
    types.push_back(subarray);
    #else
    MPI_Type_free(&subarray);
    #endif

    if (iErr)
    {
      return -5;
    }
  }

  if (rank == this->SrcRank)
  {
    // use mpi to send the data
    if (srcData == NULL)
    {
      return -1;
    }

    MPI_Datatype subarray;
    iErr = vtkMPIPixelViewNew<SOURCE_TYPE>(
          this->SrcWholeExt,
          this->SrcExt,
          nComps,
          subarray);
    if (iErr)
    {
      return -2;
    }

    if (this->UseBlockingSend)
    {
      iErr = MPI_Ssend(
            srcData,
            1,
            subarray,
            this->DestRank,
            tag,
            comm);
    }
    else
    {
      MPI_Request req;
      iErr = MPI_Isend(
            srcData,
            1,
            subarray,
            this->DestRank,
            tag,
            comm,
            &req);
      #define SAVE_SEND_REQS
      #ifdef SAVE_SEND_REQS
      reqs.push_back(req);
      #else
      MPI_Request_free(&req);
      #endif
    }

    #define HOLD_SEND_TYPES
    #ifdef HOLD_SEND_TYPES
    types.push_back(subarray);
    #else
    MPI_Type_free(&subarray);
    #endif

    if (iErr)
    {
      return -3;
    }
  }

  return iErr;
}

VTKRENDERINGPARALLELLIC_EXPORT
ostream &operator<<(std::ostream &os, const vtkPPixelTransfer &gt);

#endif
// VTK-HeaderTest-Exclude: vtkPPixelTransfer.h
