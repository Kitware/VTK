// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGrid
 * @brief   Deprecated vtkImageData
 *
 * vtkUniformGrid is an empty subclass of vtkImageData that will be deprecated
 */

#ifndef vtkUniformGrid_h
#define vtkUniformGrid_h

#include "vtkAMRBox.h"                // Fox vtkAMRBox
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImageData.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkEmptyCell;
class vtkUnsignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkUniformGrid : public vtkImageData
{
public:
  ///@{
  /**
   * Construct an empty uniform grid.
   */
  static vtkUniformGrid* New();
  vtkTypeMacro(vtkUniformGrid, vtkImageData);
  ///@}

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_GRID; }

protected:
  vtkUniformGrid();
  ~vtkUniformGrid() override;

private:
  vtkUniformGrid(const vtkUniformGrid&) = delete;
  void operator=(const vtkUniformGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
