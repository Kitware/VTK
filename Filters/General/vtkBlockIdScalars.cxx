/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlockIdScalars.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkBlockIdScalars);
//----------------------------------------------------------------------------
vtkBlockIdScalars::vtkBlockIdScalars()
{
}

//----------------------------------------------------------------------------
vtkBlockIdScalars::~vtkBlockIdScalars()
{
}

//----------------------------------------------------------------------------
// Map ids into attribute data
int vtkBlockIdScalars::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet *input = vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input) 
    {
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) 
    {
    return 0;
    }

  unsigned int numBlocks = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->TraverseSubTreeOff();
  iter->VisitOnlyLeavesOff();

  int blockIdx = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem(), blockIdx++)
    {
    vtkDataObject* dObj = iter->GetCurrentDataObject();
    if (dObj)
      {
      vtkDataObject* block = this->ColorBlock(dObj, blockIdx);
      if (block)
        {
        output->SetDataSet(iter, block);
        block->Delete();
        }
      }
    }

  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkBlockIdScalars::ColorBlock(
  vtkDataObject* input, int group)
{
  vtkDataObject* output = 0;
  if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* mbInput = vtkCompositeDataSet::SafeDownCast(input);

    output = input->NewInstance();
    vtkCompositeDataSet* mbOutput =
      vtkCompositeDataSet::SafeDownCast(output);
    mbOutput->CopyStructure(mbInput);

    vtkCompositeDataIterator* inIter = mbInput->NewIterator();
    inIter->VisitOnlyLeavesOn();
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); 
      inIter->GoToNextItem())
      {
      vtkDataObject* src = inIter->GetCurrentDataObject();
      vtkDataObject* dest = 0;
      if (src)
        {
        dest = this->ColorBlock(src, group);
        }
      mbOutput->SetDataSet(inIter, dest);
      }
    }
  else
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    if (ds)
      {
      output = ds->NewInstance();
      output->ShallowCopy(ds);
      vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
      vtkIdType numCells = dsOutput->GetNumberOfCells();
      vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
      cArray->SetNumberOfTuples(numCells);
      for (vtkIdType cellIdx=0; cellIdx<numCells; cellIdx++)
        {
        cArray->SetValue(cellIdx, group);
        }
      cArray->SetName("BlockIdScalars");
      dsOutput->GetCellData()->AddArray(cArray);
      cArray->Delete();
      }
    }
  return output;
}

//----------------------------------------------------------------------------
void vtkBlockIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

