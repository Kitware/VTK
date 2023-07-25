// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// This tests the code used in Documentation/Doxygen/SMPTools.md.

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

#include <array>
#include <vector>

namespace
{

// Compute a bounding hull with the planes provided.
struct HullFunctor
{
  vtkPoints* InPts;
  std::vector<double>& Planes;

  HullFunctor(vtkPoints* inPts, std::vector<double>& planes)
    : InPts(inPts)
    , Planes(planes)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkPoints* inPts = this->InPts;
    std::vector<double>& planes = this->Planes;
    auto numPlanes = planes.size() / 4;

    for (; ptId < endPtId; ++ptId)
    {
      double v, coord[3];
      inPts->GetPoint(ptId, coord);
      for (size_t j = 0; j < numPlanes; j++)
      {
        v = -(planes[j * 4 + 0] * coord[0] + planes[j * 4 + 1] * coord[1] +
          planes[j * 4 + 2] * coord[2]);
        // negative means further in + direction of plane
        if (v < planes[j * 4 + 3])
        {
          planes[j * 4 + 3] = v;
        }
      }
    }
  }
}; // HullFunctor

using BoundsArray = std::array<double, 6>;
using TLS = vtkSMPThreadLocal<BoundsArray>;

// Compute bounds of a set of points.
struct BoundsFunctor
{
  vtkFloatArray* Pts;
  BoundsArray Bounds;
  TLS LocalBounds;

  BoundsFunctor(vtkFloatArray* pts)
    : Pts(pts)
  {
  }

  // Initialize thread local storage
  void Initialize()
  {
    // The first call to .Local() will create the array,
    // all others will return the same.
    std::array<double, 6>& bds = this->LocalBounds.Local();
    bds[0] = VTK_DOUBLE_MAX;
    bds[1] = -VTK_DOUBLE_MAX;
    bds[2] = VTK_DOUBLE_MAX;
    bds[3] = -VTK_DOUBLE_MAX;
    bds[4] = VTK_DOUBLE_MAX;
    bds[5] = -VTK_DOUBLE_MAX;
  }

  // Process the range of points [begin,end)
  void operator()(vtkIdType begin, vtkIdType end)
  {
    BoundsArray& lbounds = this->LocalBounds.Local();
    float* x = this->Pts->GetPointer(3 * begin);
    for (vtkIdType i = begin; i < end; i++)
    {
      lbounds[0] = (x[0] < lbounds[0] ? x[0] : lbounds[0]);
      lbounds[1] = (x[0] > lbounds[1] ? x[0] : lbounds[1]);
      lbounds[2] = (x[1] < lbounds[2] ? x[1] : lbounds[2]);
      lbounds[3] = (x[1] > lbounds[3] ? x[1] : lbounds[3]);
      lbounds[4] = (x[2] < lbounds[4] ? x[2] : lbounds[4]);
      lbounds[5] = (x[2] > lbounds[5] ? x[2] : lbounds[5]);

      x += 3;
    }
  }

  // Composite / combine the thread local storage into a global result.
  void Reduce()
  {
    this->Bounds[0] = VTK_DOUBLE_MAX;
    this->Bounds[1] = -VTK_DOUBLE_MAX;
    this->Bounds[2] = VTK_DOUBLE_MAX;
    this->Bounds[3] = -VTK_DOUBLE_MAX;
    this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[5] = -VTK_DOUBLE_MAX;

    using TLSIter = TLS::iterator;
    TLSIter end = this->LocalBounds.end();
    for (TLSIter itr = this->LocalBounds.begin(); itr != end; ++itr)
    {
      BoundsArray& lBounds = *itr;
      this->Bounds[0] = (this->Bounds[0] < lBounds[0] ? this->Bounds[0] : lBounds[0]);
      this->Bounds[1] = (this->Bounds[1] > lBounds[1] ? this->Bounds[1] : lBounds[1]);
      this->Bounds[2] = (this->Bounds[2] < lBounds[2] ? this->Bounds[2] : lBounds[2]);
      this->Bounds[3] = (this->Bounds[3] > lBounds[3] ? this->Bounds[3] : lBounds[3]);
      this->Bounds[4] = (this->Bounds[4] < lBounds[4] ? this->Bounds[4] : lBounds[4]);
      this->Bounds[5] = (this->Bounds[5] > lBounds[5] ? this->Bounds[5] : lBounds[5]);
    }
  }
}; // BoundsFunctor

// Support for the atomic example.
int Total = 0;
std::atomic<vtkTypeInt32> TotalAtomic(0);
constexpr int Target = 1000000;
constexpr int NumThreads = 2;

VTK_THREAD_RETURN_TYPE MyFunction(void*)
{
  for (int i = 0; i < Target / NumThreads; i++)
  {
    ++Total;
    ++TotalAtomic;
  }
  return VTK_THREAD_RETURN_VALUE;
}

} // anonymous namespace

int TestSMPFeatures(int, char*[])
{
  // Create a random set of points
  constexpr vtkIdType numPts = 1000;
  constexpr vtkIdType numPlanes = 6;

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToFloat();
  pts->SetNumberOfPoints(numPts);
  for (auto i = 0; i < numPts; ++i)
  {
    pts->SetPoint(i, vtkMath::Random(-1, 1), vtkMath::Random(-1, 1), vtkMath::Random(-1, 1));
  }

  // Define the plane normals
  std::vector<double> planes(numPlanes * 4, 0);
  planes[0] = -1; // define six normals (-x,+x, -y,+y, -z,+z)
  planes[4] = 1;
  planes[9] = -1;
  planes[13] = 1;
  planes[18] = -1;
  planes[22] = 1;

  // // Use a functor to compute the planes
  HullFunctor hull(pts, planes);
  vtkSMPTools::For(0, numPts, hull);
  std::cout << "Planes (functor): " << planes[3] << ", " << planes[7] << ", " << planes[11] << ", "
            << planes[15] << ", " << planes[19] << ", " << planes[23] << "\n";

  // Use a lambda to compute the planes
  planes[3] = 0; // reset v
  planes[7] = 0;
  planes[11] = 0;
  planes[15] = 0;
  planes[19] = 0;
  planes[23] = 0;
  vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
    for (; ptId < endPtId; ++ptId)
    {
      double v, coord[3];
      pts->GetPoint(ptId, coord);
      for (auto j = 0; j < numPlanes; j++)
      {
        v = -(planes[j * 4 + 0] * coord[0] + planes[j * 4 + 1] * coord[1] +
          planes[j * 4 + 2] * coord[2]);
        // negative means further in + direction of plane
        if (v < planes[j * 4 + 3])
        {
          planes[j * 4 + 3] = v;
        }
      }
    }
  }); // end lambda

  std::cout << "Planes (lambda): " << planes[3] << ", " << planes[7] << ", " << planes[11] << ", "
            << planes[15] << ", " << planes[19] << ", " << planes[23] << "\n";

  // Compute bounds using Initialize() and Reduce().
  vtkFloatArray* ptsArray = vtkFloatArray::SafeDownCast(pts->GetData());
  BoundsFunctor calcBounds(ptsArray);
  vtkSMPTools::For(0, numPts, calcBounds);
  std::array<double, 6>& bds = calcBounds.Bounds;
  std::cout << "Bounds (: " << bds[0] << "," << bds[1] << ", " << bds[2] << "," << bds[3] << ", "
            << bds[4] << "," << bds[5] << ")\n";

  // Now exercise atomics
  vtkNew<vtkMultiThreader> mt;
  mt->SetSingleMethod(MyFunction, nullptr);
  mt->SetNumberOfThreads(NumThreads);
  mt->SingleMethodExecute();
  std::cout << Total << " " << TotalAtomic.load() << endl;

  return EXIT_SUCCESS;
}
