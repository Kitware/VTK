/*=========================================================================

  Program:   Visualization Library
  Module:    CellLoc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "CellLoc.hh"
#include "Locator.hh"
#include "vlMath.hh"
#include "IntArray.hh"

// supporting class
class vlNeighborCells
{
public:
  vlNeighborCells(const int sz, const int ext=1000):P(3*sz,3*ext){};
  int GetNumberOfNeighbors() {return (P.GetMaxId()+1)/3;};
  void Reset() {this->P.Reset();};

  int *GetCell(int i) {return this->P.GetPtr(3*i);};
  int InsertNextCell(int *x);

protected:
  vlIntArray P;
};

inline int vlNeighborCells::InsertNextCell(int *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]);
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

static vlNeighborCells Cells(26,50);

// Description:
// Construct with automatic computation of divisions, averaging
// 25 cells per bucket.
vlCellLocator::vlCellLocator()
{
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->Automatic = 1;
  this->NumberOfCellsInBucket = 25;
  this->Tolerance = 0.01;
  this->HashTable = NULL;
  this->NumberOfCells = 0;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  this->InsertionTol2 = 0.0001;
}

vlCellLocator::~vlCellLocator()
{
  this->Initialize();
}

void vlCellLocator::Initialize()
{
  // free up hash table
  this->FreeSearchStructure();
}

void vlCellLocator::FreeSearchStructure()
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
// Given a position x, return the id of the cell closest to it. Cell may 
// contain the point, or just lie near it. Parametric coordinates, subId,
// and distance squared to cell are returned.
int vlCellLocator::FindClosestCell(float x[3], float dist2,
                                   int& subId, float pcoords[3])
{
  int closest=(-1);

  return closest;
}

// Description:
// Intersect with line returning cells that lie in buckets intersected by line.
int vlCellLocator::IntersectWithLine(float a0[3], float a1[3], vlIdList& cells)
{
  return 0;
}

// Description:
// Intersect against another vlCellLocator returning cells that lie in 
// intersecting buckets.
int vlCellLocator::IntersectWithCellLocator(vlCellLocator& locator, vlIdList cells)
{
  return 0;
}

//
//  Method to form subdivision of space based on the cells provided and
//  subject to the constraints of levels and NumberOfCellsInBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vlCellLocator::SubDivide()
{
  vlCell *cell;
  float *bounds, *cellBounds;
  int numCells;
  float level;
  int ndivs[3], product;
  int i, j, k, cellId, ijkMin[3], ijkMax[3];
  int idx;
  vlIdList *bucket;
  int numCellsInBucket = this->NumberOfCellsInBucket;
  typedef vlIdList *vlIdListPtr;

  if ( this->HashTable != NULL && this->SubDivideTime > this->MTime ) return;

  vlDebugMacro( << "Subdividing cells..." );

  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vlErrorMacro( << "No cells to subdivide");
    return;
    }
//
//  Make sure the appropriate data is available
//
  if ( this->HashTable ) this->FreeSearchStructure();
//
//  Size the root cell.  Initialize cell data structure, compute 
//  level and divisions.
//
  bounds = this->DataSet->GetBounds();
  for (i=0; i<6; i++) this->Bounds[i] = bounds[i];

  if ( this->Automatic ) 
    {
    level = (float) numCells / numCellsInBucket;
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
  memset (this->HashTable, (int)NULL, numCells*sizeof(vlIdListPtr));
//
//  Compute width of bucket in three directions
//
  for (i=0; i<3; i++) this->H[i] = (bounds[2*i+1] - bounds[2*i]) / ndivs[i] ;
//
//  Insert each cell into the appropriate bucket.  Make sure cell
//  falls within bucket.
//
  product = ndivs[0]*ndivs[1];
  for (cellId=0; cellId<numCells; cellId++) 
    {
    cell = this->DataSet->GetCell(cellId);
    cellBounds = cell->GetBounds();

    // find min/max locations of bounding box
    for (i=0; i<3; i++)
      {
      ijkMin[i] = (int) ((float) ((cellBounds[2*i] - bounds[2*i])*0.999 / 
                         (bounds[2*i+1] - bounds[2*i])) * ndivs[i]);
      ijkMax[i] = (int) ((float) ((cellBounds[2*i+1] - bounds[2*i])*0.999 / 
                         (bounds[2*i+1] - bounds[2*i])) * ndivs[i]);
      }
    
    // each bucket inbetween min/max point may have cell in it
    for ( k = ijkMin[2]; k <= ijkMin[2]; k++ )
      {
      for ( j = ijkMin[1]; j <= ijkMax[1]; j++ )
        {
        for ( i = ijkMin[0]; i <= ijkMin[0]; i++ )
          {
          idx = i + j*ndivs[0] + k*product;
          bucket = this->HashTable[idx];
          if ( ! bucket )
            {
            bucket = new vlIdList(numCellsInBucket/2);
            this->HashTable[idx] = bucket;
            }
          bucket->InsertNextId(cellId);
          }
        }
      }

    } //for all cells

  this->SubDivideTime.Modified();
}


//
//  Internal function to get cell neighbors at specified level
//
void vlCellLocator::GetCellNeighbors(int ijk[3], int ndivs[3], int level)
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
    Cells.InsertNextCell(ijk);
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
          Cells.InsertNextCell(nei);
          }
        }
      }
    }

  return;
  }

