/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEdgeCenters.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkEdgeCenters);

// Construct object with vertex cell generation turned off.
vtkEdgeCenters::vtkEdgeCenters()
{
  this->VertexCells = 0;
}

// Generate points
int vtkEdgeCenters::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numEdges;
  vtkDataSetAttributes *inED;
  vtkDataSetAttributes *outPD;
  vtkPoints *newPts;
  double x[3];

  inED=input->GetEdgeData();
  outPD=output->GetPointData();

  if ( (numEdges = input->GetNumberOfEdges()) < 1 )
    {
    vtkDebugMacro(<<"No cells to generate center points for");
    return 1;
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numEdges);

  int abort=0;
  vtkIdType progressInterval = numEdges/10 + 1;
  vtkEdgeListIterator *edges = vtkEdgeListIterator::New();
  input->GetEdges(edges);
  vtkIdType processed = 0;
  while (edges->HasNext() && !abort)
    {
    vtkEdgeType e = edges->Next();
    if ( ! (processed % progressInterval) ) 
      {
      vtkDebugMacro(<<"Processing #" << processed);
      this->UpdateProgress (0.5*processed/numEdges);
      abort = this->GetAbortExecute();
      }
    double p1[3];
    double p2[3];
    input->GetPoint(e.Source, p1);
    input->GetPoint(e.Target, p2);
    vtkIdType npts = 0;
    double* pts = 0;
    input->GetEdgePoints(e.Id, npts, pts);
    // Find the length of the edge
    if (npts == 0)
      {
      for (int c = 0; c < 3; ++c)
        {
        x[c] = (p1[c] + p2[c]) / 2.0;
        }
      }
    else
      {
      vtkIdType nptsFull = npts + 2;
      double* ptsFull = new double[3*(nptsFull)];
      ptsFull[0] = p1[0];
      ptsFull[1] = p1[1];
      ptsFull[2] = p1[2];
      memcpy(ptsFull+3, pts, sizeof(double)*3*npts);
      ptsFull[3*(nptsFull-1)+0] = p2[0];
      ptsFull[3*(nptsFull-1)+1] = p2[1];
      ptsFull[3*(nptsFull-1)+2] = p2[2];

      double len = 0.0;
      double* curPt = ptsFull;
      for (vtkIdType i = 0; i < nptsFull-1; ++i, curPt += 3)
        {
        len += sqrt(vtkMath::Distance2BetweenPoints(curPt, curPt+3));
        }
      double half = len / 2;
      len = 0.0;
      curPt = ptsFull;
      for (vtkIdType i = 0; i < nptsFull-1; ++i, curPt += 3)
        {
        double curDist = sqrt(vtkMath::Distance2BetweenPoints(curPt, curPt+3));
        if (len + curDist > half)
          {
          double alpha = (half - len)/curDist;
          for (int c = 0; c < 3; ++c)
            {
            x[c] = (1.0 - alpha)*curPt[c] + alpha*curPt[c+3];
            }
          break;
          }
        len += curDist;
        }
      delete [] ptsFull;
      }
    newPts->SetPoint(e.Id, x);
    ++processed;
    }
  edges->Delete();

  if ( this->VertexCells )
    {
    vtkIdType pts[1];
    vtkCellData *outCD = output->GetCellData();
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(numEdges, 2), 1);

    processed = 0;
    edges = vtkEdgeListIterator::New();
    input->GetEdges(edges);
    while (edges->HasNext() && !abort)
      {
      vtkEdgeType e = edges->Next();
      if ( ! (processed % progressInterval) ) 
        {
        vtkDebugMacro(<<"Processing #" << processed);
        this->UpdateProgress (0.5+0.5*processed/numEdges);
        abort = this->GetAbortExecute();
        }

      pts[0] = e.Id;
      verts->InsertNextCell(1, pts);
      ++processed;
      }
    edges->Delete();

    output->SetVerts(verts);
    verts->Delete();
    outCD->PassData(inED); //only if verts are generated
    }

  // clean up and update output
  output->SetPoints(newPts);
  newPts->Delete();

  outPD->PassData(inED); //because number of points = number of cells

  return 1;
}

int vtkEdgeCenters::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

void vtkEdgeCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}

