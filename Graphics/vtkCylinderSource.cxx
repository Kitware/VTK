/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinderSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCylinderSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>

vtkStandardNewMacro(vtkCylinderSource);

vtkCylinderSource::vtkCylinderSource (int res)
{
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->SetNumberOfInputPorts(0);
}

int vtkCylinderSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double angle= 2.0*3.141592654/this->Resolution;
  int numPolys, numPts;
  double xbot[3], tcbot[2], nbot[3];
  double xtop[3], tctop[2], ntop[3];
  double *center = this->Center;
  int i, idx;
  vtkIdType pts[VTK_CELL_SIZE];
  vtkPoints *newPoints; 
  vtkFloatArray *newNormals;
  vtkFloatArray *newTCoords;
  vtkCellArray *newPolys;
  
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
    nbot[0] = ntop[0] = cos(i*angle);
    xbot[0] = (nbot[0] * this->Radius) + center[0]; 
    xtop[0] = (ntop[0] * this->Radius) + center[0]; 
    tcbot[0] = tctop[0] = fabs(2.0*i/this->Resolution - 1.0);

    // y coordinate
    xbot[1] = 0.5 * this->Height + center[1];
    xtop[1] = -0.5 * this->Height + center[1];
    nbot[1] = ntop[1] = 0.0;
    tcbot[1] = 0.0;
    tctop[1] = 1.0;

    // z coordinate
    nbot[2] = ntop[2] = -sin(i*angle);
    xbot[2] = (nbot[2] * this->Radius) + center[2]; 
    xtop[2] = (ntop[2] * this->Radius) + center[2]; 

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
      xbot[0] = xtop[0] = this->Radius * cos(i*angle);
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
      xbot[2] = xtop[2] = -this->Radius * sin(i*angle);
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

  return 1;
}

void vtkCylinderSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << " )\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
}
