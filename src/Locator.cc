/*=========================================================================

  Program:   Visualization Library
  Module:    Locator.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "Locator.hh"
#include "vlMath.hh"

#include "IntArray.hh"

class vlNeighborPoints
{
public:
  vlNeighborPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  int GetNumberOfNeighbors() {return (P.GetMaxId()+1)/3;};
  void Reset() {this->P.Reset();};

  int *GetPoint(int i) {return this->P.GetPtr(3*i);};
  int InsertNextPoint(int *x);

protected:
  vlIntArray P;
};

inline int vlNeighborPoints::InsertNextPoint(int *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]);
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

static vlNeighborPoints Cells(26,50);

// Description:
// Construct with automatic computation of divisions, averaging
// 40 points per cell.
vlLocator::vlLocator()
{
  this->Points = NULL;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 10;
  this->Automatic = 1;
  this->NumberOfPointsInCell = 40;
  this->Tolerance = 0.01;
  this->HashTable = NULL;
  this->NumberOfCells = 0;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
}

vlLocator::~vlLocator()
{
  this->Initialize();
}

void vlLocator::Initialize()
{
  if (this->Points) this->Points->UnRegister(this);
  this->Points = NULL;

  // free up hash table
  this->FreeHashTable();
}

void vlLocator::FreeHashTable()
{
  vlIdList *ptIds;
  int i;

  if ( this->HashTable )
    {
    for (i=0; i<this->NumberOfCells; i++)
      {
      if ( (ptIds = this->HashTable[i]) ) delete ptIds;
      }
    this->HashTable = NULL;
    }
}

// Description:
// Given a position x, return the id of the point closest to it.
int vlLocator::FindClosestPoint(float x[3])
{
  int i, j;
  float minDist2, dist2;
  float *pt;
  int closest, level;
  int ptId, cno;
  vlIdList *ptIds;
  int ijk[3], *nei;
  int MULTIPLES;
  float diff;
  vlMath math;

  this->SubDivide(); // will subdivide if modified; otherwise returns
//
//  Make sure candidate point is in bounds.  If not, it is outside.
//
  for (i=0; i<3; i++)
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      return -1;
//
//  Find cell point is in.  
//
  for (j=0; j<3; j++) 
    {
    ijk[j] = (int)(((x[j] - this->Bounds[2*j])*0.999 / 
        (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);
    }
//
//  Need to search this cell for closest point.  If there are no
//  points in this cell, search 1st level neighbors, and so on,
//  until closest point found.
//
  for (closest=0,minDist2=LARGE_FLOAT,level=0; (closest == 0) && 
  (level < this->Divisions[0] || level < this->Divisions[1] || 
  level < this->Divisions[2]); level++) 
    {
    this->GetCellNeighbors (ijk, this->Divisions, level);

    for (i=0; i<Cells.GetNumberOfNeighbors(); i++) 
      {
      nei = Cells.GetPoint(i);
      cno = nei[0] + nei[1]*this->Divisions[0] + 
            nei[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j<=ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->Points->GetPoint(ptId);
          if ( (dist2 = math.Distance2BetweenPoints(x,pt)) < minDist2 ) 
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
//  may not be the closest point.  Have to search those cell
//  neighbors (one level further out) that might also contain point.
//
  this->GetCellNeighbors (ijk, this->Divisions, level);
//
//  Don't want to search all the neighbors, only those that could
//  possibly have points closer than the current closest.
//
  for (i=0; i<Cells.GetNumberOfNeighbors(); i++) 
    {
    nei = Cells.GetPoint(i);

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
          for (j=0; j<=ptIds->GetNumberOfIds(); j++) 
            {
            ptId = ptIds->GetId(j);
            pt = this->Points->GetPoint(ptId);
            if ( (dist2 = math.Distance2BetweenPoints(x,pt)) < minDist2 ) 
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
int *vlLocator::MergePoints()
{
  float *bounds, tol2;
  int ptId, i, j, k;
  int numPts;
  int *index;
  int newPtId;
  int maxDivs;
  float hmin, *pt, *p;
  int ijk[3], *nei;
  int level, lvl, cno;
  vlIdList *ptIds;
  vlMath math;

  vlDebugMacro(<<"Merging points");

  if ( this->Points == NULL || 
  (numPts=this->Points->GetNumberOfPoints()) < 1 ) return NULL;

  this->SubDivide(); // subdivides if necessary

  bounds = this->Points->GetBounds();

  index = new int[numPts];
  for (i=0; i < numPts; i++) index[i] = -1;

  tol2 = this->Tolerance * this->Tolerance;
  newPtId = 0; // renumbering points

  for (maxDivs=0, hmin=LARGE_FLOAT, i=0; i<3; i++) 
    {
    hmin = (this->H[i] < hmin ? this->H[i] : hmin);
    maxDivs = (maxDivs > this->Divisions[i] ? maxDivs : this->Divisions[i]);
    }
  level = ceil ((double) this->Tolerance / hmin);
  level = (level > maxDivs ? maxDivs : level);
//
//  Traverse each point, find cell that point is in, check the list of
//  points in that cell for merging.  Also need to search all
//  neighboring cells within the tolerance.  The number and level of
//  neighbors to search depends upon the tolerance and the cell width.
//
  for ( i=0; i < numPts; i++ ) //loop over all points
    {
    // Only try to merge the point if it hasn't yet been merged.

    if ( index[i] == -1 ) 
      {
      p = this->Points->GetPoint(i);
      index[i] = newPtId;

      for (j=0; j<3; j++) 
        ijk[j] = (int) ((float)((p[j] - bounds[2*j])*0.999 / 
                       (bounds[2*j+1] - bounds[2*j])) * this->Divisions[j]);

      for (lvl=0; lvl <= level; lvl++) 
        {
        this->GetCellNeighbors (ijk, this->Divisions, lvl);

        for ( k=0; k < Cells.GetNumberOfNeighbors(); k++ ) 
          {
          nei = Cells.GetPoint(k);
          cno = nei[0] + nei[1]*this->Divisions[0] + 
                nei[2]*this->Divisions[0]*this->Divisions[1];

           if ( (ptIds = this->HashTable[cno]) != NULL )
            {
            for (j=0; j < ptIds->GetNumberOfIds(); j++) 
              {
              ptId = ptIds->GetId(j);
              pt = this->Points->GetPoint(ptId);

              if ( index[ptId] == -1 && math.Distance2BetweenPoints(p,pt) <= tol2 )
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
//  subject to the constraints of levels and NumberOfPointsInCell.
//  The result is directly addressable and of uniform subdivision.
//
void vlLocator::SubDivide()
{
  float *bounds;
  int numCells;
  float level;
  int ndivs[3];
  int i, j, ijk[3];
  int idx;
  vlIdList *cell;
  int numPts;
  int numPtsInCell = this->NumberOfPointsInCell;
  float *x;
  typedef vlIdList *vlIdListPtr;

  if ( this->HashTable != NULL && this->SubDivideTime > this->MTime ) return;

  vlDebugMacro( << "Hashing points..." );

  if ( !this->Points || (numPts = this->Points->GetNumberOfPoints()) < 1 )
    {
    vlErrorMacro( << "No points to subdivide");
    return;
    }
//
//  Make sure the appropriate data is available
//
  if ( this->HashTable ) this->FreeHashTable();
//
//  Size the root cell.  Initialize cell data structure, compute 
//  level and divisions.
//
  bounds = this->Points->GetBounds();
  for (i=0; i<6; i++) this->Bounds[i] = bounds[i];

  if ( this->Automatic ) 
    {
    level = (float) numPts / numPtsInCell;
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

  this->NumberOfCells = numCells = ndivs[0]*ndivs[1]*ndivs[2];
  this->HashTable = new vlIdListPtr[numCells];
//
//  Compute width of cell in three directions
//
  for (i=0; i<3; i++) this->H[i] = (bounds[2*i+1] - bounds[2*i]) / ndivs[i] ;
//
//  Insert each point into the appropriate cell.  Make sure point
//  falls within cell.
//
  for (i=0; i<numPts; i++) 
    {
    x = this->Points->GetPoint(i);
    for (j=0; j<3; j++) 
      {
      ijk[j] = (int) ((float) ((x[j] - bounds[2*j])*0.999 / 
                         (bounds[2*j+1] - bounds[2*j])) * ndivs[j]);
      }
    idx = ijk[0] + ijk[1]*ndivs[0] + ijk[2]*ndivs[0]*ndivs[1];
    cell = this->HashTable[idx];
    if ( ! cell )
      {
      cell = new vlIdList(numPtsInCell/2);
      this->HashTable[idx] = cell;
      }
    cell->InsertNextId(i);
    }

  this->SubDivideTime.Modified();
}


//
//  Internal function to get cell neighbors at specified level
//
void vlLocator::GetCellNeighbors(int ijk[3], int ndivs[3], int level)
{
    int i, j, k, min, max, minLevel[3], maxLevel[3];
    int nei[3];
//
//  Initialize
//
    Cells.Reset();
//
//  If at this cell, just place into list
//
  if ( level == 0 ) 
    {
    Cells.InsertNextPoint(ijk);
    return;
    }
//
//  Create permutations of the ijk indices that are at the level
//  required. If these are legal cells, add to list for searching.
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
          Cells.InsertNextPoint(nei);
          }
        }
      }
    }

  return;
}
