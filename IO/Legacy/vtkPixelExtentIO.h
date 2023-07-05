// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPixelExtentIO
 *
 * A small collection of I/O routines that can write vtkPixelExtent's
 * or collections of them to disk for visualization as unstructured
 * grids.
 */

#ifndef vtkPixelExtentIO_h
#define vtkPixelExtentIO_h

#include "vtkIOLegacyModule.h" // for export
#include "vtkPixelExtent.h"    // for pixel extent
#include <deque>               // for std::deque

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkPixelExtentIO
{
public:
  /**
   * Writes deque of extents for each MPI rank to disk
   * as an unstructured grid. Each extent is converted to
   * a QUAD cell. Rank is encoded in a cell data array.
   * It's assumed that the data is duplicated on all
   * ranks thus only rank 0 writes the data to disk.
   */
  static void Write(int commRank, VTK_FILEPATH const char* fileName,
    const std::deque<std::deque<vtkPixelExtent>>& exts);

  /**
   * Writes an extent for each MPI rank to disk as an
   * unstructured grid. It's expected that the index into
   * the deque identifies the rank. Each extent is converted
   * to a QUAD cell. Rank is encoded in a cell data array.
   * It's assumed that the data is duplicated on all
   * ranks thus only rank 0 writes the data to disk.
   */
  static void Write(
    int commRank, VTK_FILEPATH const char* fileName, const std::deque<vtkPixelExtent>& exts);

  ///@{
  /**
   * Write an extent per MPI rank to disk. All ranks
   * write. It's assumed that each rank passes a unique
   * filename.
   */
  static void Write(int commRank, VTK_FILEPATH const char* fileName, const vtkPixelExtent& ext);
  ///@}
};

/**
 * Insert the extent into an unstructured grid.
 */
VTKIOLEGACY_EXPORT
vtkUnstructuredGrid& operator<<(vtkUnstructuredGrid& data, const vtkPixelExtent& ext);

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPixelExtentIO.h
