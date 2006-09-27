/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReflectionFilter.h"

#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkReflectionFilter, "1.17");
vtkStandardNewMacro(vtkReflectionFilter);

//---------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = USE_X_MIN;
  this->Center = 0.0;
  this->CopyInput = 1;
}

//---------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter()
{
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::FlipVector(double tuple[3], int mirrorDir[3])
{
  for(int j=0; j<3; j++)
    {
    tuple[j] *= mirrorDir[j];
    }
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType i;
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  double bounds[6];
  double tuple[3];
  vtkPoints *outPoints;
  double point[3];
  double constant[3] = {0.0, 0.0, 0.0};
  int mirrorDir[3] = { 1, 1, 1};
  int ptId, cellId, j;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdList *ptIds = vtkIdList::New();

  input->GetBounds(bounds);
  outPoints = vtkPoints::New();

  if (this->CopyInput)
    {
    outPoints->Allocate(2* numPts);
    output->Allocate(numCells * 2);
    }
  else
    {
    outPoints->Allocate(numPts);
    output->Allocate(numCells);
    }
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
  if (this->CopyInput)
    {
    for (i = 0; i < numPts; i++)
      {
      input->GetPoint(i, point);
      ptId = outPoints->InsertNextPoint(point);
      outPD->CopyData(inPD, i, ptId);
      }
    }
  
  // Copy reflected points.
  switch (this->Plane)
    {
    case USE_X_MIN:
      constant[0] = 2*bounds[0];
      mirrorDir[0] = -1;
      break;
    case USE_X_MAX:
      constant[0] = 2*bounds[1];
      mirrorDir[0] = -1;
      break;
    case USE_X:
      constant[0] = 2*this->Center;
      mirrorDir[0] = -1;
      break;
    case USE_Y_MIN:
      constant[1] = 2*bounds[2];
      mirrorDir[1] = -1;
      break;
    case USE_Y_MAX:
      constant[1] = 2*bounds[3];
      mirrorDir[1] = -1;
      break;
    case USE_Y:
      constant[1] = 2*this->Center;
      mirrorDir[1] = -1;
      break;
    case USE_Z_MIN:
      constant[2] = 2*bounds[4];
      mirrorDir[2] = -1;
      break;
    case USE_Z_MAX:
      constant[2] = 2*bounds[5];
      mirrorDir[2] = -1;
      break;
    case USE_Z:
      constant[2] = 2*this->Center;
      mirrorDir[2] = -1;
      break;
    }

  for (i = 0; i < numPts; i++)
    {
    input->GetPoint(i, point);
    ptId =
      outPoints->InsertNextPoint( mirrorDir[0]*point[0] + constant[0], 
                                  mirrorDir[1]*point[1] + constant[1], 
                                  mirrorDir[2]*point[2] + constant[2] );
    outPD->CopyData(inPD, i, ptId);
    if (inPtVectors)
      {
      inPtVectors->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outPtVectors->SetTuple(ptId, tuple);
      }
    if (inPtNormals)
      {
      inPtNormals->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outPtNormals->SetTuple(ptId, tuple);
      }
    }

  
  int numCellPts,  cellType;
  vtkIdType *newCellPts;
  vtkIdList *cellPts;
  
  // Copy original cells.
  if (this->CopyInput)
    {
    for (i = 0; i < numCells; i++)
      {
      input->GetCellPoints(i, ptIds);
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, i);
      }
    }
  
  // Generate reflected cells.
  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    numCellPts = cell->GetNumberOfPoints();
    cellType = cell->GetCellType();
    cellPts = cell->GetPointIds();
    // Triangle strips with even number of triangles have
    // to be handled specially. A degenerate triangle is
    // introduce to flip all the triangles properly.
    if (cellType == VTK_TRIANGLE_STRIP && numCellPts % 2 == 0)
      {
      numCellPts++;  
      newCellPts = new vtkIdType[numCellPts];
      newCellPts[0] = cellPts->GetId(0);
      newCellPts[1] = cellPts->GetId(2);
      newCellPts[2] = cellPts->GetId(1);
      newCellPts[3] = cellPts->GetId(2);
      for (j = 4; j < numCellPts; j++)
        {
        newCellPts[j] = cellPts->GetId(j-1);
        if (this->CopyInput)
          {
          newCellPts[j] += numPts;
          }
        }
      }
    else
      {
      newCellPts = new vtkIdType[numCellPts];
      for (j = numCellPts-1; j >= 0; j--)
        {
        newCellPts[numCellPts-1-j] = cellPts->GetId(j);
        if (this->CopyInput)
          {
          newCellPts[numCellPts-1-j] += numPts;
          }
        }
      }
    cellId = output->InsertNextCell(cellType, numCellPts, newCellPts);
    delete [] newCellPts;
    outCD->CopyData(inCD, i, cellId);
    if (inCellVectors)
      {
      inCellVectors->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outCellVectors->SetTuple(cellId, tuple);
      }
    if (inCellNormals)
      {
      inCellNormals->GetTuple(i, tuple);
      this->FlipVector(tuple, mirrorDir);
      outCellNormals->SetTuple(cellId, tuple);
      }
    }
  
  cell->Delete();
  ptIds->Delete();
  output->SetPoints(outPoints);
  outPoints->Delete();
  output->CheckAttributes();

  return 1;
}

//---------------------------------------------------------------------------
int vtkReflectionFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
}
