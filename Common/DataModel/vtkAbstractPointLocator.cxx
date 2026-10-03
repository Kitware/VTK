// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractPointLocator.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLocatorInterface.h" // For vtkDist2TupleArray
#include "vtkMath.h"
#include "vtkNew.h"

#include <algorithm>
#include <cmath>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractPointLocator::vtkAbstractPointLocator()
{
  for (int i = 0; i < 6; i++)
  {
    this->Bounds[i] = 0;
  }
  this->NumberOfBuckets = 0;
  this->Tolerance = 0.001;
}

//------------------------------------------------------------------------------
vtkAbstractPointLocator::~vtkAbstractPointLocator() = default;

//------------------------------------------------------------------------------
// Given a position x-y-z, return the id of the point closest to it.
vtkIdType vtkAbstractPointLocator::FindClosestPoint(double x, double y, double z)
{
  double xyz[3];

  xyz[0] = x;
  xyz[1] = y;
  xyz[2] = z;
  return this->FindClosestPoint(xyz);
}

//------------------------------------------------------------------------------
void vtkAbstractPointLocator::FindClosestNPoints(
  int N, double x, double y, double z, vtkIdList* result)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->FindClosestNPoints(N, p, result);
}

//------------------------------------------------------------------------------
void vtkAbstractPointLocator::FindPointsWithinRadius(
  double R, double x, double y, double z, vtkIdList* result)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->FindPointsWithinRadius(R, p, result);
}

//------------------------------------------------------------------------------
void vtkAbstractPointLocator::FindPointsInShell(const double x[3], vtkDist2TupleArray& results,
  double minDist2, double maxDist2, bool sort, vtkDoubleArray* petals)
{
  results.clear();
  if (!this->DataSet || maxDist2 < 0.0 || maxDist2 <= minDist2)
  {
    return;
  }

  // sqrt() and the squaring done by FindPointsWithinRadius() round, and some locators
  // (vtkOctreePointLocator) keep the points in float, so without a margin a point at
  // exactly maxDist2 can be lost. The radius is therefore enlarged by a relative
  // RadiusSlack, far above the 1 ulp of the roundings in double, and by a FloatSlack
  // relative to the magnitude of the coordinates of the points within it (at most
  // |x| + radius), far above their rounding in float. This only adds candidates: each one
  // is tested exactly against [minDist2, maxDist2] below, so any larger value gives the
  // same results.
  constexpr double RadiusSlack = 1.0e-12;
  constexpr double FloatSlack = 1.0e-6;
  const double radius = std::sqrt(maxDist2);
  const double magnitude = std::max({ std::abs(x[0]), std::abs(x[1]), std::abs(x[2]) }) + radius;
  vtkNew<vtkIdList> ids;
  this->FindPointsWithinRadius(radius * (1.0 + RadiusSlack) + FloatSlack * magnitude, x, ids);

  // A petal is a sphere (x,y,z,r2), or a circle (x,y,r2) in the x-y plane.
  const vtkIdType numPetals = petals ? petals->GetNumberOfTuples() : 0;
  const int petalSize = petals ? petals->GetNumberOfComponents() : 0;
  const double* petal0 = numPetals > 0 ? petals->GetPointer(0) : nullptr;
  double p[3];
  for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
  {
    const vtkIdType ptId = ids->GetId(i);
    this->DataSet->GetPoint(ptId, p);
    const double d2 = vtkMath::Distance2BetweenPoints(p, x);
    if (d2 <= minDist2 || d2 > maxDist2)
    {
      continue;
    }
    bool inPetal = (numPetals == 0);
    const double* petal = petal0;
    for (vtkIdType k = 0; k < numPetals && !inPetal; ++k, petal += petalSize)
    {
      const double petalD2 = (petalSize == 3) ? vtkMath::Distance2BetweenPoints2D(p, petal)
                                              : vtkMath::Distance2BetweenPoints(p, petal);
      inPetal = (petalD2 <= petal[petalSize - 1]);
    }
    if (inPetal)
    {
      results.emplace_back(ptId, d2);
    }
  }

  if (sort)
  {
    std::sort(results.begin(), results.end());
  }
}

//------------------------------------------------------------------------------
void vtkAbstractPointLocator::GetBounds(double* bnds)
{
  for (int i = 0; i < 6; i++)
  {
    bnds[i] = this->Bounds[i];
  }
}

//------------------------------------------------------------------------------
void vtkAbstractPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for (int i = 0; i < 6; i++)
  {
    os << indent << "Bounds[" << i << "]: " << this->Bounds[i] << "\n";
  }

  os << indent << "Number of Buckets: " << this->NumberOfBuckets << "\n";
}
VTK_ABI_NAMESPACE_END
