/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellLoc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
#include <math.h>
#include "CellLoc.hh"
#include "Locator.hh"
#include "vtkMath.hh"
#include "IntArray.hh"

#define OUTSIDE 0
#define INSIDE 1

typedef vtkIdList *vtkIdListPtr;

// Description:
// Construct with automatic computation of divisions, averaging
// 25 cells per octant.
vtkCellLocator::vtkCellLocator()
{
  this->DataSet = NULL;
  this->Level = 4;
  this->MaxLevel = 5;
  this->Automatic = 1;
  this->NumberOfCellsInOctant = 25;
  this->Tolerance = 0.01;
  this->Tree = NULL;
  this->H[0] = this->H[1] = this->H[2] = 1.0;
  this->NumberOfDivisions = 1;
}

vtkCellLocator::~vtkCellLocator()
{
  this->Initialize();
}

void vtkCellLocator::Initialize()
{
  // free up octants
  this->FreeSearchStructure();
}

void vtkCellLocator::FreeSearchStructure()
{
  vtkIdList *cellIds;
  int i;

  if ( this->Tree )
    {
    for (i=0; i<this->NumberOfOctants; i++)
      {
      if ( (cellIds = this->Tree[i]) ) delete cellIds;
      }
    this->Tree = NULL;
    }
}

// Description:
// Given a position x, return the id of the cell closest to it. Cell may 
// contain the point, or just lie near it. Parametric coordinates, subId,
// and distance squared to cell are returned.
int vtkCellLocator::FindClosestCell(float x[3], float dist2,
                                   int& subId, float pcoords[3])
{
  int closest=(-1);

  return closest;
}

// Description:
// Return list of octants that are intersected by line. The octants
// are ordered along the line and are represented by octant number. To obtain
// the cells in the octant, use the method GetOctantCells().
int vtkCellLocator::IntersectWithLine(float a0[3], float a1[3], vtkIdList& cells)
{
  return 0;
}

// Description:
// Get the cells in an octant.
vtkIdList* vtkCellLocator::GetOctantCells(int octantId)
{
  return NULL;
}

// Description:
// Intersect against another vtkCellLocator returning cells that lie in 
// intersecting octants.
int vtkCellLocator::IntersectWithCellLocator(vtkCellLocator& locator, vtkIdList cells)
{
  return 0;
}

//
//  Method to form subdivision of space based on the cells provided and
//  subject to the constraints of levels and NumberOfCellsInOctant.
//  The result is directly addressable and of uniform subdivision.
//
void vtkCellLocator::SubDivide()
{
  vtkCell *cell;
  float *bounds, *cellBounds;
  int numCells;
  float level;
  int ndivs, product;
  int i, j, k, cellId, ijkMin[3], ijkMax[3];
  int idx, parentOffset;
  vtkIdList *octant;
  int numCellsInOctant = this->NumberOfCellsInOctant;
  typedef vtkIdList *vtkIdListPtr;
  int prod, numOctants;

  if ( this->Tree != NULL && this->SubDivideTime > this->MTime ) return;

  vtkDebugMacro( << "Subdividing octree..." );

  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro( << "No cells to subdivide");
    return;
    }
//
//  Make sure the appropriate data is available
//
  if ( this->Tree ) this->FreeSearchStructure();
//
//  Size the root cell.  Initialize cell data structure, compute 
//  level and divisions.
//
  bounds = this->DataSet->GetBounds();
  for (i=0; i<6; i++) this->Bounds[i] = bounds[i];

  if ( this->Automatic ) 
    {
    this->Level = (int) (ceil(log((double)numCells/numCellsInOctant) / 
                              (log((double) 8.0))));
    this->Level =(this->Level > this->MaxLevel ? this->MaxLevel : this->Level);
    } 

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
  memset (this->Tree, (int)NULL, numOctants*sizeof(vtkIdListPtr));
//
//  Compute width of leaf octant in three directions
//
  for (i=0; i<3; i++) this->H[i] = (bounds[2*i+1] - bounds[2*i]) / ndivs;
//
//  Insert each cell into the appropriate octant.  Make sure cell
//  falls within octant.
//
  parentOffset = numOctants - (ndivs * ndivs * ndivs);
  product = ndivs * ndivs;
  for (cellId=0; cellId<numCells; cellId++) 
    {
    cell = this->DataSet->GetCell(cellId);
    cellBounds = cell->GetBounds();

    // find min/max locations of bounding box
    for (i=0; i<3; i++)
      {
      ijkMin[i] = (int) ((float) ((cellBounds[2*i] - bounds[2*i])*0.999 / 
                         (bounds[2*i+1] - bounds[2*i])) * ndivs);
      ijkMax[i] = (int) ((float) ((cellBounds[2*i+1] - bounds[2*i])*0.999 / 
                         (bounds[2*i+1] - bounds[2*i])) * ndivs);
      }
    
    // each octant inbetween min/max point may have cell in it
    for ( k = ijkMin[2]; k <= ijkMin[2]; k++ )
      {
      for ( j = ijkMin[1]; j <= ijkMax[1]; j++ )
        {
        for ( i = ijkMin[0]; i <= ijkMin[0]; i++ )
          {
          this->MarkParents((void*)INSIDE,i,j,k);
          idx = parentOffset + i + j*ndivs + k*product;
          octant = this->Tree[idx];
          if ( ! octant )
            {
            octant = new vtkIdList(numCellsInOctant);
            this->Tree[idx] = octant;
            }
          octant->InsertNextId(cellId);
          }
        }
      }

    } //for all cells

  this->SubDivideTime.Modified();
}

void vtkCellLocator::MarkParents(void* a, int i, int j, int k)
{
  // will's code
}

