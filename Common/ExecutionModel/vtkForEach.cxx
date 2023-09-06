// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkForEach.h"

#include "vtkExecutionRange.h"

#include "vtkExecutive.h"
#include "vtkTimeRange.h"
#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkInformationObjectBaseKey.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
struct vtkForEach::Internals
{
  Internals()
    : CurrentIteration(0)
  {
    this->Range = vtkSmartPointer<vtkTimeRange>::New();
  }
  vtkSmartPointer<vtkExecutionRange> Range;
  std::size_t CurrentIteration = 0;
  vtkEndFor* EndFor = nullptr;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkForEach);

//------------------------------------------------------------------------------
vtkInformationKeyMacro(vtkForEach, FOR_EACH_FILTER, ObjectBase);

//------------------------------------------------------------------------------
vtkForEach::vtkForEach()
  : Internal(new Internals)
{
}

//------------------------------------------------------------------------------
void vtkForEach::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent.GetNextIndent() << "Range: ";
  if (this->Internal->Range)
  {
    os << std::endl;
    this->Internal->Range->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent.GetNextIndent() << " is empty" << std::endl;
  }
  os << indent.GetNextIndent() << "IsIterating: " << (this->IsIterating() ? "True" : "False")
     << std::endl;
}

//------------------------------------------------------------------------------
void vtkForEach::SetRange(vtkExecutionRange* range)
{
  if (range != this->Internal->Range)
  {
    this->Internal->Range = range;
    this->Internal->CurrentIteration = 0;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkForEach::IsIterating()
{
  return (
    this->Internal->EndFor && this->Internal->CurrentIteration < this->Internal->Range->Size());
}

//------------------------------------------------------------------------------
void vtkForEach::RegisterEndFor(vtkEndFor* endFor)
{
  this->Internal->EndFor = endFor;
}

//------------------------------------------------------------------------------
int vtkForEach::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retieve input information");
    return 0;
  }

  if (inInfo->Has(vtkForEach::FOR_EACH_FILTER()))
  {
    vtkErrorMacro("Input info already seems to have a vtkForEach filter that is not this one. Only "
                  "one allowed at a time.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retrieve output information");
    return 0;
  }

  if (outInfo->Has(vtkForEach::FOR_EACH_FILTER()))
  {
    vtkForEach* pipelineFilter =
      vtkForEach::SafeDownCast(outInfo->Get(vtkForEach::FOR_EACH_FILTER()));
    if (pipelineFilter != this)
    {
      vtkErrorMacro("Output info already seems to have a vtkForEach filter that is not this one. "
                    "Only one allowed at a time.");
      return 0;
    }

    // if this filter is already in output info
    return 1;
  }

  outInfo->Set(vtkForEach::FOR_EACH_FILTER(), this);

  if (!this->Internal->Range)
  {
    vtkErrorMacro("Must set Range before requesting information");
    return 0;
  }

  request->Append(vtkExecutive::KEYS_TO_COPY(), vtkForEach::FOR_EACH_FILTER());

  return this->Internal->Range->RequestInformation(inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkForEach::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Internal->Range)
  {
    vtkErrorMacro("Must set Range before requesting data object");
    return 0;
  }
  return this->Internal->Range->RequestDataObject(inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkForEach::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Internal->Range)
  {
    vtkErrorMacro("Must set Range before requesting update extent");
    return 0;
  }
  return this->Internal->Range->RequestUpdateExtent(
    this->Internal->CurrentIteration, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkForEach::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internal->Range)
  {
    vtkErrorMacro("The Range must be set before running the filter.");
    return 0;
  }

  if (!this->IsIterating())
  {
    this->Internal->CurrentIteration = 0;
  }

  std::cout << "CurrentIteration: " << this->Internal->CurrentIteration << std::endl;

  int res =
    this->Internal->Range->RequestData(this->Internal->CurrentIteration, inputVector, outputVector);
  this->Internal->CurrentIteration++;

  return res;
}

VTK_ABI_NAMESPACE_END
