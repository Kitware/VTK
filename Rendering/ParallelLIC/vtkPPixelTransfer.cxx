/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPixelTransfer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPPixelTransfer.h"
using std::ostream;
using std::vector;
using std::deque;

//*****************************************************************************
ostream &operator<<(ostream &os, const vtkPPixelTransfer &pt)
{
  os
    << "[" << pt.GetSourceRank() << "]"
    << " "  << pt.GetSourceWholeExtent()
    << " "  << pt.GetSourceExtent()
    << " -> "
    << "[" << pt.GetDestinationRank() << "]"
    << " "  << pt.GetDestinationWholeExtent()
    << " "  << pt.GetDestinationExtent();
  return os;
}

//-----------------------------------------------------------------------------
int vtkPPixelTransfer::Execute(
       MPI_Comm comm,
       int rank,
       int nComps,
       int srcType,
       void *srcData,
       int destType,
       void *destData,
       vector<MPI_Request> &reqs,
       deque<MPI_Datatype> &types,
       int tag)
{
  // first layer of dispatch
  switch(srcType)
  {
    vtkTemplateMacro(
        return this->Execute(
            comm,
            rank,
            nComps,
            (VTK_TT*)srcData,
            destType,
            destData,
            reqs,
            types,
            tag););
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPPixelTransfer::Blit(
         int nComps,
         int srcType,
         void *srcData,
         int destType,
         void *destData)
{
  return vtkPixelTransfer::Blit(
        this->SrcWholeExt,
        this->SrcExt,
        this->DestWholeExt,
        this->DestExt,
        nComps,
        srcType,
        srcData,
        nComps,
        destType,
        destData);
}
