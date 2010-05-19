/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRotationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRotationFilter.h"

#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMath.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkRotationFilter);

//---------------------------------------------------------------------------
vtkRotationFilter::vtkRotationFilter()
{
  this->Axis = 2;
  this->CopyInput = 0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0;
  this->NumberOfCopies = 0;
  this->Angle = 0;
}

//---------------------------------------------------------------------------
vtkRotationFilter::~vtkRotationFilter()
{
}

//---------------------------------------------------------------------------
void vtkRotationFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Axis: " << this->Axis << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
  os << indent << "Center: (" << this->Center[0] << "," << this->Center[1] 
               << "," << this->Center[2] << ")" << endl;
  os << indent << "NumberOfCopies: " << this->NumberOfCopies << endl;
  os << indent << "Angle: " << this->Angle << endl;
}

//---------------------------------------------------------------------------
int vtkRotationFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType i;
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();

  if (!this->GetNumberOfCopies())
    {
    vtkErrorMacro("No number of copy set!");
    return 1;
    }

  double tuple[3];
  vtkPoints *outPoints;
  double point[3], center[3], negativCenter[3];
  int ptId, cellId, j, k;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdList *ptIds = vtkIdList::New();

  outPoints = vtkPoints::New();

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (this->CopyInput)
    {
    outPoints->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
    output->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
    }
  else
    {
    outPoints->Allocate( this->GetNumberOfCopies() * numPts);
    output->Allocate( this->GetNumberOfCopies() * numPts);
    }

  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  vtkDataArray *inPtVectors, *outPtVectors, *inPtNormals;
  vtkDataArray *inCellVectors, *outCellVectors, *inCellNormals;

  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inPtNormals = inPD->GetNormals();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();
  inCellNormals = inCD->GetNormals();

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
  vtkTransform *localTransform = vtkTransform::New();
  // Rotate points.
  // double angle = vtkMath::RadiansFromDegrees( this->GetAngle() );
  this->GetCenter(center);
  negativCenter[0] = -center[0];
  negativCenter[1] = -center[1];
  negativCenter[2] = -center[2];

  for (k = 0; k < this->GetNumberOfCopies(); k++)
   {
   localTransform->Identity();
   localTransform->Translate(center);
   switch (this->Axis)
    {
     case USE_X:
        localTransform->RotateX((k+1)*this->GetAngle());
     break;

     case USE_Y:
        localTransform->RotateY((k+1)*this->GetAngle());
     break;

     case USE_Z:
        localTransform->RotateZ((k+1)*this->GetAngle());
     break;
     }
   localTransform->Translate(negativCenter);
   for (i = 0; i < numPts; i++)
    {
    input->GetPoint(i, point);
    localTransform->TransformPoint(point, point);
    ptId = outPoints->InsertNextPoint(point);
    outPD->CopyData(inPD, i, ptId);
    if (inPtVectors)
      {
      inPtVectors->GetTuple(i, tuple);
      outPtVectors->SetTuple(ptId, tuple);
      }
    if (inPtNormals)
      {
      //inPtNormals->GetTuple(i, tuple);
      //outPtNormals->SetTuple(ptId, tuple);
      }
    }
   }

  localTransform->Delete();

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

  // Generate rotated cells.
  for (k = 0; k < this->GetNumberOfCopies(); k++)  
    {
    for (i = 0; i < numCells; i++)
      {
       input->GetCellPoints(i, ptIds);
       input->GetCell(i, cell);
       numCellPts = cell->GetNumberOfPoints();
       cellType = cell->GetCellType();
       cellPts = cell->GetPointIds();
      // Triangle strips with even number of triangles have
      // to be handled specially. A degenerate triangle is
      // introduce to flip all the triangles properly.
      if (cellType == VTK_TRIANGLE_STRIP && numCellPts % 2 == 0)
        {
        vtkErrorMacro(<< "Triangles with bad points");
        return 0;
        }
      else
        {
        vtkDebugMacro(<< "celltype " << cellType << " numCellPts " << numCellPts);
        newCellPts = new vtkIdType[numCellPts];
        //for (j = numCellPts-1; j >= 0; j--)
        for (j = 0; j < numCellPts; j++)
          {
          //newCellPts[numCellPts-1-j] = cellPts->GetId(j) + numPts*k;
          newCellPts[j] = cellPts->GetId(j) + numPts*k;
           if (this->CopyInput)
            {
             //newCellPts[numCellPts-1-j] += numPts;
             newCellPts[j] += numPts;
            }
          }
        }
      cellId = output->InsertNextCell(cellType, numCellPts, newCellPts);
      delete [] newCellPts;
      outCD->CopyData(inCD, i, cellId);
      if (inCellVectors)
        {
        inCellVectors->GetTuple(i, tuple);
        outCellVectors->SetTuple(cellId, tuple);
        }
      if (inCellNormals)
        {
        //inCellNormals->GetTuple(i, tuple);
        //outCellNormals->SetTuple(cellId, tuple);
        }
      }
    }

  cell->Delete();
  ptIds->Delete();
  output->SetPoints(outPoints);
  outPoints->Delete();
  output->CheckAttributes();

  return 1;
}

int vtkRotationFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
