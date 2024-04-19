// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBlockIdScalars.h"

#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBlockIdScalars);
//------------------------------------------------------------------------------
vtkBlockIdScalars::vtkBlockIdScalars() = default;

//------------------------------------------------------------------------------
vtkBlockIdScalars::~vtkBlockIdScalars() = default;

//------------------------------------------------------------------------------
// Map ids into attribute data
int vtkBlockIdScalars::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet* input =
    vtkMultiBlockDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  unsigned int numBlocks = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);

  vtkDataObjectTreeIterator* iter = input->NewTreeIterator();
  iter->TraverseSubTreeOff();
  iter->VisitOnlyLeavesOff();

  int blockIdx = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), blockIdx++)
  {
    if (this->CheckAbort())
    {
      break;
    }
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

//------------------------------------------------------------------------------
vtkDataObject* vtkBlockIdScalars::ColorBlock(vtkDataObject* input, int group)
{
  vtkDataObject* output = nullptr;
  if (input->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* mbInput = vtkCompositeDataSet::SafeDownCast(input);

    output = input->NewInstance();
    vtkCompositeDataSet* mbOutput = vtkCompositeDataSet::SafeDownCast(output);
    mbOutput->CopyStructure(mbInput);

    vtkSmartPointer<vtkCompositeDataIterator> inIter =
      vtk::TakeSmartPointer(mbInput->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      vtkDataObject* src = inIter->GetCurrentDataObject();
      vtkDataObject* dest = nullptr;
      if (src)
      {
        dest = this->ColorBlock(src, group);
      }
      mbOutput->SetDataSet(inIter, dest);
      dest->Delete();
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

      vtkNew<vtkConstantArray<unsigned char>> blockIdArray;
      blockIdArray->ConstructBackend(group);
      blockIdArray->SetNumberOfComponents(1);
      blockIdArray->SetNumberOfTuples(numCells);
      blockIdArray->SetName("BlockIdScalars");
      dsOutput->GetCellData()->AddArray(blockIdArray);
    }
  }
  return output;
}

//------------------------------------------------------------------------------
void vtkBlockIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
