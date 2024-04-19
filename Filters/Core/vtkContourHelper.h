// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkContourHelper
 * @brief A utility class used by various contour filters
 *
 * This is a simple utility class that can be used by various contour filters to
 * produce either triangles and/or polygons based on the outputTriangles parameter.
 * If outputTriangles is set to false, trisEstimatedSize is used to allocate memory
 * for temporary triangles created by contouring before merging them.
 * If outputTriangles is set to true, contouring triangles are outputted and
 * trisEstimatedSize is not used.
 *
 * When working with multidimensional dataset, it is needed to process cells
 * from low to high dimensions.
 *
 * @sa
 * vtkContourGrid vtkCutter vtkContourFilter
 */

#ifndef vtkContourHelper_h
#define vtkContourHelper_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkWeakPointer.h"       // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;
class vtkCellArray;
class vtkPointData;
class vtkCellData;
class vtkCell;
class vtkDataArray;

class VTKFILTERSCORE_EXPORT vtkContourHelper
{
public:
  /**
   * Contour helper constructor.
   *
   * @param locator Locator used to "carry" and merge contour points (avoid duplicates)
   * @param outVerts Contour vertices, incremented at each Contour call
   * @param outLines Contour lines, incremented at each Contour call
   * @param outPolys Contour polys, incremented at each Contour call
   * @param inPd Input point data, that will be interpolated on output contour point data
   * @param inCd Input cell data, that will be copied on output contour cell data
   * @param outPd If not nullptr, will contains contour point data, interpolated from inPd
   * @param outCd If not nullptr, will contains contour cell data, copied from inCd
   * @param trisEstimatedSize used to allocate memory for temporary triangles created by
   * contouring before merging them. Only used if outputTriangles is true.
   * @param outputTriangles if true, the contour helper will output triangles directly and
   * will not merge them.
   */
  vtkContourHelper(vtkIncrementalPointLocator* locator, vtkCellArray* outVerts,
    vtkCellArray* outLines, vtkCellArray* outPolys, vtkPointData* inPd, vtkCellData* inCd,
    vtkPointData* outPd, vtkCellData* outCd, int trisEstimatedSize, bool outputTriangles);
  ~vtkContourHelper() = default;

  /**
   * Generate contour for the given cell (and add it to the final result).
   *
   * @param cell Cell to contour
   * @param value Contour value
   * @param cellScalars Scalar values at each point to contour with.
   * Should be indexed with local cell points ids.
   * @param cellId Contoured cell id, used to copy cell data to the contour
   * (see inCd and outCd parameters in the constructor)
   *
   * @attention This method is not thread safe. A multi-threaded program would have
   * to create one instance of the helper per thread, with isolated critical sections
   * (output data pointers).
   */
  void Contour(vtkCell* cell, double value, vtkDataArray* cellScalars, vtkIdType cellId);

private:
  vtkContourHelper(const vtkContourHelper&) = delete;
  vtkContourHelper& operator=(const vtkContourHelper&) = delete;

  // Filled upon construction
  vtkWeakPointer<vtkIncrementalPointLocator> Locator;
  vtkWeakPointer<vtkCellArray> OutVerts;
  vtkWeakPointer<vtkCellArray> OutLines;
  vtkWeakPointer<vtkCellArray> OutPolys;
  vtkWeakPointer<vtkPointData> InPd;
  vtkWeakPointer<vtkCellData> InCd;
  vtkWeakPointer<vtkPointData> OutPd;
  vtkWeakPointer<vtkCellData> OutCd;
  int TrisEstimatedSize = 0;
  bool OutputTriangles = false;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkContourHelper.h
