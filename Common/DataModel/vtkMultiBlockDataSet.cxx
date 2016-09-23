/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataSet.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMultiBlockDataSet);
//----------------------------------------------------------------------------
vtkMultiBlockDataSet::vtkMultiBlockDataSet()
{
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet::~vtkMultiBlockDataSet()
{
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSet::GetData(vtkInformation* info)
{
  return
    info? vtkMultiBlockDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSet::GetData(vtkInformationVector* v,
                                                    int i)
{
  return vtkMultiBlockDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::SetNumberOfBlocks(unsigned int numBlocks)
{
  this->Superclass::SetNumberOfChildren(numBlocks);
}


//----------------------------------------------------------------------------
unsigned int vtkMultiBlockDataSet::GetNumberOfBlocks()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiBlockDataSet::GetBlock(unsigned int blockno)
{
  return this->Superclass::GetChild(blockno);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::SetBlock(unsigned int blockno, vtkDataObject* block)
{
  if (block && block->IsA("vtkCompositeDataSet") &&
    !block->IsA("vtkMultiBlockDataSet") && !block->IsA("vtkMultiPieceDataSet"))
  {
    vtkErrorMacro(<< block->GetClassName() << " cannot be added as a block.");
    return;
  }
  this->Superclass::SetChild(blockno, block);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::RemoveBlock(unsigned int blockno)
{
  this->Superclass::RemoveChild(blockno);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

