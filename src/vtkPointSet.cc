/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.cc
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


=========================================================================*/
#include "vtkPointSet.hh"

vtkPointSet::vtkPointSet ()
{
  this->Points = NULL;
  this->Locator = NULL;
}

vtkPointSet::vtkPointSet(const vtkPointSet& ps) :
vtkDataSet(ps)
{
  this->Points = ps.Points;
  if (this->Points) this->Points->Register(this);

  this->Locator = ps.Locator;
}

vtkPointSet::~vtkPointSet ()
{
  this->Initialize();
  if ( this->Locator ) this->Locator->Delete();
}

// Description:
// Copy the geometric structure of an input point set object.
void vtkPointSet::CopyStructure(vtkDataSet *ds)
{
  vtkPointSet *ps=(vtkPointSet *)ds;
  this->Initialize();

  this->Points = ps->Points;
  if (this->Points) this->Points->Register(this);
}


void vtkPointSet::Initialize()
{
  vtkDataSet::Initialize();

  if ( this->Points ) 
  {
    this->Points->UnRegister(this);
    this->Points = NULL;
  }

  if ( this->Locator ) 
  {
    this->Locator->Initialize();
  }
}
void vtkPointSet::ComputeBounds()
{
  float *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (int i=0; i<6; i++) this->Bounds[i] = bounds[i];
    this->ComputeTime.Modified();
    }
}

unsigned long int vtkPointSet::GetMTime()
{
  unsigned long int dsTime = vtkDataSet::GetMTime();

  if ( this->Points ) 
    {
    if ( this->Points->GetMTime() > dsTime ) dsTime = this->Points->GetMTime();
    }

  // don't get locator's mtime because its an internal object that cannot be 
  // modified directly from outside. Causes problems due to FindCell() 
  // SetPoints() method.

  return dsTime;
}

int vtkPointSet::FindPoint(float x[3])
{
  if ( !this->Points ) return -1;

  if ( !this->Locator )
    {
    this->Locator = new vtkPointLocator;
    this->Locator->SetDataSet(this);
    }

  if ( this->Points->GetMTime() > this->Locator->GetMTime() )
    {
    this->Locator->SetDataSet(this);
    }

  return this->Locator->FindClosestPoint(x);
}

int vtkPointSet::FindCell(float x[3], vtkCell *cell, float tol2, int& subId,
                          float pcoords[3], float *weights)
{
  int i, j;
  int closestCell = -1;
  int ptId, cellId;
  float dist2, minDist2=VTK_LARGE_FLOAT;
  int sId;
  float pc[3], closestPoint[3];
  static vtkIdList cellIds(VTK_CELL_SIZE);
  static float *w=new float[this->GetMaxCellSize()];

  if ( !this->Points ) return -1;

  if ( !this->Locator )
    {
    this->Locator = new vtkPointLocator;
    this->Locator->SetDataSet(this);
    }

  if ( this->Points->GetMTime() > this->Locator->GetMTime() )
    {
    this->Locator->SetDataSet(this);
    }

// Find the closest point to the input position.  Then get the cells that 
// use the point.  Then determine if point is in any of the cells.

  if ( (ptId = this->Locator->FindClosestPoint(x)) >= 0 )
    {
    this->GetPointCells(ptId, cellIds);
    for (i=0; i<cellIds.GetNumberOfIds(); i++)
      {
      cellId = cellIds.GetId(i);
      cell = this->GetCell(cellId);
      if ( cell->EvaluatePosition(x,closestPoint,sId,pc,dist2,w) != -1 &&
      dist2 <= tol2 && dist2 < minDist2 )
        {
        minDist2 = dist2;
        closestCell = cellId;
        subId = sId;
        pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = pc[2]; 
        for (j=0; j < cell->GetNumberOfPoints(); j++) weights[j] = w[j];
        }
      }
    }
  return closestCell;
}

void vtkPointSet::Squeeze()
{
  if ( this->Points ) this->Points->Squeeze();
  vtkDataSet::Squeeze();
}

void vtkPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Point Data: " << this->Points << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
}

