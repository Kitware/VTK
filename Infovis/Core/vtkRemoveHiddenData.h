// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRemoveHiddenData
 * @brief   Removes the rows/edges/vertices of
 * input data flagged by ann.
 *
 *
 * Output only those rows/vertices/edges of the input vtkDataObject that
 * are visible, as defined by the vtkAnnotation::HIDE() flag of the input
 * vtkAnnotationLayers.
 * Inputs:
 *    Port 0 - vtkDataObject
 *    Port 1 - vtkAnnotationLayers (optional)
 *
 */

#ifndef vtkRemoveHiddenData_h
#define vtkRemoveHiddenData_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h" // For Smartpointer

VTK_ABI_NAMESPACE_BEGIN
class vtkExtractSelectedGraph;
class vtkExtractSelectedRows;

class VTKINFOVISCORE_EXPORT vtkRemoveHiddenData : public vtkPassInputTypeAlgorithm
{
public:
  static vtkRemoveHiddenData* New();
  vtkTypeMacro(vtkRemoveHiddenData, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkRemoveHiddenData();
  ~vtkRemoveHiddenData() override;

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkRemoveHiddenData(const vtkRemoveHiddenData&) = delete;
  void operator=(const vtkRemoveHiddenData&) = delete;

  vtkSmartPointer<vtkExtractSelectedGraph> ExtractGraph;
  vtkSmartPointer<vtkExtractSelectedRows> ExtractTable;
};

VTK_ABI_NAMESPACE_END
#endif
