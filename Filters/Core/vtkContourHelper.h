// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkContourHelper
 * @brief A utility class used by various contour filters
 *
 * This is a simple utility class that can be used by various contour filters to
 * produce either triangles and/or polygons based on the outputTriangles parameter.
 * If outputTriangles is set to true, trisEstimatedSize is used to allocate memory
 * for temporary triangles created by contouring before merging them.
 * If outputTriangles is set to false, contouring triangles are outputted and
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
  vtkContourHelper(vtkIncrementalPointLocator* locator, vtkCellArray* outVerts,
    vtkCellArray* outLines, vtkCellArray* outPolys, vtkPointData* inPd, vtkCellData* inCd,
    vtkPointData* outPd, vtkCellData* outCd, int trisEstimatedSize, bool outputTriangles);
  ~vtkContourHelper() = default;

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
