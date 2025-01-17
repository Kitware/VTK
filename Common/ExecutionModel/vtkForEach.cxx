// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkForEach.h"

#include "vtkExecutionRange.h"

#include "vtkCallbackCommand.h"
#include "vtkDataObject.h"
#include "vtkEndFor.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTimeRange.h"
#include "vtkWeakPointer.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

//------------------------------------------------------------------------------
void RangeModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkForEach*>(clientdata)->Modified();
}

}

//------------------------------------------------------------------------------
struct vtkForEach::Internals
{
  Internals() = default;
  ~Internals() = default;
  vtkSmartPointer<vtkExecutionRange> Range;
  std::size_t CurrentIteration = 0;
  vtkWeakPointer<vtkEndFor> EndFor;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkForEach);

//------------------------------------------------------------------------------
vtkInformationKeyMacro(vtkForEach, FOR_EACH_FILTER, ObjectBase);

//------------------------------------------------------------------------------
vtkForEach::vtkForEach()
  : Internal(new Internals)
{
  this->SetRange(vtkSmartPointer<vtkTimeRange>::New());
}

//------------------------------------------------------------------------------
vtkForEach::~vtkForEach() = default;

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

    // connect the modified method of the range object to this one's
    auto rangeObserver = vtkSmartPointer<vtkCallbackCommand>::New();
    rangeObserver->SetCallback(::RangeModifiedCallback);
    rangeObserver->SetClientData(this);

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
void vtkForEach::Iter()
{
  this->Internal->CurrentIteration++;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkForEach::RegisterEndFor(vtkEndFor* endFor)
{
  this->Internal->EndFor = endFor;
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
int vtkForEach::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retrieve input information");
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
  request->Append(vtkExecutive::KEYS_TO_COPY(), vtkForEach::FOR_EACH_FILTER());

  if (!this->Internal->Range)
  {
    vtkErrorMacro("Must set Range before requesting information");
    return 0;
  }

  return this->Internal->Range->RequestInformation(inputVector, outputVector);
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
    // reset
    this->Internal->CurrentIteration = 0;
  }

  vtkDebugMacro("ForEach Iteration: " << this->Internal->CurrentIteration);

  int res =
    this->Internal->Range->RequestData(this->Internal->CurrentIteration, inputVector, outputVector);

  return res;
}

VTK_ABI_NAMESPACE_END
