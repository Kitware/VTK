/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellDataToPointData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPCellDataToPointData
 * @brief   Compute point arrays from cell arrays.
 *
 * This class is deprecated. Use `vtkCellDataToPointData` which now supports
 * the `PieceInvariant` flag.
 */

#ifndef vtkPCellDataToPointData_h
#define vtkPCellDataToPointData_h

#include "vtkCellDataToPointData.h"
#include "vtkDeprecation.h"           // For `VTK_DEPRECATED_IN_9_3_0`
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_3_0(
  "Please use vtkCellDataToPointData instead") VTKFILTERSPARALLEL_EXPORT vtkPCellDataToPointData
  : public vtkCellDataToPointData
{
public:
  vtkTypeMacro(vtkPCellDataToPointData, vtkCellDataToPointData);

  static vtkPCellDataToPointData* New();

protected:
  vtkPCellDataToPointData() = default;
  ~vtkPCellDataToPointData() override = default;

private:
  vtkPCellDataToPointData(const vtkPCellDataToPointData&) = delete;
  void operator=(const vtkPCellDataToPointData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPCellDataToPointData.h
