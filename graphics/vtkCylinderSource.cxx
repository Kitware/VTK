/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinderSource.cxx
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
#include <math.h>
#include "vtkCylinderSource.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkCylinderSource* vtkCylinderSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCylinderSource");
  if(ret)
    {
    return (vtkCylinderSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCylinderSource;
}




vtkCylinderSource::vtkCylinderSource (int res)
{
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

void vtkCylinderSource::Execute()
{
  float angle= 2.0*3.141592654/this->Resolution;
  int numPolys, numPts;
  float xbot[3], tcbot[2], nbot[3];
  float xtop[3], tctop[2], ntop[3];
  float *center = this->Center;
  int i, idx;
  int pts[VTK_CELL_SIZE];
  vtkPoints *newPoints; 
  vtkFloatArray *newNormals;
  vtkFloatArray *newTCoords;
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
//
// Set things up; allocate memory
//

  if ( this->Capping )
    {
    numPts = 4*this->Resolution;
    numPolys = this->Resolution + 2;
    }
  else 
    {
    numPts = 2*this->Resolution;
    numPolys = this->Resolution;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(numPts);
  newTCoords->SetName("TCoords");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,this->Resolution));
//
// Generate points and point data for sides
//
  for (i=0; i<this->Resolution; i++)
    {
    // x coordinate
    xbot[0] = xtop[0] = nbot[0] = ntop[0] = this->Radius * cos((double)i*angle);
    xbot[0] += center[0]; xtop[0] += center[0];
    tcbot[0] = tctop[0] = fabs(2.0*i/this->Resolution - 1.0);

    // y coordinate
    xbot[1] = 0.5 * this->Height + center[1];
    xtop[1] = -0.5 * this->Height + center[1];
    nbot[1] = ntop[1] = 0.0;
    tcbot[1] = 0.0;
    tctop[1] = 1.0;

    // z coordinate
    xbot[2] = xtop[2] = nbot[2] = ntop[2] = -this->Radius * sin((double)i*angle);
    xbot[2] += center[2]; xtop[2] += center[2];

    idx = 2*i;
    newPoints->InsertPoint(idx,xbot);
    newPoints->InsertPoint(idx+1,xtop);
    newTCoords->InsertTuple(idx,tcbot);
    newTCoords->InsertTuple(idx+1,tctop);
    newNormals->InsertTuple(idx,nbot);
    newNormals->InsertTuple(idx+1,ntop);
    }
//
// Generate polygons for sides
//
  for (i=0; i<this->Resolution; i++)
    {
    pts[0] = 2*i;
    pts[1] = pts[0] + 1;
    pts[2] = (pts[1] + 2) % (2*this->Resolution);
    pts[3] = pts[2] - 1;
    newPolys->InsertNextCell(4,pts);
    }
//
// Generate points and point data for top/bottom polygons
//
  if ( this->Capping )
    {
    for (i=0; i<this->Resolution; i++)
      {
      // x coordinate
      xbot[0] = xtop[0] = this->Radius * cos((double)i*angle);
      nbot[0] = ntop[0] = 0.0;
      tcbot[0] = tctop[0] = xbot[0];
      xbot[0] += center[0]; xtop[0] += center[0];

      // y coordinate
      xbot[1] = 0.5 * this->Height;
      xtop[1] = -0.5 * this->Height;
      nbot[1] = 1.0;
      ntop[1] = -1.0;
      xbot[1] += center[1]; xtop[1] += center[1];

      // z coordinate
      xbot[2] = xtop[2] = -this->Radius * sin((double)i*angle);
      tcbot[1] = tctop[1] = xbot[2];
      xbot[2] += center[2]; xtop[2] += center[2];
      nbot[2] = 0.0;
      ntop[2] = 0.0;

      idx = 2*this->Resolution;
      newPoints->InsertPoint(idx+i,xbot);
      newTCoords->InsertTuple(idx+i,tcbot);
      newNormals->InsertTuple(idx+i,nbot);

      idx = 3*this->Resolution;
      newPoints->InsertPoint(idx+this->Resolution-i-1,xtop);
      newTCoords->InsertTuple(idx+this->Resolution-i-1,tctop);
      newNormals->InsertTuple(idx+this->Resolution-i-1,ntop);
      }
//
// Generate polygons for top/bottom polygons
//
    for (i=0; i<this->Resolution; i++)
      {
      pts[i] = 2*this->Resolution + i;
      }
    newPolys->InsertNextCell(this->Resolution,pts);
    for (i=0; i<this->Resolution; i++)
      {
      pts[i] = 3*this->Resolution + i;
      }
    newPolys->InsertNextCell(this->Resolution,pts);

    } // if capping
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkCylinderSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << " )\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
