/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPointLocator.hh"
#include "vtkMath.hh"
#include "vtkIntArray.hh"
#include "vtkPolyData.hh"

class vtkNeighborPoints
{
public:
  vtkNeighborPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  int GetNumberOfNeighbors() {return (P.GetMaxId()+1)/3;};
  void Reset() {this->P.Reset();};

  int *GetPoint(int i) {return this->P.GetPtr(3*i);};
  int InsertNextPoint(int *x);

protected:
  vtkIntArray P;
};
// some compiler can't initialize static file scope objects -ugh
static vtkNeighborPoints *Buckets; 

inline int vtkNeighborPoints::InsertNextPoint(int *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]);
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

// Description:
// Construct with automatic computation of divisions, averaging
// 25 points per bucket.
vtkPointLocator::vtkPointLocator()
{
  static vtkNeighborPoints BucketStorage(26,50);
  Buckets = &BucketStorage;

  this->Points = NULL;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->NumberOfPointsPerBucket = 10;
  this->HashTable = NULL;
  this->NumberOfBuckets = 0;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  this->InsertionPointId = 0;
  this->InsertionTol2 = 0.0001;
}

void vtkPointLocator::Initialize()
{
  if ( this->Points ) this->Points->UnRegister(this);

  this->FreeSearchStructure();
}

void vtkPointLocator::FreeSearchStructure()
{
  vtkIdList *ptIds;
  int i;

  if ( this->HashTable )
    {
    for (i=0; i<this->NumberOfBuckets; i++)
      {
      if ( (ptIds = this->HashTable[i]) ) ptIds->Delete();
      }
    delete [] this->HashTable;
    this->HashTable = NULL;
    }
}

// Description:
// Given a position x, return the id of the point closest to it.
int vtkPointLocator::FindClosestPoint(float x[3])
{
  int i, j;
  float minDist2, dist2;
  float *pt;
  int closest, level;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  int MULTIPLES;
  float diff;

  this->BuildLocator(); // will subdivide if modified; otherwise returns
//
//  Make sure candidate point is in bounds.  If not, it is outside.
//
  for (i=0; i<3; i++)
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      return -1;
//
//  Find bucket point is in.  
//
  for (j=0; j<3; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j])*0.999 / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);
    }
//
//  Need to search this bucket for closest point.  If there are no
//  points in this bucket, search 1st level neighbors, and so on,
//  until closest point found.
//
  for (closest=0,minDist2=VTK_LARGE_FLOAT,level=0; (closest == 0) && 
  (level < this->Divisions[0] || level < this->Divisions[1] || 
  level < this->Divisions[2]); level++) 
    {
    this->GetBucketNeighbors (ijk, this->Divisions, level);

    for (i=0; i<Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] + 
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);
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
  this->GetBucketNeighbors (ijk, this->Divisions, level);
//
//  Don't want to search all the neighbors, only those that could
//  possibly have points closer than the current closest.
//
  for (i=0; i<Buckets->GetNumberOfNeighbors(); i++) 
    {
    nei = Buckets->GetPoint(i);

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
            pt = this->DataSet->GetPoint(ptId);
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

// Description:
// Merge points together based on tolerance specified. Return a list 
// that maps unmerged point ids into new point ids.
int *vtkPointLocator::MergePoints()
{
  float tol2;
  int ptId, i, j, k;
  int numPts;
  int *index;
  int newPtId;
  int maxDivs;
  float hmin, *pt, *p;
  int ijk[3], *nei;
  int level, lvtk, cno;
  vtkIdList *ptIds;

  vtkDebugMacro(<<"Merging points");

  if ( this->DataSet == NULL || 
  (numPts=this->DataSet->GetNumberOfPoints()) < 1 ) return NULL;

  this->BuildLocator(); // subdivides if necessary

  index = new int[numPts];
  for (i=0; i < numPts; i++) index[i] = -1;

  tol2 = this->Tolerance * this->Tolerance;
  newPtId = 0; // renumbering points

  for (maxDivs=0, hmin=VTK_LARGE_FLOAT, i=0; i<3; i++) 
    {
    hmin = (this->H[i] < hmin ? this->H[i] : hmin);
    maxDivs = (maxDivs > this->Divisions[i] ? maxDivs : this->Divisions[i]);
    }
  level = (int) (ceil ((double) this->Tolerance / hmin));
  level = (level > maxDivs ? maxDivs : level);
//
//  Traverse each point, find bucket that point is in, check the list of
//  points in that bucket for merging.  Also need to search all
//  neighboring buckets within the tolerance.  The number and level of
//  neighbors to search depends upon the tolerance and the bucket width.
//
  for ( i=0; i < numPts; i++ ) //loop over all points
    {
    // Only try to merge the point if it hasn't yet been merged.

    if ( index[i] == -1 ) 
      {
      p = this->DataSet->GetPoint(i);
      index[i] = newPtId;

      for (j=0; j<3; j++) 
        ijk[j] = (int) ((float)((p[j] - this->Bounds[2*j])*0.999 / 
              (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);

      for (lvtk=0; lvtk <= level; lvtk++) 
        {
        this->GetBucketNeighbors (ijk, this->Divisions, lvtk);

        for ( k=0; k < Buckets->GetNumberOfNeighbors(); k++ ) 
          {
          nei = Buckets->GetPoint(k);
          cno = nei[0] + nei[1]*this->Divisions[0] + 
                nei[2]*this->Divisions[0]*this->Divisions[1];

           if ( (ptIds = this->HashTable[cno]) != NULL )
            {
            for (j=0; j < ptIds->GetNumberOfIds(); j++) 
              {
              ptId = ptIds->GetId(j);
              pt = this->DataSet->GetPoint(ptId);

              if ( index[ptId] == -1 && vtkMath::Distance2BetweenPoints(p,pt) <= tol2 )
                {
                index[ptId] = newPtId;
                }
              }
            }
          }
        }
      newPtId++;
      } // if point hasn't been merged
    } // for all points

  return index;
}

//
//  Method to form subdivision of space based on the points provided and
//  subject to the constraints of levels and NumberOfPointsPerBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vtkPointLocator::BuildLocator()
{
  float *bounds;
  int numBuckets;
  float level;
  int ndivs[3], product;
  int i, j, ijk[3];
  int idx;
  vtkIdList *bucket;
  int numPts;
  int numPtsPerBucket = this->NumberOfPointsPerBucket;
  float *x;
  typedef vtkIdList *vtkIdListPtr;

  if ( this->HashTable != NULL && this->BuildTime > this->MTime ) return;

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
  if ( this->HashTable ) this->FreeSearchStructure();
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
      this->Bounds[2*i+1] = this->Bounds[2*i] + 1.0;
    }

  if ( this->Automatic ) 
    {
    level = (float) numPts / numPtsPerBucket;
    level = ceil( pow((double)level,(double)0.33333333) );
    for (i=0; i<3; i++) ndivs[i] = (int) level;
    } 
  else 
    {
    for (i=0; i<3; i++) ndivs[i] = (int) this->Divisions[i];
    }

  for (i=0; i<3; i++) 
    {
    ndivs[i] = (ndivs[i] > 0 ? ndivs[i] : 1);
    this->Divisions[i] = ndivs[i];
    }

  this->NumberOfBuckets = numBuckets = ndivs[0]*ndivs[1]*ndivs[2];
  this->HashTable = new vtkIdListPtr[numBuckets];
  memset (this->HashTable, (int)NULL, numBuckets*sizeof(vtkIdListPtr));
//
//  Compute width of bucket in three directions
//
  for (i=0; i<3; i++) 
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / ndivs[i] ;
//
//  Insert each point into the appropriate bucket.  Make sure point
//  falls within bucket.
//
  product = ndivs[0]*ndivs[1];
  for (i=0; i<numPts; i++) 
    {
    x = this->DataSet->GetPoint(i);
    for (j=0; j<3; j++) 
      {
      ijk[j] = (int) ((float) ((x[j] - this->Bounds[2*j])*0.999 / 
                        (this->Bounds[2*j+1] - this->Bounds[2*j])) * ndivs[j]);
      }
    idx = ijk[0] + ijk[1]*ndivs[0] + ijk[2]*product;
    bucket = this->HashTable[idx];
    if ( ! bucket )
      {
      bucket = new vtkIdList(numPtsPerBucket/2,numPtsPerBucket/3);
      this->HashTable[idx] = bucket;
      }
    bucket->InsertNextId(i);
    }

  this->BuildTime.Modified();
}


//
//  Internal function to get bucket neighbors at specified level
//
void vtkPointLocator::GetBucketNeighbors(int ijk[3], int ndivs[3], int level)
{
  int i, j, k, min, max, minLevel[3], maxLevel[3];
  int nei[3];
//
//  Initialize
//
  Buckets->Reset();
//
//  If at this bucket, just place into list
//
  if ( level == 0 ) 
    {
    Buckets->InsertNextPoint(ijk);
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
          Buckets->InsertNextPoint(nei);
          }
        }
      }
    }

  return;
}

// specifically for point insertion
static float InsertionLevel;

// Description:
// Initialize the point insertion process. The newPts is an object representing
// point coordinates into which incremental insertion methods place their 
// data. Bounds are the box that the points lie in.
int vtkPointLocator::InitPointInsertion(vtkPoints *newPts, float bounds[6])
{
  int i;
  int maxDivs;
  typedef vtkIdList *vtkIdListPtr;
  float hmin;

  this->InsertionPointId = 0;
  if ( this->HashTable ) this->FreeSearchStructure();
  if ( newPts == NULL )
    {
    vtkErrorMacro(<<"Must define points for point insertion");
    return 0;
    }
  this->Points = newPts;
  this->Points->Register(this);

  for (i=0; i<3; i++)
    {
    this->Bounds[2*i] = bounds[2*i];
    this->Bounds[2*i+1] = bounds[2*i+1];
    if ( this->Bounds[2*i+1] <= this->Bounds[2*i] )
      this->Bounds[2*i+1] = this->Bounds[2*i] + 1.0;
    }

  for (this->NumberOfBuckets=1, i=0; i<3; i++) 
    this->NumberOfBuckets *= this->Divisions[i];

  this->HashTable = new vtkIdListPtr[this->NumberOfBuckets];
  memset (this->HashTable, (int)NULL, this->NumberOfBuckets*sizeof(vtkIdListPtr));
//
//  Compute width of bucket in three directions
//
  for (i=0; i<3; i++) 
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i])/this->Divisions[i];

  this->InsertionTol2 = this->Tolerance * this->Tolerance;

  for (maxDivs=0, hmin=VTK_LARGE_FLOAT, i=0; i<3; i++) 
    {
    hmin = (this->H[i] < hmin ? this->H[i] : hmin);
    maxDivs = (maxDivs > this->Divisions[i] ? maxDivs : this->Divisions[i]);
    }
  InsertionLevel = ceil ((double) this->Tolerance / hmin);
  InsertionLevel = (InsertionLevel > maxDivs ? maxDivs : InsertionLevel);
  
  return 1;
}


// Description:
// Incrementally insert a point into search structure. The method returns
// the insertion location (i.e., point id). You should use the method 
// IsInsertedPoint() to see whether this point has already been
// inserted (that is, if you desire to prevent dulicate points).
// Before using this method you must make sure that newPts have been
// supplied, the bounds has been set properly, and that divs are 
// properly set. (See InitPointInsertion().)
int vtkPointLocator::InsertNextPoint(float x[3])
{
  int i, ijk[3];
  int idx;
  vtkIdList *bucket;
//
//  Locate bucket that point is in.
//
  for (i=0; i<3; i++)
    {
    ijk[i] = (int) ((float) ((x[i] - this->Bounds[2*i])*0.999 / 
             (this->Bounds[2*i+1] - this->Bounds[2*i])) * this->Divisions[i]);
    }
  idx = ijk[0] + ijk[1]*this->Divisions[0] + 
        ijk[2]*this->Divisions[0]*this->Divisions[1];

  if ( ! (bucket = this->HashTable[idx]) )
    {
    bucket = new vtkIdList(this->NumberOfPointsPerBucket/2,
			   this->NumberOfPointsPerBucket/3);
    this->HashTable[idx] = bucket;
    }

  bucket->InsertNextId(this->InsertionPointId);
  this->Points->InsertPoint(this->InsertionPointId,x);
  return this->InsertionPointId++;
}

// Description:
// Incrementally insert a point into search structure with a particular
// index value. You should use the method IsInsertedPoint() to see whether 
// this point has already been inserted (that is, if you desire to prevent
// dulicate points). Before using this method you must make sure that 
// newPts have been supplied, the bounds has been set properly, and that 
// divs are properly set. (See InitPointInsertion().)
void vtkPointLocator::InsertPoint(int ptId, float x[3])
{
  int i, ijk[3];
  int idx;
  vtkIdList *bucket;
//
//  Locate bucket that point is in.
//
  for (i=0; i<3; i++)
    {
    ijk[i] = (int) ((float) ((x[i] - this->Bounds[2*i])*0.999 / 
             (this->Bounds[2*i+1] - this->Bounds[2*i])) * this->Divisions[i]);
    }
  idx = ijk[0] + ijk[1]*this->Divisions[0] + 
        ijk[2]*this->Divisions[0]*this->Divisions[1];

  if ( ! (bucket = this->HashTable[idx]) )
    {
    bucket = new vtkIdList(this->NumberOfPointsPerBucket/2,
			   this->NumberOfPointsPerBucket/3);
    this->HashTable[idx] = bucket;
    }

  bucket->InsertNextId(ptId);
  this->Points->InsertPoint(ptId,x);
}

// Description:
// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
int vtkPointLocator::IsInsertedPoint(float x[3])
{
  int i, j, ijk[3];
  int idx;
  vtkIdList *bucket;
//
//  Locate bucket that point is in.
//
  for (i=0; i<3; i++)
    {
    ijk[i] = (int) ((float) ((x[i] - this->Bounds[2*i])*0.999 / 
             (this->Bounds[2*i+1] - this->Bounds[2*i])) * this->Divisions[i]);
    }
  idx = ijk[0] + ijk[1]*this->Divisions[0] + 
        ijk[2]*this->Divisions[0]*this->Divisions[1];

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

    for (lvtk=0; lvtk <= InsertionLevel; lvtk++)
      {
      this->GetBucketNeighbors (ijk, this->Divisions, lvtk);

      for ( i=0; i < Buckets->GetNumberOfNeighbors(); i++ ) 
        {
        nei = Buckets->GetPoint(i);
        cno = nei[0] + nei[1]*this->Divisions[0] + 
              nei[2]*this->Divisions[0]*this->Divisions[1];

        if ( (ptIds = this->HashTable[cno]) != NULL )
          {
          for (j=0; j < ptIds->GetNumberOfIds(); j++) 
            {
            ptId = ptIds->GetId(j);
            pt = this->Points->GetPoint(ptId);

            if ( vtkMath::Distance2BetweenPoints(x,pt) <= this->InsertionTol2 )
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

// Description:
// Given a position x, return the id of the point closest to it. This method
// is used when performing incremental point insertion.
int vtkPointLocator::FindClosestInsertedPoint(float x[3])
{
  int i, j;
  float minDist2, dist2;
  float *pt;
  int closest, level;
  int ptId, cno;
  vtkIdList *ptIds;
  int ijk[3], *nei;
  int MULTIPLES;
  float diff;
//
//  Make sure candidate point is in bounds.  If not, it is outside.
//
  for (i=0; i<3; i++)
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      return -1;
//
//  Find bucket point is in.  
//
  for (j=0; j<3; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j])*0.999 / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);
    }
//
//  Need to search this bucket for closest point.  If there are no
//  points in this bucket, search 1st level neighbors, and so on,
//  until closest point found.
//
  for (closest=0,minDist2=VTK_LARGE_FLOAT,level=0; (closest == 0) && 
  (level < this->Divisions[0] || level < this->Divisions[1] || 
  level < this->Divisions[2]); level++) 
    {
    this->GetBucketNeighbors (ijk, this->Divisions, level);

    for (i=0; i<Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = Buckets->GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] + 
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->Points->GetPoint(ptId);
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
  this->GetBucketNeighbors (ijk, this->Divisions, level);
//
//  Don't want to search all the neighbors, only those that could
//  possibly have points closer than the current closest.
//
  for (i=0; i<Buckets->GetNumberOfNeighbors(); i++) 
    {
    nei = Buckets->GetPoint(i);

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
            pt = this->Points->GetPoint(ptId);
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

// Build polygonal representation of locator. Create faces that separate
// inside/outside buckets, or separate inside/boundary of locator.
void vtkPointLocator::GenerateRepresentation(int vtkNotUsed(level), vtkPolyData *pd)
{
  vtkFloatPoints *pts;
  vtkCellArray *polys;
  int ii, i, j, k, idx, offset[3], minusOffset[3], inside, sliceSize;

  if ( this->HashTable == NULL ) 
    {
    vtkErrorMacro(<<"Can't build representation...no data!");
    return;
    }

  pts = new vtkFloatPoints(5000);
  polys = new vtkCellArray(10000);

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
        if ( this->HashTable[idx] == NULL ) inside = 0;
        else inside = 1;

        //check "negative" neighbors
        for (ii=0; ii < 3; ii++)
          {
          if ( minusOffset[ii] < 0 )
            {
            if ( inside ) this->GenerateFace(ii,i,j,k,pts,polys);
            }
          else
            {
            if ( ii == 0 ) idx = minusOffset[0] + offset[1] + offset[2];
            else if ( ii == 1 ) idx = offset[0] + minusOffset[1] + offset[2];
            else idx = offset[0] + offset[1] + minusOffset[2];

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
                                   vtkFloatPoints *pts, vtkCellArray *polys)
{
  int ids[4];
  float origin[3], x[3];

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


void vtkPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLocator::PrintSelf(os,indent);

  os << indent << "Number of Points Per Bucket: " << this->NumberOfPointsPerBucket << "\n";
  os << indent << "Divisions: (" << this->Divisions[0] << ", " 
     << this->Divisions[1] << ", " << this->Divisions[2] << ")\n";

}

