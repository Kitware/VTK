/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinderSource.cxx
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
#include <math.h>
#include "vtkCylinderSource.h"
#include "vtkFloatPoints.h"
#include "vtkFloatNormals.h"
#include "vtkFloatTCoords.h"

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
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
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

  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);
  newTCoords = new vtkFloatTCoords(numPts,2);

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
    newTCoords->InsertTCoord(idx,tcbot);
    newTCoords->InsertTCoord(idx+1,tctop);
    newNormals->InsertNormal(idx,nbot);
    newNormals->InsertNormal(idx+1,ntop);
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
      nbot[1] = -1.0;
      ntop[1] =  1.0;
      xbot[1] += center[1]; xtop[1] += center[1];

      // z coordinate
      xbot[2] = xtop[2] = -this->Radius * sin((double)i*angle);
      tcbot[1] = tctop[1] = xbot[2];
      xbot[2] += center[2]; xtop[2] += center[2];

      idx = 2*this->Resolution;
      newPoints->InsertPoint(idx+i,xbot);
      newTCoords->InsertTCoord(idx+i,tcbot);
      newNormals->InsertNormal(idx+i,nbot);

      idx = 3*this->Resolution;
      newPoints->InsertPoint(idx+i,xtop);
      newTCoords->InsertTCoord(idx+i,tctop);
      newNormals->InsertNormal(idx+i,ntop);
      }
//
// Generate polygons for top/bottom polygons
//
    for (i=0; i<this->Resolution; i++)
      {
      pts[this->Resolution-i-1] = 2*this->Resolution + i;
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
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
