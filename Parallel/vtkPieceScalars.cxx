/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPieceScalars.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkPieceScalars* vtkPieceScalars::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPieceScalars");
  if(ret)
    {
    return (vtkPieceScalars*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPieceScalars;
}

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
  vtkIntArray *ia;

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
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);
  
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
