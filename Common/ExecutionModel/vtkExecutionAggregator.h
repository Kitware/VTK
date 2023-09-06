// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkExecutionAggregator_h
#define vtkExecutionAggregator_h

#include "vtkObject.h"

#include "vtkCommonExecutionModelModule.h" // for export macro
#include <vtkDataObject.h>                 // for smart pointer signature
#include <vtkSmartPointer.h>               // for smart pointer signature

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExecutionAggregator : public vtkObject
{
public:
  vtkTypeMacro(vtkExecutionAggregator, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  virtual vtkSmartPointer<vtkDataObject> RequestDataObject(vtkDataObject* input);

  virtual bool Aggregate(vtkDataObject* input) = 0;

  virtual vtkSmartPointer<vtkDataObject> Output() = 0;

  virtual void Clear() = 0;

protected:
  vtkExecutionAggregator() = default;
  ~vtkExecutionAggregator() override = default;

private:
  vtkExecutionAggregator(const vtkExecutionAggregator&) = delete;
  void operator=(const vtkExecutionAggregator&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkExecutionAggregator_h
