// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPOutlineFilter.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPOutlineFilterInternals.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPOutlineFilter);
vtkCxxSetObjectMacro(vtkPOutlineFilter, Controller, vtkMultiProcessController);

vtkPOutlineFilter::vtkPOutlineFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->OutlineSource = vtkOutlineSource::New();
}

vtkPOutlineFilter::~vtkPOutlineFilter()
{
  this->SetController(nullptr);
  if (this->OutlineSource != nullptr)
  {
    this->OutlineSource->Delete();
    this->OutlineSource = nullptr;
  }
}

int vtkPOutlineFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPOutlineFilterInternals internals;
  internals.SetIsCornerSource(false);
  internals.SetController(this->Controller);

  return internals.RequestData(request, inputVector, outputVector);
}

int vtkPOutlineFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

void vtkPOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
