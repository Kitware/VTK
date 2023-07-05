// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataTangents
 * @brief   compute tangents for triangulated polydata
 *
 * vtkPolyDataTangents is a filter that computes point and/or cell tangents for a triangulated
 * polydata.
 * This filter requires an input with both normals and tcoords on points.
 */

#ifndef vtkPolyDataTangents_h
#define vtkPolyDataTangents_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;
class vtkIdList;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkPolyDataTangents : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPolyDataTangents, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPolyDataTangents* New();

  ///@{
  /**
   * Turn on/off the computation of point tangents.
   * Default is true.
   */
  vtkSetMacro(ComputePointTangents, bool);
  vtkGetMacro(ComputePointTangents, bool);
  vtkBooleanMacro(ComputePointTangents, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the computation of cell tangents.
   * Default is false.
   */
  vtkSetMacro(ComputeCellTangents, bool);
  vtkGetMacro(ComputeCellTangents, bool);
  vtkBooleanMacro(ComputeCellTangents, bool);
  ///@}

protected:
  vtkPolyDataTangents() = default;
  ~vtkPolyDataTangents() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool ComputePointTangents = true;
  bool ComputeCellTangents = false;

private:
  vtkPolyDataTangents(const vtkPolyDataTangents&) = delete;
  void operator=(const vtkPolyDataTangents&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
