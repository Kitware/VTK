// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPPixelTransfer.h"
using std::deque;
using std::ostream;
using std::vector;

VTK_ABI_NAMESPACE_BEGIN

//*****************************************************************************
ostream& operator<<(ostream& os, const vtkPPixelTransfer& pt)
{
  os << "[" << pt.GetSourceRank() << "]"
     << " " << pt.GetSourceWholeExtent() << " " << pt.GetSourceExtent() << " -> "
     << "[" << pt.GetDestinationRank() << "]"
     << " " << pt.GetDestinationWholeExtent() << " " << pt.GetDestinationExtent();
  return os;
}

//------------------------------------------------------------------------------
int vtkPPixelTransfer::Execute(MPI_Comm comm, int rank, int nComps, int srcType, void* srcData,
  int destType, void* destData, vector<MPI_Request>& reqs, deque<MPI_Datatype>& types, int tag)
{
  // first layer of dispatch
  switch (srcType)
  {
    vtkTemplateMacro(return this->Execute(
      comm, rank, nComps, (VTK_TT*)srcData, destType, destData, reqs, types, tag););
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkPPixelTransfer::Blit(int nComps, int srcType, void* srcData, int destType, void* destData)
{
  return vtkPixelTransfer::Blit(this->SrcWholeExt, this->SrcExt, this->DestWholeExt, this->DestExt,
    nComps, srcType, srcData, nComps, destType, destData);
}
VTK_ABI_NAMESPACE_END
