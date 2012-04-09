/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessIdScalars.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkProcessIdScalars);

vtkCxxSetObjectMacro(vtkProcessIdScalars,Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkProcessIdScalars::vtkProcessIdScalars()
{
  this->CellScalarsFlag = 0;
  this->RandomMode = 0;

  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
    {
    this->Controller->Register(this);
    }
}

//----------------------------------------------------------------------------
vtkProcessIdScalars::~vtkProcessIdScalars()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = 0;
    }
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkProcessIdScalars::RequestData(
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
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *pieceColors;
  vtkIdType num;

  if (this->CellScalarsFlag)
    {
    num = input->GetNumberOfCells();
    }
  else
    {
    num = input->GetNumberOfPoints();
    }

  int piece = (this->Controller?this->Controller->GetLocalProcessId():0);

  if (this->RandomMode)
    {
    pieceColors = this->MakeRandomScalars(piece, num);
    }
  else
    {
    pieceColors = this->MakeProcessIdScalars(piece, num);
    }

  output->ShallowCopy(input);
  pieceColors->SetName("ProcessId");
  if (this->CellScalarsFlag)
    {
    output->GetCellData()->AddArray(pieceColors);
    output->GetCellData()->SetActiveScalars(pieceColors->GetName());
    }
  else
    {
    output->GetPointData()->AddArray(pieceColors);
    output->GetPointData()->SetActiveScalars(pieceColors->GetName());
    }

  pieceColors->Delete();

  return 1;
}

//----------------------------------------------------------------------------
vtkIntArray *vtkProcessIdScalars::MakeProcessIdScalars(int piece, vtkIdType num)
{
  vtkIdType i;
  vtkIntArray *pieceColors = NULL;

  pieceColors = vtkIntArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (i = 0; i < num; ++i)
    {
    pieceColors->SetValue(i, piece);
    }

  return pieceColors;
}

//----------------------------------------------------------------------------
vtkFloatArray *vtkProcessIdScalars::MakeRandomScalars(int piece, vtkIdType num)
{
  vtkIdType i;
  vtkFloatArray *pieceColors = NULL;
  float randomValue;

  vtkMath::RandomSeed(piece);
  randomValue = vtkMath::Random();

  pieceColors = vtkFloatArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (i = 0; i < num; ++i)
    {
    pieceColors->SetValue(i, randomValue);
    }

  return pieceColors;
}

//----------------------------------------------------------------------------
void vtkProcessIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RandomMode: " << this->RandomMode << endl;
  if (this->CellScalarsFlag)
    {
    os << indent << "ScalarMode: CellData\n";
    }
  else
    {
    os << indent << "ScalarMode: PointData\n";
    }

  os << indent << "Controller: ";
  if (this->Controller)
    {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
