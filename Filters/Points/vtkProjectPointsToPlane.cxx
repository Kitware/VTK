// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProjectPointsToPlane.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProjectPointsToPlane);

// Projection algorithms
namespace
{ // anonymous

// Dispatching real types; use a slow path otherwise
using Reals = vtkArrayDispatch::Reals;
using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;

// Project onto a coordinate plane. Coordinate plane defined by xi[idx[2]] = pc.
// (idx[0], idx[1]) refer to in-plane coordinates; idx[2] refers to the fixed
// coordinate.
struct ProjectToCoordinatePlaneWorker
{
  template <class InArrayT, class OutArrayT>
  void operator()(InArrayT* in, OutArrayT* out, vtkIdType numPts, int idx[3], double pc)
  {
    const auto ipts = vtk::DataArrayTupleRange<3>(in);
    auto opts = vtk::DataArrayTupleRange<3>(out);

    vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
      for (; ptId < endPtId; ++ptId)
      {
        const auto xi = ipts[ptId];
        auto xo = opts[ptId];

        xo[idx[0]] = xi[idx[0]];
        xo[idx[1]] = xi[idx[1]];
        xo[idx[2]] = pc;
      }
    }); // lambda
  }
};

// Project a set of input points to a set of output points which are constrained to lie on a
// coordinate plane.
void ProjectToCoordinatePlane(
  vtkIdType numPts, vtkPoints* inPts, int idx[3], double pc, vtkPoints* newPts)
{
  ProjectToCoordinatePlaneWorker worker;
  if (!Dispatcher::Execute(inPts->GetData(), newPts->GetData(), worker, numPts, idx, pc))
  { // fallback for unknown arrays and integral value types:
    worker(inPts->GetData(), newPts->GetData(), numPts, idx, pc);
  }
}

// Given an input set of points, fit a plane to the points. This means producing a plane
// origin and normal.
void FitPlane(vtkPoints* inPts, double o[3], double n[3])
{
  vtkPlane::ComputeBestFittingPlane(inPts, o, n);
}

// Project points onto a specified plane
struct ProjectToPlaneWorker
{
  template <class InArrayT, class OutArrayT>
  void operator()(InArrayT* in, OutArrayT* out, vtkIdType numPts, double o[3], double n[3])
  {
    const auto ipts = vtk::DataArrayTupleRange<3>(in);
    auto opts = vtk::DataArrayTupleRange<3>(out);

    vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
      double x[3], xProj[3];
      for (; ptId < endPtId; ++ptId)
      {
        const auto xi = ipts[ptId];
        auto xo = opts[ptId];

        x[0] = xi[0];
        x[1] = xi[1];
        x[2] = xi[2];

        vtkPlane::ProjectPoint(x, o, n, xProj);

        xo[0] = xProj[0];
        xo[1] = xProj[1];
        xo[2] = xProj[2];
      }
    }); // lambda
  }
};

// Project a set of input points to a set of output points which lie on the plane defined by
// an origin point and normal.
void ProjectToPlane(vtkIdType numPts, vtkPoints* inPts, vtkPoints* newPts, double o[3], double n[3])
{
  ProjectToPlaneWorker worker;
  if (!Dispatcher::Execute(inPts->GetData(), newPts->GetData(), worker, numPts, o, n))
  { // fallback for unknown arrays and integral value types:
    worker(inPts->GetData(), newPts->GetData(), numPts, o, n);
  }
}

// Determine which coordinate plane is most orthogonal to the specified normal.
void ComputeNormalAxis(double n[3], int idx[3])
{
  if (fabs(n[0]) > fabs(n[1]))
  {
    if (fabs(n[0]) > fabs(n[2]))
    {
      idx[0] = 1;
      idx[1] = 2;
      idx[2] = 0;
    }
    else
    {
      idx[0] = 0;
      idx[1] = 1;
      idx[2] = 2;
    }
  }
  else
  {
    if (fabs(n[1]) > fabs(n[2]))
    {
      idx[0] = 0;
      idx[1] = 2;
      idx[2] = 1;
    }
    else
    {
      idx[0] = 0;
      idx[1] = 1;
      idx[2] = 2;
    }
  }
}

} // anonymous

//------------------------------------------------------------------------------
vtkProjectPointsToPlane::vtkProjectPointsToPlane()
{
  this->ProjectionType = Z_PLANE;

  std::fill_n(this->Origin, 3, 0.0);

  this->Normal[0] = this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
int vtkProjectPointsToPlane::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // (Shallow) copy everything over, then replace the points later
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Make sure there is something to process.
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPoints* inPts = input->GetPoints();
  if (!inPts || numPts <= 0)
  {
    return 1;
  }

  // Instantiate some new points of the right type.
  vtkNew<vtkPoints> newPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numPts);

  // Depending on the nature of the projection, we'll produce new points via
  // two different threaded paths.
  // See if we can project to a x-y-z coordinate plane.
  if (this->ProjectionType <= Z_PLANE)
  {
    int idx[3];
    std::fill_n(this->Origin, 3, 0.0);
    std::fill_n(this->Normal, 3, 0.0);
    switch (this->ProjectionType)
    {
      case X_PLANE:
        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 0;
        break;

      case Y_PLANE:
        idx[0] = 0;
        idx[1] = 2;
        idx[2] = 1;
        break;

      case Z_PLANE:
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 2;
        break;
    }
    this->Normal[idx[2]] = 1.0;
    ProjectToCoordinatePlane(numPts, inPts, idx, 0.0, newPts);
  }

  // Else project to an oriented plane (origin,normal), or to coordinate
  // plane passing through the origin of the plane.
  else
  {
    double o[3], n[3];
    int idx[3];
    switch (this->ProjectionType)
    {
      case SPECIFIED_PLANE:
        std::copy_n(this->Origin, 3, o);
        std::copy_n(this->Normal, 3, n);
        ProjectToPlane(numPts, inPts, newPts, o, n);
        break;

      case BEST_FIT_PLANE:
        FitPlane(inPts, o, n);
        std::copy_n(o, 3, this->Origin);
        std::copy_n(n, 3, this->Normal);
        ProjectToPlane(numPts, inPts, newPts, o, n);
        break;

      case BEST_COORDINATE_PLANE:
        FitPlane(inPts, o, n);
        std::copy_n(o, 3, this->Origin);
        ComputeNormalAxis(n, idx);
        this->Normal[idx[0]] = this->Normal[idx[1]] = 0.0;
        this->Normal[idx[2]] = 1.0;
        ProjectToCoordinatePlane(numPts, inPts, idx, this->Origin[idx[2]], newPts);
        break;
    }
  }

  // It's a wrap
  output->SetPoints(newPts);

  return 1;
}

//------------------------------------------------------------------------------
void vtkProjectPointsToPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane Projection Type: " << this->ProjectionType << "\n";

  os << indent << "Origin: (" << this->Origin[0] << "," << this->Origin[1] << "," << this->Origin[2]
     << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << "," << this->Normal[1] << "," << this->Normal[2]
     << ")\n";

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
