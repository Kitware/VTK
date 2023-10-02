// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExecutionRange.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkExecutionRange::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkExecutionRange::RequestDataObject(
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!inputVector && !inputVector[0])
  {
    vtkErrorMacro("No input information vectors");
    return 0;
  }

  auto inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro("No input information");
    return 0;
  }

  auto input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!input)
  {
    return 1;
  }
  vtkSmartPointer<vtkDataObject> output = vtk::TakeSmartPointer(input->NewInstance());

  if (!outputVector)
  {
    vtkErrorMacro("No output vector");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retrieve output information");
    return 0;
  }

  if (output)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkExecutionRange::RequestInformation(
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkExecutionRange::RequestUpdateExtent(std::size_t vtkNotUsed(iteration),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkExecutionRange::RequestData(std::size_t vtkNotUsed(iteration),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!inputVector && !inputVector[0])
  {
    vtkErrorMacro("No input information vectors");
    return 0;
  }

  auto inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro("No input information");
    return 0;
  }

  auto input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkSmartPointer<vtkDataObject> output = vtk::TakeSmartPointer(input->NewInstance());
  output->ShallowCopy(input);

  if (!outputVector)
  {
    vtkErrorMacro("No output vector");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retrieve output information");
    return 0;
  }

  if (output)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
  }

  return 1;
}

//------------------------------------------------------------------------------
std::size_t vtkExecutionRange::Size()
{
  vtkWarningMacro("This is the default implementation for Size, will return just 1.");
  return 1;
}

VTK_ABI_NAMESPACE_END
