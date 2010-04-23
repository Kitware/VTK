/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFillHolesFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFillHolesFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTriangleStrip.h"
#include "vtkPolygon.h"
#include "vtkSphere.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkFillHolesFilter);

//------------------------------------------------------------------------
vtkFillHolesFilter::vtkFillHolesFilter()
{
  this->HoleSize = 1.0;
}

//------------------------------------------------------------------------
vtkFillHolesFilter::~vtkFillHolesFilter()
{
}

//------------------------------------------------------------------------
int vtkFillHolesFilter::RequestData(
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

  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();

  vtkDebugMacro(<<"Executing hole fill operation");
  
  // check the input, build data structures as necessary
  vtkIdType numPts, npts, *pts;
  vtkPoints *inPts=input->GetPoints();
  vtkIdType numPolys = input->GetNumberOfPolys();
  vtkIdType numStrips = input->GetNumberOfStrips();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || !inPts || 
       (numPolys < 1 && numStrips < 1) )
    {
    vtkDebugMacro(<<"No input data!");
    return 1;
    }

  vtkPolyData *Mesh = vtkPolyData::New();
  Mesh->SetPoints(inPts);
  vtkCellArray *newPolys, *inPolys=input->GetPolys(), *inStrips=input->GetStrips();
  if ( numStrips > 0 )
    {
    newPolys = vtkCellArray::New();
    if ( numPolys > 0 )
      {
      newPolys->DeepCopy(inPolys);
      }
    else
      {
      newPolys->Allocate(newPolys->EstimateSize(numStrips,5));
      }
    inStrips = input->GetStrips();
    for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      vtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
      }
    Mesh->SetPolys(newPolys);
    newPolys->Delete();
    }
  else
    {
    newPolys = inPolys;
    Mesh->SetPolys(newPolys);
    }
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocation sizes)
  //
  vtkPolyData *Lines = vtkPolyData::New();
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(numPts/10);
  Lines->SetLines(newLines);
  Lines->SetPoints(inPts);
  
  // grab all free edges and place them into a temporary polydata
  int abort=0;
  vtkIdType cellId, p1, p2, numNei, i, numCells=newPolys->GetNumberOfCells();
  vtkIdType progressInterval=numCells/20+1;
  vtkIdList *neighbors = vtkIdList::New();
  neighbors->Allocate(VTK_CELL_SIZE);
  for (cellId=0, newPolys->InitTraversal(); 
       newPolys->GetNextCell(npts,pts) && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) ) //manage progress / early abort
      {
      this->UpdateProgress (static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
      }

    for (i=0; i < npts; i++) 
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];

      Mesh->GetCellEdgeNeighbors(cellId,p1,p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      if ( numNei < 1 )
        {
        newLines->InsertNextCell(2);
        newLines->InsertCellPoint(p1);
        newLines->InsertCellPoint(p2);
        }
      }
    }
  
  // Track all free edges and see whether polygons can be built from them.
  // For each polygon of appropriate HoleSize, triangulate the hole and
  // add to the output list of cells
  vtkIdType numHolesFilled=0;
  numCells = newLines->GetNumberOfCells();
  vtkCellArray *newCells = NULL;
  if ( numCells > 3 ) //only do the work if there are free edges
    {
    double sphere[4];
    vtkIdType startId, neiId, currentCellId, hints[2]; hints[0]=0; hints[1]=0;
    vtkPolygon *polygon=vtkPolygon::New();
    polygon->Points->SetDataTypeToDouble();
    vtkIdList *endId = vtkIdList::New();
    endId->SetNumberOfIds(1);
    char *visited = new char [numCells];
    memset(visited, 0, numCells);
    Lines->BuildLinks(); //build the neighbor data structure
    newCells = vtkCellArray::New();
    newCells->DeepCopy(inPolys);

    for (cellId=0; cellId < numCells && !abort; cellId++)
      {
      if ( ! visited[cellId] )
        {
        visited[cellId] = 1;
        // Setup the polygon
        Lines->GetCellPoints(cellId, npts, pts);
        startId = pts[0];
        polygon->PointIds->Reset();
        polygon->Points->Reset();
        polygon->PointIds->InsertId(0,pts[0]);
        polygon->Points->InsertPoint(0,inPts->GetPoint(pts[0]));

        // Work around the loop and terminate when the loop ends
        endId->SetId(0,pts[1]);
        int valid = 1;
        currentCellId = cellId;
        while ( startId != endId->GetId(0) && valid )
          {
          polygon->PointIds->InsertNextId(endId->GetId(0));
          polygon->Points->InsertNextPoint(inPts->GetPoint(endId->GetId(0)));
          Lines->GetCellNeighbors(currentCellId,endId,neighbors);
          if ( neighbors->GetNumberOfIds() == 0 )
            {
            valid = 0;
            }
          else if ( neighbors->GetNumberOfIds() > 1 )
            {
            //have to logically split this vertex
            valid = 0;
            }
          else
            {
            neiId = neighbors->GetId(0);
            visited[neiId] = 1;
            Lines->GetCellPoints(neiId,npts,pts);
            endId->SetId( 0, (pts[0] != endId->GetId(0) ? pts[0] : pts[1] ) );
            currentCellId = neiId;
            }
          }//while loop connected

        // Evaluate the size of the loop and see if it is small enough
        if ( valid )
          {
          vtkSphere::ComputeBoundingSphere(static_cast<vtkDoubleArray*>(polygon->Points->GetData())->GetPointer(0),
                                           polygon->PointIds->GetNumberOfIds(),sphere,hints);
          if ( sphere[3] <= this->HoleSize )
            {
            // Now triangulate the loop and pass to the output
            numHolesFilled++;
            polygon->NonDegenerateTriangulate(neighbors);
            for ( i=0; i < neighbors->GetNumberOfIds(); i+=3 )
              {
              newCells->InsertNextCell(3);
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i)));
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i+1)));
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i+2)));
              }
            }//if hole small enough
          }//if a valid loop
        }//if not yet visited a line
      }//for all lines
    polygon->Delete();
    endId->Delete();
    delete [] visited;
    }//if loops present in the input
  
  // Clean up
  neighbors->Delete();
  Lines->Delete();

  // No new points are created, so the points and point data can be passed
  // through to the output.
  output->SetPoints(inPts);
  outPD->PassData(pd);

  // New cells are created, so currently we do not pass the cell data.
  // It would be pretty easy to extend the existing cell data and mark
  // the new cells with special data values.
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  if ( newCells )
    {
    output->SetPolys(newCells);
    newCells->Delete();
    }
  output->SetStrips(input->GetStrips());

  Mesh->Delete();
  newLines->Delete();
  return 1;
}

//------------------------------------------------------------------------
void vtkFillHolesFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Hole Size: " << this->HoleSize << "\n";
}
