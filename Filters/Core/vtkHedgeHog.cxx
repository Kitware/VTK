/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHedgeHog.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHedgeHog);

vtkHedgeHog::vtkHedgeHog()
{
  this->ScaleFactor = 1.0;
  this->VectorMode = VTK_USE_VECTOR;
}

int vtkHedgeHog::RequestData(
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
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts;
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkDataArray *inVectors;
  vtkDataArray *inNormals;
  vtkIdType ptId;
  int i;
  vtkIdType pts[2];
  vtkCellArray *newLines;
  double x[3], v[3];
  double newX[3];
  vtkPointData *outputPD = output->GetPointData();

  // Initialize
  //
  numPts = input->GetNumberOfPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No input data");
    return 1;
    }
  if ( !inVectors && this->VectorMode == VTK_USE_VECTOR)
    {
    vtkErrorMacro(<<"No vectors in input data");
    return 1;
    }

  inNormals = pd->GetNormals();
  if ( !inNormals && this->VectorMode == VTK_USE_NORMAL)
    {
    vtkErrorMacro(<<"No normals in input data");
    return 1;
    }
  outputPD->CopyAllocate(pd, 2*numPts);

  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(2*numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numPts,2));

  // Loop over all points, creating oriented line
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 10000) ) //abort/progress
      {
        this->UpdateProgress(static_cast<double>(ptId)/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    input->GetPoint(ptId, x);
    if (this->VectorMode == VTK_USE_VECTOR)
      {
      inVectors->GetTuple(ptId, v);
      }
    else
      {
      inNormals->GetTuple(ptId, v);
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }

    pts[0] = ptId;
    pts[1] = ptId + numPts;;

    newPts->SetPoint(pts[0], x);
    newPts->SetPoint(pts[1], newX);

    newLines->InsertNextCell(2,pts);

    outputPD->CopyData(pd,ptId,pts[0]);
    outputPD->CopyData(pd,ptId,pts[1]);
    }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

int vtkHedgeHog::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkHedgeHog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ?
                                       "Orient by vector\n" : "Orient by normal\n");
}
