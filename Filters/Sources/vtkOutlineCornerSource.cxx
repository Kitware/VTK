/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineCornerSource.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkOutlineCornerSource);

//----------------------------------------------------------------------------
vtkOutlineCornerSource::vtkOutlineCornerSource()
    : vtkOutlineSource()
{
  this->CornerFactor = 0.2;
  this->OutputPointsPrecision = vtkAlgorithm::SINGLE_PRECISION;
}

//----------------------------------------------------------------------------
int vtkOutlineCornerSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  double *bounds;
  double inner_bounds[6];

  int i, j, k;

  // Initialize
  double delta;

  bounds = this->Bounds;
  for (i = 0; i < 3; i++)
  {
    delta = (bounds[2*i + 1] - bounds[2*i]) * this->CornerFactor;
    inner_bounds[2*i] = bounds[2*i] + delta;
    inner_bounds[2*i + 1] = bounds[2*i + 1] - delta;
  }

  // Allocate storage and create outline
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  newPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPts->SetDataType(VTK_FLOAT);
  }

  newPts->Allocate(32);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(24,2));

  double x[3];
  vtkIdType pts[2];

  int pid = 0;

  // 32 points and 24 lines
  for (i = 0; i <= 1; i++)
  {
    for (j = 2; j <= 3; j++)
    {
      for (k = 4; k <= 5; k++)
      {
        pts[0] = pid;
        x[0] = bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
        newPts->InsertPoint(pid++, x);

        pts[1] = pid;
        x[0] = inner_bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
        newPts->InsertPoint(pid++, x);
        newLines->InsertNextCell(2,pts);

        pts[1] = pid;
        x[0] = bounds[i]; x[1] = inner_bounds[j]; x[2] = bounds[k];
        newPts->InsertPoint(pid++, x);
        newLines->InsertNextCell(2,pts);

        pts[1] = pid;
        x[0] = bounds[i]; x[1] = bounds[j]; x[2] = inner_bounds[k];
        newPts->InsertPoint(pid++, x);
        newLines->InsertNextCell(2,pts);
      }
    }
  }

  // Update selves and release memory
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkOutlineCornerSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}
