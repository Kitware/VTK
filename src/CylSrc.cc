/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CylSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "CylSrc.hh"
#include "FPoints.hh"
#include "FNormals.hh"
#include "FTCoords.hh"

vtkCylinderSource::vtkCylinderSource (int res)
{
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
}

void vtkCylinderSource::Execute()
{
  float angle= 2.0*3.141592654/this->Resolution;
  int numPolys, numPts;
  float xbot[3], tcbot[2], nbot[3];
  float xtop[3], tctop[2], ntop[3];
  int i, idx;
  int pts[MAX_CELL_SIZE];
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
  vtkCellArray *newPolys;
//
// Set things up; allocate memory
//
  this->Initialize();

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

  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,this->Resolution));
//
// Generate points and point data for sides
//
  for (i=0; i<this->Resolution; i++)
    {
    // x coordinate
    xbot[0] = xtop[0] = nbot[0] = ntop[0] = this->Radius * cos((double)i*angle);
    tcbot[0] = tctop[0] = fabs(2.0*i/this->Resolution - 1.0);

    // y coordinate
    xbot[1] = 0.5 * this->Height;
    xtop[1] = -0.5 * this->Height;
    nbot[1] = ntop[1] = 0.0;
    tcbot[1] = 0.0;
    tctop[1] = 1.0;

    // z coordinate
    xbot[2] = xtop[2] = nbot[2] = ntop[2] = -this->Radius * sin((double)i*angle);

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

      // y coordinate
      xbot[1] = 0.5 * this->Height;
      xtop[1] = -0.5 * this->Height;
      nbot[1] = -1.0;
      ntop[1] =  1.0;

      // z coordinate
      xbot[2] = xtop[2] = -this->Radius * sin((double)i*angle);
      tcbot[1] = tctop[1] = xbot[2];

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
  this->SetPoints(newPoints);
  newPoints->Delete();

  this->PointData.SetNormals(newNormals);
  newNormals->Delete();

  this->PointData.SetTCoords(newTCoords);
  newTCoords->Delete();

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  this->SetPolys(newPolys);
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
