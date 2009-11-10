/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineGraphEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkSplineGraphEdges.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkSplineFilter.h"
#include "vtkStdString.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkSplineGraphEdges, "1.3");
vtkStandardNewMacro(vtkSplineGraphEdges);

vtkSplineGraphEdges::vtkSplineGraphEdges()
{
  this->GraphToPoly = vtkGraphToPolyData::New();
  this->Spline = vtkSplineFilter::New();
}

vtkSplineGraphEdges::~vtkSplineGraphEdges()
{
  this->GraphToPoly->Delete();
  this->Spline->Delete();
}

unsigned long vtkSplineGraphEdges::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if (this->Spline->GetMTime() > mtime)
    {
    mtime = this->Spline->GetMTime();
    }
  if (this->GraphToPoly->GetMTime() > mtime)
    {
    mtime = this->GraphToPoly->GetMTime();
    }
  return mtime;
}

int vtkSplineGraphEdges::RequestData(
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
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);
  output->DeepCopyEdgePoints(input);

  vtkGraph* copy = vtkGraph::SafeDownCast(input->NewInstance());
  copy->ShallowCopy(input);
  this->GraphToPoly->SetInput(copy);
  this->Spline->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->Spline->Update();
  vtkPolyData* splined = this->Spline->GetOutput();
  vtkCellArray* lines = splined->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();
  vtkPoints* pts = splined->GetPoints();
  double pt[3];
  lines->InitTraversal();
  vtkIdType numCellPts = 0;
  vtkIdType* cellPts = 0;
  for (vtkIdType i = 0; i < numLines; ++i)
    {
    lines->GetNextCell(numCellPts, cellPts);
    // Add spline points for the edge.
    // Don't include the start and end, since those are the
    // vertex locations.
    output->ClearEdgePoints(i);
    for (vtkIdType j = 1; j < numCellPts-1; ++j)
      {
      pts->GetPoint(cellPts[j], pt);
      output->AddEdgePoint(i, pt);
      }
    }
  copy->Delete();

  return 1;
}

void vtkSplineGraphEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
