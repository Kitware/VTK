// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 */

#ifndef vtkPPolyDataNormals_h
#define vtkPPolyDataNormals_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataNormals.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLEL_EXPORT vtkPPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeMacro(vtkPPolyDataNormals, vtkPolyDataNormals);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPPolyDataNormals* New();

  ///@{
  /**
   * To get piece invariance, this filter has to request an
   * extra ghost level.  By default piece invariance is on.
   */
  vtkSetMacro(PieceInvariant, vtkTypeBool);
  vtkGetMacro(PieceInvariant, vtkTypeBool);
  vtkBooleanMacro(PieceInvariant, vtkTypeBool);
  ///@}

protected:
  vtkPPolyDataNormals();
  ~vtkPPolyDataNormals() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool PieceInvariant;

private:
  vtkPPolyDataNormals(const vtkPPolyDataNormals&) = delete;
  void operator=(const vtkPPolyDataNormals&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
