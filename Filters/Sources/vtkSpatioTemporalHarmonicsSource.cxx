// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSpatioTemporalHarmonicsSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSpatioTemporalHarmonicsSource);

namespace
{
constexpr int NB_TIMESTEPS = 20;
constexpr double VALUE_STEP =
  2.0 * vtkMath::Pi() / NB_TIMESTEPS; // So that a loop get back to initial state

constexpr double VECTOR_LENGTH =
  vtkMath::Pi() / 5.0; // To have two periods, based on default extent
constexpr int NB_HARMONICS = 4;
constexpr double AMPLITUDES[NB_HARMONICS] = { 1.0, 3.0, 2.0, 1.0 };
constexpr double TEMPORAL_FREQUENCIES[NB_HARMONICS] = { 1.0, 1.0, 2.0, 3.0 };
constexpr double X_WAVE_VECTORS[NB_HARMONICS] = { VECTOR_LENGTH, VECTOR_LENGTH, 0.0, 0.0 };
constexpr double Y_WAVE_VECTORS[NB_HARMONICS] = { VECTOR_LENGTH, 0.0, VECTOR_LENGTH, 0.0 };
constexpr double Z_WAVE_VECTORS[NB_HARMONICS] = { VECTOR_LENGTH, 0.0, 0.0, VECTOR_LENGTH };
constexpr double PHASES[NB_HARMONICS] = { 0.0, vtkMath::Pi() / 2.0, vtkMath::Pi(),
  3.0 * vtkMath::Pi() / 2.0 };
}

//------------------------------------------------------------------------------
struct vtkSpatioTemporalHarmonicsSource::vtkInternals
{
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> HarmonicsFilter;
  std::vector<double> TimeStepValues;
};

//------------------------------------------------------------------------------
vtkSpatioTemporalHarmonicsSource::vtkSpatioTemporalHarmonicsSource()
  : Internals(new vtkInternals)
{
  this->SetNumberOfInputPorts(0);
  this->ResetHarmonics();
  this->ResetTimeStepValues();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Whole Extent: (" << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
     << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << ")\n";

  os << indent << "Time Step Values:\n";
  const int nbTimeSteps = static_cast<int>(this->Internals->TimeStepValues.size());
  if (nbTimeSteps > 0)
  {
    for (int i = 0; i < nbTimeSteps; ++i)
    {
      os << indent << this->Internals->TimeStepValues.at(i) << "\n";
    }
  }
  else
  {
    os << indent << "None.\n";
  }

  os << indent << "Internal Harmonics Filter:\n";
  this->Internals->HarmonicsFilter->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::AddTimeStepValue(double timeStepValue)
{
  this->Internals->TimeStepValues.emplace_back(timeStepValue);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ClearTimeStepValues()
{
  this->Internals->TimeStepValues.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ResetTimeStepValues()
{
  this->Internals->TimeStepValues.clear();
  this->Internals->TimeStepValues.resize(NB_TIMESTEPS);

  double timeValue = 0.0;
  std::for_each(this->Internals->TimeStepValues.begin(), this->Internals->TimeStepValues.end(),
    [&timeValue](double& time) {
      time = timeValue;
      timeValue += VALUE_STEP;
    });

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::AddHarmonic(double amplitude, double temporalFrequency,
  double xWaveVector, double yWaveVector, double zWaveVector, double phase)
{
  this->Internals->HarmonicsFilter->AddHarmonic(
    amplitude, temporalFrequency, xWaveVector, yWaveVector, zWaveVector, phase);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ClearHarmonics()
{
  this->Internals->HarmonicsFilter->ClearHarmonics();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsSource::ResetHarmonics()
{
  this->Internals->HarmonicsFilter->ClearHarmonics();
  for (int i = 0; i < ::NB_HARMONICS; ++i)
  {
    this->Internals->HarmonicsFilter->AddHarmonic(::AMPLITUDES[i], ::TEMPORAL_FREQUENCIES[i],
      ::X_WAVE_VECTORS[i], ::Y_WAVE_VECTORS[i], ::Z_WAVE_VECTORS[i], ::PHASES[i]);
  }
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
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  if (this->Internals->TimeStepValues.empty())
  {
    // In case we re-apply the source with no time steps
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  else
  {
    std::sort(this->Internals->TimeStepValues.begin(), this->Internals->TimeStepValues.end());

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      this->Internals->TimeStepValues.data(),
      static_cast<int>(this->Internals->TimeStepValues.size()));

    double range[2] = { this->Internals->TimeStepValues[0],
      this->Internals->TimeStepValues[this->Internals->TimeStepValues.size() - 1] };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkSpatioTemporalHarmonicsSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double timeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  vtkImageData* output = vtkImageData::GetData(outInfo);
  output->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  this->Internals->HarmonicsFilter->SetInputData(output);
  this->Internals->HarmonicsFilter->UpdateTimeStep(timeValue);
  this->Internals->HarmonicsFilter->Update();

  vtkImageData* filterOutput = this->Internals->HarmonicsFilter->GetImageDataOutput();
  output->ShallowCopy(filterOutput);

  return 1;
}
VTK_ABI_NAMESPACE_END
