/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.cxx
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
#include "vtkPieceScalars.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkPieceScalars, "1.10");
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
void vtkPieceScalars::Execute()
{  
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
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
  
  if (this->RandomMode)
    {
    pieceColors = this->MakeRandomScalars(output->GetUpdatePiece(), num);
    }
  else
    {
    pieceColors = this->MakePieceScalars(output->GetUpdatePiece(), num);
    }
    
  output->ShallowCopy(input);
  pieceColors->SetName("Piece");  
  if (this->CellScalarsFlag)
    {
    output->GetCellData()->SetScalars(pieceColors);
    }
  else
    {
    output->GetPointData()->SetScalars(pieceColors);
    }
    
  pieceColors->Delete();
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
