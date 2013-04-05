/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelExtentIO.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPixelExtentIO -- I/O routines for vtkPixelExtent
// .SECTION Description
// A small collection of I/O routines that can write vtkPixelExtent's
// or collections of them to disk for visualization as unstructured
// grids.
#ifndef __vtkPixelExtentIO_h
#define __vtkPixelExtentIO_h

#include "vtkIOLegacyModule.h" // for export
#include "vtkPixelExtent.h" // for pixel extent
#include <deque> // for std::deque

class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkPixelExtentIO
{
public:
  // Description:
  // Writes deque of extents for each MPI rank to disk
  // as an unstructured grid. Each extent is converted to
  // a QUAD cell. Rank is encoded in a cell data array.
  // It's aassumed that the data is duplicated on all
  // ranks thus only rank 0 writes the data to disk.
  static
  void Write(
        int commRank,
        const char *fileName,
        const std::deque<std::deque<vtkPixelExtent> >&exts);

  // Description:
  // Writes an extent for each MPI rank to disk as an
  // unstructured grid. It's expected that the index into
  // the deque identifies the rank. Each extent is converted
  // to a QUAD cell. Rank is encoded in a cell data array.
  // It's aassumed that the data is duplicated on all
  // ranks thus only rank 0 writes the data to disk.
  static
  void Write(
        int commRank,
        const char *fileName,
        const std::deque<vtkPixelExtent> &exts);

  // Description:
  // Write an extent per MPI rank to disk. All ranks
  // write. It's assumed that each rank passes a unique
  // filename.
  static
  void Write(
        int commRank,
        const char *fileName,
        const vtkPixelExtent &ext);
};


// Description:
// Insert the extent into an unstructured grid.
VTKIOLEGACY_EXPORT
vtkUnstructuredGrid &operator<<(vtkUnstructuredGrid &data, const vtkPixelExtent &ext);

#endif
// VTK-HeaderTest-Exclude: vtkPixelExtentIO.h
