// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMaskPolyData
 * @brief   sample subset of input polygonal data cells
 *
 * vtkMaskPolyData is a filter that sub-samples the cells of input polygonal
 * data. The user specifies every nth item, with an initial offset to begin
 * sampling.
 *
 * @sa
 * vtkMaskPoints
 */

#ifndef vtkMaskPolyData_h
#define vtkMaskPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkMaskPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkMaskPolyData* New();
  vtkTypeMacro(vtkMaskPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on every nth entity (cell).
   */
  vtkSetClampMacro(OnRatio, int, 1, VTK_INT_MAX);
  vtkGetMacro(OnRatio, int);
  ///@}

  ///@{
  /**
   * Start with this entity (cell).
   */
  vtkSetClampMacro(Offset, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(Offset, vtkIdType);
  ///@}

protected:
  vtkMaskPolyData();
  ~vtkMaskPolyData() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int OnRatio;      // every OnRatio entity is on; all others are off.
  vtkIdType Offset; // offset (or starting point id)

private:
  vtkMaskPolyData(const vtkMaskPolyData&) = delete;
  void operator=(const vtkMaskPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
