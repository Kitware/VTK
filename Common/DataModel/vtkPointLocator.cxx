/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointLocator.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkPointLocator);

static const int VTK_INITIAL_SIZE=1000;

// Utility class to store an array of ijk values
class vtkNeighborPoints
{
public:
  vtkNeighborPoints()
    {
      this->Count = 0;
      this->P=&(this->InitialBuffer[0]);
      this->MaxSize = VTK_INITIAL_SIZE;
    }
  ~vtkNeighborPoints()
    {
      this->Count = 0;
      if ( this->P != &(this->InitialBuffer[0]) )
        {
        delete[] this->P;
        }
    }
  int GetNumberOfNeighbors() { return this->Count; }
  void Reset() { this->Count = 0; }

  int *GetPoint(int i)
    {
      return (this->Count > i ?  &(this->P[3*i]) : 0);
    }

  int InsertNextPoint(const int x[3])
    {
      int* tmp;

      // Re-allocate if beyond the current max size.
      // (Increase by VTK_INITIAL_SIZE)
      if (this->Count == this->MaxSize)
        {
        tmp = this->P;

        this->MaxSize += VTK_INITIAL_SIZE;
        this->P = new int[this->MaxSize*3];

        for(int i=0; i<3*this->Count; i++)
          {
          this->P[i] = tmp[i];
          }
        if ( tmp != &(this->InitialBuffer[0]) )
          {
          delete[] tmp;
          }
        }

      this->P[3*this->Count] = x[0];
      this->P[3*this->Count+1] = x[1];
      this->P[3*this->Count+2] = x[2];
      this->Count++;
      return this->Count-1;
    }

protected:
// Start with an array to avoid memory allocation overhead
  int InitialBuffer[VTK_INITIAL_SIZE*3];
  int *P;
  int Count;
  int MaxSize;
};


// Construct with automatic computation of divisions, averaging
// 25 points per bucket.
vtkPointLocator::vtkPointLocator()
{

  this->Points = NULL;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->NumberOfPointsPerBucket = 3;
  this->HashTable = NULL;
  this->NumberOfBuckets = 0;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  this->InsertionPointId = 0;
  this->InsertionTol2 = 0.0001;
  this->InsertionLevel = 0;
}

vtkPointLocator::~vtkPointLocator()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }
  this->FreeSearchStructure();
}

void vtkPointLocator::Initialize()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }
  this->FreeSearchStructure();
}

void vtkPointLocator::FreeSearchStructure()
{
  vtkIdList *ptIds;
  vtkIdType i;

  if ( this->HashTable )
    {
    for (i=0; i<this->NumberOfBuckets; i++)
      {
      if ( (ptIds = this->HashTable[i]) )
        {
        ptIds->Delete();
        }
      }
    delete [] this->HashTable;
    this->HashTable = NULL;
    }
}

// Given a position x, return the id of the point closest to it.
vtkIdType vtkPointLocator::FindClosestPoint(const double x[3])
{
  int i, j;
  double minDist2;
  double dist2 = VTK_DOUBLE_MAX;
  double pt[3];
  int closest, level;
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  vtkNeighborPoints buckets;

  if ( !this->DataSet || this->DataSet->GetNumberOfPoints() < 1 )
    {
    return -1;
    }

  this->BuildLocator(); // will subdivide if modified; otherwise returns

  //
  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  //
  //  Need to search this bucket for closest point.  If there are no
  //  points in this bucket, search 1st level neighbors, and so on,
  //  until closest point found.
  //
  for (closest=(-1),minDist2=VTK_DOUBLE_MAX,level=0; (closest == -1) &&
         (level < this->Divisions[0] || level < this->Divisions[1] ||
          level < this->Divisions[2]); level++)
    {
    this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);

    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          this->DataSet->GetPoint(ptId, pt);
          if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
            {
            closest = ptId;
            minDist2 = dist2;
            }
          }
        }
      }
    }
  //
  // Because of the relative location of the points in the buckets, the
  // point found previously may not be the closest point.  Have to
  // search those bucket neighbors that might also contain point.
  //
  if ( dist2 > 0.0 )
    {
    this->GetOverlappingBuckets (&buckets, x, ijk, sqrt(dist2),0);
    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          this->DataSet->GetPoint(ptId, pt);
          if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
            {
            closest = ptId;
            minDist2 = dist2;
            }
          }//for each point
        }//if points in bucket
      }//for each overlapping bucket
    }//if not identical point

  return closest;
}

vtkIdType vtkPointLocator::FindClosestPointWithinRadius(double radius,
                                                        const double x[3],
                                                        double& dist2)
{
  return this->FindClosestPointWithinRadius(radius, x, this->DataSet->GetLength(),
                                            dist2);
}

vtkIdType vtkPointLocator::FindClosestPointWithinRadius(double radius,
                                                        const double x[3],
                                                        double inputDataLength,
                                                        double& dist2)
{
  int i, j;
  double pt[3];
  vtkIdType ptId, closest = -1;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  double minDist2;

  double refinedRadius, radius2, refinedRadius2;
  double currentRadius;
  double distance2ToDataBounds, maxDistance;
  int ii, radiusLevels[3], radiusLevel, prevMinLevel[3], prevMaxLevel[3];
  vtkNeighborPoints buckets;

  this->BuildLocator(); // will subdivide if modified; otherwise returns

  dist2 = -1.0;
  radius2 = radius*radius;
  minDist2 = 1.01*radius2;   // something slightly bigger....

  vtkDataArray *pointData =
    static_cast<vtkPointSet *>(this->DataSet)->GetPoints()->GetData();
  int flag = 1;

  //
  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  // Start by searching the bucket that the point is in.
  //
  if ( (ptIds = this->HashTable[ijk[0] + ijk[1]*this->Divisions[0] +
    ijk[2]*this->Divisions[0]*this->Divisions[1]]) != NULL )
    {
    for (j=0; j < ptIds->GetNumberOfIds(); j++)
      {
      ptId = ptIds->GetId(j);
      if (flag)
        {
        pointData->GetTuple(ptId, pt);
        }
      else
        {
        this->DataSet->GetPoint(ptId, pt);
        }
      if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
        {
        closest = ptId;
        minDist2 = dist2;
        }
      }
    }


  // Now, search only those buckets that are within a radius. The radius used
  // is the smaller of sqrt(dist2) and the radius that is passed in. To avoid
  // checking a large number of buckets unnecessarily, if the radius is
  // larger than the dimensions of a bucket, we search outward using a
  // simple heuristic of rings.  This heuristic ends up collecting inner
  // buckets multiple times, but this only happens in the case where these
  // buckets are empty, so they are discarded quickly.
  //
  if (dist2 < radius2 && dist2 >= 0.0)
    {
    refinedRadius = sqrt(dist2);
    refinedRadius2 = dist2;
    }
  else
    {
    refinedRadius = radius;
    refinedRadius2 = radius2;
    }

  if (inputDataLength)
    {
    distance2ToDataBounds = this->Distance2ToBounds(x, this->Bounds);
    maxDistance = sqrt(distance2ToDataBounds) + inputDataLength;
    if (refinedRadius > maxDistance)
      {
      refinedRadius = maxDistance;
      refinedRadius2 = maxDistance*maxDistance;
      }
    }

  for (i = 0; i < 3; i++)
    {
    radiusLevels[i] = static_cast<int>(refinedRadius/this->H[i]);
    if (radiusLevels[i] > this->Divisions[i] / 2)
      {
      radiusLevels[i] = this->Divisions[i] / 2;
      }
    }

  radiusLevel = radiusLevels[0];
  radiusLevel = radiusLevels[1] > radiusLevel ? radiusLevels[1] : radiusLevel;
  radiusLevel = radiusLevels[2] > radiusLevel ? radiusLevels[2] : radiusLevel;
  if (radiusLevel == 0)
    {
    radiusLevel = 1;
    }

  // radius schedule increases the radius each iteration, this is currently
  // implemented by decreasing ii by 1 each iteration.  another alternative
  // is to double the radius each iteration, i.e. ii = ii >> 1
  // In practice, reducing ii by one has been found to be more efficient.
  int numberOfBucketsPerPlane;
  numberOfBucketsPerPlane = this->Divisions[0]*this->Divisions[1];
  prevMinLevel[0] = prevMaxLevel[0] = ijk[0];
  prevMinLevel[1] = prevMaxLevel[1] = ijk[1];
  prevMinLevel[2] = prevMaxLevel[2] = ijk[2];
  for (ii=radiusLevel; ii >= 1; ii--)
    {
    currentRadius = refinedRadius; // used in if at bottom of this for loop

    // Build up a list of buckets that are arranged in rings
    this->GetOverlappingBuckets(&buckets, x, refinedRadius/ii, prevMinLevel,
                                prevMaxLevel);

    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);

      // do we still need to test this bucket?
      if (this->Distance2ToBucket(x, nei) < refinedRadius2)
        {
        ptIds = this->HashTable[nei[0] + nei[1]*this->Divisions[0] +
          nei[2]*numberOfBucketsPerPlane];

        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          if (flag)
            {
            pointData->GetTuple(ptId, pt);
            }
          else
            {
            this->DataSet->GetPoint(ptId, pt);
            }
          if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
            {
            closest = ptId;
            minDist2 = dist2;
            refinedRadius = sqrt(minDist2);
            refinedRadius2 = minDist2;
            }
          }//for each pt in bucket
        }//if bucket is within the current best distance
      }//for each overlapping bucket

    // don't want to checker a smaller radius than we just checked so update
    // ii appropriately
    if (refinedRadius < currentRadius && ii > 2) //always check ii==1
      {
      ii = static_cast<int>(static_cast<double>(ii)
                            * (refinedRadius / currentRadius)) + 1;
      if (ii < 2)
        {
        ii = 2;
        }
      }
    }//for each radius in the radius schedule

  if ((closest != -1) && (minDist2 <= radius2))
    {
    dist2 = minDist2;
    }
  else
    {
    closest = -1;
    }

  return closest;
  }



struct idsort
{
  vtkIdType id;
  double dist;
};

#ifdef _WIN32_WCE
static int __cdecl vtkidsortcompare(const void *arg1, const void *arg2)
#else
extern "C"
{
  int vtkidsortcompare(const void *arg1, const void *arg2)
#endif
{
  idsort *v1 = (idsort *)arg1;
  idsort *v2 = (idsort *)arg2;
  if (v1->dist < v2->dist)
    {
    return -1;
    }
  if (v1->dist > v2->dist)
    {
    return 1;
    }
  return 0;
}
#ifndef _WIN32_WCE
} // close extern "C"
#endif

void vtkPointLocator::FindDistributedPoints(int N, double x,
                                            double y, double z,
                                            vtkIdList *result, int M)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->FindDistributedPoints(N,p,result, M);
}

static int GetOctent(const double x[3], const double pt[3])
{
  double tmp[3];
  int res = 0;

  tmp[0] = pt[0] - x[0];
  tmp[1] = pt[1] - x[1];
  tmp[2] = pt[2] - x[2];

  if (tmp[0] > 0.0)
    {
    res += 1;
    }
  if (tmp[1] > 0.0)
    {
    res += 2;
    }
  if (tmp[2] > 0.0)
    {
    res += 4;
    }

  return res;
}

static int GetMin(const int foo[8])
{
  int result = foo[0];
  int i;

  for (i = 1; i < 8; i++)
    {
    if (foo[i] < result)
      {
      result = foo[i];
      }
    }
  return result;
}

static double GetMax(const double foo[8])
{
  double result = foo[0];
  int i;

  for (i = 1; i < 8; i++)
    {
    if (foo[i] > result)
      {
      result = foo[i];
      }
    }
  return result;
}

void vtkPointLocator::FindDistributedPoints(int N, const double x[3],
                                            vtkIdList *result, int M)
{
  int i, j;
  double dist2;
  double pt[3];
  int level;
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  int oct;
  int pointsChecked = 0;
  vtkNeighborPoints buckets;

  // clear out the result
  result->Reset();

  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<3; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return;
      }
    }
  //
  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  // there are two steps, first a simple expanding wave of buckets until
  // we have enough points. Then a refinement to make sure we have the
  // N closest points.
  level = 0;
  double maxDistance[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  int currentCount[8] = {0,0,0,0,0,0,0,0};
  int minCurrentCount = 0;

  idsort *res[8];
  for (i = 0; i < 8; i++)
    {
    res[i] = new idsort [N];
    }

  this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);
  while (buckets.GetNumberOfNeighbors() &&
         minCurrentCount < N &&
         pointsChecked < M)
    {
    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          pointsChecked++;
          ptId = ptIds->GetId(j);
          this->DataSet->GetPoint(ptId, pt);
          dist2 = vtkMath::Distance2BetweenPoints(x,pt);
          oct = GetOctent(x,pt);
          if (currentCount[oct] < N)
            {
            res[oct][currentCount[oct]].dist = dist2;
            res[oct][currentCount[oct]].id = ptId;
            if (dist2 > maxDistance[oct])
              {
              maxDistance[oct] = dist2;
              }
            currentCount[oct] = currentCount[oct] + 1;
            // compute new minCurrentCount
            minCurrentCount = GetMin(currentCount);
            if (currentCount[oct] == N)
              {
              qsort(res[oct], currentCount[oct], sizeof(idsort),vtkidsortcompare);
              }
            }
          else if (dist2 < maxDistance[oct])
            {
            res[oct][N-1].dist = dist2;
            res[oct][N-1].id = ptId;
            qsort(res[oct], N, sizeof(idsort), vtkidsortcompare);
            maxDistance[oct] = res[oct][N-1].dist;
            }
          }
        }
      }
    level++;
    this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);
    }

  // do a sort
  for (i = 0; i < 8; i++)
    {
    qsort(res[i], currentCount[i], sizeof(idsort), vtkidsortcompare);
    }

  // Now do the refinement
  this->GetOverlappingBuckets (&buckets,
                               x, ijk, sqrt(GetMax(maxDistance)),level-1);

  for (i=0; pointsChecked < M && i<buckets.GetNumberOfNeighbors(); i++)
    {
    nei = buckets.GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0] +
      nei[2]*this->Divisions[0]*this->Divisions[1];

    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++)
        {
        pointsChecked++;
        ptId = ptIds->GetId(j);
        this->DataSet->GetPoint(ptId, pt);
        dist2 = vtkMath::Distance2BetweenPoints(x,pt);
        oct = GetOctent(x,pt);
        if (dist2 < maxDistance[oct])
          {
          res[oct][N-1].dist = dist2;
          res[oct][N-1].id = ptId;
          qsort(res[oct], N, sizeof(idsort), vtkidsortcompare);
          maxDistance[oct] = res[oct][N-1].dist;
          }
        }
      }
    }

  // Fill in the IdList
  for (j = 0; j < 8; j++)
    {
    for (i = 0; i < currentCount[j]; i++)
      {
      result->InsertNextId(res[j][i].id);
      }
    delete [] res[j];
    }
}

void vtkPointLocator::FindClosestNPoints(int N, const double x[3],
                                         vtkIdList *result)
{
  int i, j;
  double dist2;
  double pt[3];
  int level;
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  vtkNeighborPoints buckets;

  // clear out the result
  result->Reset();

  this->BuildLocator(); // will subdivide if modified; otherwise returns

  //
  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  // there are two steps, first a simple expanding wave of buckets until
  // we have enough points. Then a refinement to make sure we have the
  // N closest points.
  level = 0;
  double maxDistance = 0.0;
  int currentCount = 0;
  idsort *res = new idsort [N];

  this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);
  while (buckets.GetNumberOfNeighbors() && currentCount < N)
    {
    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          this->DataSet->GetPoint(ptId, pt);
          dist2 = vtkMath::Distance2BetweenPoints(x,pt);
          if (currentCount < N)
            {
            res[currentCount].dist = dist2;
            res[currentCount].id = ptId;
            if (dist2 > maxDistance)
              {
              maxDistance = dist2;
              }
            currentCount++;
            if (currentCount == N)
              {
              qsort(res, currentCount, sizeof(idsort), vtkidsortcompare);
              }
            }
          else if (dist2 < maxDistance)
            {
            res[N-1].dist = dist2;
            res[N-1].id = ptId;
            qsort(res, N, sizeof(idsort), vtkidsortcompare);
            maxDistance = res[N-1].dist;
            }
          }
        }
      }
    level++;
    this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);
    }

  // do a sort
  qsort(res, currentCount, sizeof(idsort), vtkidsortcompare);

  // Now do the refinement
  this->GetOverlappingBuckets (&buckets, x, ijk, sqrt(maxDistance),level-1);

  for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
    {
    nei = buckets.GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0] +
      nei[2]*this->Divisions[0]*this->Divisions[1];

    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++)
        {
        ptId = ptIds->GetId(j);
        this->DataSet->GetPoint(ptId, pt);
        dist2 = vtkMath::Distance2BetweenPoints(x,pt);
        if (dist2 < maxDistance)
          {
          res[N-1].dist = dist2;
          res[N-1].id = ptId;
          qsort(res, N, sizeof(idsort), vtkidsortcompare);
          maxDistance = res[N-1].dist;
          }
        }
      }
    }

  // Fill in the IdList
  result->SetNumberOfIds(currentCount);
  for (i = 0; i < currentCount; i++)
    {
    result->SetId(i,res[i].id);
    }

  delete [] res;
}

void vtkPointLocator::FindPointsWithinRadius(double R, const double x[3],
                                             vtkIdList *result)
{
  int i, j;
  double dist2;
  double pt[3];
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  double R2 = R*R;
  vtkNeighborPoints buckets;

  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  // get all buckets within a distance
  this->GetOverlappingBuckets (&buckets, x, ijk, R, 0);
  // add the original bucket
  buckets.InsertNextPoint(ijk);

  // clear out the result
  result->Reset();

  for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
    {
    nei = buckets.GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0] +
      nei[2]*this->Divisions[0]*this->Divisions[1];

    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++)
        {
        ptId = ptIds->GetId(j);
        this->DataSet->GetPoint(ptId, pt);
        dist2 = vtkMath::Distance2BetweenPoints(x,pt);
        if (dist2 <= R2)
          {
          result->InsertNextId(ptId);
          }
        }
      }
    }

}

//
//  Method to form subdivision of space based on the points provided and
//  subject to the constraints of levels and NumberOfPointsPerBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vtkPointLocator::BuildLocator()
{
  double *bounds;
  vtkIdType numBuckets;
  double level;
  int ndivs[3];
  int i;
  vtkIdType idx;
  vtkIdList *bucket;
  vtkIdType numPts;
  double x[3];
  typedef vtkIdList *vtkIdListPtr;

  if ( (this->HashTable != NULL) && (this->BuildTime > this->MTime)
       && (this->BuildTime > this->DataSet->GetMTime()) )
    {
    return;
    }

  vtkDebugMacro( << "Hashing points..." );
  this->Level = 1; //only single lowest level

  if ( !this->DataSet || (numPts = this->DataSet->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro( << "No points to subdivide");
    return;
    }
  //
  //  Make sure the appropriate data is available
  //
  if ( this->HashTable )
    {
    this->FreeSearchStructure();
    }
  //
  //  Size the root bucket.  Initialize bucket data structure, compute
  //  level and divisions.
  //
  bounds = this->DataSet->GetBounds();
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i] = bounds[2*i];
    this->Bounds[2*i+1] = bounds[2*i+1];
    if ( this->Bounds[2*i+1] <= this->Bounds[2*i] ) //prevent zero width
      {
      this->Bounds[2*i+1] = this->Bounds[2*i] + 1.0;
      }
    }

  if ( this->Automatic )
    {
    level = static_cast<double>(numPts) / this->NumberOfPointsPerBucket;
    level = ceil( pow(static_cast<double>(level),
                      static_cast<double>(0.33333333)));
    for (i=0; i<3; i++)
      {
      ndivs[i] = static_cast<int>(level);
      }
    }
  else
    {
    for (i=0; i<3; i++)
      {
      ndivs[i] = static_cast<int>(this->Divisions[i]);
      }
    }

  for (i=0; i<3; i++)
    {
    ndivs[i] = (ndivs[i] > 0 ? ndivs[i] : 1);
    this->Divisions[i] = ndivs[i];
    }

  this->NumberOfBuckets = numBuckets = ndivs[0]*ndivs[1]*ndivs[2];
  this->HashTable = new vtkIdListPtr[numBuckets];
  memset (this->HashTable, 0, numBuckets*sizeof(vtkIdListPtr));
  //
  //  Compute width of bucket in three directions
  //
  for (i=0; i<3; i++)
    {
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / ndivs[i] ;
    }
  //
  //  Insert each point into the appropriate bucket.  Make sure point
  //  falls within bucket.
  //
  for (i=0; i<numPts; i++)
    {
    this->DataSet->GetPoint(i, x);
    idx = this->GetBucketIndex(x);
    bucket = this->HashTable[idx];
    if ( ! bucket )
      {
      bucket = vtkIdList::New();
      bucket->Allocate(this->NumberOfPointsPerBucket,
                       this->NumberOfPointsPerBucket/3);
      this->HashTable[idx] = bucket;
      }
    bucket->InsertNextId(i);
    }

  this->BuildTime.Modified();
}


//
//  Internal function to get bucket neighbors at specified level
//
void vtkPointLocator::GetBucketNeighbors(vtkNeighborPoints* buckets,
                                         const int ijk[3], const int ndivs[3],
                                         int level)
{
  int i, j, k, min, max, minLevel[3], maxLevel[3];
  int nei[3];
  //
  //  Initialize
  //
  buckets->Reset();
  //
  //  If at this bucket, just place into list
  //
  if ( level == 0 )
    {
    buckets->InsertNextPoint(ijk);
    return;
    }
  //
  //  Create permutations of the ijk indices that are at the level
  //  required. If these are legal buckets, add to list for searching.
  //
  for ( i=0; i<3; i++ )
    {
    min = ijk[i] - level;
    max = ijk[i] + level;
    minLevel[i] = ( min > 0 ? min : 0);
    maxLevel[i] = ( max < (ndivs[i]-1) ? max : (ndivs[i]-1));
    }

  for ( i= minLevel[0]; i <= maxLevel[0]; i++ )
    {
    for ( j= minLevel[1]; j <= maxLevel[1]; j++ )
      {
      for ( k= minLevel[2]; k <= maxLevel[2]; k++ )
        {
        if (i == (ijk[0] + level) || i == (ijk[0] - level) ||
            j == (ijk[1] + level) || j == (ijk[1] - level) ||
            k == (ijk[2] + level) || k == (ijk[2] - level) )
          {
          nei[0]=i; nei[1]=j; nei[2]=k;
          buckets->InsertNextPoint(nei);
          }
        }
      }
    }

  return;
}


//
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ijk are returned
void vtkPointLocator::GetOverlappingBuckets(vtkNeighborPoints* buckets,
                                            const double x[3],
                                            const int ijk[3],
                                            double dist, int level)
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  for (i=0; i < 3; i++)
    {
    minLevel[i] = static_cast<int>(
      static_cast<double>(((x[i]-dist) - this->Bounds[2*i]) /
                          (this->Bounds[2*i+1] - this->Bounds[2*i]))
      * this->Divisions[i]);
    maxLevel[i] = static_cast<int>(
      static_cast<double>(((x[i]+dist) - this->Bounds[2*i]) /
                          (this->Bounds[2*i+1] - this->Bounds[2*i]))
      * this->Divisions[i]);

    if ( minLevel[i] < 0 )
      {
      minLevel[i] = 0;
      }
    if ( maxLevel[i] >= this->Divisions[i] )
      {
      maxLevel[i] = this->Divisions[i] - 1;
      }
    }

  for ( i= minLevel[0]; i <= maxLevel[0]; i++ )
    {
    for ( j= minLevel[1]; j <= maxLevel[1]; j++ )
      {
      for ( k= minLevel[2]; k <= maxLevel[2]; k++ )
        {
        if ( i < (ijk[0]-level) || i > (ijk[0]+level) ||
             j < (ijk[1]-level) || j > (ijk[1]+level) ||
             k < (ijk[2]-level) || k > (ijk[2]+level))
          {
          nei[0]=i; nei[1]=j; nei[2]=k;
          buckets->InsertNextPoint(nei);
          }
        }
      }
    }
}


//
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ijk are returned
void vtkPointLocator::GetOverlappingBuckets(vtkNeighborPoints* buckets,
                                            const double x[3], double dist,
                                            int prevMinLevel[3],
                                            int prevMaxLevel[3])
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];
  int kFactor, jFactor;
  int jkSkipFlag, kSkipFlag;

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  for (i=0; i < 3; i++)
    {
    minLevel[i] = static_cast<int>(
      static_cast<double>(((x[i]-dist) - this->Bounds[2*i]) / this->H[i]));
    maxLevel[i] = static_cast<int>(
      static_cast<double>(((x[i]+dist) - this->Bounds[2*i]) / this->H[i]));

    if ( minLevel[i] < 0 )
      {
      minLevel[i] = 0;
      }
    else if (minLevel[i] >= this->Divisions[i] )
      {
      minLevel[i] = this->Divisions[i] - 1;
      }
    if ( maxLevel[i] >= this->Divisions[i] )
      {
      maxLevel[i] = this->Divisions[i] - 1;
      }
    else if ( maxLevel[i] < 0 )
      {
      maxLevel[i] = 0;
      }
    }


  if (minLevel[0] == prevMinLevel[0] && maxLevel[0] == prevMaxLevel[0] &&
      minLevel[1] == prevMinLevel[1] && maxLevel[1] == prevMaxLevel[1] &&
      minLevel[2] == prevMinLevel[2] && maxLevel[2] == prevMaxLevel[2] )
    {
    return;
    }


  for ( k= minLevel[2]; k <= maxLevel[2]; k++ )
    {
    kFactor = k*this->Divisions[0]*this->Divisions[1];
    if (k >= prevMinLevel[2] && k <= prevMaxLevel[2])
      {
      kSkipFlag = 1;
      }
    else
      {
      kSkipFlag = 0;
      }
    for ( j= minLevel[1]; j <= maxLevel[1]; j++ )
      {
      if (kSkipFlag && j >= prevMinLevel[1] && j <= prevMaxLevel[1])
        {
        jkSkipFlag = 1;
        }
      else
        {
        jkSkipFlag = 0;
        }
      jFactor = j*this->Divisions[0];
      for ( i= minLevel[0]; i <= maxLevel[0]; i++ )
        {
        if ( jkSkipFlag && i == prevMinLevel[0] )
          {
          i = prevMaxLevel[0];
          continue;
          }
        // if this bucket has any cells, add it to the list
        if (this->HashTable[i + jFactor + kFactor])
          {
          nei[0]=i; nei[1]=j; nei[2]=k;
          buckets->InsertNextPoint(nei);
          }
        }
      }
    }

  prevMinLevel[0] = minLevel[0];
  prevMinLevel[1] = minLevel[1];
  prevMinLevel[2] = minLevel[2];
  prevMaxLevel[0] = maxLevel[0];
  prevMaxLevel[1] = maxLevel[1];
  prevMaxLevel[2] = maxLevel[2];
  }


// Initialize the point insertion process. The newPts is an object representing
// point coordinates into which incremental insertion methods place their
// data. Bounds are the box that the points lie in.
int vtkPointLocator::InitPointInsertion(vtkPoints *newPts,
                                        const double bounds[6])
{
  return this->InitPointInsertion(newPts,bounds,0);
}

// Initialize the point insertion process. The newPts is an object representing
// point coordinates into which incremental insertion methods place their
// data. Bounds are the box that the points lie in.
int vtkPointLocator::InitPointInsertion(vtkPoints *newPts,
                                        const double bounds[6],
                                        vtkIdType estNumPts)
{
  int i;
  int maxDivs;
  typedef vtkIdList *vtkIdListPtr;
  double hmin;
  int ndivs[3];
  double level;

  this->InsertionPointId = 0;
  if ( this->HashTable )
    {
    this->FreeSearchStructure();
    }
  if ( newPts == NULL )
    {
    vtkErrorMacro(<<"Must define points for point insertion");
    return 0;
    }
  if (this->Points != NULL)
    {
    this->Points->UnRegister(this);
    }
  this->Points = newPts;
  this->Points->Register(this);

  for (i=0; i<3; i++)
    {
    this->Bounds[2*i] = bounds[2*i];
    this->Bounds[2*i+1] = bounds[2*i+1];
    if ( this->Bounds[2*i+1] <= this->Bounds[2*i] )
      {
      this->Bounds[2*i+1] = this->Bounds[2*i] + 1.0;
      }
    }

  if ( this->Automatic && (estNumPts > 0) )
    {
    level = static_cast<double>(estNumPts) / this->NumberOfPointsPerBucket;
    level = ceil( pow(static_cast<double>(level),
                      static_cast<double>(0.33333333)) );
    for (i=0; i<3; i++)
      {
      ndivs[i] = static_cast<int>(level);
      }
    }
  else
    {
    for (i=0; i<3; i++)
      {
      ndivs[i] = static_cast<int>(this->Divisions[i]);
      }
    }

  for (i=0; i<3; i++)
    {
    ndivs[i] = (ndivs[i] > 0 ? ndivs[i] : 1);
    this->Divisions[i] = ndivs[i];
    }

  this->NumberOfBuckets = ndivs[0]*ndivs[1]*ndivs[2];
  this->HashTable = new vtkIdListPtr[this->NumberOfBuckets];
  memset (this->HashTable, 0, this->NumberOfBuckets*
          sizeof(vtkIdListPtr));
  //
  //  Compute width of bucket in three directions
  //
  for (i=0; i<3; i++)
    {
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / ndivs[i] ;
    }

  this->InsertionTol2 = this->Tolerance * this->Tolerance;

  for (maxDivs=0, hmin=VTK_DOUBLE_MAX, i=0; i<3; i++)
    {
    hmin = (this->H[i] < hmin ? this->H[i] : hmin);
    maxDivs = (maxDivs > this->Divisions[i] ? maxDivs : this->Divisions[i]);
    }
  this->InsertionLevel = ceil (static_cast<double>(this->Tolerance) / hmin);
  this->InsertionLevel =
    (this->InsertionLevel > maxDivs ? maxDivs : this->InsertionLevel);
  return 1;
}


// Incrementally insert a point into search structure. The method returns
// the insertion location (i.e., point id). You should use the method
// IsInsertedPoint() to see whether this point has already been
// inserted (that is, if you desire to prevent dulicate points).
// Before using this method you must make sure that newPts have been
// supplied, the bounds has been set properly, and that divs are
// properly set. (See InitPointInsertion().)
vtkIdType vtkPointLocator::InsertNextPoint(const double x[3])
{
  vtkIdType idx;
  vtkIdList *bucket;

  idx = this->GetBucketIndex(x);

  if ( ! (bucket = this->HashTable[idx]) )
    {
    bucket = vtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket/2,
                     this->NumberOfPointsPerBucket/3);
    this->HashTable[idx] = bucket;
    }

  bucket->InsertNextId(this->InsertionPointId);
  this->Points->InsertPoint(this->InsertionPointId,x);
  return this->InsertionPointId++;
}

// Incrementally insert a point into search structure with a particular
// index value. You should use the method IsInsertedPoint() to see whether
// this point has already been inserted (that is, if you desire to prevent
// duplicate points). Before using this method you must make sure that
// newPts have been supplied, the bounds has been set properly, and that
// divs are properly set. (See InitPointInsertion().)
void vtkPointLocator::InsertPoint(vtkIdType ptId, const double x[3])
{
  vtkIdType idx;
  vtkIdList *bucket;

  idx = this->GetBucketIndex(x);

  if ( ! (bucket = this->HashTable[idx]) )
    {
    bucket = vtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket,
                     this->NumberOfPointsPerBucket/3);
    this->HashTable[idx] = bucket;
    }

  bucket->InsertNextId(ptId);
  this->Points->InsertPoint(ptId,x);
}

// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
vtkIdType vtkPointLocator::IsInsertedPoint(const double x[3])
{
  int i, j, ijk[3];
  vtkNeighborPoints buckets;

  //  Locate bucket that point is in.
  //
  this->GetBucketIndices(x, ijk);

  // Check the list of points in that bucket for merging.  Also need to
  // search all neighboring buckets within the tolerance.  The number
  // and level of neighbors to search depends upon the tolerance and
  // the bucket width.
  //
  int *nei, lvtk;
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  double pt[3];

  for (lvtk=0; lvtk <= this->InsertionLevel; lvtk++)
    {
    this->GetBucketNeighbors (&buckets, ijk, this->Divisions, lvtk);

    for ( i=0; i < buckets.GetNumberOfNeighbors(); i++ )
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
        nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          this->Points->GetPoint(ptId, pt);

          if ( vtkMath::Distance2BetweenPoints(x,pt) <= this->InsertionTol2 )
            {
            return ptId;
            }
          }
        } //if points in bucket
      } //for each neighbor
    } //for neighbors at this level

  return -1;
}

int vtkPointLocator::InsertUniquePoint(const double x[3], vtkIdType &id)
{
  vtkIdType ptId;

  ptId = this->IsInsertedPoint(x);

  if (ptId > -1)
    {
    id = ptId;
    return 0;
    }
  else
    {
    id = this->InsertNextPoint(x);
    return 1;
    }
}


// Given a position x, return the id of the point closest to it. This method
// is used when performing incremental point insertion.
vtkIdType vtkPointLocator::FindClosestInsertedPoint(const double x[3])
{
  int i;
  double minDist2, dist2;
  double pt[3];
  int level;
  vtkIdType closest, j;
  vtkIdType ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  int MULTIPLES;
  double diff;
  vtkNeighborPoints buckets;

  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<3; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return -1;
      }
    }

  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  //  Need to search this bucket for closest point.  If there are no
  //  points in this bucket, search 1st level neighbors, and so on,
  //  until closest point found.
  //
  for (closest=0,minDist2=VTK_DOUBLE_MAX,level=0; (closest == 0) &&
  (level < this->Divisions[0] || level < this->Divisions[1] ||
  level < this->Divisions[2]); level++)
    {
    this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);

    for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
      {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] +
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptId = ptIds->GetId(j);
          this->Points->GetPoint(ptId, pt);
          if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
            {
            closest = ptId;
            minDist2 = dist2;
            }
          }
        }
      }
    }
  //
  //  Because of the relative location of the points in the spatial_hash, this
  //  may not be the closest point.  Have to search those bucket
  //  neighbors (one level further out) that might also contain point.
  //
  this->GetBucketNeighbors (&buckets, ijk, this->Divisions, level);
  //
  //  Don't want to search all the neighbors, only those that could
  //  possibly have points closer than the current closest.
  //
  for (i=0; i<buckets.GetNumberOfNeighbors(); i++)
    {
    nei = buckets.GetPoint(i);

    for (dist2=0,j=0; j<3; j++)
      {
      if ( ijk[j] != nei[j] )
        {
        MULTIPLES = (ijk[j]>nei[j] ? (nei[j]+1) : nei[j]);
        diff = (this->Bounds[2*j] + MULTIPLES * this->H[j]) - x[j];
        dist2 += diff*diff;
        }
      }

      if ( dist2 < minDist2 )
        {
        cno = nei[0] + nei[1]*this->Divisions[0] + nei[2]*this->Divisions[0]*this->Divisions[1];

        if ( (ptIds = this->HashTable[cno]) )
          {
          for (j=0; j < ptIds->GetNumberOfIds(); j++)
            {
            ptId = ptIds->GetId(j);
            this->Points->GetPoint(ptId, pt);
            if ( (dist2 = vtkMath::Distance2BetweenPoints(x,pt)) < minDist2 )
              {
              closest = ptId;
              minDist2 = dist2;
              }
            }
          }
        }
      }

    return closest;
}

// Return the list of points in the bucket containing x.
vtkIdList *vtkPointLocator::GetPointsInBucket(const double x[3],
                                              int ijk[3])
{
  int i;

  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<3; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return NULL;
      }
    }

  this->GetBucketIndices(x, ijk);

  // Get the id list, if any
  if ( this->HashTable )
    {
    vtkIdType idx = this->GetBucketIndex(x);
    return this->HashTable[idx];
    }

  return NULL;
}


// Build polygonal representation of locator. Create faces that separate
// inside/outside buckets, or separate inside/boundary of locator.
void vtkPointLocator::GenerateRepresentation(int vtkNotUsed(level),
                                             vtkPolyData *pd)
{
  vtkPoints *pts;
  vtkCellArray *polys;
  int ii, i, j, k, idx, offset[3], minusOffset[3], inside, sliceSize;

  if ( this->HashTable == NULL )
    {
    vtkErrorMacro(<<"Can't build representation...no data!");
    return;
    }

  pts = vtkPoints::New();
  pts->Allocate(5000);
  polys = vtkCellArray::New();
  polys->Allocate(10000);

  // loop over all buckets, creating appropriate faces
  sliceSize = this->Divisions[0] * this->Divisions[1];
  for ( k=0; k < this->Divisions[2]; k++)
    {
    offset[2] = k * sliceSize;
    minusOffset[2] = (k-1) * sliceSize;
    for ( j=0; j < this->Divisions[1]; j++)
      {
      offset[1] = j * this->Divisions[0];
      minusOffset[1] = (j-1) * this->Divisions[0];
      for ( i=0; i < this->Divisions[0]; i++)
        {
        offset[0] = i;
        minusOffset[0] = i - 1;
        idx = offset[0] + offset[1] + offset[2];
        if ( this->HashTable[idx] == NULL )
          {
          inside = 0;
          }
        else
          {
          inside = 1;
          }

        //check "negative" neighbors
        for (ii=0; ii < 3; ii++)
          {
          if ( minusOffset[ii] < 0 )
            {
            if ( inside )
              {
              this->GenerateFace(ii,i,j,k,pts,polys);
              }
            }
          else
            {
            if ( ii == 0 )
              {
              idx = minusOffset[0] + offset[1] + offset[2];
              }
            else if ( ii == 1 )
              {
              idx = offset[0] + minusOffset[1] + offset[2];
              }
            else
              {
              idx = offset[0] + offset[1] + minusOffset[2];
              }

            if ( (this->HashTable[idx] == NULL && inside) ||
            (this->HashTable[idx] != NULL && !inside) )
              {
              this->GenerateFace(ii,i,j,k,pts,polys);
              }
            }
          //those buckets on "positive" boundaries can generate faces specially
          if ( (i+1) >= this->Divisions[0] && inside )
            {
            this->GenerateFace(0,i+1,j,k,pts,polys);
            }
          if ( (j+1) >= this->Divisions[1] && inside )
            {
            this->GenerateFace(1,i,j+1,k,pts,polys);
            }
          if ( (k+1) >= this->Divisions[2] && inside )
            {
            this->GenerateFace(2,i,j,k+1,pts,polys);
            }

          }//over negative faces
        }//over i divisions
      }//over j divisions
    }//over k divisions


  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

void vtkPointLocator::GenerateFace(int face, int i, int j, int k,
                                   vtkPoints *pts, vtkCellArray *polys)
{
  vtkIdType ids[4];
  double origin[3], x[3];

  // define first corner
  origin[0] = this->Bounds[0] + i * this->H[0];
  origin[1] = this->Bounds[2] + j * this->H[1];
  origin[2] = this->Bounds[4] + k * this->H[2];
  ids[0] = pts->InsertNextPoint(origin);

  if ( face == 0 ) //x face
    {
    x[0] = origin[0];
    x[1] = origin[1] + this->H[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + this->H[1];
    x[2] = origin[2] + this->H[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + this->H[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  else if ( face == 1 ) //y face
    {
    x[0] = origin[0] + this->H[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + this->H[0];
    x[1] = origin[1];
    x[2] = origin[2] + this->H[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + this->H[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  else //z face
    {
    x[0] = origin[0] + this->H[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + this->H[0];
    x[1] = origin[1] + this->H[1];
    x[2] = origin[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + this->H[1];
    x[2] = origin[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  polys->InsertNextCell(4,ids);
}


// Calculate the distance between the point x to the bucket "nei".
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
//
double vtkPointLocator::Distance2ToBucket(const double x[3],
                                         const int nei[3])
{
  double bounds[6];

  bounds[0] =     nei[0]*this->H[0] + this->Bounds[0];
  bounds[1] = (nei[0]+1)*this->H[0] + this->Bounds[0];
  bounds[2] =     nei[1]*this->H[1] + this->Bounds[2];
  bounds[3] = (nei[1]+1)*this->H[1] + this->Bounds[2];
  bounds[4] =     nei[2]*this->H[2] + this->Bounds[4];
  bounds[5] = (nei[2]+1)*this->H[2] + this->Bounds[4];

  return this->Distance2ToBounds(x, bounds);
}

// Calculate the distance between the point x and the specified bounds
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
double vtkPointLocator::Distance2ToBounds(const double x[3],
                                         const double bounds[6])
{
  double distance;
  double deltas[3];

  // Are we within the bounds?
  if (x[0] >= bounds[0] && x[0] <= bounds[1]
    && x[1] >= bounds[2] && x[1] <= bounds[3]
    && x[2] >= bounds[4] && x[2] <= bounds[5])
    {
    return 0.0;
    }

  deltas[0] = deltas[1] = deltas[2] = 0.0;

  // dx
  //
  if (x[0] < bounds[0])
    {
    deltas[0] = bounds[0] - x[0];
    }
  else if (x[0] > bounds[1])
    {
    deltas[0] = x[0] - bounds[1];
    }

  // dy
  //
  if (x[1] < bounds[2])
    {
    deltas[1] = bounds[2] - x[1];
    }
  else if (x[1] > bounds[3])
    {
    deltas[1] = x[1] - bounds[3];
    }

  // dz
  //
  if (x[2] < bounds[4])
    {
    deltas[2] = bounds[4] - x[2];
    }
  else if (x[2] > bounds[5])
    {
    deltas[2] = x[2] - bounds[5];
    }

  distance = vtkMath::Dot(deltas, deltas);
  return distance;
}

vtkIdType vtkPointLocator::GetBucketIndex(const double x[3])
{
  int ijk[3];
  this->GetBucketIndices(x, ijk);
  return ( ijk[0] + ijk[1]*this->Divisions[0] +
           ijk[2]*this->Divisions[0]*this->Divisions[1] );
}

void vtkPointLocator::GetBucketIndices(const double x[3], int ijk[3])
{
  for (int j=0; j<3; j++)
    {
    ijk[j] = static_cast<int>(
      ((x[j] - this->Bounds[2*j]) /
       (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);

    if (ijk[j] < 0)
      {
      ijk[j] = 0;
      }
    else if (ijk[j] >= this->Divisions[j])
      {
      ijk[j] = this->Divisions[j] - 1;
      }
    }
}

void vtkPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Points Per Bucket: " << this->NumberOfPointsPerBucket << "\n";
  os << indent << "Divisions: (" << this->Divisions[0] << ", "
     << this->Divisions[1] << ", " << this->Divisions[2] << ")\n";
  if ( this->Points )
    {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Points: (none)\n";
    }
}

