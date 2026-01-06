// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnnularBinIterator.h"
#include "vtkStaticPointLocator2DPrivate.h"

VTK_ABI_NAMESPACE_BEGIN

namespace // anonymous
{

//------------------------------------------------------------------------------
// Hard coded bin iteration traversals for lower levels. Note the ping-pong
// order (traversal of opposite bins).
const int Level1[8][2] = {
  { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 },  // four edge centers, ping pong style
  { -1, -1 }, { 1, 1 }, { 1, -1 }, { -1, 1 } // four patch corners, ping ponged
};                                           // Level1

const int Level2[16][2] = {
  { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 },   // edge centers
  { -1, -2 }, { 1, 2 }, { -2, -1 }, { 2, 1 }, // along edges
  { 1, -2 }, { -1, 2 }, { -2, 1 }, { 2, -1 }, { -2, -2 }, { 2, 2 }, { 2, -2 }, { -2, 2 } // corners
};                                                                                       // Level2

const int Level3[24][2] = { { -3, 0 }, { 3, 0 }, { 0, -3 }, { 0, 3 }, { -1, -3 }, { 1, 3 },
  { 3, -1 }, { -3, 1 }, { 1, -3 }, { -1, 3 }, { 3, 1 }, { -3, -1 }, { -2, -3 }, { 2, 3 }, { 3, -2 },
  { -3, 2 }, { 2, -3 }, { -2, 3 }, { 3, 2 }, { -3, -2 }, { -3, -3 }, { 3, 3 }, { 3, -3 },
  { -3, 3 } }; // Level3

const int Level4[32][2] = { { -4, 0 }, { 4, 0 }, { 0, -4 }, { 0, 4 }, { -1, -4 }, { 1, 4 },
  { 4, -1 }, { -4, 1 }, { 1, -4 }, { -1, 4 }, { 4, 1 }, { -4, -1 }, { -2, -4 }, { 2, 4 }, { 4, -2 },
  { -4, 2 }, { 2, -4 }, { -2, 4 }, { 4, 2 }, { -4, -2 }, { -3, -4 }, { 3, 4 }, { 4, -3 }, { -4, 3 },
  { 3, -4 }, { -3, 4 }, { 4, 3 }, { -4, -3 }, { -4, -4 }, { 4, 4 }, { 4, -4 },
  { -4, 4 } }; // Level4

const int Level5[40][2] = { { -5, 0 }, { 5, 0 }, { 0, -5 }, { 0, 5 }, { -1, -5 }, { 1, 5 },
  { 5, -1 }, { -5, 1 }, { 1, -5 }, { -1, 5 }, { 5, 1 }, { -5, -1 }, { -2, -5 }, { 2, 5 }, { 5, -2 },
  { -5, 2 }, { 2, -5 }, { -2, 5 }, { 5, 2 }, { -5, -2 }, { -3, -5 }, { 3, 5 }, { 5, -3 }, { -5, 3 },
  { 3, -5 }, { -3, 5 }, { 5, 3 }, { -5, -3 }, { -4, -5 }, { 4, 5 }, { 5, -4 }, { -5, 4 }, { 4, -5 },
  { -4, 5 }, { 5, 4 }, { -5, -4 }, { -5, -5 }, { 5, 5 }, { 5, -5 }, { -5, 5 } }; // Level5

//------------------------------------------------------------------------------
// Iterate over a specified level of an i-j patch (i.e., a hollow square of bins,
// iteration occurs over the perimeter of the patch at a specified level).
struct PatchIterator
{
  int Divs[2];  // the topology of a regular (2D locator) binning
  int MaxLevel; // the maximum level of iteration based on locator

  // The current state of iteration.
  int Level;     // the current level over which we are iterating
  int Center[2]; // the center of iteration of the current patch
  int Span;      // the topological width of the patch at the current patch level
  int NumBins;   // the number of bins to process at the current level
  int BinNum;    // the current iteration position at the current level
  int Interior;  // indicate whether (center+level) is interior to the patch
  int IJ[2];     // the current IJ index of iteration

  // Constructor
  PatchIterator() = default;

  // Initialize the patch iterator.
  void Initialize(int divs[3], int maxLevel)
  {
    this->Divs[0] = divs[0];
    this->Divs[1] = divs[1];
    this->MaxLevel = maxLevel;
  }

  // Determine if the current bin is within the locator binning.
  bool IsValid()
  {
    if (this->Interior)
    {
      return true;
    }

    if (this->IJ[0] < 0 || this->IJ[0] >= this->Divs[0] || this->IJ[1] < 0 ||
      this->IJ[1] >= this->Divs[1])
    {
      return false;
    }
    return true;
  }

  // Return the current bin id.
  vtkIdType GetBinId() { return (this->IJ[0] + this->IJ[1] * this->Divs[0]); }

  // Update the IJ index.
  void GetIJ(int ij[2])
  {
    ij[0] = this->IJ[0];
    ij[1] = this->IJ[1];
  }

  // Initialize the patch iteration process. Indicate the center of the
  // patch. This is used to specify the patch center of iteration.
  vtkIdType Begin(int level, int center[2])
  {
    this->Level = level;
    this->Center[0] = center[0];
    this->Center[1] = center[1];
    this->BinNum = 0; // starting iteration bin

    if (level == 0)
    {
      this->IJ[0] = this->Center[0];
      this->IJ[1] = this->Center[1];
      this->NumBins = 1;
      return this->GetBinId();
    }

    // For levels>0, further iteration will occur via repeated invocations
    // of Next().

    // The span: the number of bins spanning a face edge.
    this->Span = 2 * level + 1;

    // The total number of bins to process.
    this->NumBins =
      (level == 0 ? 1
                  : (this->Span * this->Span) - ((2 * (level - 1) + 1) * (2 * (level - 1) + 1)));

    // Determine whether the iteration region is inside of the locator. If the iteration
    // region overlaps the locator boundary (i.e., is not interior), then bin validity checks
    // have to be performed.
    this->Interior = true;
    if ((this->Center[0] - level) < 0 || (this->Center[0] + level) >= this->Divs[0] ||
      (this->Center[1] - level) < 0 || (this->Center[1] + level) >= this->Divs[1])
    {
      this->Interior = false;
    }

    // Special case for levels 1-5. The bins are traversed in ping-pong order
    // in increasing distance from center: first -/+ edge centers, then along edges,
    // and finally corner points,
    if (level == 1)
    {
      this->IJ[0] = this->Center[0] + Level1[0][0];
      this->IJ[1] = this->Center[1] + Level1[0][1];
    }
    else if (level == 2)
    {
      this->IJ[0] = this->Center[0] + Level2[0][0];
      this->IJ[1] = this->Center[1] + Level2[0][1];
    }
    else if (level == 3)
    {
      this->IJ[0] = this->Center[0] + Level3[0][0];
      this->IJ[1] = this->Center[1] + Level3[0][1];
    }
    else if (level == 4)
    {
      this->IJ[0] = this->Center[0] + Level4[0][0];
      this->IJ[1] = this->Center[1] + Level4[0][1];
    }
    else if (level == 5)
    {
      this->IJ[0] = this->Center[0] + Level5[0][0];
      this->IJ[1] = this->Center[1] + Level5[0][1];
    }
    else // general case for larger levels
    {
      this->IJ[0] = this->Center[0];
      this->IJ[1] = this->Center[0] - level;
    }

    // Make sure that the starting bin is valid
    if (!this->IsValid())
    {
      return this->Next();
    }

    return this->GetBinId();
  }

  // Return binId<0 when iteration over the entirety of the current face patch
  // completes.
  vtkIdType Next()
  {
    // Loop until valid bin found, or bin traversal for this level is complete.
    while (true)
    {
      // Advance the current bin number
      this->BinNum++;

      // Special case for level 0, or if visited all bins in the current level
      int level = this->Level;
      if (level == 0 || this->BinNum >= this->NumBins || level > this->MaxLevel)
      {
        return (-1);
      }

      // Special cases for levels 1-5
      if (level == 1)
      {
        this->IJ[0] = this->Center[0] + Level1[this->BinNum][0];
        this->IJ[1] = this->Center[1] + Level1[this->BinNum][1];
      }
      else if (level == 2)
      {
        this->IJ[0] = this->Center[0] + Level2[this->BinNum][0];
        this->IJ[1] = this->Center[1] + Level2[this->BinNum][1];
      }
      else if (level == 3)
      {
        this->IJ[0] = this->Center[0] + Level3[this->BinNum][0];
        this->IJ[1] = this->Center[1] + Level3[this->BinNum][1];
      }
      else if (level == 4)
      {
        this->IJ[0] = this->Center[0] + Level4[this->BinNum][0];
        this->IJ[1] = this->Center[1] + Level4[this->BinNum][1];
      }
      else if (level == 5)
      {
        this->IJ[0] = this->Center[0] + Level5[this->BinNum][0];
        this->IJ[1] = this->Center[1] + Level5[this->BinNum][1];
      }
      else // general iteration case - ping-pong rotation around patch
      {
        // four edge centers - recall that Begin() processed BinNum==0.
        if (this->BinNum < 4)
        {
          if (this->BinNum == 1)
          {
            this->IJ[0] = this->Center[0];
            this->IJ[1] = this->Center[1] + level;
          }
          else if (this->BinNum == 2)
          {
            this->IJ[0] = this->Center[0] - level;
            this->IJ[1] = this->Center[1];
          }
          else // if ( this->BinNum == 3 )
          {
            this->IJ[0] = this->Center[0] + level;
            this->IJ[1] = this->Center[1];
          }
        }

        // four patch corners
        else if (this->BinNum >= (this->NumBins - 4))
        {
          if (this->BinNum == (this->NumBins - 4))
          {
            this->IJ[0] = this->Center[0] - level;
            this->IJ[1] = this->Center[1] - level;
          }
          else if (this->BinNum == (this->NumBins - 3))
          {
            this->IJ[0] = this->Center[0] + level;
            this->IJ[1] = this->Center[1] + level;
          }
          else if (this->BinNum == (this->NumBins - 2))
          {
            this->IJ[0] = this->Center[0] + level;
            this->IJ[1] = this->Center[1] - level;
          }
          else // if ( this->BinNum == (this->NumBins-1) )
          {
            this->IJ[0] = this->Center[0] - level;
            this->IJ[1] = this->Center[1] + level;
          }
        }

        // ping-pong rotation
        else
        {
          int idx = (this->BinNum - 4) % 8;
          int offset = ((this->BinNum - 4) / 8) + 1;
          if (idx == 0)
          {
            this->IJ[0] = this->Center[0] - offset;
            this->IJ[1] = this->Center[1] - level;
          }
          else if (idx == 1)
          {
            this->IJ[0] = this->Center[0] + offset;
            this->IJ[1] = this->Center[1] + level;
          }
          else if (idx == 2)
          {
            this->IJ[0] = this->Center[0] - level;
            this->IJ[1] = this->Center[1] - offset;
          }
          else if (idx == 3)
          {
            this->IJ[0] = this->Center[0] + level;
            this->IJ[1] = this->Center[1] + offset;
          }
          else if (idx == 4)
          {
            this->IJ[0] = this->Center[0] + offset;
            this->IJ[1] = this->Center[1] - level;
          }
          else if (idx == 5)
          {
            this->IJ[0] = this->Center[0] - offset;
            this->IJ[1] = this->Center[1] + level;
          }
          else if (idx == 6)
          {
            this->IJ[0] = this->Center[0] - level;
            this->IJ[1] = this->Center[1] + offset;
          }
          else // if ( idx == 7 )
          {
            this->IJ[0] = this->Center[0] + level;
            this->IJ[1] = this->Center[1] - offset;
          }
        }
      }

      // Return when valid bin is found
      if (this->IsValid())
      {
        return this->GetBinId();
      }
    } // while iterating over patch at current level
  }   // Next()

}; // PatchIterator

} // anonymous namespace

// Support the templated dispatch process.
struct InternalAnnularBinIterator
{
  // Stuff that doesn't change once the locator is built
  vtkStaticPointLocator2D* Locator; // the locator being iterated over
  vtkDataSet* DataSet;              // access dataset points
  double* FastPoints;               // fast path for points access
  int Divs[2];                      // locator bin divisions

  // Stuff that changes over the course of the iteration
  int Level;            // the current level of iteration
  double X[2];          // the center of the iterator in physical space
  int Center[2];        // the center of the iterator in index space
  vtkIdType NumCircles; // the number of inclusive circles
  double* Circles;      // the circles, three-tuples (x,y,r2)
  double MinD2;         // minimum distance of the current level to the query point

  // Use to enable / disable bin culling - it's not worth it for low levels.
  int LEVEL_QUERY_THRESHOLD = 3;

  // The core class that performs iteration over patches of different levels.
  PatchIterator PIter;

  // Fast path for double points. Note: even though this class assumes x-y points, for
  // convenience the points are represented with an AOS array as x-y-z. This simplifies
  // integration with VTK.
  double* GetPoint(vtkIdType ptId) { return (this->FastPoints + 3 * ptId); }

  // Get the current bin/bucket id
  vtkIdType GetBinId() { return this->PIter.GetBinId(); }

  // Get the current bin/bucket IJ position
  void GetBin(int ij[2])
  {
    ij[0] = this->PIter.IJ[0];
    ij[1] = this->PIter.IJ[1];
  }
}; // InternalAnnularBinIterator

namespace // anonymous
{

//------------------------------------------------------------------------------
// Coordinate the iteration process.
template <typename TIds>
struct AnnularBinIterator : public InternalAnnularBinIterator
{
  // Stuff that doesn't change once the locator is built
  BucketList2D<TIds>* Bins; // templated data buckets

  AnnularBinIterator(vtkStaticPointLocator2D* locator)
  {
    this->Locator = locator;
    this->Bins = static_cast<BucketList2D<TIds>*>(locator->GetBuckets());
    this->DataSet = locator->GetDataSet();
    this->FastPoints = this->Bins->FastPoints;
    this->Divs[0] = this->Bins->Divisions[0];
    this->Divs[1] = this->Bins->Divisions[1];
    this->PIter.Initialize(this->Divs, this->Bins->MaxLevel);
  }

  // Compute the minimum distance of the patch of bins to the center of iteration.
  void ComputeMinD2(int level)
  {
    this->MinD2 = VTK_FLOAT_MAX;
    double d, d2;
    for (auto i = 0; i < 2; ++i)
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
  bool Begin(vtkIdType pid, double x[2], vtkDist2TupleArray& results)
  {
    // Clear out any previous results.
    results.clear();

    // Initialize starting values
    this->Level = 0;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->MinD2 = 0.0;

    // Find the bucket/bin the point is in. This is the center of the patch
    // to iterate over.
    this->Bins->GetBucketIndices(x, this->Center);
    vtkIdType binIdx = this->PIter.Begin(this->Level, this->Center);

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
        double d2 = Distance2BetweenPoints2D(this->X, pt);
        results.emplace_back(ptId, d2);
      }
    }

    // Initial call at level==0 it's best to sort the points
    std::sort(results.begin(), results.end());

    // Begin() always returns true (i.e., indicates that the traversal is to
    // continue).
    return true;
  }

  bool Next(double rad22, vtkDoubleArray* circles, vtkDist2TupleArray& results)
  {
    // Grab points in the bin. Make sure there is something useful to
    // return.
    vtkIdType binId, numIds;
    bool foundBin = false;
    while (!foundBin)
    {
      if ((binId = this->PIter.Next()) < 0)
      {
        if (++this->Level >= this->PIter.MaxLevel)
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
        binId = this->PIter.Begin(this->Level, this->Center);
      } // if have a valid binId

      // Make sure there are some points in the bin. Otherwise skip.
      if ((numIds = this->Bins->GetNumberOfIds(binId)) <= 0)
      {
        continue;
      }

      // See whether the bin can be culled with the Voronoi flower or circumflower.
      // Culling is most effective at higher levels of patch iteration.
      if (this->Level >= this->LEVEL_QUERY_THRESHOLD)
      {
        // See if the bin is outside of the Circumflower / radius of security
        double min[2], max[2];
        this->Bins->GetBucketBounds(PIter.IJ[0], PIter.IJ[1], min, max);
        if (!IntersectsCircle(min, max, this->X, rad22))
        {
          continue;
        }
        // In Voronoi flower
        if (circles)
        {
          vtkIdType sNum, numCircles = circles->GetNumberOfTuples();
          const double* circle = circles->GetPointer(0);
          for (sNum = 0; sNum < numCircles; ++sNum, circle += 3)
          {
            if (IntersectsCircle(min, max, circle, circle[2]))
            {
              break;
            }
          }                       // See if the bin falls into any one of the petals
          if (sNum >= numCircles) // no intersection
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
        double d2 = Distance2BetweenPoints2D(this->X, pt);
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
          double d2 = Distance2BetweenPoints2D(this->X, pt);
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
}; // AnnularBinIterator

} // anonymous namespace

//==============================================================================
// Define various dispatch methods.
//------------------------------------------------------------------------------
void vtkAnnularBinIteratorDispatch::Initialize(vtkStaticPointLocator2D* locator)
{
  this->LargeIds = locator->GetLargeIds();
  if (this->LargeIds)
  {
    this->Iterator = new AnnularBinIterator<vtkIdType>(locator);
  }
  else
  {
    this->Iterator = new AnnularBinIterator<int>(locator);
  }
}

//------------------------------------------------------------------------------
bool vtkAnnularBinIteratorDispatch::Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<AnnularBinIterator<vtkIdType>*>(this->Iterator)->Begin(pid, x, results);
  }
  else
  {
    return static_cast<AnnularBinIterator<int>*>(this->Iterator)->Begin(pid, x, results);
  }
}

//------------------------------------------------------------------------------
bool vtkAnnularBinIteratorDispatch::Next(
  double radius2, vtkDoubleArray* circles, vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<AnnularBinIterator<vtkIdType>*>(this->Iterator)
      ->Next(radius2, circles, results);
  }
  else
  {
    return static_cast<AnnularBinIterator<int>*>(this->Iterator)->Next(radius2, circles, results);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkAnnularBinIteratorDispatch::GetBinId()
{
  return this->Iterator->GetBinId();
}

//------------------------------------------------------------------------------
void vtkAnnularBinIteratorDispatch::GetBin(int IJ[2])
{
  this->Iterator->GetBin(IJ);
}

//------------------------------------------------------------------------------
double vtkAnnularBinIteratorDispatch::GetMinD2()
{
  return this->Iterator->MinD2;
}

VTK_ABI_NAMESPACE_END
