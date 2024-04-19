// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyLineSource
 * @brief   create a poly line from a list of input points
 *
 * vtkPolyLineSource is a source object that creates a poly line from
 * user-specified points. The output is a vtkPolyLine.
 */

#ifndef vtkPolyLineSource_h
#define vtkPolyLineSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyPointSource.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkPolyLineSource : public vtkPolyPointSource
{
public:
  static vtkPolyLineSource* New();
  vtkTypeMacro(vtkPolyLineSource, vtkPolyPointSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set whether to close the poly line by connecting the last and first points.
   */
  vtkSetMacro(Closed, vtkTypeBool);
  vtkGetMacro(Closed, vtkTypeBool);
  vtkBooleanMacro(Closed, vtkTypeBool);
  ///@}

protected:
  vtkPolyLineSource();
  ~vtkPolyLineSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool Closed;

private:
  vtkPolyLineSource(const vtkPolyLineSource&) = delete;
  void operator=(const vtkPolyLineSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
