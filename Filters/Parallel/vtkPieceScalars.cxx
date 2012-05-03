/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieceScalars.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPieceScalars);

//----------------------------------------------------------------------------
vtkPieceScalars::vtkPieceScalars()
{
  this->CellScalarsFlag = 0;
  this->RandomMode = 0;
}

//----------------------------------------------------------------------------
vtkPieceScalars::~vtkPieceScalars()
{
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkPieceScalars::RequestData(
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

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  if (this->RandomMode)
    {
    pieceColors = this->MakeRandomScalars(piece, num);
    }
  else
    {
    pieceColors = this->MakePieceScalars(piece, num);
    }

  output->ShallowCopy(input);
  pieceColors->SetName("Piece");
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
vtkIntArray *vtkPieceScalars::MakePieceScalars(int piece, vtkIdType num)
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
vtkFloatArray *vtkPieceScalars::MakeRandomScalars(int piece, vtkIdType num)
{
  vtkIdType i;
  vtkFloatArray *pieceColors = NULL;
  float randomValue;

  vtkMath::RandomSeed(piece);
  randomValue = static_cast<float>(vtkMath::Random());

  pieceColors = vtkFloatArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (i = 0; i < num; ++i)
    {
    pieceColors->SetValue(i, randomValue);
    }

  return pieceColors;
}

//----------------------------------------------------------------------------
void vtkPieceScalars::PrintSelf(ostream& os, vtkIndent indent)
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
}
