/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineSource.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkOutlineSource);

//----------------------------------------------------------------------------
vtkOutlineSource::vtkOutlineSource()
{
  this->BoxType = VTK_BOX_TYPE_AXIS_ALIGNED;

  this->GenerateFaces = 0;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  for (int i=0; i<3; i++)
    {
    this->Bounds[2*i] = -1.0;
    this->Bounds[2*i+1] = 1.0;
    }

  // Sensible initial values
  this->Corners[0] = 0.0;
  this->Corners[1] = 0.0;
  this->Corners[2] = 0.0;
  this->Corners[3] = 1.0;
  this->Corners[4] = 0.0;
  this->Corners[5] = 0.0;
  this->Corners[6] = 0.0;
  this->Corners[7] = 1.0;
  this->Corners[8] = 0.0;
  this->Corners[9] = 1.0;
  this->Corners[10] = 1.0;
  this->Corners[11] = 0.0;
  this->Corners[12] = 0.0;
  this->Corners[13] = 0.0;
  this->Corners[14] = 1.0;
  this->Corners[15] = 1.0;
  this->Corners[16] = 0.0;
  this->Corners[17] = 1.0;
  this->Corners[18] = 0.0;
  this->Corners[19] = 1.0;
  this->Corners[20] = 1.0;
  this->Corners[21] = 1.0;
  this->Corners[22] = 1.0;
  this->Corners[23] = 1.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkOutlineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double bounds[6];
  double x[3];
  vtkIdType pts[4];
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkCellArray *newPolys = 0;

  //
  // Initialize
  //
  for (int i = 0; i < 6; i+=2)
    {
    int j = i+1;
    bounds[i] = this->Bounds[i];
    bounds[j] = this->Bounds[j];
    if (bounds[i] > bounds[j])
      {
      double tmp = bounds[i];
      bounds[i] = bounds[j];
      bounds[j] = tmp;
      }
    }

  //
  // Allocate storage and create outline
  //
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

  newPts->Allocate(8);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(12,2));

  if (this->GenerateFaces)
    {
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(6,4));
    }

  if (this->BoxType==VTK_BOX_TYPE_AXIS_ALIGNED)
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    newPts->InsertPoint(0,x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    newPts->InsertPoint(1,x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    newPts->InsertPoint(2,x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    newPts->InsertPoint(3,x);
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    newPts->InsertPoint(4,x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    newPts->InsertPoint(5,x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    newPts->InsertPoint(6,x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    newPts->InsertPoint(7,x);
    }
  else //VTK_BOX_TYPE_ORIENTED
    {
    newPts->InsertPoint(0, &Corners[0]);
    newPts->InsertPoint(1, &Corners[3]);
    newPts->InsertPoint(2, &Corners[6]);
    newPts->InsertPoint(3, &Corners[9]);
    newPts->InsertPoint(4, &Corners[12]);
    newPts->InsertPoint(5, &Corners[15]);
    newPts->InsertPoint(6, &Corners[18]);
    newPts->InsertPoint(7, &Corners[21]);
    }

  pts[0] = 0; pts[1] = 1;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 6; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 2;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 5; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 4;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 3; pts[1] = 7;
  newLines->InsertNextCell(2,pts);

  if (newPolys)
    {
    pts[0] = 1; pts[1] = 0; pts[2] = 2; pts[3] = 3;
    newPolys->InsertNextCell(4,pts);
    pts[0] = 0; pts[1] = 1; pts[2] = 5; pts[3] = 4;
    newPolys->InsertNextCell(4,pts);
    pts[0] = 2; pts[1] = 0; pts[2] = 4; pts[3] = 6;
    newPolys->InsertNextCell(4,pts);
    pts[0] = 3; pts[1] = 2; pts[2] = 6; pts[3] = 7;
    newPolys->InsertNextCell(4,pts);
    pts[0] = 1; pts[1] = 3; pts[2] = 7; pts[3] = 5;
    newPolys->InsertNextCell(4,pts);
    pts[0] = 7; pts[1] = 6; pts[2] = 4; pts[3] = 5;
    newPolys->InsertNextCell(4,pts);
    }

  // Update selves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if (newPolys)
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkOutlineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Faces: "
     << (this->GenerateFaces ? "On\n" : "Off\n");

  os << indent << "Box Type: ";
  if ( this->BoxType == VTK_BOX_TYPE_AXIS_ALIGNED )
    {
    os << "Axis Aligned\n";
    os << indent << "Bounds: "
       << "(" << this->Bounds[0] << ", " << this->Bounds[1] << ") "
       << "(" << this->Bounds[2] << ", " << this->Bounds[3] << ") "
       << "(" << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";
    }
  else
    {
    os << "Corners: (\n";
    for (int i=0; i<8; i++)
      {
      os << "\t" << this->Corners[3*i] << ", "
         << this->Corners[3*i+1] << ", "
         << this->Corners[3*i+2] << "\n";
      }
    os << ")\n";
    }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}
