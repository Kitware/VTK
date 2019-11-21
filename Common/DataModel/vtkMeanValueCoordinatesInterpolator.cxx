/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeanValueCoordinatesInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkObjectFactory.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkConfigure.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkPoints.h"

#include <algorithm>
#include <numeric>
#include <vector>

vtkStandardNewMacro(vtkMeanValueCoordinatesInterpolator);

// Special class that can iterate over different type of triangle representations
// This is needed since we may be provided just a connectivity vtkIdList instead
// of a vtkCellArray.
class vtkMVCTriIterator
{
public:
  vtkIdType Offset;
  vtkIdType* Tris;
  vtkIdType* Current;
  vtkIdType NumberOfTriangles;
  vtkIdType Id;

  vtkMVCTriIterator(vtkIdType numIds, vtkIdType offset, vtkIdType* t)
  {
    this->Offset = offset;
    this->Tris = t;
    this->Current = t + (this->Offset - 3); // leave room for three indices
    this->NumberOfTriangles = numIds / offset;
    this->Id = 0;
  }

  const vtkIdType* operator++()
  {
    this->Current += this->Offset;
    this->Id++;
    return this->Current;
  }
};

// Special class that can iterate over different type of polygon representations
class vtkMVCPolyIterator
{
public:
  vtkSmartPointer<vtkCellArrayIterator> Iter;
  vtkIdType CurrentPolygonSize;
  const vtkIdType* Current;
  vtkIdType Id;
  vtkIdType MaxPolygonSize;
  vtkIdType NumberOfPolygons;

  vtkMVCPolyIterator(vtkCellArray* cells)
  {
    this->NumberOfPolygons = cells->GetNumberOfCells();
    this->MaxPolygonSize = cells->GetMaxCellSize();
    this->Iter = vtk::TakeSmartPointer(cells->NewIterator());
    this->Iter->GoToFirstCell();
    if (!this->Iter->IsDoneWithTraversal())
    {
      this->Iter->GetCurrentCell(this->CurrentPolygonSize, this->Current);
    }
    else
    {
      this->CurrentPolygonSize = 0;
      this->Current = nullptr;
    }
    this->Id = this->Iter->GetCurrentCellId();
  }

  const vtkIdType* operator++()
  {
    this->Iter->GoToNextCell();
    if (!this->Iter->IsDoneWithTraversal())
    {
      this->Iter->GetCurrentCell(this->CurrentPolygonSize, this->Current);
    }
    else
    {
      this->CurrentPolygonSize = 0;
      this->Current = nullptr;
    }
    this->Id = this->Iter->GetCurrentCellId();
    return this->Current;
  }
};

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkMeanValueCoordinatesInterpolator::vtkMeanValueCoordinatesInterpolator() = default;

//----------------------------------------------------------------------------
vtkMeanValueCoordinatesInterpolator::~vtkMeanValueCoordinatesInterpolator() = default;

namespace
{

//----------------------------------------------------------------------------
// Templated function to generate weights of a general polygonal mesh.
// This class actually implements the algorithm.
struct ComputeWeightsForPolygonMesh
{
  template <typename PtArrayT>
  void operator()(PtArrayT* ptArray, const double x[3], vtkMVCPolyIterator& iter, double* weights)
  {
    const auto points = vtk::DataArrayTupleRange<3>(ptArray);
    const vtkIdType npts = points.size();

    // Begin by initializing weights.
    std::fill_n(weights, static_cast<size_t>(npts), 0.);

    // create local array for storing point-to-vertex vectors and distances
    std::vector<double> dist(npts);
    std::vector<double> uVec(3 * npts);
    static constexpr double eps = 0.00000001;

    for (vtkIdType pid = 0; pid < npts; ++pid)
    {
      // point-to-vertex vector
      const auto pt = points[pid];
      uVec[3 * pid] = pt[0] - x[0];
      uVec[3 * pid + 1] = pt[1] - x[1];
      uVec[3 * pid + 2] = pt[2] - x[2];

      // distance
      dist[pid] = vtkMath::Norm(uVec.data() + 3 * pid);

      // handle special case when the point is really close to a vertex
      if (dist[pid] < eps)
      {
        weights[pid] = 1.0;
        return;
      }

      // project onto unit sphere
      uVec[3 * pid] /= dist[pid];
      uVec[3 * pid + 1] /= dist[pid];
      uVec[3 * pid + 2] /= dist[pid];
    }

    // Now loop over all triangle to compute weights
    std::vector<double*> u(iter.MaxPolygonSize);
    std::vector<double> alpha(iter.MaxPolygonSize);
    std::vector<double> theta(iter.MaxPolygonSize);

    const vtkIdType* poly = iter.Current;
    while (iter.Id < iter.NumberOfPolygons)
    {
      int nPolyPts = iter.CurrentPolygonSize;

      for (int j = 0; j < nPolyPts; j++)
      {
        u[j] = uVec.data() + 3 * poly[j];
      }

      // unit vector v.
      double v[3] = { 0., 0., 0. };
      double l;
      double angle;
      double temp[3];
      for (int j = 0; j < nPolyPts - 1; j++)
      {
        vtkMath::Cross(u[j], u[j + 1], temp);
        vtkMath::Normalize(temp);

        l = sqrt(vtkMath::Distance2BetweenPoints(u[j], u[j + 1]));
        angle = 2.0 * asin(l / 2.0);

        v[0] += 0.5 * angle * temp[0];
        v[1] += 0.5 * angle * temp[1];
        v[2] += 0.5 * angle * temp[2];
      }
      l = sqrt(vtkMath::Distance2BetweenPoints(u[nPolyPts - 1], u[0]));
      angle = 2.0 * asin(l / 2.0);
      vtkMath::Cross(u[nPolyPts - 1], u[0], temp);
      vtkMath::Normalize(temp);
      v[0] += 0.5 * angle * temp[0];
      v[1] += 0.5 * angle * temp[1];
      v[2] += 0.5 * angle * temp[2];

      const double vNorm = vtkMath::Normalize(v);

      // The direction of v depends on the orientation (clockwise or
      // contour-clockwise) of the polygon. We want to make sure that v
      // starts from x and point towards the polygon.
      if (vtkMath::Dot(v, u[0]) < 0)
      {
        v[0] = -v[0];
        v[1] = -v[1];
        v[2] = -v[2];
      }

      // angles between edges
      double n0[3], n1[3];
      for (int j = 0; j < nPolyPts - 1; j++)
      {
        // alpha
        vtkMath::Cross(u[j], v, n0);
        vtkMath::Normalize(n0);
        vtkMath::Cross(u[j + 1], v, n1);
        vtkMath::Normalize(n1);

        // alpha[j] = acos(vtkMath::Dot(n0, n1));
        l = sqrt(vtkMath::Distance2BetweenPoints(n0, n1));
        alpha[j] = 2.0 * asin(l / 2.0);
        vtkMath::Cross(n0, n1, temp);
        if (vtkMath::Dot(temp, v) < 0)
        {
          alpha[j] = -alpha[j];
        }

        // theta_j
        // theta[j] = acos(vtkMath::Dot(u[j], v));
        l = sqrt(vtkMath::Distance2BetweenPoints(u[j], v));
        theta[j] = 2.0 * asin(l / 2.0);
      }

      vtkMath::Cross(u[nPolyPts - 1], v, n0);
      vtkMath::Normalize(n0);
      vtkMath::Cross(u[0], v, n1);
      vtkMath::Normalize(n1);
      // alpha[nPolyPts-1] = acos(vtkMath::Dot(n0, n1));
      l = sqrt(vtkMath::Distance2BetweenPoints(n0, n1));
      alpha[nPolyPts - 1] = 2.0 * asin(l / 2.0);
      vtkMath::Cross(n0, n1, temp);
      if (vtkMath::Dot(temp, v) < 0)
      {
        alpha[nPolyPts - 1] = -alpha[nPolyPts - 1];
      }

      // theta[nPolyPts-1] = acos(vtkMath::Dot(u[nPolyPts-1], v));
      l = sqrt(vtkMath::Distance2BetweenPoints(u[nPolyPts - 1], v));
      theta[nPolyPts - 1] = 2.0 * asin(l / 2.0);

      bool outlierFlag = false;
      for (int j = 0; j < nPolyPts; j++)
      {
        if (fabs(theta[j]) < eps)
        {
          outlierFlag = true;
          weights[poly[j]] += vNorm / dist[poly[j]];
          break;
        }
      }

      if (outlierFlag)
      {
        poly = ++iter;
        continue;
      }

      double sum = 0.0;
      sum += 1.0 / tan(theta[0]) * (tan(alpha[0] / 2.0) + tan(alpha[nPolyPts - 1] / 2.0));
      for (int j = 1; j < nPolyPts; j++)
      {
        sum += 1.0 / tan(theta[j]) * (tan(alpha[j] / 2.0) + tan(alpha[j - 1] / 2.0));
      }

      // the special case when x lies on the polygon, handle it using 2D mvc.
      // in the 2D case, alpha = theta
      if (fabs(sum) < eps)
      {
        std::fill_n(weights, static_cast<size_t>(npts), 0.0);

        // recompute theta, the theta computed previously are not robust
        for (int j = 0; j < nPolyPts - 1; j++)
        {
          l = sqrt(vtkMath::Distance2BetweenPoints(u[j], u[j + 1]));
          theta[j] = 2.0 * asin(l / 2.0);
        }
        l = sqrt(vtkMath::Distance2BetweenPoints(u[nPolyPts - 1], u[0]));
        theta[nPolyPts - 1] = 2.0 * asin(l / 2.0);

        double sumWeight;
        weights[poly[0]] =
          1.0 / dist[poly[0]] * (tan(theta[nPolyPts - 1] / 2.0) + tan(theta[0] / 2.0));
        sumWeight = weights[poly[0]];
        for (int j = 1; j < nPolyPts; j++)
        {
          weights[poly[j]] = 1.0 / dist[poly[j]] * (tan(theta[j - 1] / 2.0) + tan(theta[j] / 2.0));
          sumWeight = sumWeight + weights[poly[j]];
        }

        if (sumWeight < eps)
        {
          return;
        }

        for (int j = 0; j < nPolyPts; j++)
        {
          weights[poly[j]] /= sumWeight;
        }

        return;
      }

      // weight
      weights[poly[0]] += vNorm / sum / dist[poly[0]] / sin(theta[0]) *
        (tan(alpha[0] / 2.0) + tan(alpha[nPolyPts - 1] / 2.0));
      for (int j = 1; j < nPolyPts; j++)
      {
        weights[poly[j]] += vNorm / sum / dist[poly[j]] / sin(theta[j]) *
          (tan(alpha[j] / 2.0) + tan(alpha[j - 1] / 2.0));
      }

      // next iteration
      poly = ++iter;
    }

    // normalize weight
    double* weightsEnd = weights + static_cast<size_t>(npts);
    const double sumWeight = std::accumulate(weights, weightsEnd, 0.);

    if (fabs(sumWeight) < eps)
    {
      return;
    }

    std::for_each(weights, weightsEnd, [&](double& w) { w /= sumWeight; });
  }
};

//----------------------------------------------------------------------------
// Templated function to generate weights of a triangle mesh.
// This class actually implements the algorithm.
struct ComputeWeightsForTriangleMesh
{
  template <typename PtArrayT>
  void operator()(PtArrayT* ptArray, const double x[3], vtkMVCTriIterator& iter, double* weights)
  {
    // Points are organized {(x,y,z), (x,y,z), ....}
    // Tris are organized {(i,j,k), (i,j,k), ....}
    // Weights per point are computed

    const auto points = vtk::DataArrayTupleRange<3>(ptArray);
    const vtkIdType npts = points.size();

    double* const weightsEnd = weights + static_cast<size_t>(npts);

    // Begin by initializing weights.
    std::fill(weights, weightsEnd, 0.);

    // create local array for storing point-to-vertex vectors and distances
    std::vector<double> dist(static_cast<size_t>(npts));
    std::vector<double> uVec(static_cast<size_t>(3 * npts));
    static constexpr double eps = 0.000000001;
    for (vtkIdType pid = 0; pid < npts; ++pid)
    {
      // point-to-vertex vector
      auto pt = points[pid];
      uVec[3 * pid] = pt[0] - x[0];
      uVec[3 * pid + 1] = pt[1] - x[1];
      uVec[3 * pid + 2] = pt[2] - x[2];

      // distance
      dist[pid] = vtkMath::Norm(uVec.data() + 3 * pid);

      // handle special case when the point is really close to a vertex
      if (dist[pid] < eps)
      {
        weights[pid] = 1.0;
        return;
      }

      // project onto unit sphere
      uVec[3 * pid] /= dist[pid];
      uVec[3 * pid + 1] /= dist[pid];
      uVec[3 * pid + 2] /= dist[pid];
    }

    // Now loop over all triangle to compute weights
    while (iter.Id < iter.NumberOfTriangles)
    {
      // vertex id
      vtkIdType pid0 = iter.Current[0];
      vtkIdType pid1 = iter.Current[1];
      vtkIdType pid2 = iter.Current[2];

      // unit vector
      double* u0 = uVec.data() + 3 * pid0;
      double* u1 = uVec.data() + 3 * pid1;
      double* u2 = uVec.data() + 3 * pid2;

      // edge length
      double l0 = sqrt(vtkMath::Distance2BetweenPoints(u1, u2));
      double l1 = sqrt(vtkMath::Distance2BetweenPoints(u2, u0));
      double l2 = sqrt(vtkMath::Distance2BetweenPoints(u0, u1));

      // angle
      double theta0 = 2.0 * asin(l0 / 2.0);
      double theta1 = 2.0 * asin(l1 / 2.0);
      double theta2 = 2.0 * asin(l2 / 2.0);
      double halfSum = (theta0 + theta1 + theta2) / 2.0;

      // special case when the point lies on the triangle
      if (vtkMath::Pi() - halfSum < eps)
      {
        std::fill(weights, weightsEnd, 0.);

        weights[pid0] = sin(theta0) * dist[pid1] * dist[pid2];
        weights[pid1] = sin(theta1) * dist[pid2] * dist[pid0];
        weights[pid2] = sin(theta2) * dist[pid0] * dist[pid1];

        double sumWeight = weights[pid0] + weights[pid1] + weights[pid2];

        weights[pid0] /= sumWeight;
        weights[pid1] /= sumWeight;
        weights[pid2] /= sumWeight;

        return;
      }

      // coefficient
      double sinHalfSum = sin(halfSum);
      double sinHalfSumSubTheta0 = sin(halfSum - theta0);
      double sinHalfSumSubTheta1 = sin(halfSum - theta1);
      double sinHalfSumSubTheta2 = sin(halfSum - theta2);
      double sinTheta0 = sin(theta0);
      double sinTheta1 = sin(theta1);
      double sinTheta2 = sin(theta2);

      double c0 = 2 * sinHalfSum * sinHalfSumSubTheta0 / sinTheta1 / sinTheta2 - 1;
      double c1 = 2 * sinHalfSum * sinHalfSumSubTheta1 / sinTheta2 / sinTheta0 - 1;
      double c2 = 2 * sinHalfSum * sinHalfSumSubTheta2 / sinTheta0 / sinTheta1 - 1;

      if (fabs(c0) > 1)
      {
        c0 = c0 > 0 ? 1 : -1;
      }
      if (fabs(c1) > 1)
      {
        c1 = c1 > 0 ? 1 : -1;
      }
      if (fabs(c2) > 1)
      {
        c2 = c2 > 0 ? 1 : -1;
      }

      // sign
      double det = vtkMath::Determinant3x3(u0, u1, u2);

      if (fabs(det) < eps)
      {
        ++iter;
        continue;
      }

      double detSign = det > 0 ? 1 : -1;
      double sign0 = detSign * sqrt(1 - c0 * c0);
      double sign1 = detSign * sqrt(1 - c1 * c1);
      double sign2 = detSign * sqrt(1 - c2 * c2);

      // if x lies on the plane of current triangle but outside it, ignore
      // the current triangle.
      if (fabs(sign0) < eps || fabs(sign1) < eps || fabs(sign2) < eps)
      {
        ++iter;
        continue;
      }

      // weight
      weights[pid0] += (theta0 - c1 * theta2 - c2 * theta1) / (dist[pid0] * sinTheta1 * sign2);
      weights[pid1] += (theta1 - c2 * theta0 - c0 * theta2) / (dist[pid1] * sinTheta2 * sign0);
      weights[pid2] += (theta2 - c0 * theta1 - c1 * theta0) / (dist[pid2] * sinTheta0 * sign1);

      //
      // increment id and next triangle
      ++iter;
    }

    // normalize weight
    const double sumWeight = std::accumulate(weights, weightsEnd, 0.);

    if (fabs(sumWeight) < eps)
    {
      return;
    }

    std::for_each(weights, weightsEnd, [&](double& w) { w /= sumWeight; });
  }
};

} // end anon namespace

//----------------------------------------------------------------------------
// Static function to compute weights for triangle mesh (with vtkIdList)
// Satisfy classes' public API.
void vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
  const double x[3], vtkPoints* pts, vtkIdList* tris, double* weights)
{
  // Check the input
  if (!tris)
  {
    vtkGenericWarningMacro("Did not provide triangles");
    return;
  }

  vtkIdType* t = tris->GetPointer(0);
  // Below the vtkCellArray has three entries per triangle {(i,j,k), (i,j,k), ....}
  vtkMVCTriIterator iter(tris->GetNumberOfIds(), 3, t);

  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeightsForTriangleMesh(
    x, pts, iter, weights);
}

//----------------------------------------------------------------------------
// Static function to compute weights for triangle or polygonal mesh
// (with vtkCellArray). Satisfy classes' public API.
void vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
  const double x[3], vtkPoints* pts, vtkCellArray* cells, double* weights)
{
  // Check the input
  if (!cells)
  {
    vtkGenericWarningMacro("Did not provide cells");
    return;
  }

  // We can use a fast path if:
  // 1) The mesh only consists of triangles, and
  // 2) `cells` is using vtkIdType for storage.
  bool canFastPath =
#ifdef VTK_USE_64BIT_IDS
    cells->IsStorage64Bit();
#else  // VTK_USE_64BIT_IDS
    !cells->IsStorage64Bit();
#endif // VTK_USE_64BIT_IDS

  // check if input is a triangle mesh
  if (canFastPath)
  {
    canFastPath = (cells->IsHomogeneous() == 3);
  }

  if (canFastPath)
  {
#ifdef VTK_USE_64BIT_IDS
    vtkIdType* t = reinterpret_cast<vtkIdType*>(cells->GetConnectivityArray64()->GetPointer(0));
#else  // 32 bit ids
    vtkIdType* t = reinterpret_cast<vtkIdType*>(cells->GetConnectivityArray32()->GetPointer(0));
#endif // VTK_USE_64BIT_IDS

    vtkMVCTriIterator iter(cells->GetNumberOfConnectivityIds(), 3, t);

    vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeightsForTriangleMesh(
      x, pts, iter, weights);
  }
  else
  {
    vtkMVCPolyIterator iter(cells);

    vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeightsForPolygonMesh(
      x, pts, iter, weights);
  }
}

//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeightsForTriangleMesh(
  const double x[3], vtkPoints* pts, vtkMVCTriIterator& iter, double* weights)
{
  // Check the input
  if (!pts || !weights)
  {
    vtkGenericWarningMacro("Did not provide proper input");
    return;
  }

  // Prepare the arrays
  vtkIdType numPts = pts->GetNumberOfPoints();
  if (numPts <= 0)
  {
    return;
  }

  // float/double points get fast path, everything else goes slow.
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<Reals>;

  ComputeWeightsForTriangleMesh worker;
  if (!Dispatcher::Execute(pts->GetData(), worker, x, iter, weights))
  { // fallback for weird arrays:
    worker(pts->GetData(), x, iter, weights);
  }
}

//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeightsForPolygonMesh(
  const double x[3], vtkPoints* pts, vtkMVCPolyIterator& iter, double* weights)
{
  // Check the input
  if (!pts || !weights)
  {
    vtkGenericWarningMacro("Did not provide proper input");
    return;
  }

  // Prepare the arrays
  vtkIdType numPts = pts->GetNumberOfPoints();
  if (numPts <= 0)
  {
    return;
  }

  // float/double points get fast path, everything else goes slow.
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<Reals>;

  ComputeWeightsForPolygonMesh worker;
  if (!Dispatcher::Execute(pts->GetData(), worker, x, iter, weights))
  { // fallback for weird arrays:
    worker(pts->GetData(), x, iter, weights);
  }
}

//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
