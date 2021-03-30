/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractEdges.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkEdgeTable.h"
#include "vtkGenericCell.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkExtractEdges);

//------------------------------------------------------------------------------
// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
  this->Locator = nullptr;
  this->UseAllPoints = false;
}

//------------------------------------------------------------------------------
vtkExtractEdges::~vtkExtractEdges()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//------------------------------------------------------------------------------
// Generate feature edges for mesh
int vtkExtractEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* newPts;
  vtkCellArray* newLines;
  vtkIdType numCells, cellNum, numPts, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  vtkIdType pts[2];
  vtkIdType pt1 = 0, pt2;
  double x[3];
  vtkEdgeTable* edgeTable;
  vtkGenericCell* cell;
  vtkCell* edge;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;

  vtkDebugMacro(<< "Executing edge extractor");

  //  Check input
  //
  numPts = input->GetNumberOfPoints();
  if ((numCells = input->GetNumberOfCells()) < 1 || numPts < 1)
  {
    return 1;
  }

  // If we are using all of the points use a
  // non-locator based approach
  if (this->UseAllPoints)
  {
    return this->NonLocatorExtraction(input, output);
  }

  // Set up processing
  //
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(numPts);
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(numPts * 4, 2);

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd, numPts);

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd, numCells);

  cell = vtkGenericCell::New();
  vtkIdList *edgeIds, *HEedgeIds = vtkIdList::New();
  vtkPoints *edgePts, *HEedgePts = vtkPoints::New();

  // Get our locator for merging points
  //
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds());

  // Loop over all cells, extracting non-visited edges.
  //
  vtkIdType tenth = numCells / 10 + 1;
  for (cellNum = 0; cellNum < numCells && !abort; cellNum++)
  {
    if (!(cellNum % tenth)) // manage progress reports / early abort
    {
      this->UpdateProgress(static_cast<double>(cellNum) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellNum, cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum = 0; edgeNum < numCellEdges; edgeNum++)
    {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();

      // Tessellate higher-order edges
      if (!edge->IsLinear())
      {
        edge->Triangulate(0, HEedgeIds, HEedgePts);
        edgeIds = HEedgeIds;
        edgePts = HEedgePts;

        for (i = 0; i < (edgeIds->GetNumberOfIds() / 2); i++)
        {
          pt1 = edgeIds->GetId(2 * i);
          pt2 = edgeIds->GetId(2 * i + 1);
          edgePts->GetPoint(2 * i, x);
          if (this->Locator->InsertUniquePoint(x, pts[0]))
          {
            outPD->CopyData(pd, pt1, pts[0]);
          }
          edgePts->GetPoint(2 * i + 1, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        }
      } // if non-linear edge

      else // linear edges
      {
        edgeIds = edge->PointIds;
        edgePts = edge->Points;

        for (i = 0; i < numEdgePts; i++, pt1 = pt2, pts[0] = pts[1])
        {
          pt2 = edgeIds->GetId(i);
          edgePts->GetPoint(i, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (i > 0 && edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        } // if linear edge
      }
    } // for all edges of cell
  }   // for all cells

  vtkDebugMacro(<< "Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  HEedgeIds->Delete();
  HEedgePts->Delete();
  edgeTable->Delete();
  cell->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
// Generate feature edges for mesh without a locator - meaning all of the
// original points exist in the output
int vtkExtractEdges::NonLocatorExtraction(vtkDataSet* input, vtkPolyData* output)
{
  vtkCellArray* newLines;
  vtkIdType numCells, cellNum, numPts, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  vtkIdType pts[2];
  vtkEdgeTable* edgeTable;
  vtkGenericCell* cell;
  vtkCell* edge;
  vtkCellData *cd, *outCD;

  vtkDebugMacro(<< "Executing edge extractor");

  //  Check input
  //
  numPts = input->GetNumberOfPoints();
  if ((numCells = input->GetNumberOfCells()) < 1 || numPts < 1)
  {
    return 1;
  }

  // Set up processing
  //
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(numPts);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(numPts * 4, 2);

  // Since we are using all of the points, we can
  // simply pass through the Point Data
  output->GetPointData()->PassData(input->GetPointData());

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd, numCells);

  cell = vtkGenericCell::New();
  vtkIdList *edgeIds, *HEedgeIds = vtkIdList::New();
  vtkPoints* HEedgePts = vtkPoints::New();

  // Is the input a pointset?  In that case we can just
  // reuse the input's points
  vtkPointSet* ps = vtkPointSet::SafeDownCast(input);
  if (ps)
  {
    output->SetPoints(ps->GetPoints());
  }
  else
  {
    // We need to copy the points
    vtkPoints* newPts;
    double pnt[3];
    newPts = vtkPoints::New();
    newPts->Allocate(numPts);
    for (vtkIdType pid = 0; pid < numPts; ++pid)
    {
      input->GetPoint(pid, pnt);
      newPts->InsertNextPoint(pnt);
    }
  }

  // Loop over all cells, extracting non-visited edges.
  //
  vtkIdType tenth = numCells / 10 + 1;
  for (cellNum = 0; cellNum < numCells && !abort; cellNum++)
  {
    if (!(cellNum % tenth)) // manage progress reports / early abort
    {
      this->UpdateProgress(static_cast<double>(cellNum) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellNum, cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum = 0; edgeNum < numCellEdges; edgeNum++)
    {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();

      // Tessellate higher-order edges
      if (!edge->IsLinear())
      {
        edge->Triangulate(0, HEedgeIds, HEedgePts);
        edgeIds = HEedgeIds;

        for (i = 0; i < (edgeIds->GetNumberOfIds() / 2); i++)
        {
          pts[0] = edgeIds->GetId(2 * i);
          pts[1] = edgeIds->GetId(2 * i + 1);
          if (edgeTable->IsEdge(pts[0], pts[1]) == -1)
          {
            edgeTable->InsertEdge(pts[0], pts[1]);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        }
      } // if non-linear edge

      else // linear edges
      {
        edgeIds = edge->PointIds;

        for (i = 0; i < numEdgePts; i++, pts[0] = pts[1])
        {
          pts[1] = edgeIds->GetId(i);
          if (i > 0 && edgeTable->IsEdge(pts[0], pts[1]) == -1)
          {
            edgeTable->InsertEdge(pts[0], pts[1]);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        } // if linear edge
      }
    } // for all edges of cell
  }   // for all cells

  vtkDebugMacro(<< "Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  HEedgeIds->Delete();
  HEedgePts->Delete();
  edgeTable->Delete();
  cell->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkExtractEdges::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExtractEdges::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    vtkMergePoints* locator = vtkMergePoints::New();
    this->SetLocator(locator);
    locator->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkExtractEdges::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << " UseAllPoints:" << this->UseAllPoints << "\n";
  }
  else
  {
    os << indent << "Locator: (none) UseAllPoints:" << this->UseAllPoints << "\n";
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkExtractEdges::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
