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

vtkCxxRevisionMacro(vtkRotationFilter, "1.3");
vtkStandardNewMacro(vtkRotationFilter);

//---------------------------------------------------------------------------
vtkRotationFilter::vtkRotationFilter()
{
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
  os << indent << "Center: " << this->Center << endl;
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

  double tuple[3];
  vtkPoints *outPoints;
  double point[3], center[3];
  int ptId, cellId, j;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdList *ptIds = vtkIdList::New();

  outPoints = vtkPoints::New();

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (!this->GetNumberOfCopies())
    {
    vtkErrorMacro("No number of copy set!");
    return 1;
    }

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

  // Rotate points.
  double angle = this->GetAngle()*vtkMath::DegreesToRadians();
  this->GetCenter(center);
  switch (this->Axis)
    {
    case USE_X:
      for (int k = 0; k < this->GetNumberOfCopies(); k++)
        {
         for (i = 0; i < numPts; i++)
           {
           input->GetPoint(i, point);
           ptId =
             outPoints->InsertNextPoint((point[0]-center[0]),
                                        (point[1]-center[1])*cos(angle*(1+k)) - (point[2]-center[2])*sin(angle*(1+k)),
                                        (point[1]-center[1])*sin(angle*(1+k)) + (point[2]-center[2])*cos(angle*(1+k)));
           outPD->CopyData(inPD, i, ptId);
           if (inPtVectors)
             {
             inPtVectors->GetTuple(i, tuple);
             outPtVectors->SetTuple(ptId, tuple);
             }
           if (inPtNormals)
             {
             inPtNormals->GetTuple(i, tuple);
            outPtNormals->SetTuple(ptId, tuple);
             }
           }
        }
    break;

    case USE_Y:
      for (int k = 0; k < this->GetNumberOfCopies(); k++)
        {
         for (i = 0; i < numPts; i++)
           {
           input->GetPoint(i, point);
           ptId =
             outPoints->InsertNextPoint((point[0]-center[0])*cos(angle*(1+k)) + (point[2]-center[2])*sin(angle*(1+k)),
                                        (point[1]-center[1]),
                                       -(point[0]-center[0])*sin(angle*(1+k)) + (point[2]-center[2])*cos(angle*(1+k)));
           outPD->CopyData(inPD, i, ptId);
           if (inPtVectors)
             {
             inPtVectors->GetTuple(i, tuple);
             outPtVectors->SetTuple(ptId, tuple);
             }
           if (inPtNormals)
             {
             inPtNormals->GetTuple(i, tuple);
             outPtNormals->SetTuple(ptId, tuple);
            }
           }
        }
    break;

    case USE_Z:
      for (int k = 0; k < this->GetNumberOfCopies(); k++)
        {
        for (i = 0; i < numPts; i++)
           {
          input->GetPoint(i, point);
          ptId =
            outPoints->InsertNextPoint( (point[0]-center[0])*cos(angle*(1+k)) - (point[1]-center[1])*sin(angle*(1+k)),
                                        (point[0]-center[0])*sin(angle*(1+k)) + (point[1]-center[1])*cos(angle*(1+k)),
                                        (point[2]-center[2]));
          outPD->CopyData(inPD, i, ptId);
          if (inPtVectors)
            {
            inPtVectors->GetTuple(i, tuple);
            outPtVectors->SetTuple(ptId, tuple);
            }
           if (inPtNormals)
             {
             inPtNormals->GetTuple(i, tuple);
             outPtNormals->SetTuple(ptId, tuple);
             }
           }
        }
      break;
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

  // Generate rotated cells.
  for (int k = 0; k < this->GetNumberOfCopies(); k++)
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
        numCellPts++;  
        newCellPts = new vtkIdType[numCellPts];
         newCellPts[0] = cellPts->GetId(0) + numPts;
        newCellPts[1] = cellPts->GetId(2) + numPts;
        newCellPts[2] = cellPts->GetId(1) + numPts;
        newCellPts[3] = cellPts->GetId(2) + numPts;
        for (j = 4; j < numCellPts; j++)
          {
          newCellPts[j] = cellPts->GetId(j-1) + numPts*k;
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
          newCellPts[numCellPts-1-j] = cellPts->GetId(j) + numPts*k;
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
        outCellVectors->SetTuple(cellId, tuple);
        }
      if (inCellNormals)
        {
        inCellNormals->GetTuple(i, tuple);
        outCellNormals->SetTuple(cellId, tuple);
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
