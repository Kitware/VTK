// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkForceTime_h
#define vtkForceTime_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSHYBRID_EXPORT vtkForceTime : public vtkPassInputTypeAlgorithm
{
public:
  static vtkForceTime* New();
  vtkTypeMacro(vtkForceTime, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Replace the pipeline time by this one.
  vtkSetMacro(ForcedTime, double);
  vtkGetMacro(ForcedTime, double);

  // Description:
  // Use the ForcedTime. If disabled, use usual pipeline time.
  vtkSetMacro(IgnorePipelineTime, bool);
  vtkGetMacro(IgnorePipelineTime, bool);
  vtkBooleanMacro(IgnorePipelineTime, bool);

protected:
  vtkForceTime();
  ~vtkForceTime() override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkForceTime(const vtkForceTime&) = delete;
  void operator=(const vtkForceTime&) = delete;

  double ForcedTime;
  bool IgnorePipelineTime;
  double PipelineTime;
  bool PipelineTimeFlag;
  vtkDataObject* Cache;
};

VTK_ABI_NAMESPACE_END
#endif // vtkForceTime_h
