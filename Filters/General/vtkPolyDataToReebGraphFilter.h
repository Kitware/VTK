// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataToReebGraphFilter
 * @brief   generate a Reeb graph from a scalar
 * field defined on a vtkPolyData.
 *
 * The filter will first try to pull as a scalar field the vtkDataArray with
 * Id 'fieldId' of the mesh's vtkPointData.
 * If this field does not exist, the filter will use the vtkElevationFilter to
 * generate a default scalar field.
 */

#ifndef vtkPolyDataToReebGraphFilter_h
#define vtkPolyDataToReebGraphFilter_h

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkReebGraph;

class VTKFILTERSGENERAL_EXPORT vtkPolyDataToReebGraphFilter : public vtkDirectedGraphAlgorithm
{
public:
  static vtkPolyDataToReebGraphFilter* New();
  vtkTypeMacro(vtkPolyDataToReebGraphFilter, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the scalar field id (default = 0).
   */
  vtkSetMacro(FieldId, int);
  vtkGetMacro(FieldId, int);
  ///@}

  vtkReebGraph* GetOutput();

protected:
  vtkPolyDataToReebGraphFilter();
  ~vtkPolyDataToReebGraphFilter() override;

  int FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPolyDataToReebGraphFilter(const vtkPolyDataToReebGraphFilter&) = delete;
  void operator=(const vtkPolyDataToReebGraphFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
