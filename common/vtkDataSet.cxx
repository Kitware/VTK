/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSet.cxx
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
//
// DataSet methods
//
#include <math.h>
#include "vtkDataSet.h"
#include "vtkSource.h"

// Initialize static member that controls global data release after use by filter
static int vtkDataSetGlobalReleaseDataFlag = 0;

// Description:
// Constructor with default bounds (0,1, 0,1, 0,1).
vtkDataSet::vtkDataSet ()
{
  this->Bounds[0] = 0.0;
  this->Bounds[1] = 1.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 1.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 1.0;
  this->DataReleased = 1;
  this->ReleaseDataFlag = 0;
  this->Source = NULL;
}

// Description:
// Copy constructor.
vtkDataSet::vtkDataSet (const vtkDataSet& ds) :
PointData(ds.PointData)
{
  for (int i=0; i < 6; i++) this->Bounds[i] = ds.Bounds[i];
  this->DataReleased = 1;
  this->ReleaseDataFlag = ds.ReleaseDataFlag;
  this->Source = NULL;
}

void vtkDataSet::Initialize()
{
//
// We don't modify ourselves because the "ReleaseData" methods depend upon
// no modification when initialized.
//
  this->PointData.Initialize();
};

void vtkDataSet::SetGlobalReleaseDataFlag(int val)
{
  if (val == vtkDataSetGlobalReleaseDataFlag) return;
  vtkDataSetGlobalReleaseDataFlag = val;
  vtkDebugMacro( << "setting GlobalReleaseDataFlag to " << val );
  this->Modified(); 
}

int vtkDataSet::GetGlobalReleaseDataFlag()
{
  return vtkDataSetGlobalReleaseDataFlag;
}

void vtkDataSet::ReleaseData()
{
  this->Initialize();
  this->DataReleased = 1;
}

int vtkDataSet::ShouldIReleaseData()
{
  if ( vtkDataSetGlobalReleaseDataFlag || this->ReleaseDataFlag ) return 1;
  else return 0;
}

void vtkDataSet::Update()
{
  if (this->Source)
    {
    this->Source->Update();
    }
}

void vtkDataSet::ForceUpdate()
{
  if (this->Source)
    {
    this->Source->Modified();
    this->Source->Update();
    }
}

// Description:
// Compute the data bounding box from data points.
void vtkDataSet::ComputeBounds()
{
  int i, j;
  float *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    for (i=0; i<this->GetNumberOfPoints(); i++)
      {
      x = this->GetPoint(i);
      for (j=0; j<3; j++)
        {
        if ( x[j] < this->Bounds[2*j] ) this->Bounds[2*j] = x[j];
        if ( x[j] > this->Bounds[2*j+1] ) this->Bounds[2*j+1] = x[j];
        }
      }

    this->ComputeTime.Modified();
    }
}

float *vtkDataSet::GetScalarRange()
{
  vtkScalars *tmp;
  static float res[2] = {0.0,1.0};
  
  tmp = this->PointData.GetScalars();
  
  if (tmp) return tmp->GetRange();
  
  return res;
}

// Description:
// Return a pointer to the geometry bounding box in the form
// (xmin,xmax, ymin,ymax, zmin,zmax).
float *vtkDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}
  
void vtkDataSet::GetBounds(float bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}
  
// Description:
// Get the center of the bounding box.
float *vtkDataSet::GetCenter()
{
  static float center[3];

  this->ComputeBounds();
  for (int i=0; i<3; i++) 
    center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
  return center;
}

void vtkDataSet::GetCenter(float center[3])
{
  float *c=this->GetCenter();
  for (int i=0; i<3; i++) center[i] = c[i];
}
  
// Description:
// Return the length of the diagonal of the bounding box.
float vtkDataSet::GetLength()
{
  double diff, l=0.0;
  int i;

  this->ComputeBounds();
  for (i=0; i<3; i++)
    {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
    }
 
  return (float)sqrt(l);
}

unsigned long int vtkDataSet::GetMTime()
{
  unsigned long result, pointDataMTime;
  
  if (this->Source)
    {
    result = this->Source->GetMTime();
    if (result <  this->MTime)
      {
      result = this->MTime;
      }
    }
  else
    {
    result = this->MTime;
    }
  
  pointDataMTime = this->PointData.GetMTime();
  return ( pointDataMTime > result ? pointDataMTime : result );
}

vtkCell *vtkDataSet::FindAndGetCell (float x[3], vtkCell *cell, int cellId, 
                                     float tol2, int& subId,
                                     float pcoords[3], float *weights)
{
  int newCell = this->FindCell(x,cell,cellId,tol2,subId,pcoords,weights);
  if (newCell >= 0 )
    {
    cell = this->GetCell (newCell);
    }
  else
    {
    return NULL;
    }
  return cell;
}

void vtkDataSet::GetCellNeighbors(int cellId, vtkIdList &ptIds,
                                  vtkIdList &cellIds)
{
  int i;
  vtkIdList otherCells(VTK_CELL_SIZE);

  // load list with candidate cells, remove current cell
  this->GetPointCells(ptIds.GetId(0),cellIds);
  cellIds.DeleteId(cellId);

  // now perform multiple intersections on list
  if ( cellIds.GetNumberOfIds() > 0 )
    {
    for ( i=1; i < ptIds.GetNumberOfIds(); i++)
      {
      this->GetPointCells(ptIds.GetId(i), otherCells);
      cellIds.IntersectWith(otherCells);
      }
    }
}

void vtkDataSet::Squeeze()
{
  this->PointData.Squeeze();
}

void vtkDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  float *bounds;

  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Number Of Cells: " << this->GetNumberOfCells() << "\n";
  os << indent << "Point Data:\n";
  this->PointData.PrintSelf(os,indent.GetNextIndent());
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" <<bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" <<bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" <<bounds[4] << ", " << bounds[5] << ")\n";
  os << indent << "Compute Time: " <<this->ComputeTime.GetMTime() << "\n";
  os << indent << "Release Data: " << (this->ReleaseDataFlag ? "On\n" : "Off\n");
  os << indent << "Global Release Data: " << (vtkDataSetGlobalReleaseDataFlag ? "On\n" : "Off\n");
}

