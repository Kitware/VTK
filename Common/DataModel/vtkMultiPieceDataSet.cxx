/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiPieceDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiPieceDataSet.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMultiPieceDataSet);
//----------------------------------------------------------------------------
vtkMultiPieceDataSet::vtkMultiPieceDataSet()
{
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet::~vtkMultiPieceDataSet()
{
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformation* info)
{
  return
    info? vtkMultiPieceDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformationVector* v,
                                                    int i)
{
  return vtkMultiPieceDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkMultiPieceDataSet::SetNumberOfPieces(unsigned int numPieces)
{
  this->Superclass::SetNumberOfChildren(numPieces);
}


//----------------------------------------------------------------------------
unsigned int vtkMultiPieceDataSet::GetNumberOfPieces()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
vtkDataSet* vtkMultiPieceDataSet::GetPiece(unsigned int blockno)
{
  return vtkDataSet::SafeDownCast(this->GetPieceAsDataObject(blockno));
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiPieceDataSet::GetPieceAsDataObject(unsigned int blockno)
{
  return this->Superclass::GetChild(blockno);
}

//----------------------------------------------------------------------------
void vtkMultiPieceDataSet::SetPiece(unsigned int blockno, vtkDataObject* block)
{
  if (block && block->IsA("vtkCompositeDataSet"))
    {
    vtkErrorMacro("Piece cannot be a vtkCompositeDataSet.");
    return;
    }

  this->Superclass::SetChild(blockno, block);
}

//----------------------------------------------------------------------------
void vtkMultiPieceDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

