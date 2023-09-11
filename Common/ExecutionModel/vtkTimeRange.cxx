// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTimeRange.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTimeRange);

//------------------------------------------------------------------------------
void vtkTimeRange::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent.GetNextIndent() << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << std::endl;
  os << indent.GetNextIndent() << "TimeValues: [";
  if (!this->TimeValues.empty())
  {
    std::for_each(this->TimeValues.begin(), this->TimeValues.end(),
      [&os](const double& val) { os << " " << val; });
  }
  os << " ]" << std::endl;
}

//------------------------------------------------------------------------------
int vtkTimeRange::RequestInformation(
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!inputVector || !inputVector[0])
  {
    vtkErrorMacro("Input vector is nullptr");
    return 0;
  }

  auto inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Input info is nullptr");
    return 0;
  }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeValues.resize(this->NumberOfTimeSteps, 0.0);
    auto timeValues = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    std::copy(timeValues, timeValues + this->NumberOfTimeSteps, this->TimeValues.begin());
  }
  else
  {
    this->NumberOfTimeSteps = 1;
    this->TimeValues.resize(this->NumberOfTimeSteps, 0.0);
    this->TimeValues[0] = 0.0;
  }

  // Output is no longer temporal
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
int vtkTimeRange::RequestUpdateExtent(std::size_t iteration, vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!inputVector || !inputVector[0])
  {
    vtkErrorMacro("Input vector is nullptr");
    return false;
  }

  auto inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Input info is nullptr");
    return false;
  }

  if (!this->TimeValues.empty())
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeValues[iteration]);
  }

  return 1;
}

//------------------------------------------------------------------------------
std::size_t vtkTimeRange::Size()
{
  return this->NumberOfTimeSteps;
}

VTK_ABI_NAMESPACE_END
