/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxes.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkAxes);


//----------------------------------------------------------------------------
// Construct with origin=(0,0,0) and scale factor=1.
vtkAxes::vtkAxes()
{
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->ScaleFactor = 1.0;

  this->Symmetric = 0;
  this->ComputeNormals = 1;

  this->SetNumberOfInputPorts(0);
}

int vtkAxes::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numPts=6, numLines=3;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkFloatArray *newScalars;
  vtkFloatArray *newNormals;
  double x[3], n[3];
  vtkIdType ptIds[2];

  vtkDebugMacro(<<"Creating x-y-z axes");

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts);
  newScalars->SetName("Axes");
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");

//
// Create axes
//
  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[0] = this->Origin[0] - this->ScaleFactor;
  }
  n[0] = 0.0; n[1] = 1.0; n[2] = 0.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0] + this->ScaleFactor;
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newLines->InsertNextCell(2,ptIds);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[1] = this->Origin[1] - this->ScaleFactor;
  }
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1] + this->ScaleFactor;
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2,ptIds);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[2] = this->Origin[2] - this->ScaleFactor;
  }
  n[0] = 1.0; n[1] = 0.0; n[2] = 0.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2] + this->ScaleFactor;
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2,ptIds);

  //
  // Update our output and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  if (this->ComputeNormals)
  {
    output->GetPointData()->SetNormals(newNormals);
  }
  newNormals->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// This source does not know how to generate pieces yet.
int vtkAxes::ComputeDivisionExtents(vtkDataObject *vtkNotUsed(output),
                                      int idx, int numDivisions)
{
  if (idx == 0 && numDivisions == 1)
  {
    // I will give you the whole thing
    return 1;
  }
  else
  {
    // I have nothing to give you for this piece.
    return 0;
  }
}


//----------------------------------------------------------------------------
void vtkAxes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Origin: (" << this->Origin[0] << ", "
               << this->Origin[1] << ", "
               << this->Origin[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Symmetric: " << this->Symmetric << "\n";
  os << indent << "ComputeNormals: " << this->ComputeNormals << "\n";
}
