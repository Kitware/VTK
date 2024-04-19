// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSubdivisionFilter
 * @brief   base class for subvision filters
 *
 * vtkSubdivisionFilter is an abstract class that defines
 * the protocol for subdivision surface filters.
 *
 */

#ifndef vtkSubdivisionFilter_h
#define vtkSubdivisionFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkCellData;
class vtkIdList;
class vtkIntArray;
class vtkPoints;
class vtkPointData;

class VTKFILTERSGENERAL_EXPORT vtkSubdivisionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSubdivisionFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the number of subdivisions.
   * Default is 1.
   */
  vtkSetMacro(NumberOfSubdivisions, int);
  vtkGetMacro(NumberOfSubdivisions, int);
  ///@}

  ///@{
  /**
   * Set/get CheckForTriangles
   * Should subdivision check that the dataset only contains triangles?
   * Default is On (1).
   */
  vtkSetClampMacro(CheckForTriangles, vtkTypeBool, 0, 1);
  vtkGetMacro(CheckForTriangles, vtkTypeBool);
  vtkBooleanMacro(CheckForTriangles, vtkTypeBool);
  ///@}

protected:
  vtkSubdivisionFilter();
  ~vtkSubdivisionFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int NumberOfSubdivisions;
  vtkTypeBool CheckForTriangles;

private:
  vtkSubdivisionFilter(const vtkSubdivisionFilter&) = delete;
  void operator=(const vtkSubdivisionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
