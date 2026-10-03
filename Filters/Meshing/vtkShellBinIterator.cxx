// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_8_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkShellBinIterator.h"
#include "vtkStaticPointLocatorPrivate.h"

VTK_ABI_NAMESPACE_BEGIN

// Support the templated dispatch process.
struct InternalShellBinIterator
{
  // Stuff that doesn't change once the locator is built
  vtkStaticPointLocator* Locator; // the locator being iterated over
  vtkDataSet* DataSet;            // access dataset points
  double* FastPoints;             // fast path for points access
  int Divs[3];                    // locator bin divisions

  // Stuff that changes over the course of the iteration
  int Level;            // the current level of iteration
  double X[3];          // the center of the iterator in physical space
  int Center[3];        // the center of the iterator in index space
  vtkIdType NumSpheres; // the number of inclusive spheres
  double* Spheres;      // the spheres, 4 tuples with (x,y,z,r2)
  double MinD2;         // minimum distance of the current level to the query point

  // Use to enable / disable bin culling - it's not worth it for low levels.
  int LEVEL_QUERY_THRESHOLD = 3;

  // The core class that performs iteration over blocks of different levels.
  vtkBucketBlockIterator BIter;

  // Fast path for double points.
  double* GetPoint(vtkIdType ptId) { return (this->FastPoints + 3 * ptId); }

  // Get the current bin/bucket id
  vtkIdType GetBinId() { return this->BIter.GetBinId(); }

  // Get the current bin/bucket IJK position
  void GetBin(int ijk[3])
  {
    ijk[0] = this->BIter.IJK[0];
    ijk[1] = this->BIter.IJK[1];
    ijk[2] = this->BIter.IJK[2];
  }
};

namespace // anonymous
{

//------------------------------------------------------------------------------
// Coordinate the iteration process.
template <typename TIds>
struct ShellBinIterator : public InternalShellBinIterator
{
  // Stuff that doesn't change once the locator is built
  BucketList<TIds>* Bins; // templated data buckets

  ShellBinIterator(vtkStaticPointLocator* locator)
  {
    this->Locator = locator;
    this->Bins = static_cast<BucketList<TIds>*>(locator->GetBuckets());
    this->DataSet = locator->GetDataSet();
    this->FastPoints = this->Bins->FastPoints;
    this->Divs[0] = this->Bins->Divisions[0];
    this->Divs[1] = this->Bins->Divisions[1];
    this->Divs[2] = this->Bins->Divisions[2];
    this->BIter.Initialize(this->Divs, this->Bins->MaxLevel);
  }

  // Compute the minimum distance of the block of bins to the center of iteration.
  void ComputeMinD2(int level)
  {
    this->MinD2 = VTK_FLOAT_MAX;
    double d, d2;
    for (auto i = 0; i < 3; ++i)
    {
      if ((this->Center[i] - level) >= 0)
      {
        d = this->X[i] -
          (this->Bins->Bounds[2 * i] + (this->Center[i] - level + 1) * this->Bins->H[i]);
        d2 = d * d;
        this->MinD2 = std::min(d2, this->MinD2);
      }
      if ((this->Center[i] + level) < this->Divs[i])
      {
        d = (this->Bins->Bounds[2 * i] + (this->Center[i] + level) * this->Bins->H[i]) - this->X[i];
        d2 = d * d;
        this->MinD2 = std::min(d2, this->MinD2);
      }
    }
  }

  // Begin iterating over bins, starting at level==0.
  bool Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results)
  {
    // Clear out any previous results.
    results.clear();

    // Initialize starting values
    this->Level = 0;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
    this->MinD2 = 0.0;

    // Find the bucket/bin the point is in. This is the center of the request
    // footprint.
    this->Bins->GetBucketIndices(x, this->Center);
    vtkIdType binIdx = this->BIter.Begin(this->Level, this->Center);

    // Prepare the points for processing. Add all points at level==0.
    vtkIdType numIds = this->Bins->GetNumberOfIds(binIdx);
    const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binIdx);
    double* pt;
    for (vtkIdType i = 0; i < numIds; ++i)
    {
      vtkIdType ptId = ids[i].PtId;
      if (pid != ptId)
      {
        pt = this->GetPoint(ptId);
        double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
        results.emplace_back(ptId, d2);
      }
    }

    // Initial call at level==0 it's best to sort the points
    std::sort(results.begin(), results.end());

    return true;
  }

  bool Next(double rad22, vtkDoubleArray* spheres, vtkDist2TupleArray& results)
  {
    // Grab points in the bin. Make sure there is something useful to
    // return.
    vtkIdType binId, numIds;
    bool foundBin = false;
    while (!foundBin)
    {
      if ((binId = this->BIter.Next()) < 0)
      {
        if (++this->Level >= this->BIter.MaxLevel)
        {
          return false; // completed iteration
        }
        // Increasing to the next level
        this->ComputeMinD2(this->Level);
        if (rad22 < this->MinD2)
        {
          return false; // completed iteration
        }

        // Move on to next level
        binId = this->BIter.Begin(this->Level, this->Center);
      } // if have a valid binId

      // Make sure there are some points in the bin. Otherwise skip.
      if ((numIds = this->Bins->GetNumberOfIds(binId)) <= 0)
      {
        continue;
      }

      // See whether the bin can be culled with the Voronoi flower or circumflower.
      // Culling is most effective at higher levels of block iteration.
      if (this->Level >= this->LEVEL_QUERY_THRESHOLD)
      {
        // See if the bin is outside of the Circumflower / radius of security
        double min[3], max[3];
        this->Bins->GetBucketBounds(BIter.IJK[0], BIter.IJK[1], BIter.IJK[2], min, max);
        if (!vtkBoundingBox::IntersectsSphere(min, max, this->X, rad22))
        {
          continue;
        }
        // In Voronoi flower
        if (spheres)
        {
          vtkIdType sNum, numSpheres = spheres->GetNumberOfTuples();
          const double* sphere = spheres->GetPointer(0);
          for (sNum = 0; sNum < numSpheres; ++sNum, sphere += 4)
          {
            if (vtkBoundingBox::IntersectsSphere(min, max, sphere, sphere[3]))
            {
              break;
            }
          } // See if the bin falls into any one of the petals
          if (sNum >= numSpheres) // no intersection
          {
            continue;
          }
        }
      } // if level is large enough to warrant in culling.

      // At this point we can load data from the current bin.
      results.clear();
      const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binId);
      double* pt;

      if (numIds == 1)
      {
        vtkIdType ptId = ids[0].PtId;
        pt = this->GetPoint(ptId);
        double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
        if (d2 <= rad22)
        {
          results.emplace_back(ptId, d2);
        }
      } // adding a single point

      // Find the single point closest to the generator point. This will
      // insert it before the others in the bin, avoiding a few hull clips.
      else // processing more than one point
      {
        double minR2 = VTK_FLOAT_MAX;
        size_t pos = 0;
        for (vtkIdType i = 0; i < numIds; ++i)
        {
          vtkIdType ptId = ids[i].PtId;
          pt = this->GetPoint(ptId);
          double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
          if (d2 <= rad22)
          {
            results.emplace_back(ptId, d2);
            if (d2 < minR2)
            {
              pos = results.size() - 1;
              minR2 = d2;
            }
          }
        }
        // Swap the closest point in the bin to first position.
        if (results.size() > 1 && pos != 0)
        {
          std::swap(results[0], results[pos]);
        }
      } // potentially insert more than 1 point

      // Make sure there is something to return
      if (!results.empty())
      {
        foundBin = true;
      }
    } // while haven't found non-culled bin with points

    return true;
  }
}; // ShellBinIterator

} // anonymous namespace

//==============================================================================
// Define various dispatch methods.
//------------------------------------------------------------------------------
void vtkShellBinIteratorDispatch::Initialize(vtkStaticPointLocator* locator)
{
  this->LargeIds = locator->GetLargeIds();
  if (this->LargeIds)
  {
    this->Iterator = new ShellBinIterator<vtkIdType>(locator);
  }
  else
  {
    this->Iterator = new ShellBinIterator<int>(locator);
  }
}

//------------------------------------------------------------------------------
bool vtkShellBinIteratorDispatch::Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<ShellBinIterator<vtkIdType>*>(this->Iterator)->Begin(pid, x, results);
  }
  else
  {
    return static_cast<ShellBinIterator<int>*>(this->Iterator)->Begin(pid, x, results);
  }
}

//------------------------------------------------------------------------------
bool vtkShellBinIteratorDispatch::Next(
  double radius2, vtkDoubleArray* spheres, vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<ShellBinIterator<vtkIdType>*>(this->Iterator)
      ->Next(radius2, spheres, results);
  }
  else
  {
    return static_cast<ShellBinIterator<int>*>(this->Iterator)->Next(radius2, spheres, results);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkShellBinIteratorDispatch::GetBinId()
{
  return this->Iterator->GetBinId();
}

//------------------------------------------------------------------------------
void vtkShellBinIteratorDispatch::GetBin(int IJK[3])
{
  this->Iterator->GetBin(IJK);
}

//------------------------------------------------------------------------------
double vtkShellBinIteratorDispatch::GetMinD2()
{
  return this->Iterator->MinD2;
}

VTK_ABI_NAMESPACE_END
