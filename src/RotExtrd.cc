/*=========================================================================

  Program:   Visualization Library
  Module:    RotExtrd.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "RotExtrd.hh"
#include "vlMath.hh"
#include "IdList.hh"

// Description:
// Create object with capping on, angle of 360 degrees, resolution = 12, and
// no translation along z-axis.
// vector (0,0,1), and point (0,0,0).
vlRotationalExtrusionFilter::vlRotationalExtrusionFilter()
{
  this->Capping = 1;
  this->Angle = 360.0;
  this->DeltaRadius = 0.0;
  this->Translation = 0.0;
  this->Resolution = 12; // 30 degree increments
}

void vlRotationalExtrusionFilter::Execute()
{
  int numPts, numCells;
  vlPolyData *input=(vlPolyData *)this->Input;
  vlPointData *pd=input->GetPointData();
  vlPolyData mesh;
  vlPoints *inPts;
  vlCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int npts, *pts, numEdges, cellId, dim;
  int ptId, ncellsl;
  float *x, newX[3], radius, angleIncr, radIncr, transIncr;
  vlFloatPoints *newPts;
  vlCellArray *newLines=NULL, *newPolys=NULL, *newStrips=NULL;
  vlCell *cell, *edge;
  vlIdList cellIds(MAX_CELL_SIZE), *cellPts;
  vlMath math;
//
// Initialize / check input
//
  vlDebugMacro(<<"Rotationally extruding data");
  this->Initialize();

  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
  (numCells=input->GetNumberOfCells()) < 1 )
    {
    vlErrorMacro(<<"No data to extrude!");
    return;
    }
//
// Build cell data structure.
//
  inPts = input->GetPoints();
  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  mesh.SetPoints(inPts);
  mesh.SetVerts(inVerts);
  mesh.SetLines(inLines);
  mesh.SetPolys(inPolys);
  mesh.SetStrips(inStrips);
  if ( inPolys || inStrips ) mesh.BuildLinks();
//
// Allocate memory for output. We don't copy normals because surface geometry
// is modified.
//
  this->PointData.CopyNormalsOff();
  this->PointData.CopyAllocate(pd,(this->Resolution+1)*numPts);
  newPts = new vlFloatPoints((this->Resolution+1)*numPts);
  if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
    {
    newLines = new vlCellArray;
    newLines->Allocate(newLines->EstimateSize(ncells,this->Resolution+1));
    }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
           inStrips->GetNumberOfCells()/10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(ncells,2*(this->Resolution+1)));

  // copy points
  for (ptId=0; ptId < numPts; ptId++) //base level
    {
    newPts->SetPoint(ptId,inPts->GetPoint(ptId));
    this->PointData.CopyData(pd,ptId,ptId);
    }

  radIncr = this->DeltaRadius / this->Resolution;
  transIncr = this->Translation / this->Resolution;
  angleIncr = this->Angle / this->Resolution;
  for ( i = 1; i <= this->Resolution; i++ )
    {
    for (ptId=0; ptId < numPts; ptId++)
      {
      x = inPts->GetPoint(ptId);
      radius = sqrt(x[0]*x[0] + x[1]*x[1]) + i * radIncr;
      newX[0] = radius * cos (i*angleIncr*math.DegreesToRadians());
      newX[1] = radius * sin (i*angleIncr*math.DegreesToRadians());
      newX[2] = x[2] + i * transIncr;

      newPts->SetPoint(ptId+i*numPts,newX);
      this->PointData.CopyData(pd,ptId,ptId+i*numPts);
      }
    }
//
// If capping is on, copy 2D cells to output (plus create cap)
//
  if ( this->Capping && (this->Angle != 360.0 || this->DeltaRadius != 0.0 ||
  this->Translation != 0.0) )
    {
    if ( inPolys->GetNumberOfCells() > 0 )
      {
      newPolys = new vlCellArray(inPolys->GetSize());
      for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        newPolys->InsertNextCell(npts,pts);
        newPolys->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          newPolys->InsertCellPoint(pts[i] + this->Resolution*numPts);
        }
      }
    
    if ( inStrips->GetNumberOfCells() > 0 )
      {
      for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
        {
        newStrips->InsertNextCell(npts,pts);
        newStrips->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          newStrips->InsertCellPoint(pts[i] + this->Resolution*numPts);
        }
      }
    }
//
// Loop over all polygons and triangle strips searching for boundary edges. 
// If boundary edge found, extrude triangle strip.
//
  for ( cellId=0; cellId < numCells; cellId++)
    {
    cell = mesh.GetCell(cellId);
    cellPts = cell->GetPointIds();

    if ( (dim=cell->GetCellDimension()) == 0 ) //create lines from points
      {
      for (i=0; i<cellPts->GetNumberOfIds(); i++)
        {
        ptId = cellPts->GetId(i);
        newLines->InsertNextCell(this->Resolution+1);

        for ( j=0; j<=this->Resolution; j++ )
          newLines->InsertCellPoint(ptId + j*numPts);
        }
      }

    else if ( dim == 1 ) // create strips from lines
      {
      for (i=0; i < (cellPts->GetNumberOfIds()-1); i++)
        {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i+1);
        newStrips->InsertNextCell(2*(this->Resolution+1));
        for ( j=0; j<=this->Resolution; j++)
          {
          newStrips->InsertCellPoint(p1 + j*numPts);
          newStrips->InsertCellPoint(p2 + j*numPts);
          }
        }
      }

    else if ( dim == 2 ) // create strips from boundary edges
      {
      numEdges = cell->GetNumberOfEdges();
      for (i=0; i<numEdges; i++)
        {
        edge = cell->GetEdge(i);
        for (j=0; j<(edge->GetNumberOfPoints()-1); j++)
          {
          p1 = pts[j];
          p2 = pts[(j+1)%npts];
          mesh.GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

          if ( cellIds.GetNumberOfIds() < 1 ) //generate strip
            {
            newStrips->InsertNextCell(2*(this->Resolution+1));
            for (k=0; k<=this->Resolution; k++)
              {
              newStrips->InsertCellPoint(p1 + k*numPts);
              newStrips->InsertCellPoint(p2 + k*numPts);
              }
            } //if boundary edge
          } //for each sub-edge
        } //for each edge
      } //for each polygon or triangle strip
    } //for each cell
//
// Send data to output
//
  this->SetPoints(newPts);
  if ( newLines ) this->SetLines(newLines);
  if ( newPolys ) this->SetPolys(newLines);
  this->SetStrips(newStrips);

  this->Squeeze();
}



void vlRotationalExtrusionFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
}

