/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPointLocator2D.h"
#include "vtkMath.h"
#include "vtkIntArray.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPointLocator2D* vtkPointLocator2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPointLocator2D");
  if(ret)
    {
    return (vtkPointLocator2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPointLocator2D;
}




class vtkNeighborPoints2D
{
public:
  vtkNeighborPoints2D(const int sz, const int ext=1000)
    {this->P = vtkIntArray::New(); this->P->Allocate(2*sz,2*ext);};
  ~vtkNeighborPoints2D()
    {
    if (this->P)
      {
      this->P->Delete();
      this->P = NULL;
      }
    }
  int GetNumberOfNeighbors() {return (this->P->GetMaxId()+1)/2;};
  void Reset() {this->P->Reset();};

  int *GetPoint(int i) {return this->P->GetPointer(2*i);};
  int InsertNextPoint(int *x);

protected:
  vtkIntArray *P;
};

inline int vtkNeighborPoints2D::InsertNextPoint(int *x) 
{
  int id = this->P->GetMaxId() + 2;
  this->P->InsertValue(id,x[1]);
  this->P->SetValue(id-1, x[0]);
  return id/2;
}

// Construct with automatic computation of divisions, averaging
// 25 points per bucket.
vtkPointLocator2D::vtkPointLocator2D()
{

  this->Buckets = new vtkNeighborPoints2D(26,50);
  this->Points = NULL;
  this->Divisions[0] = this->Divisions[1] = 50;
  this->NumberOfPointsPerBucket = 3;
  this->HashTable = NULL;
  this->NumberOfBuckets = 0;
  this->H[0] = this->H[1] = 0.0;
  this->InsertionTol2 = 0.0001;
}

vtkPointLocator2D::~vtkPointLocator2D()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }
  if ( this->Buckets)
    {
     delete this->Buckets;
     this->Buckets = NULL;
    }
  this->FreeSearchStructure();
}

void vtkPointLocator2D::Initialize()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }
  this->FreeSearchStructure();
}

void vtkPointLocator2D::FreeSearchStructure()
{
  vtkIdList *ptIds;
  int i;

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
int vtkPointLocator2D::FindClosestPoint(float x[2])
{
  int i, j;
  float minDist2;
  float dist2 = VTK_LARGE_FLOAT;
  float *pt;
  int closest, level;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[2], *nei;

  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<2; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return -1;
      }
    }
  //
  //  Find bucket point is in.  
  //
  for (j=0; j<2; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j]) / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * (this->Divisions[j]-1));
    }
  //
  //  Need to search this bucket for closest point.  If there are no
  //  points in this bucket, search 1st level neighbors, and so on,
  //  until closest point found.
  //
  for (closest=(-1),minDist2=VTK_LARGE_FLOAT,level=0; (closest == -1) && 
	 (level < this->Divisions[0] || level < this->Divisions[1]); level++) 
    {
    this->GetBucketNeighbors (ijk, this->Divisions, level);

    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);
          if ((dist2 =
	       ((x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]))) < minDist2) 
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
    this->GetOverlappingBuckets (x, ijk, sqrt(dist2), 0);

    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);
          if ((dist2 =
	       ((x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]))) < minDist2) 
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

struct idsort
{
  int id;
  float dist;
};

static int idsortcompare(const void *arg1, const void *arg2)
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

void vtkPointLocator2D::FindDistributedPoints(int N, float x,
					      float y,
					      vtkIdList *result, int M)
{
  float p[2];
  p[0] = x;
  p[1] = y;
  this->FindDistributedPoints(N,p,result, M);
}

int GetQuadrant(float *x,float *pt)
{
  float tmp[2];
  int res = 0;
  
  tmp[0] = pt[0] - x[0];
  tmp[1] = pt[1] - x[1];
  
  if (tmp[0] > 0.0)
    {
    res += 1;
    }
  if (tmp[1] > 0.0)
    {
    res += 2;
    }
  return res;
}

static int GetMin(int *foo)
{
  int result = foo[0];
  int i;
  
  for (i = 1; i < 4; i++)
    {
    if (foo[i] < result)
      {
      result = foo[i];
      }
    }
  return result;
}

static float GetMax(float *foo)
{
  float result = foo[0];
  int i;
  
  for (i = 1; i < 4; i++)
    {
    if (foo[i] > result)
      {
      result = foo[i];
      }
    }
  return result;
}

void vtkPointLocator2D::FindDistributedPoints(int N, float x[2],
					    vtkIdList *result, int M)
{
  int i, j;
  float dist2;
  float *pt;
  int level;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[2], *nei;
  int oct;
  int pointsChecked = 0;
  
  // clear out the result
  result->Reset();
    
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<2; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return;
      }
    }
  //
  //  Find bucket point is in.  
  //
  for (j=0; j<2; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j]) / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * (this->Divisions[j]-1));
    }

  // there are two steps, first a simple expanding wave of buckets until
  // we have enough points. Then a refinement to make sure we have the
  // N closest points.
  level = 0;
  float maxDistance[4] = {0.0, 0.0, 0.0, 0.0};
  int currentCount[4] = {0,0,0,0};
  int minCurrentCount = 0;
  
  idsort *res[4];
  for (i = 0; i < 4; i++) 
    {
    res[i] = new idsort [N];
    }
  
  this->GetBucketNeighbors (ijk, this->Divisions, level);
  while (this->Buckets->GetNumberOfNeighbors() && 
	 minCurrentCount < N &&
	 pointsChecked < M)
    {
    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
	  pointsChecked++;
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);
	  dist2 = (x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]);
	  oct = GetQuadrant(x,pt);
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
	      qsort(res[oct], currentCount[oct], sizeof(idsort),idsortcompare);
	      }
	    }
	  else if (dist2 < maxDistance[oct])
	    {
	    res[oct][N-1].dist = dist2;
	    res[oct][N-1].id = ptId;
	    qsort(res[oct], N, sizeof(idsort), idsortcompare);
	    maxDistance[oct] = res[oct][N-1].dist;
	    }
          }
        }
      }
    level++;
    this->GetBucketNeighbors (ijk, this->Divisions, level);
    }

  // do a sort
  for (i = 0; i < 4; i++)
    {
    qsort(res[i], currentCount[i], sizeof(idsort), idsortcompare);
    }
  
  // Now do the refinement
  this->GetOverlappingBuckets (x, ijk, sqrt(GetMax(maxDistance)),level-1);
  
  for (i=0; pointsChecked < M && i<this->Buckets->GetNumberOfNeighbors(); i++) 
    {
    nei = this->Buckets->GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0];
    
    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++) 
	{
	pointsChecked++;
	ptId = ptIds->GetId(j);
	pt = this->DataSet->GetPoint(ptId);
	dist2 = (x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]);
	oct = GetQuadrant(x,pt);
	if (dist2 < maxDistance[oct])
	  {
	  res[oct][N-1].dist = dist2;
	  res[oct][N-1].id = ptId;
	  qsort(res[oct], N, sizeof(idsort), idsortcompare);
	  maxDistance[oct] = res[oct][N-1].dist;
	  }
	}
      }
    }
    
  // Fill in the IdList
  for (j = 0; j < 4; j++)
    {
    for (i = 0; i < currentCount[j]; i++)
      {
      result->InsertNextId(res[j][i].id);
      }
    delete [] res[j];
    }
}

void vtkPointLocator2D::FindClosestNPoints(int N, float x,
					 float y,
					 vtkIdList *result)
{
  float p[2];
  p[0] = x;
  p[1] = y;
  this->FindClosestNPoints(N,p,result);
}

void vtkPointLocator2D::FindClosestNPoints(int N, float x[2],vtkIdList *result)
{
  int i, j;
  float dist2;
  float *pt;
  int level;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[2], *nei;
  
  // clear out the result
  result->Reset();
    
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<2; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return;
      }
    }
  //
  //  Find bucket point is in.  
  //
  for (j=0; j<2; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j]) / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * (this->Divisions[j]-1));
    }

  // there are two steps, first a simple expanding wave of buckets until
  // we have enough points. Then a refinement to make sure we have the
  // N closest points.
  level = 0;
  float maxDistance = 0.0;
  int currentCount = 0;
  idsort *res = new idsort [N];
  
  this->GetBucketNeighbors (ijk, this->Divisions, level);
  while (this->Buckets->GetNumberOfNeighbors() && currentCount < N)
    {
    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);
	  dist2 = (x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]);
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
	      qsort(res, currentCount, sizeof(idsort), idsortcompare);
	      }
	    }
	  else if (dist2 < maxDistance)
	    {
	    res[N-1].dist = dist2;
	    res[N-1].id = ptId;
	    qsort(res, N, sizeof(idsort), idsortcompare);
	    maxDistance = res[N-1].dist;
	    }
          }
        }
      }
    level++;
    this->GetBucketNeighbors (ijk, this->Divisions, level);
    }

  // do a sort
  qsort(res, currentCount, sizeof(idsort), idsortcompare);

  // Now do the refinement
  this->GetOverlappingBuckets (x, ijk, sqrt(maxDistance),level-1);
  
  for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
    {
    nei = this->Buckets->GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0];
    
    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++) 
	{
	ptId = ptIds->GetId(j);
	pt = this->DataSet->GetPoint(ptId);
	dist2 = (x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]);
	if (dist2 < maxDistance)
	  {
	  res[N-1].dist = dist2;
	  res[N-1].id = ptId;
	  qsort(res, N, sizeof(idsort), idsortcompare);
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

void vtkPointLocator2D::FindPointsWithinRadius(float R, float x,
					       float y,
					       vtkIdList *result)
{
  float p[2];
  p[0] = x;
  p[1] = y;
  this->FindPointsWithinRadius(R,p,result);
}

void vtkPointLocator2D::FindPointsWithinRadius(float R, float x[2],
					       vtkIdList *result)
{
  int i, j;
  float dist2;
  float *pt;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[2], *nei;
  float R2 = R*R;
  
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<2; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return;
      }
    }
  //
  //  Find bucket point is in.  
  //
  for (j=0; j<2; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j]) / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * (this->Divisions[j]-1));
    }

  // get all buckets within a distance
  this->GetOverlappingBuckets (x, ijk, R, 0);
  // add the original bucket
  this->Buckets->InsertNextPoint(ijk);

  // clear out the result
  result->Reset();
  
  for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
    {
    nei = this->Buckets->GetPoint(i);
    cno = nei[0] + nei[1]*this->Divisions[0];
    
    if ( (ptIds = this->HashTable[cno]) != NULL )
      {
      for (j=0; j < ptIds->GetNumberOfIds(); j++) 
	{
	ptId = ptIds->GetId(j);
	pt = this->DataSet->GetPoint(ptId);
	dist2 = (x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]);
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
void vtkPointLocator2D::BuildLocator()
{
  float *bounds;
  int numBuckets;
  float level;
  int ndivs[3];
  int i, j, ijk[2];
  int idx;
  vtkIdList *bucket;
  int numPts;
  float *x;
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
  for (i=0; i<2; i++)
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
    level = (float) numPts / this->NumberOfPointsPerBucket;
    level = ceil( pow((double)level,(double)0.5) );
    for (i=0; i<2; i++)
      {
      ndivs[i] = (int) level;
      }
    } 
  else 
    {
    for (i=0; i<2; i++)
      {
      ndivs[i] = (int) this->Divisions[i];
      }
    }

  for (i=0; i<2; i++) 
    {
    ndivs[i] = (ndivs[i] > 0 ? ndivs[i] : 1);
    this->Divisions[i] = ndivs[i];
    }

  this->NumberOfBuckets = numBuckets = ndivs[0]*ndivs[1];
  this->HashTable = new vtkIdListPtr[numBuckets];
  memset (this->HashTable, 0, numBuckets*sizeof(vtkIdListPtr));
  //
  //  Compute width of bucket in three directions
  //
  for (i=0; i<2; i++) 
    {
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / ndivs[i] ;
    }
  //
  //  Insert each point into the appropriate bucket.  Make sure point
  //  falls within bucket.
  //
  for (i=0; i<numPts; i++) 
    {
    x = this->DataSet->GetPoint(i);
    for (j=0; j<2; j++) 
      {
      ijk[j] = (int) ((float) ((x[j] - this->Bounds[2*j]) / 
                        (this->Bounds[2*j+1] - this->Bounds[2*j])) * (ndivs[j]- 1));
      }

    idx = ijk[0] + ijk[1]*ndivs[0];
    bucket = this->HashTable[idx];
    if ( ! bucket )
      {
      bucket = vtkIdList::New();
      bucket->Allocate(this->NumberOfPointsPerBucket/2,
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
void vtkPointLocator2D::GetBucketNeighbors(int ijk[2], int ndivs[2], int level)
{
  int i, j, min, max, minLevel[2], maxLevel[2];
  int nei[2];
  //
  //  Initialize
  //
  this->Buckets->Reset();
  //
  //  If at this bucket, just place into list
  //
  if ( level == 0 ) 
    {
    this->Buckets->InsertNextPoint(ijk);
    return;
    }
  //
  //  Create permutations of the ijk indices that are at the level
  //  required. If these are legal buckets, add to list for searching.
  //
  for ( i=0; i<2; i++ ) 
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
      if (i == (ijk[0] + level) || i == (ijk[0] - level) ||
	  j == (ijk[1] + level) || j == (ijk[1] - level)) 
	{
        nei[0]=i; nei[1]=j;
	this->Buckets->InsertNextPoint(nei);
	}
      }
    }

  return;
}

//
// Internal method to find those buckets that are within distance specified
void vtkPointLocator2D::GetOverlappingBuckets(float x[2], int ijk[2], 
					      float dist, int level)
{
  int i, j, nei[2], minLevel[2], maxLevel[2];

  // Initialize
  this->Buckets->Reset();

  // Determine the range of indices in each direction
  for (i=0; i < 2; i++)
    {
    minLevel[i] = (int) ((float) (((x[i]-dist) - this->Bounds[2*i]) / 
        (this->Bounds[2*i+1] - this->Bounds[2*i])) * (this->Divisions[i] - 1));
    maxLevel[i] = (int) ((float) (((x[i]+dist) - this->Bounds[2*i]) / 
        (this->Bounds[2*i+1] - this->Bounds[2*i])) * (this->Divisions[i] - 1));

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
      if ( i < (ijk[0]-level) || i > (ijk[0]+level) ||
	   j < (ijk[1]-level) || j > (ijk[1]+level))
	{
        nei[0]=i; nei[1]=j;
	this->Buckets->InsertNextPoint(nei);
	}
      }
    }
}



// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
int vtkPointLocator2D::IsInsertedPoint(float x[2])
{
  int i, j, ijk[2];
  int idx;
  vtkIdList *bucket;
  //
  //  Locate bucket that point is in.
  //
  for (i=0; i<2; i++)
    {
    ijk[i] = (int) ((float) ((x[i] - this->Bounds[2*i]) / 
        (this->Bounds[2*i+1] - this->Bounds[2*i])) * (this->Divisions[i] - 1));
    }

  idx = ijk[0] + ijk[1]*this->Divisions[0];

  bucket = this->HashTable[idx];
  if ( ! bucket )
    {
    return -1;
    }
  else // see whether we've got duplicate point
    {
    //
    // Check the list of points in that bucket for merging.  Also need to 
    // search all neighboring buckets within the tolerance.  The number 
    // and level of neighbors to search depends upon the tolerance and 
    // the bucket width.
    //
    int *nei, lvtk, cno, ptId;
    vtkIdList *ptIds;
    float *pt;

    // the InsertionLevel stuff is wacky 
    for (lvtk=0; lvtk <= -1; lvtk++)
      {
      this->GetBucketNeighbors (ijk, this->Divisions, lvtk);

      for ( i=0; i < this->Buckets->GetNumberOfNeighbors(); i++ ) 
        {
        nei = this->Buckets->GetPoint(i);
        cno = nei[0] + nei[1]*this->Divisions[0];

        if ( (ptIds = this->HashTable[cno]) != NULL )
          {
          for (j=0; j < ptIds->GetNumberOfIds(); j++) 
            {
            ptId = ptIds->GetId(j);
            pt = this->Points->GetPoint(ptId);

            if ( ((x[0]-pt[0])*(x[0]-pt[0])+(x[1]-pt[1])*(x[1]-pt[1]))
		 <= this->InsertionTol2 )
              {
              return ptId;
              }
            }
          } //if points in bucket
        } //for each neighbor
      } //for neighbors at this level
    } // else check duplicate point

  return -1;
}


// Build polygonal representation of locator. Create faces that separate
// inside/outside buckets, or separate inside/boundary of locator.
void vtkPointLocator2D::GenerateRepresentation(int vtkNotUsed(level),
					       vtkPolyData *vtkNotUsed(pd))
{
  // to be done
}

void vtkPointLocator2D::GenerateFace(int face, int i, int j, int k, 
                                   vtkPoints *pts, vtkCellArray *polys)
{
  // to be done
  face = face;
  i = i;
  j = j;
  k = k;
  pts = pts;
  polys = polys;
}


void vtkPointLocator2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLocator::PrintSelf(os,indent);

  os << indent << "Number of Points Per Bucket: " << this->NumberOfPointsPerBucket << "\n";
  os << indent << "Divisions: (" << this->Divisions[0] << ", " 
     << this->Divisions[1] << ")\n";

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

