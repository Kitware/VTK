/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocator.cxx
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
#include <stdlib.h>
#include <math.h>
#include "vtkCellLocator.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCellLocator* vtkCellLocator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellLocator");
  if(ret)
    {
    return (vtkCellLocator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellLocator;
}

#define VTK_CELL_OUTSIDE 0
#define VTK_CELL_INSIDE 1

typedef vtkIdList *vtkIdListPtr;

class vtkNeighborCells
{
  public:
    vtkNeighborCells(const int sz, const int ext=1000)
      {this->P = vtkIntArray::New(); this->P->Allocate(3*sz,3*ext);};
    ~vtkNeighborCells(){this->P->Delete();}; 
    int GetNumberOfNeighbors() {return (this->P->GetMaxId()+1)/3;};
    void Reset() {this->P->Reset();};
    
    int *GetPoint(int i) {return this->P->GetPointer(3*i);};
    int InsertNextPoint(int *x);
    
  protected:
    vtkIntArray *P;
};

inline int vtkNeighborCells::InsertNextPoint(int *x) 
{
  int id = this->P->GetMaxId() + 3;
  this->P->InsertValue(id,x[2]);
  this->P->SetValue(id-2, x[0]);
  this->P->SetValue(id-1, x[1]);
  return id/3;
}

// Construct with automatic computation of divisions, averaging
// 25 cells per bucket.
vtkCellLocator::vtkCellLocator()
{
  this->NumberOfCellsPerBucket = 25;
  this->Tree = NULL;
  this->CellHasBeenVisited = NULL;
  this->QueryNumber = 0;
  this->H[0] = this->H[1] = this->H[2] = 1.0;
  this->NumberOfDivisions = 1;

  this->Buckets = new vtkNeighborCells(10, 10);
  this->CacheCellBounds = 0;
  this->CellBounds = NULL;
}

vtkCellLocator::~vtkCellLocator()
{
  if (this->Buckets)
    {
    delete this->Buckets;
    this->Buckets = NULL;
    }
  
  this->FreeSearchStructure();
 
  if (this->CellHasBeenVisited)
    {
    delete [] this->CellHasBeenVisited;
    this->CellHasBeenVisited = NULL;
    }

  if (this->CellBounds)
    {
    delete [] this->CellBounds;
    this->CellBounds = NULL;
    }
}

void vtkCellLocator::FreeSearchStructure()
{
  vtkIdList *cellIds;
  int i;
  
  if ( this->Tree )
    {
    for (i=0; i<this->NumberOfOctants; i++)
      {
      cellIds = this->Tree[i];
      if (cellIds == (void *)VTK_CELL_INSIDE)
        {
        cellIds = 0;
        }
      if (cellIds)
        {
        cellIds->Delete();
        }
      }
    delete [] this->Tree;
    this->Tree = NULL;
    }
}

// Given an offset into the structure, the number of divisions in the octree,
// an i,j,k location in the octree; return the index (idx) into the structure.
// Method returns 1 is the specified i,j,k location is "outside" of the octree.
int vtkCellLocator::GenerateIndex(int offset, int numDivs, int i, int j,
                                  int k, int &idx)
{
  if ( i < 0 || i >= numDivs || 
       j < 0 || j >= numDivs || k < 0 || k >= numDivs )
    {
    return 1;
    }
  
  idx = offset + i + j*numDivs + k*numDivs*numDivs;
  
  return 0;
}


// Return intersection point (if any) of finite line with cells contained
// in cell locator.
int vtkCellLocator::IntersectWithLine(float a0[3], float a1[3], float tol,
                                      float& t, float x[3], float pcoords[3],
                                      int &subId)
{
  int cellId = -1;
  
  return this->IntersectWithLine( a0, a1, tol, t, x, pcoords,
                                  subId, cellId);
}

// Return intersection point (if any) AND the cell which was intersected by
// finite line
int vtkCellLocator::IntersectWithLine(float a0[3], float a1[3], float tol,
                                      float& t, float x[3], float pcoords[3],
                                      int &subId, int &cellId)
{
  vtkGenericCell *cell=vtkGenericCell::New();
  int returnVal;

  returnVal = this->IntersectWithLine( a0, a1, tol, t, x, pcoords, subId,
                                       cellId, cell);

  cell->Delete();
  return returnVal;
}
  
    
// Return intersection point (if any) AND the cell which was intersected by
// finite line
int vtkCellLocator::IntersectWithLine(float a0[3], float a1[3], float tol,
                                      float& t, float x[3], float pcoords[3],
                                      int &subId, int &cellId,
                                      vtkGenericCell *cell)
{
  float origin[3];
  float direction1[3];
  float direction2[3];
  float direction3[3];
  float hitPosition[3];
  float hitCellBoundsPosition[3], cellBounds[6];
  int hitCellBounds;
  float result;
  float *bounds;
  float bounds2[6];
  int i, leafStart, prod, loop;
  int bestCellId = -1;
  int idx, cId;
  float tMax, dist[3];
  int npos[3];
  int pos[3];
  int bestDir;
  float stopDist, currDist;
  
  // get the bounds
  bounds = this->DataSet->GetBounds();
  
  // convert the line into i,j,k coordinates
  tMax = 0.0;
  for (i=0; i < 3; i++) 
    {
    direction1[i] = a1[i] - a0[i];
    if ( (bounds[2*i+1] - bounds[2*i]) != 0.0)
      {
      origin[i] = (a0[i] - bounds[2*i])/(bounds[2*i+1] - bounds[2*i]);
      direction2[i] = (a1[i] - a0[i])/(bounds[2*i+1] - bounds[2*i]);
      }
    else 
      {
      origin[i] = 0.0;
      direction2[i] = 0.0;
      }
    bounds2[2*i]   = 0.0;
    bounds2[2*i+1] = 1.0;
    tMax += direction2[i]*direction2[i];
    }
  
  tMax = sqrt(tMax);
  stopDist = tMax*this->NumberOfDivisions;
  for (i = 0; i < 3; i++) 
    {
    direction3[i] = direction2[i]/tMax;
    }
  
  if (vtkCell::HitBBox(bounds2, origin, direction2, hitPosition, result))
    {
    // start walking through the octants
    prod = this->NumberOfDivisions*this->NumberOfDivisions;
    leafStart = this->NumberOfOctants - this->NumberOfDivisions*prod;
    bestCellId = -1;
    
    // Clear the array that indicates whether we have visited this cell.
    // The array is only cleared when the query number rolls over.  This
    // saves a number of calls to memset.
    this->QueryNumber++;
    if (this->QueryNumber == 0)
      {
      this->ClearCellHasBeenVisited();
      this->QueryNumber++;    // can't use 0 as a marker
      }
    
    // set up curr and stop dist
    currDist = 0;
    for (i = 0; i < 3; i++)
      {
      currDist += (hitPosition[i] - origin[i])*(hitPosition[i] - origin[i]);
      }
    currDist = sqrt(currDist)*this->NumberOfDivisions;
    
    // add one offset due to the problems around zero
    for (loop = 0; loop <3; loop++)
      {
      hitPosition[loop] = hitPosition[loop]*this->NumberOfDivisions + 1.0;
      pos[loop] = (int)hitPosition[loop];
      // Adjust right boundary condition: if we intersect from the top, right,
      // or back; then pos must be adjusted to a valid octant index 
      if (pos[loop] > this->NumberOfDivisions)
        {
        pos[loop] = this->NumberOfDivisions;
        }
      }
    
    idx = leafStart + pos[0] - 1 + (pos[1] - 1)*this->NumberOfDivisions 
      + (pos[2] - 1)*prod;
    
    while ((bestCellId < 0) && (pos[0] > 0) && (pos[1] > 0) && (pos[2] > 0) &&
      (pos[0] <= this->NumberOfDivisions) &&
      (pos[1] <= this->NumberOfDivisions) &&
      (pos[2] <= this->NumberOfDivisions) &&
      (currDist < stopDist))
      {
      if (this->Tree[idx])
        {
        for (tMax = VTK_LARGE_FLOAT, cellId=0; 
        cellId < this->Tree[idx]->GetNumberOfIds(); cellId++) 
          {
          cId = this->Tree[idx]->GetId(cellId);
          if (this->CellHasBeenVisited[cId] != this->QueryNumber)
            {
            this->CellHasBeenVisited[cId] = this->QueryNumber;
            hitCellBounds = 0;
            
            // check whether we intersect the cell bounds
            if (this->CacheCellBounds)
              {
              hitCellBounds = vtkCell::HitBBox(this->CellBounds[cId],
                                               a0, direction1,
                                               hitCellBoundsPosition, result);
              }
            else 
              {
              this->DataSet->GetCellBounds(cId, cellBounds);
              hitCellBounds = vtkCell::HitBBox(cellBounds,
                                               a0, direction1,
                                               hitCellBoundsPosition, result);
              }

            if (hitCellBounds)
              {
              // now, do the expensive GetCell call and the expensive
              // intersect with line call
              this->DataSet->GetCell(cId, cell);
            
              if (cell->IntersectWithLine(a0, a1, tol, t, x, pcoords, subId))
                {
                if (t < tMax)
                  {
                  tMax = t;
                  bestCellId = cId;
                  }
                }
              } // if (hitCellBounds)
            } // if (!this->CellHasBeenVisited[cId])
          }
        }
      
      // move to the next octant
      tMax = 10;
      bestDir = 0;
      for (loop = 0; loop < 3; loop++)
        {
        if (direction3[loop] > 0)
          {
          npos[loop] = pos[loop] + 1;
          dist[loop] = (1.0 - hitPosition[loop] + pos[loop])/direction3[loop];
          if (dist[loop] == 0)
            {
            dist[loop] = 1.0/direction3[loop];
            }
          if (dist[loop] < 0)
            {
            dist[loop] = 0;
            }
          if (dist[loop] < tMax)
            {
            bestDir = loop;
            tMax = dist[loop];
            }
          }
        if (direction3[loop] < 0)
          {
          npos[loop] = pos[loop] - 1;
          dist[loop] = (pos[loop] - hitPosition[loop])/direction3[loop];
          if (dist[loop] == 0)
            {
            dist[loop] = -0.01/direction3[loop];
            }
          if (dist[loop] < 0)
            {
            dist[loop] = 0;
            }
          if (dist[loop] < tMax)
            {
            bestDir = loop;
            tMax = dist[loop];
            }
          }
        }
      // update our position
      for (loop = 0; loop < 3; loop++)
        {
        hitPosition[loop] += dist[bestDir]*direction3[loop];
        }
      currDist += dist[bestDir];
      // now make the move, find the smallest distance
      // only cross one boundry at a time
      pos[bestDir] = npos[bestDir];
      
      idx = leafStart + pos[0] - 1 + (pos[1]-1)*this->NumberOfDivisions + 
        (pos[2]-1)*prod;
      }
    }
    
    if (bestCellId >= 0)
      {
      this->DataSet->GetCell(bestCellId, cell);
      cell->IntersectWithLine(a0, a1, tol, t, x, pcoords, subId);
      
      // store the best cell id in the return "parameter"
      cellId = bestCellId;
      return 1;
      }
    
    return 0;
}

// Return closest point (if any) AND the cell on which this closest point lies
void vtkCellLocator::FindClosestPoint(float x[3], float closestPoint[3], 
                                      vtkGenericCell *cell, int &cellId,
                                      int &subId, float& dist2)
{
  int i, j;
  int *nei;
  int closestCell = -1;
  int closestSubCell = -1;
  int leafStart;
  int level;
  int ijk[3];
  float minDist2, refinedRadius2, distance2ToBucket;
  float distance2ToCellBounds, cellBounds[6];
  float pcoords[3], point[3], cachedPoint[3], weightsArray[6];
  float *weights = weightsArray;
  int nWeights = 6, nPoints;
  vtkIdList *cellIds;
  
  leafStart = this->NumberOfOctants
    - this->NumberOfDivisions*this->NumberOfDivisions*this->NumberOfDivisions;
  
  // Clear the array that indicates whether we have visited this cell.
  // The array is only cleared when the query number rolls over.  This
  // saves a number of calls to memset.
  this->QueryNumber++;
  if (this->QueryNumber == 0)
    {
    this->ClearCellHasBeenVisited();
    this->QueryNumber++;    // can't use 0 as a marker
    }
  
  // init
  dist2 = -1.0;
  refinedRadius2 = VTK_LARGE_FLOAT;
  
  //
  //  Find bucket point is in.  
  //
  for (j=0; j<3; j++) 
    {
    ijk[j] = (int)((x[j] - this->Bounds[2*j]) / this->H[j]);
    
    if (ijk[j] < 0)
      {
      ijk[j] = 0;
      }
    else if (ijk[j] >= this->NumberOfDivisions)
      {
      ijk[j] = this->NumberOfDivisions-1;
      }
    }
  //
  //  Need to search this bucket for closest point.  If there are no
  //  cells in this bucket, search 1st level neighbors, and so on,
  //  until closest point found.
  //
  for (closestCell=(-1),minDist2=VTK_LARGE_FLOAT,level=0;
  (closestCell == -1) && (level < this->NumberOfDivisions); level++)
    {
    this->GetBucketNeighbors(ijk, this->NumberOfDivisions, level);
    
    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      
      // if a neighboring bucket has cells, 
      if ( (cellIds = 
        this->Tree[leafStart + nei[0] + nei[1]*this->NumberOfDivisions + 
          nei[2]*this->NumberOfDivisions*this->NumberOfDivisions]) != NULL )
        {
        // do we still need to test this bucket?
        distance2ToBucket = this->Distance2ToBucket(x, nei);
        
        if (distance2ToBucket < refinedRadius2)
          {
          // still a viable bucket
          for (j=0; j < cellIds->GetNumberOfIds(); j++) 
            {
            // get the cell
            cellId = cellIds->GetId(j);
            if (this->CellHasBeenVisited[cellId] != this->QueryNumber)
              {
              this->CellHasBeenVisited[cellId] = this->QueryNumber;
              
              // check whether we could be close enough to the cell by
              // testing the cell bounds
              if (this->CacheCellBounds)
                {
                distance2ToCellBounds = 
                  this->Distance2ToBounds(x, this->CellBounds[cellId]);
                }
              else
                {
                this->DataSet->GetCellBounds(cellId, cellBounds);
                distance2ToCellBounds = this->Distance2ToBounds(x, cellBounds);
                }
              
              if (distance2ToCellBounds < refinedRadius2)
                {
                this->DataSet->GetCell(cellId, cell);
                
                // make sure we have enough storage space for the weights
                nPoints = cell->GetPointIds()->GetNumberOfIds();
                if (nPoints > nWeights)
                  {
                  if (nWeights > 6)
                    {
                    delete [] weights;
                    }
                  weights = new float[2*nPoints];  // allocate some extra room
                  nWeights = 2*nPoints;
                  }
                
                // evaluate the position to find the closest point
                int stat=cell->EvaluatePosition(x, point, subId, pcoords,
                  dist2, weights);
                
                if ( stat != -1 && dist2 < minDist2 ) 
                  {
                  closestCell = cellId;
                  closestSubCell = subId;
                  minDist2 = dist2;
                  cachedPoint[0] = point[0];
                  cachedPoint[1] = point[1];
                  cachedPoint[2] = point[2];
                  refinedRadius2 = dist2;
                  }
                }
              } // if (!this->CellHasBeenVisited[cellId])
            }
          }
        }
      }
    }
  
  // Because of the relative location of the points in the buckets, the
  // cell found previously may not be the closest cell.  Have to
  // search those bucket neighbors that might also contain nearby cells.
  //
  if ( (minDist2 > 0.0) && (level < this->NumberOfDivisions))
    {
    int prevMinLevel[3], prevMaxLevel[3];
    // setup prevMinLevel and prevMaxLevel to indicate previously visited
    // buckets
    if (--level < 0)
      {
      level = 0;
      }
    for (i = 0; i < 3; i++)
      {
      prevMinLevel[i] = ijk[i] - level;
      if (prevMinLevel[i] < 0)
        {
        prevMinLevel[i] = 0;
        }
      prevMaxLevel[i] = ijk[i] + level;
      if (prevMaxLevel[i] >= this->NumberOfDivisions)
        {
        prevMaxLevel[i] = this->NumberOfDivisions - 1;
        }
      }
    this->GetOverlappingBuckets(x, ijk, sqrt(minDist2), prevMinLevel,
                                prevMaxLevel);
    
    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      
      if ( (cellIds = 
          this->Tree[leafStart + nei[0] + nei[1]*this->NumberOfDivisions + 
            nei[2]*this->NumberOfDivisions*this->NumberOfDivisions]) != NULL )
        {
        // do we still need to test this bucket?
        distance2ToBucket = this->Distance2ToBucket(x, nei);
        
        if (distance2ToBucket < refinedRadius2)
          {
          // still a viable bucket
          for (j=0; j < cellIds->GetNumberOfIds(); j++) 
            {
            // get the cell
            cellId = cellIds->GetId(j);
            if (this->CellHasBeenVisited[cellId] != this->QueryNumber)
              {
              this->CellHasBeenVisited[cellId] = this->QueryNumber;
              
              // check whether we could be close enough to the cell by
              // testing the cell bounds
              if (this->CacheCellBounds)
                {
                distance2ToCellBounds = 
                  this->Distance2ToBounds(x, this->CellBounds[cellId]);
                }
              else
                {
                this->DataSet->GetCellBounds(cellId, cellBounds);
                distance2ToCellBounds = this->Distance2ToBounds(x, cellBounds);
                }
              
              if (distance2ToCellBounds < refinedRadius2)
                {
                this->DataSet->GetCell(cellId, cell);
                
                // make sure we have enough storage space for the weights
                nPoints = cell->GetPointIds()->GetNumberOfIds();
                if (nPoints > nWeights)
                  {
                  if (nWeights > 6)
                    {
                    delete [] weights;
                    }
                  weights = new float[2*nPoints];  // allocate some extra room
                  nWeights = 2*nPoints;
                  }
                
                // evaluate the position to find the closest point
                cell->EvaluatePosition(x, point, subId, pcoords,
                  dist2, weights);
                
                if ( dist2 < minDist2 ) 
                  {
                  closestCell = cellId;
                  closestSubCell = subId;
                  minDist2 = dist2;
                  cachedPoint[0] = point[0];
                  cachedPoint[1] = point[1];
                  cachedPoint[2] = point[2];
                  refinedRadius2 = dist2;
                  }
                }//if point close enough to cell bounds
              }//if cell has not been visited
            }//for each cell
          }//if bucket is still viable
        }//if cells in bucket
      }//for each overlapping bucket
    }//if not identical point
  
  if (closestCell != -1)
    {
    dist2 = minDist2;
    cellId = closestCell;
    subId = closestSubCell;
    closestPoint[0] = cachedPoint[0];
    closestPoint[1] = cachedPoint[1];
    closestPoint[2] = cachedPoint[2];
    this->DataSet->GetCell(cellId, cell);
    }
  
  if (nWeights > 6)
    {
    delete [] weights;
    }
}


// Return closest point (if any) AND the cell on which this closest point lies
void vtkCellLocator::FindClosestPoint(float x[3], float closestPoint[3],
                                      int &cellId, int &subId, float& dist2)
{
  vtkGenericCell *cell = vtkGenericCell::New();
  
  this->FindClosestPoint(x, closestPoint, cell, cellId, subId, dist2);
  
  cell->Delete();
}

int vtkCellLocator::FindClosestPointWithinRadius(float x[3], float radius,
                      float closestPoint[3], vtkGenericCell *cell, int &cellId,
                      int &subId, float& dist2, int &inside)
{
  int i, j;
  int tmpInside;
  int *nei;
  int closestCell = -1;
  int closestSubCell = -1;
  int leafStart;
  int ijk[3];
  float minDist2;
  float pcoords[3], point[3], cachedPoint[3], weightsArray[6];
  float *weights = weightsArray;
  int nWeights = 6, nPoints;
  int returnVal = 0;
  vtkIdList *cellIds;
  
  float refinedRadius, radius2, refinedRadius2, distance2ToBucket;
  float distance2ToCellBounds, cellBounds[6], currentRadius;
  float distance2ToDataBounds, maxDistance;
  int ii, radiusLevels[3], radiusLevel, prevMinLevel[3], prevMaxLevel[3];
  
  leafStart = this->NumberOfOctants
    - this->NumberOfDivisions*this->NumberOfDivisions*this->NumberOfDivisions;
  
  // Clear the array that indicates whether we have visited this cell.
  // The array is only cleared when the query number rolls over.  This
  // saves a number of calls to memset.
  this->QueryNumber++;
  if (this->QueryNumber == 0)
    {
    this->ClearCellHasBeenVisited();
    this->QueryNumber++;    // can't use 0 as a marker
    }
  
  // init
  dist2 = -1.0;
  closestCell = -1;
  radius2 = radius*radius;
  minDist2 = 1.1*radius2;   // something slightly bigger....
  refinedRadius = radius;
  refinedRadius2 = radius2;
  
  // Find bucket point is in.  
  //
  for (j=0; j<3; j++) 
    {
    ijk[j] = (int)((x[j] - this->Bounds[2*j]) / this->H[j]);
    
    if (ijk[j] < 0)
      {
      ijk[j] = 0;
      }
    else if (ijk[j] >= this->NumberOfDivisions)
      {
      ijk[j] = this->NumberOfDivisions-1;
      }
    }
  
  // Start by searching the bucket that the point is in.
  //
  if ((cellIds = 
      this->Tree[leafStart + ijk[0] + ijk[1]*this->NumberOfDivisions + 
      ijk[2]*this->NumberOfDivisions*this->NumberOfDivisions]) != NULL )
    {
    // query each cell
    for (j=0; j < cellIds->GetNumberOfIds(); j++) 
      {
      // get the cell
      cellId = cellIds->GetId(j);
      if (this->CellHasBeenVisited[cellId] != this->QueryNumber)
        {
        this->CellHasBeenVisited[cellId] = this->QueryNumber;
        
        // check whether we could be close enough to the cell by
        // testing the cell bounds
        if (this->CacheCellBounds)
          {
          distance2ToCellBounds = 
            this->Distance2ToBounds(x, this->CellBounds[cellId]);
          }
        else
          {
          this->DataSet->GetCellBounds(cellId, cellBounds);
          distance2ToCellBounds = this->Distance2ToBounds(x, cellBounds);
          }
        
        if (distance2ToCellBounds < refinedRadius2)
          {
          this->DataSet->GetCell(cellId, cell);
          
          // make sure we have enough storage space for the weights
          nPoints = cell->GetPointIds()->GetNumberOfIds();
          if (nPoints > nWeights)
            {
            if (nWeights > 6)
              {
              delete [] weights;
              }
            weights = new float[2*nPoints];  // allocate some extra room
            nWeights = 2*nPoints;
            }
          
          // evaluate the position to find the closest point
          tmpInside = cell->EvaluatePosition(x, point, subId, pcoords,
            dist2, weights);
          if ( dist2 < minDist2 ) 
            {
            inside = tmpInside;
            closestCell = cellId;
            closestSubCell = subId;
            minDist2 = dist2;
            cachedPoint[0] = point[0];
            cachedPoint[1] = point[1];
            cachedPoint[2] = point[2];
            refinedRadius = sqrt(dist2);
            refinedRadius2 = dist2;
            }
          }
        } // if (this->CellHasBeenVisited[cellId])
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

  
  distance2ToDataBounds = this->Distance2ToBounds(x, this->Bounds);
  maxDistance = sqrt(distance2ToDataBounds) + this->DataSet->GetLength();
  if (refinedRadius > maxDistance)
    {
    refinedRadius = maxDistance;
    refinedRadius2 = maxDistance*maxDistance;
    }
  
  radiusLevels[0] = (int)(refinedRadius/this->H[0]);
  radiusLevels[1] = (int)(refinedRadius/this->H[1]);
  radiusLevels[2] = (int)(refinedRadius/this->H[2]);
  
  radiusLevel = radiusLevels[0];
  radiusLevel = radiusLevels[1] > radiusLevel ? radiusLevels[1] : radiusLevel;
  radiusLevel = radiusLevels[2] > radiusLevel ? radiusLevels[2] : radiusLevel;
  
  if (radiusLevel > this->NumberOfDivisions / 2 )
    {
    radiusLevel = this->NumberOfDivisions / 2;
    }
  if (radiusLevel == 0)
    {
    radiusLevel = 1;
    }

  // radius schedule increases the radius each iteration, this is currently
  // implemented by decreasing ii by 1 each iteration.  another alternative
  // is to double the radius each iteration, i.e. ii = ii >> 1
  // In practice, reducing ii by one has been found to be more efficient.
  int numberOfBucketsPerPlane;
  numberOfBucketsPerPlane = this->NumberOfDivisions*this->NumberOfDivisions;
  prevMinLevel[0] = prevMaxLevel[0] = ijk[0];
  prevMinLevel[1] = prevMaxLevel[1] = ijk[1];
  prevMinLevel[2] = prevMaxLevel[2] = ijk[2];
  for (ii=radiusLevel; ii >= 1; ii--)   
    {
    currentRadius = refinedRadius; // used in if at bottom of this for loop
    
    // Build up a list of buckets that are arranged in rings
    this->GetOverlappingBuckets(x, ijk, refinedRadius/ii, prevMinLevel,
                                prevMaxLevel);
    
    for (i=0; i<this->Buckets->GetNumberOfNeighbors(); i++) 
      {
      nei = this->Buckets->GetPoint(i);
      
      if ( (cellIds = 
        this->Tree[leafStart + nei[0] + nei[1]*this->NumberOfDivisions + 
          nei[2]*numberOfBucketsPerPlane]) != NULL )
        {
        // do we still need to test this bucket?
        distance2ToBucket = this->Distance2ToBucket(x, nei);
        
        if (distance2ToBucket < refinedRadius2)
          {
          // still a viable bucket
          for (j=0; j < cellIds->GetNumberOfIds(); j++) 
            {
            // get the cell
            cellId = cellIds->GetId(j);
            if (this->CellHasBeenVisited[cellId] != this->QueryNumber)
              {
              this->CellHasBeenVisited[cellId] = this->QueryNumber;
              
              // check whether we could be close enough to the cell by
              // testing the cell bounds
              if (this->CacheCellBounds)
                {
                distance2ToCellBounds = 
                  this->Distance2ToBounds(x, this->CellBounds[cellId]);
                }
              else
                {
                this->DataSet->GetCellBounds(cellId, cellBounds);
                distance2ToCellBounds = this->Distance2ToBounds(x, cellBounds);
                }
              
              if (distance2ToCellBounds < refinedRadius2)
                {
                this->DataSet->GetCell(cellId, cell);
                
                // make sure we have enough storage space for the weights
                nPoints = cell->GetPointIds()->GetNumberOfIds();
                if (nPoints > nWeights)
                  {
                  if (nWeights > 6)
                    {
                    delete [] weights;
                    }
                  weights = new float[2*nPoints];  // allocate some extra room
                  nWeights = 2*nPoints;
                  }
                
                // evaluate the position to find the closest point
                tmpInside = cell->EvaluatePosition(x, point, subId, pcoords,
                  dist2, weights);
                
                if ( dist2 < minDist2 ) 
                  {
                  inside = tmpInside;
                  closestCell = cellId;
                  closestSubCell = subId;
                  minDist2 = dist2;
                  cachedPoint[0] = point[0];
                  cachedPoint[1] = point[1];
                  cachedPoint[2] = point[2];
                  refinedRadius = sqrt(minDist2);
                  refinedRadius2 = minDist2;
                  }
                }//if point close enough to cell bounds
              }//if cell has not been visited
            }//for each cell in bucket
          }//if bucket is within the current best distance
        }//if cells in bucket
      }//for each overlapping bucket

    // don't want to checker a smaller radius than we just checked so update
    // ii appropriately
    if (refinedRadius < currentRadius && ii > 2) //always check ii==1
      {
      ii = (int)((float)ii * (refinedRadius / currentRadius)) + 1;
      if (ii < 2)
        {
        ii = 2;
        }
      }
    }//for each radius in the radius schedule
  
  if ((closestCell != -1) && (minDist2 <= radius2))
    {
    dist2 = minDist2;
    cellId = closestCell;
    subId = closestSubCell;
    closestPoint[0] = cachedPoint[0];
    closestPoint[1] = cachedPoint[1];
    closestPoint[2] = cachedPoint[2];
    this->DataSet->GetCell(cellId, cell);
    returnVal = 1;
    }
  
  if (nWeights > 6)
    {
    delete [] weights;
    }
  
  return returnVal;
}

int vtkCellLocator::FindClosestPointWithinRadius(float x[3], float radius,
                    float closestPoint[3], vtkGenericCell *cell, int &cellId,
                    int &subId, float& dist2)
{
  int inside;

  return 
    this->FindClosestPointWithinRadius(x, radius, closestPoint,
                                       cell, cellId, subId, dist2, inside);
}

int vtkCellLocator::FindClosestPointWithinRadius(float x[3], float radius,
                                             float closestPoint[3],int &cellId,
                                             int &subId, float& dist2)
{
  vtkGenericCell *cell = vtkGenericCell::New();
  int found, inside;
  
  found = 
    this->FindClosestPointWithinRadius(x, radius, closestPoint,
                                       cell, cellId, subId, dist2, inside);
  cell->Delete();
  return found;
}

//
//  Internal function to get bucket neighbors at specified "level". The
//  bucket neighbors are indices into the "leaf-node" layer of the octree.
//  These indices must be offset by number of octants before the leaf node
//  layer before they can be used. Only those buckets with cells are returned.
//
void vtkCellLocator::GetBucketNeighbors(int ijk[3], int ndivs, int level)
{
  int i, j, k, min, max, minLevel[3], maxLevel[3];
  int nei[3];
  int leafStart;
  int numberOfBucketsPerPlane;
  
  numberOfBucketsPerPlane = this->NumberOfDivisions*this->NumberOfDivisions;
  leafStart = this->NumberOfOctants
    - numberOfBucketsPerPlane*this->NumberOfDivisions;
  
  //  Initialize
  //
  this->Buckets->Reset();

  //  If at this bucket, just place into list
  //
  if ( level == 0 ) 
    {
    if (this->Tree[leafStart + ijk[0] + ijk[1]*this->NumberOfDivisions
      + ijk[2]*numberOfBucketsPerPlane])
      {
      this->Buckets->InsertNextPoint(ijk);
      }
    return;
    }

  //  Create permutations of the ijk indices that are at the level
  //  required. If these are legal buckets, add to list for searching.
  //
  for ( i=0; i<3; i++ ) 
    {
    min = ijk[i] - level;
    max = ijk[i] + level;
    minLevel[i] = ( min > 0 ? min : 0);
    maxLevel[i] = ( max < (ndivs-1) ? max : (ndivs-1));
    }
  
  for ( k= minLevel[2]; k <= maxLevel[2]; k++ ) 
    {
    for ( j= minLevel[1]; j <= maxLevel[1]; j++ ) 
      {
      for ( i= minLevel[0]; i <= maxLevel[0]; i++ ) 
        {
        if (i == (ijk[0] + level) || i == (ijk[0] - level) ||
          j == (ijk[1] + level) || j == (ijk[1] - level) ||
          k == (ijk[2] + level) || k == (ijk[2] - level) ) 
          {
          if (this->Tree[leafStart + i + j*this->NumberOfDivisions
            + k*numberOfBucketsPerPlane])
            {
            nei[0]=i; nei[1]=j; nei[2]=k;
            this->Buckets->InsertNextPoint(nei);
            }
          }
        }
      }
    }
  
  return;
}

// Internal method to find those buckets that are within distance specified.
// Only those buckets outside of level radiuses of ijk are returned. The
// bucket neighbors are indices into the "leaf-node" layer of the octree.
// These indices must be offset by number of octants before the leaf node
// layer before they can be used. Only buckets that have cells are placed
// in the bucket list.
//
void vtkCellLocator::GetOverlappingBuckets(float x[3], int vtkNotUsed(ijk)[3], 
                                           float dist, 
                                           int prevMinLevel[3],
                                           int prevMaxLevel[3])
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];
  int leafStart, kFactor, jFactor;
  int numberOfBucketsPerPlane, jkSkipFlag, kSkipFlag;

  numberOfBucketsPerPlane = this->NumberOfDivisions*this->NumberOfDivisions;
  leafStart = this->NumberOfOctants
    - numberOfBucketsPerPlane*this->NumberOfDivisions;
  
  // Initialize
  this->Buckets->Reset();
  
  // Determine the range of indices in each direction
  for (i=0; i < 3; i++)
    {
    minLevel[i] = (int) ((float) (((x[i]-dist) - this->Bounds[2*i])
                                  / this->H[i]));
    maxLevel[i] = (int) ((float) (((x[i]+dist) - this->Bounds[2*i])
                                  / this->H[i]));
    
    if ( minLevel[i] < 0 )
      {
      minLevel[i] = 0;
      }
    else if (minLevel[i] >= this->NumberOfDivisions )
      {
      minLevel[i] = this->NumberOfDivisions - 1;
      }
    if ( maxLevel[i] >= this->NumberOfDivisions )
      {
      maxLevel[i] = this->NumberOfDivisions - 1;
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
    kFactor = k*numberOfBucketsPerPlane;
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
      jFactor = j*this->NumberOfDivisions;
      for ( i= minLevel[0]; i <= maxLevel[0]; i++ ) 
        {
        if ( jkSkipFlag && i == prevMinLevel[0] )
          {
          i = prevMaxLevel[0];
          continue;
          }
        // if this bucket has any cells, add it to the list
        if (this->Tree[leafStart + i + jFactor + kFactor])
          {
          nei[0]=i; nei[1]=j; nei[2]=k;
          this->Buckets->InsertNextPoint(nei);
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

// number of buckets available
int vtkCellLocator::GetNumberOfBuckets(void) {
    if (this->Tree)
    {
      return this->NumberOfOctants;
    }
    else
    {
      vtkWarningMacro(<<"Attempting to access Tree before Locator has been built");
      return 0;
    }
}

// Get the cells in a bucket.
vtkIdList* vtkCellLocator::GetCells(int octantId)
{
  // handle parents ?             
  return this->Tree[octantId];
}


//  Method to form subdivision of space based on the cells provided and
//  subject to the constraints of levels and NumberOfCellsPerBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vtkCellLocator::BuildLocator()
  {
  float *bounds, length, cellBounds[6], *boundsPtr;
  int numCells;
  int ndivs, product;
  int i, j, k, cellId, ijkMin[3], ijkMax[3];
  int idx, parentOffset;
  vtkIdList *octant;
  int numCellsPerBucket = this->NumberOfCellsPerBucket;
  typedef vtkIdList *vtkIdListPtr;
  int prod, numOctants;
  
  if ( (this->Tree != NULL) && (this->BuildTime > this->MTime)
    && (this->BuildTime > this->DataSet->GetMTime()) )
    {
    return;
    }
  
  vtkDebugMacro( << "Subdividing octree..." );
  
  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro( << "No cells to subdivide");
    return;
    }

  //  Make sure the appropriate data is available
  //
  if ( this->Tree )
    {
    this->FreeSearchStructure();
    }
  if ( this->CellHasBeenVisited )
    {
    delete [] this->CellHasBeenVisited;
    this->CellHasBeenVisited = NULL;
    }
  if (this->CellBounds)
    {
    delete [] this->CellBounds;
    this->CellBounds = NULL;
    }
  
  //  Size the root cell.  Initialize cell data structure, compute
  //  level and divisions.
  //
  bounds = this->DataSet->GetBounds();
  length = this->DataSet->GetLength();
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i] = bounds[2*i];
    this->Bounds[2*i+1] = bounds[2*i+1];
    if (this->Bounds[2*i] == this->Bounds[2*i+1])
      {
      // bump out the bounds a little of if min==max
      this->Bounds[2*i] -= length/100.0;
      this->Bounds[2*i+1] += length/100.0;
      }
    }
  
  if ( this->Automatic ) 
    {
    this->Level = (int) (ceil(log((double)numCells/numCellsPerBucket) / 
      (log((double) 8.0))));
    } 
  this->Level =(this->Level > this->MaxLevel ? this->MaxLevel : this->Level);
  
  // compute number of octants and number of divisions
  for (ndivs=1,prod=1,numOctants=1,i=0; i<this->Level; i++) 
    {
    ndivs *= 2;
    prod *= 8;
    numOctants += prod;
    }
  this->NumberOfDivisions = ndivs;
  this->NumberOfOctants = numOctants;

  this->Tree = new vtkIdListPtr[numOctants];
  memset (this->Tree, 0, numOctants*sizeof(vtkIdListPtr));
  
  this->CellHasBeenVisited = new unsigned char [ numCells ];
  this->ClearCellHasBeenVisited();
  this->QueryNumber = 0;

  if (this->CacheCellBounds)
    {
    this->CellBounds = new float [numCells][6];
    }
  
  //  Compute width of leaf octant in three directions
  //
  for (i=0; i<3; i++)
    {
    this->H[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) / ndivs;
    }

  //  Insert each cell into the appropriate octant.  Make sure cell
  //  falls within octant.
  //
  parentOffset = numOctants - (ndivs * ndivs * ndivs);
  product = ndivs * ndivs;
  boundsPtr = cellBounds;
  for (cellId=0; cellId<numCells; cellId++) 
    {
    if (this->CacheCellBounds)
      {
      boundsPtr = this->CellBounds[cellId];
      this->DataSet->GetCellBounds(cellId, boundsPtr);
      }
    else
      {
      this->DataSet->GetCellBounds(cellId, cellBounds);
      }
    
    // find min/max locations of bounding box
    for (i=0; i<3; i++)
      {
      ijkMin[i] = (int)(((boundsPtr[2*i] - this->Bounds[2*i]) 
        / this->H[i]) * 0.999);
      ijkMax[i] = (int)(((boundsPtr[2*i+1] - this->Bounds[2*i])*1.001
        / this->H[i]));
      
      if (ijkMin[i] < 0)
        {
        ijkMin[i] = 0;
        }
      if (ijkMax[i] >= ndivs)
        {
        ijkMax[i] = ndivs-1;
        }
      }
    
    // each octant inbetween min/max point may have cell in it
    for ( k = ijkMin[2]; k <= ijkMax[2]; k++ )
      {
      for ( j = ijkMin[1]; j <= ijkMax[1]; j++ )
        {
        for ( i = ijkMin[0]; i <= ijkMax[0]; i++ )
          {
          idx = parentOffset + i + j*ndivs + k*product;
          this->MarkParents((void*)VTK_CELL_INSIDE,i,j,k,ndivs,this->Level);
          octant = this->Tree[idx];
          if ( ! octant )
            {
            octant = vtkIdList::New();
            octant->Allocate(numCellsPerBucket,numCellsPerBucket/2);
            this->Tree[idx] = octant;
            }
          octant->InsertNextId(cellId);
          }
        }
      }
    
    } //for all cells
  
  this->BuildTime.Modified();
}

void vtkCellLocator::MarkParents(void* a, int i, int j, int k, 
                                 int ndivs, int level)
{
  int offset, prod, ii, parentIdx;
  
  for (offset=0, prod=1, ii=0; ii<level-1; ii++) 
    {
    offset += prod;
    prod = prod << 3;
    }
  
  while ( level > 0  )
    {
    i = i >> 1;
    j = j >> 1;
    k = k >> 1;
    ndivs = ndivs >> 1;
    level--;
    
    parentIdx = offset + i + j*ndivs + k*ndivs*ndivs;
    
    // if it already matches just return
    if (a == this->Tree[parentIdx])
      {
      return;
      }
    
    this->Tree[parentIdx] = (vtkIdList *)a;
    
    prod = prod >> 3;
    offset -= prod;
    }
}

void vtkCellLocator::GenerateRepresentation(int level, vtkPolyData *pd)
{
  vtkPoints *pts;
  vtkCellArray *polys;
  int l, i, j, k, ii, idx, boundary[3];
  vtkIdList *inside, *Inside[3];
  int numDivs=1;
  
  if ( this->Tree == NULL )
    {
    vtkErrorMacro(<<"No tree to generate representation from");
    return;
    }
  
  pts = vtkPoints::New();
  pts->Allocate(5000);
  polys = vtkCellArray::New();
  polys->Allocate(10000);
  
  // Compute idx into tree at appropriate level; determine if
  // faces of octants are visible.
  //
  int parentIdx = 0;
  int numOctants = 1;
  
  if ( level < 0 )
    {
    level = this->Level;
    }
  for (l=0; l < level; l++)
    {
    numDivs *= 2;
    parentIdx += numOctants;
    numOctants *= 8;
    }
  
  //loop over all octabts generating visible faces
  for ( k=0; k < numDivs; k++)
    {
    for ( j=0; j < numDivs; j++)
      {
      for ( i=0; i < numDivs; i++)
        {
        this->GenerateIndex(parentIdx,numDivs,i,j,k,idx);
        inside = this->Tree[idx];
        
        if ( !(boundary[0]=this->GenerateIndex(parentIdx,numDivs,i-1,j,k,idx)))
          {
          Inside[0] = this->Tree[idx];
          }
        if ( !(boundary[1]=this->GenerateIndex(parentIdx,numDivs,i,j-1,k,idx)))
          {
          Inside[1] = this->Tree[idx];
          }
        if ( !(boundary[2]=this->GenerateIndex(parentIdx,numDivs,i,j,k-1,idx)))
          {
          Inside[2] = this->Tree[idx];
          }
        
        for (ii=0; ii < 3; ii++)
          {
          if ( boundary[ii] )
            {
            if ( inside )
              {
              this->GenerateFace(ii,numDivs,i,j,k,pts,polys);
              }
            }
          else
            {
            if ( (Inside[ii] && !inside) || (!Inside[ii] && inside) )
              {
              this->GenerateFace(ii,numDivs,i,j,k,pts,polys);
              }
            }
          //those buckets on "positive" boundaries can generate faces specially
          if ( (i+1) >= numDivs && inside )
            {
            this->GenerateFace(0,numDivs,i+1,j,k,pts,polys);
            }
          if ( (j+1) >= numDivs && inside )
            {
            this->GenerateFace(1,numDivs,i,j+1,k,pts,polys);
            }
          if ( (k+1) >= numDivs && inside )
            {
            this->GenerateFace(2,numDivs,i,j,k+1,pts,polys);
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
  
void vtkCellLocator::GenerateFace(int face, int numDivs, int i, int j, int k,
                                  vtkPoints *pts, vtkCellArray *polys)
{
  int ii;
  vtkIdType ids[4];
  float origin[3], x[3];
  float h[3];

  // define first corner; use ids[] as temporary array
  ids[0] = i; ids[1] = j; ids[2] = k;
  for (ii=0; ii<3; ii++)
    {
    h[ii] = (this->Bounds[2*ii+1] - this->Bounds[2*ii]) / numDivs;
    origin[ii] = this->Bounds[2*ii] + ids[ii]*h[ii];
    }

  ids[0] = pts->InsertNextPoint(origin);

  if ( face == 0 ) //x face
    {
    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2] + h[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  else if ( face == 1 ) //y face
    {
    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  else //z face
    {
    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + h[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[3] = pts->InsertNextPoint(x);
    }

  polys->InsertNextCell(4,ids);
}
  
void vtkCellLocator::ClearCellHasBeenVisited()
{
  if (this->CellHasBeenVisited && this->DataSet)
    {
    memset(this->CellHasBeenVisited, 0, this->DataSet->GetNumberOfCells());
    }
}
  
void vtkCellLocator::ClearCellHasBeenVisited(int id)
{
  if (this->CellHasBeenVisited
      && this->DataSet && id < this->DataSet->GetNumberOfCells())
    {
    this->CellHasBeenVisited[id] = 0;
    }
}
  
// Calculate the distance between the point x to the bucket "nei".
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
//
float vtkCellLocator::Distance2ToBucket(float x[3], int nei[3])
{
  float bounds[6];

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
float vtkCellLocator::Distance2ToBounds(float x[3], float bounds[6])
{
  float distance;
  float deltas[3];

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


void vtkCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLocator::PrintSelf(os,indent);

  os << indent << "Number of Cells Per Bucket: " 
     << this->NumberOfCellsPerBucket << "\n";
  os << indent << "Cache Cell Bounds: " << this->CacheCellBounds << "\n";  
}
  
