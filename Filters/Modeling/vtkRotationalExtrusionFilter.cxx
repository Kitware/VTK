/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRotationalExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRotationalExtrusionFilter.h"

#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"

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

int vtkRotationalExtrusionFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells;
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkPolyData *mesh;
  vtkPoints *inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int numEdges;
  vtkIdType *pts = 0;
  vtkIdType npts = 0;
  vtkIdType cellId, ptId, ncells;
  double x[3], newX[3], radius, angleIncr, radIncr, transIncr;
  double psi, theta;
  vtkPoints *newPts;
  vtkCellArray *newLines=NULL, *newPolys=NULL, *newStrips;
  vtkCell *edge;
  vtkIdList *cellIds;
  int i, j, k;
  vtkIdType p1, p2;
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  double tempd;
  int abort=0;

  // Initialize / check input
  //
  vtkDebugMacro(<<"Rotationally extruding data");

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to extrude!");
    return 1;
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
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd,ncells);

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
  angleIncr = vtkMath::RadiansFromDegrees( this->Angle ) / this->Resolution;
  for ( i = 1; i <= this->Resolution; i++ )
    {
    this->UpdateProgress(0.1 + 0.5*(i-1)/this->Resolution);
    for (ptId=0; ptId < numPts; ptId++)
      {
      inPts->GetPoint(ptId, x);
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

  // To insure that cell attributes are in consistent order with the
  // cellId's, we process the verts, lines, polys and strips in order.
  vtkIdType newCellId=0;
  int type;
  if ( newLines ) // there are verts which produce lines
    {
    for ( cellId=0; cellId < numCells && !abort; cellId++)
      {
      type = mesh->GetCellType(cellId);
      if ( type == VTK_VERTEX || type == VTK_POLY_VERTEX )
        {
        mesh->GetCellPoints(cellId,npts,pts);
        for (i=0; i<npts; i++)
          {
          ptId = pts[i];
          newLines->InsertNextCell(this->Resolution+1);
          for ( j=0; j<=this->Resolution; j++ )
            {
            newLines->InsertCellPoint(ptId + j*numPts);
            }
          outCD->CopyData(cd,cellId,newCellId++);
          }
        }//if a vertex or polyVertex
      }//for all cells
    }//if there are verts generating lines
  this->UpdateProgress (0.25);
  abort = this->GetAbortExecute();
  
  // If capping is on, copy 2D cells to output (plus create cap). Notice
  // that polygons are done first, then strips.
  //
  if ( this->Capping && (this->Angle != 360.0 || this->DeltaRadius != 0.0 
                         || this->Translation != 0.0) )
    {
    if ( inPolys->GetNumberOfCells() > 0 )
      {
      newPolys = vtkCellArray::New();
      newPolys->Allocate(inPolys->GetSize());

      for ( cellId=0; cellId < numCells && !abort; cellId++ )
        {
        type = mesh->GetCellType(cellId);
        if ( type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON )
          {
          mesh->GetCellPoints(cellId, npts, pts);
          newPolys->InsertNextCell(npts,pts);
          outCD->CopyData(cd,cellId,newCellId++);
          newPolys->InsertNextCell(npts);
          for (i=0; i < npts; i++)
            {
            newPolys->InsertCellPoint(pts[i] + this->Resolution*numPts);
            }
          outCD->CopyData(cd,cellId,newCellId++);
          }
        }
      }

    for ( cellId=0; cellId < numCells && !abort; cellId++ )
      {
      type = mesh->GetCellType(cellId);
      if ( type == VTK_TRIANGLE_STRIP )
        {
        mesh->GetCellPoints(cellId, npts, pts);
        newStrips->InsertNextCell(npts,pts);
        outCD->CopyData(cd,cellId,newCellId++);
        newStrips->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          {
          newStrips->InsertCellPoint(pts[i] + this->Resolution*numPts);
          }
        outCD->CopyData(cd,cellId,newCellId++);
        }
      }
    }//if capping
  this->UpdateProgress (0.5);
  abort = this->GetAbortExecute();

  // Now process lines, polys and/or strips to produce strips
  //
  if ( inLines->GetNumberOfCells() || inPolys->GetNumberOfCells() ||
       inStrips->GetNumberOfCells() )
    {
    cellIds = vtkIdList::New();
    cellIds->Allocate(VTK_CELL_SIZE);
    vtkGenericCell *cell = vtkGenericCell::New();

    for ( cellId=0; cellId < numCells && !abort; cellId++)
      {
      type = mesh->GetCellType(cellId);
      if ( type == VTK_LINE || type == VTK_POLY_LINE )
        {
        mesh->GetCellPoints(cellId,npts,pts);
        for (i=0; i<(npts-1); i++)
          {
          p1 = pts[i];
          p2 = pts[i+1];
          newStrips->InsertNextCell(2*(this->Resolution+1));
          for ( j=0; j<=this->Resolution; j++)
            {
            newStrips->InsertCellPoint(p2 + j*numPts);
            newStrips->InsertCellPoint(p1 + j*numPts);
            }
          outCD->CopyData(cd,cellId,newCellId++);
          }
        }//if a line

      else if ( type == VTK_TRIANGLE || type == VTK_QUAD || 
                type == VTK_POLYGON || type == VTK_TRIANGLE_STRIP ) 
        {// create strips from boundary edges
        mesh->GetCell(cellId,cell);
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
              outCD->CopyData(cd,cellId,newCellId++);
              } //if boundary edge
            } //for each sub-edge
          } //for each edge
        } //for each polygon or triangle strip
      }//for all cells

    cellIds->Delete();
    cell->Delete();
    } //if strips are being generated
  this->UpdateProgress (1.00);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
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

  return 1;
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
