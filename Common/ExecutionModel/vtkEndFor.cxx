// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEndFor.h"

#include "vtkAggregateToPartitionedDataSetCollection.h"
#include "vtkExecutionAggregator.h"
#include "vtkExecutive.h"
#include "vtkForEach.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <vtkInformationObjectBaseKey.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
struct vtkEndFor::Internals
{
  Internals()
    : Aggregator(vtkSmartPointer<vtkAggregateToPartitionedDataSetCollection>::New())
  {
  }
  vtkSmartPointer<vtkExecutionAggregator> Aggregator;
  vtkForEach* ForEach = nullptr;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkEndFor);

//------------------------------------------------------------------------------
vtkEndFor::vtkEndFor()
  : Internal(new Internals)
{
}

//------------------------------------------------------------------------------
void vtkEndFor::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent.GetNextIndent() << "Aggregator: ";
  if (this->Internal->Aggregator)
  {
    os << std::endl;
    this->Internal->Aggregator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent.GetNextIndent() << "is empty" << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkEndFor::SetAggregator(vtkExecutionAggregator* aggregator)
{
  if (aggregator != this->Internal->Aggregator)
  {
    this->Internal->Aggregator = aggregator;
    this->Internal->Aggregator->Clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkEndFor::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retieve input information");
    return 0;
  }

  if (!inInfo->Has(vtkForEach::FOR_EACH_FILTER()))
  {
    vtkErrorMacro("Input information does not have FOR_EACH_FILTER key. Must put vtkForEach filter "
                  "upstream of vtkEndFor.");
    return 0;
  }

  vtkForEach* forEach = vtkForEach::SafeDownCast(inInfo->Get(vtkForEach::FOR_EACH_FILTER()));
  if (!forEach)
  {
    vtkErrorMacro("Could not retrieve vtkForEach filter from pipeline.");
    return 0;
  }

  this->Internal->ForEach = forEach;
  this->Internal->ForEach->RegisterEndFor(this);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retieve output information");
    return 0;
  }

  if (outInfo->Has(vtkForEach::FOR_EACH_FILTER()))
  {
    outInfo->Remove(vtkForEach::FOR_EACH_FILTER());
    request->Remove(vtkExecutive::KEYS_TO_COPY(), vtkForEach::FOR_EACH_FILTER());
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkEndFor::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retieve input information");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retrieve output information");
    return 0;
  }

  if (!this->Internal->Aggregator)
  {
    vtkErrorMacro("Must set Aggregator before requesting data object");
    return 0;
  }
  vtkSmartPointer<vtkDataObject> output =
    this->Internal->Aggregator->RequestDataObject(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkEndFor::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Internal->Aggregator)
  {
    vtkErrorMacro("Aggregator must be set before running filter.");
    return 0;
  }

  if (!this->Internal->ForEach)
  {
    vtkErrorMacro("Must have a reference to the paired ForEach filter.");
    return 0;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Could not retieve input information");
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  this->Internal->Aggregator->Aggregate(input);

  std::cout << "IsIterating: " << this->Internal->ForEach->IsIterating() << std::endl;
  using SDDP = vtkStreamingDemandDrivenPipeline;
  if (this->Internal->ForEach->IsIterating())
  {
    this->Internal->ForEach->Modified();
    this->GetInputAlgorithm(0, 0)->Update();
    request->Set(SDDP::CONTINUE_EXECUTING(), 1);
    return 1;
  }

  request->Remove(SDDP::CONTINUE_EXECUTING());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Could not retrieve output information");
    return 0;
  }

  auto output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output)
  {
    vtkErrorMacro("Output is nullptr");
    return 0;
  }

  output->ShallowCopy(this->Internal->Aggregator->Output());

  return 1;
}

VTK_ABI_NAMESPACE_END
