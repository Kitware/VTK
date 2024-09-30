// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSpatioTemporalHarmonicsAttribute.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSpatioTemporalHarmonicsAttribute);

namespace
{

const char* ARRAY_NAME = "SpatioTemporalHarmonics";

// The heart of the algorithm plus interface to the SMP tools.
template <class PointArrayT>
struct vtkSpatioTemporalHarmonicsAlgorithm
{
  PointArrayT* PointArray;
  vtkDoubleArray* OutputArray;
  double TimeValue;
  vtkSpatioTemporalHarmonicsAttribute* Filter;

  vtkSpatioTemporalHarmonicsAlgorithm(PointArrayT* pointArray, vtkDoubleArray* outputArray,
    double timeValue, vtkSpatioTemporalHarmonicsAttribute* filter)
    : PointArray(pointArray)
    , OutputArray(outputArray)
    , TimeValue(timeValue)
    , Filter(filter)
  {
  }

  // Interface implicit function computation to SMP tools.
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto pointRange = vtk::DataArrayTupleRange<3>(this->PointArray);
    auto outputRange = vtk::DataArrayValueRange<1>(this->OutputArray);

    using ValueT = typename decltype(pointRange)::ComponentType;

    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType pointId = begin; pointId < end; ++pointId)
    {
      if (pointId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      double coords[3]{ pointRange[pointId][0], pointRange[pointId][1], pointRange[pointId][2] };
      double value = this->Filter->ComputeValue(coords, this->TimeValue);
      outputRange[pointId] = static_cast<ValueT>(value);
    }
  }
};

//------------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
struct SpatioTemporalHarmonicsWorker
{
  template <typename PointArrayT>
  void operator()(PointArrayT* pointArray, vtkDoubleArray* outputArray, double timeValue,
    vtkSpatioTemporalHarmonicsAttribute* filter)
  {
    // Generate samples using SMP tools
    vtkSpatioTemporalHarmonicsAlgorithm<PointArrayT> algo{ pointArray, outputArray, timeValue,
      filter };
    vtkSMPTools::For(0, pointArray->GetNumberOfTuples(), algo);
  }
};

} // end anon namespace

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Harmonics:\n";
  const int nbOctaves = static_cast<int>(this->Amplitudes.size());
  if (nbOctaves > 0)
  {
    os << indent << "Amplitude | Temporal Frequency | Wave Vector | Phase\n";
    for (int i = 0; i < nbOctaves; ++i)
    {
      os << indent << this->Amplitudes.at(i) << " | " << this->TemporalFrequencies.at(i) << " | ("
         << this->WaveVectors.at(i).at(0) << ", " << this->WaveVectors.at(i).at(1) << ", "
         << this->WaveVectors.at(i).at(2) << ") | " << this->Phases.at(i) << "\n";
    }
  }
  else
  {
    os << indent << "None.\n";
  }
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsAttribute::AddHarmonic(double amplitude, double temporalFrequency,
  double xWaveVector, double yWaveVector, double zWaveVector, double phase)
{
  this->Amplitudes.push_back(amplitude);
  this->TemporalFrequencies.push_back(temporalFrequency);
  this->WaveVectors.push_back({ xWaveVector, yWaveVector, zWaveVector });
  this->Phases.push_back(phase);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSpatioTemporalHarmonicsAttribute::ClearHarmonics()
{
  if (!this->Amplitudes.empty() || !this->TemporalFrequencies.empty() ||
    !this->WaveVectors.empty() || !this->Phases.empty())
  {
    this->Amplitudes.clear();
    this->TemporalFrequencies.clear();
    this->WaveVectors.clear();
    this->Phases.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkSpatioTemporalHarmonicsAttribute::HasHarmonics()
{
  return !this->Amplitudes.empty();
}

//------------------------------------------------------------------------------
double vtkSpatioTemporalHarmonicsAttribute::ComputeValue(double coords[3], double time)
{
  const int nbOctaves = static_cast<int>(this->Amplitudes.size());
  double value = 0.0;

  for (int i = 0; i < nbOctaves; ++i)
  {
    double temporalAddend = this->TemporalFrequencies.at(i) * time;
    double spatialAddend = vtkMath::Dot(coords, this->WaveVectors.at(i));
    double sinValue = temporalAddend + spatialAddend + this->Phases.at(i);

    value += Amplitudes.at(i) * std::sin(sinValue);
  }

  return value;
}

//------------------------------------------------------------------------------
int vtkSpatioTemporalHarmonicsAttribute::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (!input || input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  // Copy all the input geometry and data to the output.
  output->ShallowCopy(input);

  // Allocate space for the elevation scalar data.
  vtkIdType nbPts = input->GetNumberOfPoints();
  vtkNew<vtkDoubleArray> newScalars;
  newScalars->SetNumberOfTuples(nbPts);

  // Add the new scalars array to the output.
  newScalars->SetName(ARRAY_NAME);
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars(ARRAY_NAME);

  if (this->Amplitudes.size() != this->TemporalFrequencies.size() ||
    this->Amplitudes.size() != this->WaveVectors.size() ||
    this->Amplitudes.size() != this->Phases.size())
  {
    vtkErrorMacro(
      "Failed to compute harmonics. Harmonics parameters should be specified for each harmonic.");
    return 0;
  }

  if (this->Amplitudes.empty())
  {
    newScalars->FillValue(0.0);
    return 1;
  }

  // Get the current time value
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double timeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  // Create an optimized path for point set input
  vtkPointSet* ps = vtkPointSet::SafeDownCast(input);
  if (ps)
  {
    vtkPoints* points = ps->GetPoints();
    vtkDataArray* pointsArray = points->GetData();

    SpatioTemporalHarmonicsWorker worker; // Entry point to vtkSpatioTemporalHarmonicsAlgorithm

    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(pointsArray, worker, newScalars, timeValue, this))
    { // fallback for unknown arrays and integral value types:
      worker(pointsArray, newScalars, timeValue, this);
    }
  }
  else
  {
    vtkSMPTools::For(0, nbPts,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto outputRange = vtk::DataArrayValueRange<1>(newScalars);
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);

        for (vtkIdType pointId = begin; pointId < end; ++pointId)
        {
          if (pointId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              this->CheckAbort();
            }
            if (this->GetAbortOutput())
            {
              break;
            }
          }

          double coords[3];
          input->GetPoint(pointId, coords);

          outputRange[pointId] = this->ComputeValue(coords, timeValue);
        }
      });
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
