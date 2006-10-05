/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkIdList.h"
#include "vtkGraphIdList.h"
#include "vtkLine.h"
#include "vtkGenericCell.h"
#include "vtkPointLocator.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkNodeLinks.h"

#include <vtkstd/algorithm> // for sorting

// 
// Standard functions
//

double vtkAbstractGraph::DefaultPoint[3] = {0, 0, 0};

vtkCxxRevisionMacro(vtkAbstractGraph, "1.1");

//----------------------------------------------------------------------------
vtkAbstractGraph::vtkAbstractGraph()
{
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkAbstractGraph::~vtkAbstractGraph()
{
  if (this->Line)
    {
    this->Line->Delete();
    this->Line = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::Initialize()
{
  this->Line->Delete();
  this->Line = vtkLine::New();
}

void vtkAbstractGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::ShallowCopy(vtkDataObject *dataObject)
{
  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::DeepCopy(vtkDataObject *dataObject)
{
  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
double* vtkAbstractGraph::GetPoint(vtkIdType ptId)
{
  if (this->Points)
    {
    return this->Points->GetPoint(ptId);
    }
  return this->DefaultPoint;
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::GetPoint(vtkIdType ptId, double x[3])
{
  if (this->Points)
    {
    this->Points->GetPoint(ptId, x);
    }
  else
    {
    for (int i = 0; i < 3; i++)
      {
      x[i] = this->DefaultPoint[i];
      }
    }
}

//----------------------------------------------------------------------------
vtkPoints* vtkAbstractGraph::GetPoints()
{
  if (!this->Points)
    {
    this->Points = vtkPoints::New();
    }
  if (this->Points->GetNumberOfPoints() != this->GetNumberOfNodes())
    {
    this->Points->SetNumberOfPoints(this->GetNumberOfNodes());
    for (vtkIdType i = 0; i < this->GetNumberOfNodes(); i++)
      {
      this->Points->SetPoint(i, 0, 0, 0);
      }
    }
  return this->Points;
}

//----------------------------------------------------------------------------
vtkCell* vtkAbstractGraph::GetCell(vtkIdType cellId)
{
  double x[3];
  this->GetPoint(this->GetSourceNode(cellId), x);
  this->Line->Points->SetPoint(0, x);
  this->GetPoint(this->GetTargetNode(cellId), x);
  this->Line->Points->SetPoint(1, x);

  this->Line->PointIds->SetId(0, this->GetSourceNode(cellId));
  this->Line->PointIds->SetId(1, this->GetTargetNode(cellId));
  return this->Line;
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::GetCell(vtkIdType cellId, vtkGenericCell * cell)
{
  cell->SetCellType(VTK_LINE);

  cell->Points->SetNumberOfPoints(2);
  double x[3];
  this->GetPoint(this->GetSourceNode(cellId), x);
  cell->Points->SetPoint(0, x);
  this->GetPoint(this->GetTargetNode(cellId), x);
  cell->Points->SetPoint(1, x);

  cell->PointIds->SetNumberOfIds(2);
  cell->PointIds->SetId(0, this->GetSourceNode(cellId));
  cell->PointIds->SetId(1, this->GetTargetNode(cellId));
}

//----------------------------------------------------------------------------
void vtkAbstractGraph::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  ptIds->Reset();
  ptIds->InsertNextId(this->GetSourceNode(cellId));
  ptIds->InsertNextId(this->GetTargetNode(cellId));
}

void vtkAbstractGraph::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  cellIds->Reset();
  vtkGraphIdList* graphIds = vtkGraphIdList::New();
  this->GetIncidentArcs(ptId, graphIds);
  for (vtkIdType i = 0; i < graphIds->GetNumberOfIds(); i++)
    {
    cellIds->InsertNextId(graphIds->GetId(i));
    }
}


