/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearExtrusionFilter.cxx
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
#include "vtkLinearExtrusionFilter.h"

// Description:
// Create object with normal extrusion type, capping on, scale factor=1.0,
// vector (0,0,1), and extrusion point (0,0,0).
vtkLinearExtrusionFilter::vtkLinearExtrusionFilter()
{
  this->ExtrusionType = VTK_NORMAL_EXTRUSION;
  this->Capping = 1;
  this->ScaleFactor = 1.0;
  this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
  this->ExtrusionPoint[0] = this->ExtrusionPoint[1] = this->ExtrusionPoint[2] = 0.0;
}

float *vtkLinearExtrusionFilter::ViaNormal(float x[3], int id, vtkNormals *n)
{
  static float xNew[3], *normal;
  int i;

  normal = n->GetNormal(id);
  for (i=0; i<3; i++) 
    xNew[i] = x[i] + this->ScaleFactor*normal[i];

  return xNew;
}

float *vtkLinearExtrusionFilter::ViaVector(float x[3], int vtkNotUsed(id), 
					   vtkNormals *vtkNotUsed(n))
{
  static float xNew[3];
  int i;

  for (i=0; i<3; i++) 
    xNew[i] = x[i] + this->ScaleFactor*this->Vector[i];

  return xNew;
}

float *vtkLinearExtrusionFilter::ViaPoint(float x[3], int vtkNotUsed(id), 
					  vtkNormals *vtkNotUsed(n))
{
  static float xNew[3];
  int i;

  for (i=0; i<3; i++) 
    xNew[i] = x[i] + this->ScaleFactor*(x[i] - this->ExtrusionPoint[i]);

  return xNew;
}

void vtkLinearExtrusionFilter::Execute()
{
  int numPts, numCells;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkNormals *inNormals=NULL;
  vtkPolyData mesh;
  vtkPoints *inPts;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  int npts, *pts, numEdges, cellId, dim;
  int ptId, ncells, i, j, p1, p2;
  float *x;
  vtkFloatPoints *newPts;
  vtkCellArray *newLines=NULL, *newPolys=NULL, *newStrips=NULL;
  vtkCell *cell, *edge;
  vtkIdList cellIds(VTK_CELL_SIZE), *cellPts;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  //
  // Initialize / check input
  //
  vtkDebugMacro(<<"Linearly extruding data");

  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to extrude!");
    return;
    }
//
// Decide which vector to use for extrusion
//
  if ( this->ExtrusionType == VTK_POINT_EXTRUSION )
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaPoint;
    }
  else if ( this->ExtrusionType == VTK_NORMAL_EXTRUSION  &&
  (inNormals = pd->GetNormals()) != NULL )
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaNormal;
    inNormals = pd->GetNormals();
    }
  else // VTK_VECTOR_EXTRUSION
    {
    this->ExtrudePoint = &vtkLinearExtrusionFilter::ViaVector;
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
  if (inPolys->GetNumberOfCells() || inStrips->GetNumberOfCells()) 
    {
    mesh.BuildLinks();
    }
  
//
// Allocate memory for output. We don't copy normals because surface geometry
// is modified. Copy all points - this is the usual requirement and it makes
// creation of skirt much easier.
//
  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd,2*numPts);
  newPts = new vtkFloatPoints(2*numPts);
  if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
    {
    newLines = new vtkCellArray;
    newLines->Allocate(newLines->EstimateSize(ncells,2));
    }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
           inStrips->GetNumberOfCells()/10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = new vtkCellArray;
  newStrips->Allocate(newStrips->EstimateSize(ncells,4));

  // copy points
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    newPts->SetPoint(ptId,x);
    newPts->SetPoint(ptId+numPts,(this->*(ExtrudePoint))(x,ptId,inNormals));
    outputPD->CopyData(pd,ptId,ptId);
    outputPD->CopyData(pd,ptId,ptId+numPts);
    }
//
// If capping is on, copy 2D cells to output (plus create cap)
//
  if ( this->Capping )
    {
    if ( inPolys->GetNumberOfCells() > 0 )
      {
      newPolys = new vtkCellArray(inPolys->GetSize());
      for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        newPolys->InsertNextCell(npts,pts);
        newPolys->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          newPolys->InsertCellPoint(pts[i] + numPts);
        }
      }
    
    if ( inStrips->GetNumberOfCells() > 0 )
      {
      for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
        {
        newStrips->InsertNextCell(npts,pts);
        newStrips->InsertNextCell(npts);
        for (i=0; i < npts; i++)
          newStrips->InsertCellPoint(pts[i] + numPts);
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
        newLines->InsertNextCell(2);
        ptId = cellPts->GetId(i);
        newLines->InsertCellPoint(ptId);
        newLines->InsertCellPoint(ptId+numPts);
        }
      }

    else if ( dim == 1 ) // create strips from lines
      {
      for (i=0; i < (cellPts->GetNumberOfIds()-1); i++)
        {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i+1);
        newStrips->InsertNextCell(4);
        newStrips->InsertCellPoint(p1);
        newStrips->InsertCellPoint(p2);
        newStrips->InsertCellPoint(p1+numPts);
        newStrips->InsertCellPoint(p2+numPts);
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
            newStrips->InsertNextCell(4);
            newStrips->InsertCellPoint(p1);
            newStrips->InsertCellPoint(p2);
            newStrips->InsertCellPoint(p1+numPts);
            newStrips->InsertCellPoint(p2+numPts);
            }
          } //for each sub-edge
        } //for each edge
      } //for each polygon or triangle strip
    } //for each cell
//
// Send data to output and release memory
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



void vtkLinearExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  if ( this->ExtrusionType == VTK_VECTOR_EXTRUSION )
    {
    os << indent << "Extrusion Type: Extrude along vector\n";
    os << indent << "Vector: (" << this->Vector[0] << ", " 
       << this->Vector[1] << ", " << this->Vector[2] << ")\n";
    }
  else if ( this->ExtrusionType == VTK_NORMAL_EXTRUSION )
    {
    os << indent << "Extrusion Type: Extrude along vertex normals\n";
    }
  else //POINT_EXTRUSION
    {
    os << indent << "Extrusion Type: Extrude towards point\n";
    os << indent << "Extrusion Point: (" << this->ExtrusionPoint[0] << ", " 
       << this->ExtrusionPoint[1] << ", " << this->ExtrusionPoint[2] << ")\n";
    }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}

