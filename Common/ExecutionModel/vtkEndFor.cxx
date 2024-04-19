// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEndFor.h"

#include "vtkAggregateToPartitionedDataSetCollection.h"
#include "vtkCallbackCommand.h"
#include "vtkExecutionAggregator.h"
#include "vtkExecutive.h"
#include "vtkForEach.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkWeakPointer.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

//------------------------------------------------------------------------------
void AggregatorModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkEndFor*>(clientdata)->Modified();
}

}

//------------------------------------------------------------------------------
struct vtkEndFor::Internals
{
  Internals() = default;
  vtkSmartPointer<vtkExecutionAggregator> Aggregator;
  vtkWeakPointer<vtkForEach> ForEach;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkEndFor);

//------------------------------------------------------------------------------
vtkEndFor::vtkEndFor()
  : Internal(new Internals)
{
  this->SetAggregator(vtkSmartPointer<vtkAggregateToPartitionedDataSetCollection>::New());
}

//------------------------------------------------------------------------------
vtkEndFor::~vtkEndFor() = default;

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

    // connect the modified method of the aggregatorobject to this one's
    auto aggregatorObserver = vtkSmartPointer<vtkCallbackCommand>::New();
    aggregatorObserver->SetCallback(::AggregatorModifiedCallback);
    aggregatorObserver->SetClientData(this);

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
    vtkErrorMacro("Could not retrieve input information");
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
    vtkErrorMacro("Could not retrieve output information");
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
    vtkErrorMacro("Could not retrieve input information");
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
    vtkErrorMacro("Could not retrieve input information");
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  this->Internal->Aggregator->Aggregate(input);

  using SDDP = vtkStreamingDemandDrivenPipeline;
  if (this->Internal->ForEach->IsIterating())
  {
    // We need to "touch" the top of the sub-pipeline we want
    // to loop.
    this->Internal->ForEach->Modified();

    if (SDDP::SafeDownCast(this->GetExecutive()))
    {
      // Tell the executive that we want to continue
      // the current execution so the pipeline can loop
      request->Set(SDDP::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      // basic executive do not handle CONTINUE_EXECUTING,
      // so we fallback on recursive call
      this->GetInputAlgorithm(0, 0)->Update();
    }
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

  output->ShallowCopy(this->Internal->Aggregator->GetOutputDataObject());

  // reclaim unused memory
  this->Internal->Aggregator->Clear();

  return 1;
}

VTK_ABI_NAMESPACE_END
