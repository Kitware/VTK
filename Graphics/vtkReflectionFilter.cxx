/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.cxx
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
#include "vtkReflectionFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkReflectionFilter, "1.6");
vtkStandardNewMacro(vtkReflectionFilter);

//---------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = VTK_USE_X_MIN;
}

//---------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter()
{
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);
  
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);
  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPoints *points = input->GetPoints();

  float bounds[6];
  input->GetBounds(bounds);
  
  vtkIdType i;

  vtkIdList *tmpIds = vtkIdList::New();
  for (i = 0; i < numCells; i++)
    {
    tmpIds->InsertNextId(i);
    }

  output->Allocate(input);
  output->CopyCells(input, tmpIds);
  tmpIds->Delete();

  vtkPoints *outPoints = output->GetPoints();

  float point[3];
  vtkGenericCell *cell = vtkGenericCell::New();

  float constant;
  int ptId, cellId;
  vtkDataArray *inPtVectors, *outPtVectors, *inPtNormals, *outPtNormals;
  vtkDataArray *inCellVectors, *outCellVectors, *inCellNormals;
  vtkDataArray *outCellNormals;
  float *tuple;
  
  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inPtNormals = inPD->GetNormals();
  outPtNormals = outPD->GetNormals();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();
  inCellNormals = inCD->GetNormals();
  outCellNormals = outCD->GetNormals();
  
  switch (this->Plane)
    {
    case VTK_USE_X_MIN:
      constant = 2*bounds[0];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[0] = -tuple[0];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[0] = -tuple[0];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_X_MAX:
      constant = 2*bounds[1];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[0] = -tuple[0];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[0] = -tuple[0];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Y_MIN:
      constant = 2*bounds[2];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[1] = -tuple[1];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[1] = -tuple[1];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Y_MAX:
      constant = 2*bounds[3];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[1] = -tuple[1];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[1] = -tuple[1];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Z_MIN:
      constant = 2*bounds[4];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[2] = -tuple[2];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[2] = -tuple[2];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Z_MAX:
      constant = 2*bounds[5];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          tuple = inPtVectors->GetTuple(i);
          tuple[2] = -tuple[2];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          tuple = inPtNormals->GetTuple(i);
          tuple[2] = -tuple[2];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    }
  
  int numCellPts, j, cellType;
  vtkIdType *newCellPts;
  vtkIdList *cellPts;
  
  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    numCellPts = cell->GetNumberOfPoints();
    cellType = cell->GetCellType();
    newCellPts = new vtkIdType[numCellPts];
    cellPts = cell->GetPointIds();
    for (j = numCellPts-1; j >= 0; j--)
      {
      newCellPts[numCellPts-1-j] = cellPts->GetId(j) + numPts;
      }
    cellId = output->InsertNextCell(cellType, numCellPts, newCellPts);
    outCD->CopyData(inCD, i, cellId);
    if (inCellVectors)
      {
      tuple = inCellVectors->GetTuple(i);
      switch (this->Plane)
        {
        case VTK_USE_X_MIN:
        case VTK_USE_X_MAX:
          tuple[0] = -tuple[0];
          break;
        case VTK_USE_Y_MIN:
        case VTK_USE_Y_MAX:
          tuple[1] = -tuple[1];
          break;
        case VTK_USE_Z_MIN:
        case VTK_USE_Z_MAX:
          tuple[2] = -tuple[2];
          break;
        }
      outCellVectors->SetTuple(cellId, tuple);
      }
    if (inCellNormals)
      {
      tuple = inCellNormals->GetTuple(i);
      switch (this->Plane)
        {
        case VTK_USE_X_MIN:
        case VTK_USE_X_MAX:
          tuple[0] = -tuple[0];
          break;
        case VTK_USE_Y_MIN:
        case VTK_USE_Y_MAX:
          tuple[1] = -tuple[1];
          break;
        case VTK_USE_Z_MIN:
        case VTK_USE_Z_MAX:
          tuple[2] = -tuple[2];
          break;
        }
      outCellNormals->SetTuple(cellId, tuple);
      }
    delete [] newCellPts;
    }
  
  cell->Delete();
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
}
