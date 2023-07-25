// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMPIPixelView
 * MPI datatypes that describe a vtkPixelExtent.
 */

#ifndef vtkMPIPixelView_h
#define vtkMPIPixelView_h

#include "vtkMPI.h"         // for mpi
#include "vtkMPIPixelTT.h"  // for type traits
#include "vtkPixelExtent.h" // for pixel extent
#include <iostream>         // for cerr

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
int vtkMPIPixelViewNew(
  const vtkPixelExtent& domain, const vtkPixelExtent& decomp, int nComps, MPI_Datatype& view)
{
#ifndef NDEBUG
  int mpiOk = 0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
  {
    std::cerr << "This class requires the MPI runtime." << std::endl;
    return -1;
  }
#endif

  int iErr;

  MPI_Datatype nativeType;
  iErr = MPI_Type_contiguous(nComps, vtkMPIPixelTT<T>::MPIType, &nativeType);
  if (iErr)
  {
    return -2;
  }

  int domainDims[2];
  domain.Size(domainDims);

  int domainStart[2];
  domain.GetStartIndex(domainStart);

  int decompDims[2];
  decomp.Size(decompDims);

  int decompStart[2];
  decomp.GetStartIndex(decompStart, domainStart);

  // use a contiguous type when possible.
  if (domain == decomp)
  {
    unsigned long long nCells = decomp.Size();
    iErr = MPI_Type_contiguous((int)nCells, nativeType, &view);
    if (iErr)
    {
      MPI_Type_free(&nativeType);
      return -3;
    }
  }
  else
  {
    iErr = MPI_Type_create_subarray(
      2, domainDims, decompDims, decompStart, MPI_ORDER_FORTRAN, nativeType, &view);
    if (iErr)
    {
      MPI_Type_free(&nativeType);
      return -4;
    }
  }
  iErr = MPI_Type_commit(&view);
  if (iErr)
  {
    MPI_Type_free(&nativeType);
    return -5;
  }

  MPI_Type_free(&nativeType);

  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkMPIPixelView.h
