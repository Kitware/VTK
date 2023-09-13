// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkRedistributeDataSetToSubCommFilter_h
#define vtkRedistributeDataSetToSubCommFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkSmartPointer.h"              // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN

class vtkMultiProcessController;
class vtkProcessGroup;

class VTKFILTERSPARALLELDIY2_EXPORT vtkRedistributeDataSetToSubCommFilter
  : public vtkDataObjectAlgorithm
{
public:
  static vtkRedistributeDataSetToSubCommFilter* New();
  vtkTypeMacro(vtkRedistributeDataSetToSubCommFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// @brief SetController sets the multi-process controller that owns the whole input dataset
  /// @param vtkMultiProcessController: the multiprocess controller where the input dataset lives
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  /// @brief SetSubGroup sets the multi-process controller group where the output will be aggregated
  /// to. must be a subset of the Controller
  /// @param  vtkProcessGroup* SubGroup: the multi-process controller group where the output will be
  /// aggregated to
  void SetSubGroup(vtkProcessGroup*);
  vtkGetObjectMacro(SubGroup, vtkProcessGroup);

protected:
  vtkRedistributeDataSetToSubCommFilter();
  ~vtkRedistributeDataSetToSubCommFilter();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiProcessController* Controller;
  vtkProcessGroup* SubGroup;
};

VTK_ABI_NAMESPACE_END
#endif
