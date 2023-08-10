// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSpatioTemporalHarmonicsSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSpatioTemporalHarmonicsSource);

//------------------------------------------------------------------------------
struct vtkSpatioTemporalHarmonicsSource::vtkInternals
{
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> Filter;
};

//------------------------------------------------------------------------------
vtkSpatioTemporalHarmonicsSource::vtkSpatioTemporalHarmonicsSource()
  : Internals(new vtkInternals)
{
  this->SetNumberOfInputPorts(0);
  this->ResetHarmonics();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Whole Extent: (" << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
     << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << ")\n";

  os << indent << "Internal Filter:\n";
  this->Internals->Filter->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::AddHarmonic(double amplitude, double temporalFrequency,
  double xWaveVector, double yWaveVector, double zWaveVector, double phase)
{
  this->Internals->Filter->AddHarmonic(
    amplitude, temporalFrequency, xWaveVector, yWaveVector, zWaveVector, phase);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ClearHarmonics()
{
  this->Internals->Filter->ClearHarmonics();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ResetHarmonics()
{
  this->Internals->Filter->ClearHarmonics();
  this->Internals->Filter->AddHarmonic(1.0, 1.0, 1.0, 0.0, 0.0, 0.0);
  this->Internals->Filter->AddHarmonic(2.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  this->Internals->Filter->AddHarmonic(4.0, 1.0, 0.0, 0.0, 1.0, 0.0);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSpatioTemporalHarmonicsSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(vtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  return 1;
}

//------------------------------------------------------------------------------
int vtkSpatioTemporalHarmonicsSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::GetData(outInfo);
  output->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  if (!this->Internals->Filter->HasHarmonics())
  {
    this->ResetHarmonics();
  }

  this->Internals->Filter->SetInputData(output);
  this->Internals->Filter->Update();

  vtkImageData* filterOutput = this->Internals->Filter->GetImageDataOutput();
  output->ShallowCopy(filterOutput);

  return 1;
}
VTK_ABI_NAMESPACE_END
