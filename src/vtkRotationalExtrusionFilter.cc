/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRotationalExtrusionFilter.cc
  Language:  C++
  Date:      09 Oct 1995
  Version:   1.13


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
#include "vtkRotationalExtrusionFilter.hh"
#include "vtkMath.hh"
#include "vtkIdList.hh"

// Description:
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
  int numPts, numCells;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkPolyData mesh;
  vtkPoints *inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int npts, *pts, numEdges, cellId, dim;
  int ptId, ncells;
  float *x, newX[3], radius, angleIncr, radIncr, transIncr;
  float psi, theta;
  vtkFloatPoints *newPts;
  vtkCellArray *newLines=NULL, *newPolys=NULL, *newStrips=NULL;
  vtkCell *cell, *edge;
  vtkIdList cellIds(VTK_CELL_SIZE), *cellPts;
  vtkMath math;
  int i, j, k, p1, p2;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkPointData *outPD=output->GetPointData();
//
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
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd,(this->Resolution+1)*numPts);
  newPts = new vtkFloatPoints((this->Resolution+1)*numPts);
  if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
    {
    newLines = new vtkCellArray;
    newLines->Allocate(newLines->EstimateSize(ncells,this->Resolution+1));
    }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
           inStrips->GetNumberOfCells()/10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = new vtkCellArray;
  newStrips->Allocate(newStrips->EstimateSize(ncells,2*(this->Resolution+1)));

  // copy points
  for (ptId=0; ptId < numPts; ptId++) //base level
    {
    newPts->SetPoint(ptId,inPts->GetPoint(ptId));
    outPD->CopyData(pd,ptId,ptId);
    }

  // loop assumes rotation around z-axis
  radIncr = this->DeltaRadius / this->Resolution;
  transIncr = this->Translation / this->Resolution;
  angleIncr = this->Angle / this->Resolution * math.DegreesToRadians();
  for ( i = 1; i <= this->Resolution; i++ )
    {
    for (ptId=0; ptId < numPts; ptId++)
      {
      x = inPts->GetPoint(ptId);
      //convert to cylindrical
      radius = sqrt(x[0]*x[0] + x[1]*x[1]);
      theta = acos((double)x[0]/radius);
      if ( (psi=asin((double)x[1]/radius)) < 0.0 ) 
        {
        if ( theta > 0.0 ) theta = 2.0*math.Pi() + psi;
        else theta = math.Pi() - psi;
        }

      //increment angle
      radius += i*radIncr;
      newX[0] = radius * cos (i*angleIncr + theta);
      newX[1] = radius * sin (i*angleIncr + theta);
      newX[2] = x[2] + i * transIncr;

      newPts->SetPoint(ptId+i*numPts,newX);
      outPD->CopyData(pd,ptId,ptId+i*numPts);
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
      newPolys = new vtkCellArray(inPolys->GetSize());
      for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        newPolys->InsertNextCell(npts,pts);
        newPolys->InsertNextCell(npts);
	// note that we need to reverse the vertex order on the far cap
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
          p1 = edge->PointIds.GetId(j);
          p2 = edge->PointIds.GetId(j+1);
          mesh.GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

          if ( cellIds.GetNumberOfIds() < 1 ) //generate strip
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
//
// Update ourselves and release memory
//
  output->SetPoints(newPts);
  newPts->Delete();

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
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
}

