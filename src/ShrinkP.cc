/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ShrinkP.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ShrinkP.hh"

void vtkShrinkPolyData::Execute()
{
  int j, k;
  float center[3];
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewPts, numNewLines, numNewPolys, poly_alloc_size;
  int npts, *pts, newId, newIds[3];
  vtkFloatPoints *newPoints;
  vtkCellArray *newVerts, *newLines, *newPolys;
  float *p1, *p2, *p3, pt[3];
  vtkPolyData *input=(vtkPolyData *)this->Input;
//
// Initialize
//
  vtkDebugMacro(<<"Shrinking polygonal data");
  this->Initialize();

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  numNewPts = input->GetNumberOfVerts();
  numNewLines = 0;
  numNewPolys = 0;
  poly_alloc_size = 0;

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-1) * 2;
    numNewLines += npts - 1;
    }
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numNewPts += npts;
    numNewPolys++;
    poly_alloc_size += npts + 1;
    }
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-2) * 3;
    poly_alloc_size += (npts - 2) * 4;
    }
//
// Allocate
//
  newPoints = new vtkFloatPoints(numNewPts);

  newVerts = new vtkCellArray(input->GetNumberOfVerts());

  newLines = new vtkCellArray;
  newLines->Allocate(numNewLines*3);
 
  newPolys = new vtkCellArray;
  newPolys->Allocate(poly_alloc_size);

  this->PointData.CopyAllocate(pd);
//
// Copy vertices (no shrinking necessary)
//
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    newVerts->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      newId = newPoints->InsertNextPoint(inPts->GetPoint(pts[j]));
      newVerts->InsertCellPoint(newId);
      this->PointData.CopyData(pd,pts[j],newId);
      }    
    }
//
// Lines need to be shrunk, and if polyline, split into separate pieces
//
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k]) / 2.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+1],newIds[1]);

      newLines->InsertNextCell(2,newIds);
      }
    }
//
// Polygons need to be shrunk
//
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    for (center[0]=center[1]=center[2]=0.0, j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++) center[k] += p1[k];
      }

    for (k=0; k<3; k++) center[k] /= npts;

    
    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newId = newPoints->InsertNextPoint(pt);
      newPolys->InsertCellPoint(newId);
      this->PointData.CopyData(pd,pts[j],newId);
      }
    }
//
// Triangle strips need to be shrunk and split into separate pieces.
//
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      p3 = inPts->GetPoint(pts[j+1]);
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+1],newIds[1]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p3[k] - center[k]);
      newIds[2] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+2],newIds[2]);

      newPolys->InsertNextCell(3,newIds);
      }
    }
//
// Update self and release memory
//
  this->SetPoints(newPoints);
  newPoints->Delete();

  this->SetVerts(newVerts);
  newVerts->Delete();

  this->SetLines(newLines);
  newLines->Delete();
 
  this->SetPolys(newPolys);
  newPolys->Delete();
}


void vtkShrinkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
