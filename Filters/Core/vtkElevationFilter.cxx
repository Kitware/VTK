// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkElevationFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkElevationFilter);

namespace
{

// The heart of the algorithm plus interface to the SMP tools.
template <class PointArrayT>
struct vtkElevationAlgorithm
{
  using PointType = vtk::GetAPIType<PointArrayT>;
  vtkIdType NumPts;
  double LowPoint[3];
  double HighPoint[3];
  double ScalarRange[2];
  PointArrayT* PointArray;
  float* Scalars;
  const double* V;
  double L2;
  vtkElevationFilter* Filter;

  vtkElevationAlgorithm(
    PointArrayT* pointArray, vtkElevationFilter* filter, float* scalars, const double* v, double l2)
    : NumPts{ pointArray->GetNumberOfTuples() }
    , PointArray{ pointArray }
    , Scalars{ scalars }
    , V{ v }
    , L2{ l2 }
    , Filter(filter)
  {
    filter->GetLowPoint(this->LowPoint);
    filter->GetHighPoint(this->HighPoint);
    filter->GetScalarRange(this->ScalarRange);
  }

  // Interface implicit function computation to SMP tools.
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const double* range = this->ScalarRange;
    const double diffScalar = range[1] - range[0];
    const double* v = this->V;
    const double l2 = this->L2;
    const double* lp = this->LowPoint;

    // input points:
    const auto points = vtk::DataArrayTupleRange<3>(this->PointArray);
    PointType point[3];
    double vec[3];

    const bool isFirst = vtkSMPTools::GetSingleThread();
    const vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
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
      // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
      // we do that since the input points can be implicit points, and GetTypedTuple is faster
      // than accessing the component of the TupleReference using GetTypedComponent internally.
      points.GetTuple(pointId, point);
      vec[0] = point[0] - lp[0];
      vec[1] = point[1] - lp[1];
      vec[2] = point[2] - lp[2];

      const double ns = vtkMath::ClampValue(vtkMath::Dot(vec, v) / l2, 0., 1.);

      // Store the resulting scalar value.
      this->Scalars[pointId] = static_cast<float>(range[0] + ns * diffScalar);
    }
  }
};

//------------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
struct Elevate
{
  template <typename PointArrayT>
  void operator()(
    PointArrayT* pointArray, vtkElevationFilter* filter, double* v, double l2, float* scalars)
  {
    // Okay now generate samples using SMP tools
    vtkElevationAlgorithm<PointArrayT> algo{ pointArray, filter, scalars, v, l2 };
    vtkSMPTools::For(0, pointArray->GetNumberOfTuples(), algo);
  }
};

} // end anon namespace

//------------------------------------------------------------------------------
// Begin the class proper
vtkElevationFilter::vtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;

  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//------------------------------------------------------------------------------
vtkElevationFilter::~vtkElevationFilter() = default;

//------------------------------------------------------------------------------
void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Low Point: (" << this->LowPoint[0] << ", " << this->LowPoint[1] << ", "
     << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", " << this->HighPoint[1] << ", "
     << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", " << this->ScalarRange[1]
     << ")\n";
}

//------------------------------------------------------------------------------
int vtkElevationFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  // Check the size of the input.
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    vtkDebugMacro("No input!");
    return 1;
  }

  // Allocate space for the elevation scalar data.
  vtkSmartPointer<vtkFloatArray> newScalars = vtkSmartPointer<vtkFloatArray>::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system and make sure it is valid.
  double diffVector[3] = { this->HighPoint[0] - this->LowPoint[0],
    this->HighPoint[1] - this->LowPoint[1], this->HighPoint[2] - this->LowPoint[2] };
  double length2 = vtkMath::Dot(diffVector, diffVector);
  if (length2 <= 0)
  {
    vtkErrorMacro("Bad vector, using (0,0,1).");
    diffVector[0] = 0;
    diffVector[1] = 0;
    diffVector[2] = 1;
    length2 = 1.0;
  }

  vtkDebugMacro("Generating elevation scalars!");

  float* scalars = newScalars->GetPointer(0);
  vtkDataArray* pointsArray = input->GetPoints()->GetData();

  Elevate worker; // Entry point to vtkElevationAlgorithm

  // Generate an optimized fast-path for float/double
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  if (!Dispatcher::Execute(pointsArray, worker, this, diffVector, length2, scalars))
  { // fallback for unknown arrays and integral value types:
    worker(pointsArray, this, diffVector, length2, scalars);
  }

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars("Elevation");

  return 1;
}
VTK_ABI_NAMESPACE_END
