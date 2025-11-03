// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBlockIdScalars.h"

#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBlockIdScalars);

//------------------------------------------------------------------------------
namespace
{
void ColorBlock(vtkDataObject* block, int group)
{
  vtkDataSet* ds = vtkDataSet::SafeDownCast(block);
  vtkDataObjectTree* tree = vtkDataObjectTree::SafeDownCast(block);
  if (ds)
  {
    vtkNew<vtkConstantArray<unsigned char>> blockIdArray;
    blockIdArray->ConstructBackend(group);
    blockIdArray->SetNumberOfComponents(1);
    blockIdArray->SetNumberOfTuples(ds->GetNumberOfCells());
    blockIdArray->SetName("BlockIdScalars");
    ds->GetCellData()->AddArray(blockIdArray);
  }
  else if (tree)
  {
    auto localIter = vtkSmartPointer<vtkDataObjectTreeIterator>::Take(tree->NewTreeIterator());
    localIter->TraverseSubTreeOn();
    localIter->VisitOnlyLeavesOn();
    for (localIter->InitTraversal(); !localIter->IsDoneWithTraversal(); localIter->GoToNextItem())
    {
      ::ColorBlock(localIter->GetCurrentDataObject(), group);
    }
  }
}
}

//------------------------------------------------------------------------------
vtkBlockIdScalars::vtkBlockIdScalars() = default;

//------------------------------------------------------------------------------
vtkBlockIdScalars::~vtkBlockIdScalars() = default;

//------------------------------------------------------------------------------
int vtkBlockIdScalars::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}

//------------------------------------------------------------------------------
int vtkBlockIdScalars::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Recover input and outputs
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObjectTree* input =
    vtkDataObjectTree::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObjectTree* output =
    vtkDataObjectTree::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  // ShallowCopy input into output
  output->ShallowCopy(input);

  // Traverse tree as specified
  auto iter = vtkSmartPointer<vtkDataObjectTreeIterator>::Take(output->NewTreeIterator());
  iter->SetTraverseSubTree(this->TraverseSubTree);
  iter->SetVisitOnlyLeaves(this->VisitOnlyLeaves);
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
      // Color each block
      ::ColorBlock(dObj, blockIdx);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkBlockIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TraverseSubTree: " << this->TraverseSubTree << endl;
  os << indent << "VisitOnlyLeaves: " << this->VisitOnlyLeaves << endl;
}
VTK_ABI_NAMESPACE_END
