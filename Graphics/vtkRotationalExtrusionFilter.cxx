/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRotationalExtrusionFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRotationalExtrusionFilter.h"
#include "vtkMath.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRotationalExtrusionFilter, "1.44");
vtkStandardNewMacro(vtkRotationalExtrusionFilter);

// Create object with capping on, angle of 360 degrees, resolution = 12, and
// no translation along z-axis.
// vector (0,0,1), and point (0,0,0).
vtkRotationalExtrusionFilter::vtkRotationalExtrusionFilter()
{
  this->Capping = 1;
  this->Angle = 360.0;
  this->DeltaRadius = 0.0;
  this->Translation = 0.0;
  this->Resolution = 12; // 30 degree increments
}

void vtkRotationalExtrusionFilter::Execute()
{
  vtkIdType numPts, numCells;
  vtkPolyData *input= this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkPolyData *mesh;
  vtkPoints *inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int numEdges, dim;
  vtkIdType *pts, npts, cellId, ptId, ncells;
  float *x, newX[3], radius, angleIncr, radIncr, transIncr;
  float psi, theta;
  vtkPoints *newPts;
  vtkCellArray *newLines=NULL, *newPolys=NULL, *newStrips=NULL;
  vtkCell *edge;
  vtkIdList *cellIds, *cellPts;
  int i, j, k;
  vtkIdType p1, p2;
  vtkPolyData *output= this->GetOutput();
  vtkPointData *outPD=output->GetPointData();
  double tempd;

  // Initialize / check input
  //
  vtkDebugMacro(<<"Rotationally extruding data");

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to extrude!");
    return;
    }

  // Build cell data structure.
  //
  mesh = vtkPolyData::New();
  inPts = input->GetPoints();
  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  mesh->SetPoints(inPts);
  mesh->SetVerts(inVerts);
  mesh->SetLines(inLines);
  mesh->SetPolys(inPolys);
  mesh->SetStrips(inStrips);
  if ( inPolys || inStrips )
    {
    mesh->BuildLinks();
    }

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified.
  //
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd,(this->Resolution+1)*numPts);
  newPts = vtkPoints::New();
  newPts->Allocate((this->Resolution+1)*numPts);
  if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(ncells,this->Resolution+1));
    }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
           inStrips->GetNumberOfCells()/10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(ncells,2*(this->Resolution+1)));

  // copy points
  for (ptId=0; ptId < numPts; ptId++) //base level
    {
    newPts->InsertPoint(ptId,inPts->GetPoint(ptId));
    outPD->CopyData(pd,ptId,ptId);
    }
  this->UpdateProgress(0.1);

  // loop assumes rotation around z-axis
  radIncr = this->DeltaRadius / this->Resolution;
  transIncr = this->Translation / this->Resolution;
  angleIncr = this->Angle / this->Resolution * vtkMath::DegreesToRadians();
  for ( i = 1; i <= this->Resolution; i++ )
    {
    this->UpdateProgress(0.1 + 0.5*(i-1)/this->Resolution);
    for (ptId=0; ptId < numPts; ptId++)
      {
      x = inPts->GetPoint(ptId);
      //convert to cylindrical
      radius = sqrt(x[0]*x[0] + x[1]*x[1]);
      if (radius > 0.0)
        {
        tempd = (double)x[0]/radius;
        if (tempd < -1.0)
          {
          tempd = -1.0;
          }
        if (tempd > 1.0)
          {
          tempd = 1.0;
          }
        theta = acos(tempd);
        tempd = (double)x[1]/radius;
        if (tempd < -1.0)
          {
          tempd = -1.0;
          }
        if (tempd > 1.0)
          {
          tempd = 1.0;
          }
        if ( (psi=asin(tempd)) < 0.0 ) 
          {
          if ( theta < (vtkMath::Pi()/2.0) )
            {
            theta = 2.0*vtkMath::Pi() + psi;
            }
          else
            {
            theta = vtkMath::Pi() - psi;
            }
          }

        //increment angle
        radius += i*radIncr;
        newX[0] = radius * cos (i*angleIncr + theta);
        newX[1] = radius * sin (i*angleIncr + theta);
        newX[2] = x[2] + i * transIncr;
        }
      else // radius is zero
        {
        newX[0] = 0.0;
        newX[1] = 0.0;
        newX[2] = x[2] + i * transIncr;
        }
      newPts->InsertPoint(ptId+i*numPts,newX);
      outPD->CopyData(pd,ptId,ptId+i*numPts);
      }
    }

  // If capping is on, copy 2D cells to output (plus create cap)
  //
  if ( this->Capping && (this->Angle != 360.0 || this->DeltaRadius != 0.0 ||
  this->Translation != 0.0) )
    {
    if ( inPolys->GetNumberOfCells() > 0 )
      {
      newPolys = vtkCellArray::New();
      newPolys->Allocate(inPolys->GetSize());
      for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        newPolys->InsertNextCell(npts,pts);
        newPolys->InsertNextCell(npts);
        // note that we need to reverse the vertex order on the far cap
        for (i=0; i < npts; i++)
          {
          newPolys->InsertCellPoint(pts[i] + this->Resolution*numPts);
          }
        }
      }
    
    if ( inStrips->GetNumberOfCells() > 0 )
      {
      for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
        {
        newStrips->InsertNextCell(npts,pts);
        newStrips->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          {
          newStrips->InsertCellPoint(pts[i] + this->Resolution*numPts);
          }
        }
      }
    }

  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_CELL_SIZE);

  // Loop over all polygons and triangle strips searching for boundary edges. 
  // If boundary edge found, extrude triangle strip.
  //
  int abort=0;
  vtkIdType progressInterval = numCells/10 + 1;
  vtkGenericCell *cell = vtkGenericCell::New();
  for ( cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) ) //manage progress / early abort
      {
      this->UpdateProgress (0.6 + 0.4*cellId/numCells);
      abort = this->GetAbortExecute();
      }

    mesh->GetCell(cellId,cell);
    cellPts = cell->GetPointIds();

    if ( (dim=cell->GetCellDimension()) == 0 ) //create lines from points
      {
      for (i=0; i<cellPts->GetNumberOfIds(); i++)
        {
        ptId = cellPts->GetId(i);
        newLines->InsertNextCell(this->Resolution+1);

        for ( j=0; j<=this->Resolution; j++ )
          {
          newLines->InsertCellPoint(ptId + j*numPts);
          }
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
          newStrips->InsertCellPoint(p2 + j*numPts);
          newStrips->InsertCellPoint(p1 + j*numPts);
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
          p1 = edge->PointIds->GetId(j);
          p2 = edge->PointIds->GetId(j+1);
          mesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

          if ( cellIds->GetNumberOfIds() < 1 ) //generate strip
            {
            newStrips->InsertNextCell(2*(this->Resolution+1));
            for (k=0; k<=this->Resolution; k++)
              {
              newStrips->InsertCellPoint(p2 + k*numPts);
              newStrips->InsertCellPoint(p1 + k*numPts);
              }
            } //if boundary edge
          } //for each sub-edge
        } //for each edge
      } //for each polygon or triangle strip
    } //for each cell
  cell->Delete();

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  cellIds->Delete();
  mesh->Delete();

  if ( newLines ) 
    {
    output->SetLines(newLines);
    newLines->Delete();
    }

  if ( newPolys ) 
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }

  output->SetStrips(newStrips);
  newStrips->Delete();

  output->Squeeze();
}



void vtkRotationalExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
}

