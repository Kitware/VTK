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

vtkCxxRevisionMacro(vtkReflectionFilter, "1.7");
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
  vtkIdType i;
  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  float bounds[6];
  float tuple[3];
  vtkPoints *outPoints;
  float point[3];
  float constant;
  int ptId, cellId;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdList *ptIds = vtkIdList::New();

  input->GetBounds(bounds);
  outPoints = vtkPoints::New();

  outPoints->Allocate(2* numPts);
  output->Allocate(numCells * 2);
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  vtkDataArray *inPtVectors, *outPtVectors, *inPtNormals, *outPtNormals;
  vtkDataArray *inCellVectors, *outCellVectors, *inCellNormals;
  vtkDataArray *outCellNormals;
  
  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inPtNormals = inPD->GetNormals();
  outPtNormals = outPD->GetNormals();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();
  inCellNormals = inCD->GetNormals();
  outCellNormals = outCD->GetNormals();
  
  // Copy first points.
  for (i = 0; i < numPts; i++)
    {
    input->GetPoint(i, point);
    ptId = outPoints->InsertNextPoint(point);
    outPD->CopyData(inPD, i, ptId);
    }

  // Copy reflected points.
  switch (this->Plane)
    {
    case VTK_USE_X_MIN:
      constant = 2*bounds[0];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[0] = -tuple[0];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[0] = -tuple[0];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_X_MAX:
      constant = 2*bounds[1];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[0] = -tuple[0];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[0] = -tuple[0];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Y_MIN:
      constant = 2*bounds[2];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[1] = -tuple[1];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[1] = -tuple[1];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Y_MAX:
      constant = 2*bounds[3];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[1] = -tuple[1];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[1] = -tuple[1];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Z_MIN:
      constant = 2*bounds[4];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[2] = -tuple[2];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[2] = -tuple[2];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    case VTK_USE_Z_MAX:
      constant = 2*bounds[5];
      for (i = 0; i < numPts; i++)
        {
        input->GetPoint(i, point);
        ptId =
          outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        outPD->CopyData(inPD, i, ptId);
        if (inPtVectors)
          {
          inPtVectors->GetTuple(i, tuple);
          tuple[2] = -tuple[2];
          outPtVectors->SetTuple(ptId, tuple);
          }
        if (inPtNormals)
          {
          inPtNormals->GetTuple(i, tuple);
          tuple[2] = -tuple[2];
          outPtNormals->SetTuple(ptId, tuple);
          }
        }
      break;
    }
  
  int numCellPts, j, cellType;
  vtkIdType *newCellPts;
  vtkIdList *cellPts;
  
  // Copy original cells.
  for (i = 0; i < numCells; i++)
    {
    input->GetCellPoints(i, ptIds);
    output->InsertNextCell(input->GetCellType(i), ptIds);
    outCD->CopyData(inCD, i, i);
    }

  // Generate reflected cells.
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
      inCellVectors->GetTuple(i, tuple);
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
      inCellNormals->GetTuple(i, tuple);
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
  ptIds->Delete();
  output->SetPoints(outPoints);
  outPoints->Delete();
  output->CheckAttributes();
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
}
