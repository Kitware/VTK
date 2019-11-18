/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSelector.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExpandMarkedElements.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRDataIterator.h"

//----------------------------------------------------------------------------
vtkSelector::vtkSelector()
  : InsidednessArrayName()
{
}

//----------------------------------------------------------------------------
vtkSelector::~vtkSelector() {}

//----------------------------------------------------------------------------
void vtkSelector::Initialize(vtkSelectionNode* node)
{
  this->Node = node;
}

//--------------------------------------------------------------------------
void vtkSelector::ProcessBlock(
  vtkDataObject* inputBlock, vtkDataObject* outputBlock, bool forceFalse)
{
  assert(vtkCompositeDataSet::SafeDownCast(inputBlock) == nullptr &&
    vtkCompositeDataSet::SafeDownCast(outputBlock) == nullptr);

  int association =
    vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());

  const vtkIdType numElements = inputBlock->GetNumberOfElements(association);
  auto insidednessArray = this->CreateInsidednessArray(numElements);
  if (forceFalse || !this->ComputeSelectedElements(inputBlock, insidednessArray))
  {
    insidednessArray->FillValue(0);
  }

  // If selecting cells containing points, we need to map the selected points
  // to selected cells.
  auto selectionProperties = this->Node->GetProperties();
  if (association == vtkDataObject::POINT &&
    selectionProperties->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
    selectionProperties->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
  {
    // convert point insidednessArray to cell-based insidednessArray
    insidednessArray = this->ComputeCellsContainingSelectedPoints(inputBlock, insidednessArray);
    association = vtkDataObject::CELL;
  }

  auto dsa = outputBlock->GetAttributes(association);
  if (dsa && insidednessArray)
  {
    dsa->AddArray(insidednessArray);
  }
}

//--------------------------------------------------------------------------
void vtkSelector::Execute(vtkDataObject* input, vtkDataObject* output)
{
  if (vtkCompositeDataSet::SafeDownCast(input))
  {
    assert(vtkCompositeDataSet::SafeDownCast(output) != nullptr);
    auto inputDOT = vtkDataObjectTree::SafeDownCast(input);
    auto outputDOT = vtkDataObjectTree::SafeDownCast(output);
    if (inputDOT && outputDOT)
    {
      this->ProcessDataObjectTree(inputDOT, outputDOT, this->GetBlockSelection(0), 0);
    }
    else
    {
      auto inputAMR = vtkUniformGridAMR::SafeDownCast(input);
      auto outputCD = vtkCompositeDataSet::SafeDownCast(output);
      if (inputAMR && outputCD)
      {
        this->ProcessAMR(inputAMR, outputCD);
      }
    }
  }
  else
  {
    this->ProcessBlock(input, output, false);
  }

  // Expand layers, if requested.
  auto selectionProperties = this->Node->GetProperties();
  if (selectionProperties->Has(vtkSelectionNode::CONNECTED_LAYERS()))
  {
    int association =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());
    // If selecting cells containing points, we need to map the selected points
    // to selected cells.
    if (association == vtkDataObject::POINT &&
      selectionProperties->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
      selectionProperties->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
    {
      association = vtkDataObject::CELL;
    }

    const int layers = selectionProperties->Get(vtkSelectionNode::CONNECTED_LAYERS());
    if (layers >= 1 && (association == vtkDataObject::POINT || association == vtkDataObject::CELL))
    {
      vtkNew<vtkExpandMarkedElements> expander;
      expander->SetInputArrayToProcess(0, 0, 0, association, this->InsidednessArrayName.c_str());
      expander->SetNumberOfLayers(layers);
      expander->SetInputDataObject(output);
      expander->Update();
      output->ShallowCopy(expander->GetOutputDataObject(0));
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelector::ProcessDataObjectTree(vtkDataObjectTree* input, vtkDataObjectTree* output,
  vtkSelector::SelectionMode mode, unsigned int compositeIndex)
{
  auto iter = input->NewTreeIterator();
  iter->TraverseSubTreeOff();
  iter->VisitOnlyLeavesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto inputDO = iter->GetCurrentDataObject();
    auto outputDO = output->GetDataSet(iter);
    if (inputDO && outputDO)
    {
      const auto currentIndex = compositeIndex + iter->GetCurrentFlatIndex();

      auto blockMode = this->GetBlockSelection(currentIndex);
      blockMode = (blockMode == INHERIT) ? mode : blockMode;

      auto inputDT = vtkDataObjectTree::SafeDownCast(inputDO);
      auto outputDT = vtkDataObjectTree::SafeDownCast(outputDO);
      if (inputDT && outputDT)
      {
        this->ProcessDataObjectTree(inputDT, outputDT, blockMode, currentIndex);
      }
      else
      {

        this->ProcessBlock(inputDO, outputDO, blockMode == EXCLUDE);
      }
    }
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkSelector::ProcessAMR(vtkUniformGridAMR* input, vtkCompositeDataSet* output)
{
  auto iter = vtkUniformGridAMRDataIterator::SafeDownCast(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto modeDSUsingCompositeIndex = this->GetBlockSelection(iter->GetCurrentFlatIndex());
    auto modeDSUsingAMRLevel =
      this->GetAMRBlockSelection(iter->GetCurrentLevel(), iter->GetCurrentIndex());
    auto realMode =
      modeDSUsingAMRLevel == INHERIT ? modeDSUsingCompositeIndex : modeDSUsingAMRLevel;
    realMode = (realMode == INHERIT) ? EXCLUDE : realMode;

    auto inputDS = iter->GetCurrentDataObject();
    auto outputDS = output->GetDataSet(iter);
    if (inputDS && outputDS)
    {
      this->ProcessBlock(inputDS, outputDS, realMode == EXCLUDE);
    }
  }

  iter->Delete();
}

//----------------------------------------------------------------------------
vtkSelector::SelectionMode vtkSelector::GetAMRBlockSelection(unsigned int level, unsigned int index)
{
  auto properties = this->Node->GetProperties();
  auto levelKey = vtkSelectionNode::HIERARCHICAL_LEVEL();
  auto indexKey = vtkSelectionNode::HIERARCHICAL_INDEX();
  const auto hasLevel = properties->Has(levelKey);
  const auto hasIndex = properties->Has(indexKey);
  if (!hasLevel && !hasIndex)
  {
    return INHERIT;
  }
  else if (hasLevel && !hasIndex)
  {
    return static_cast<unsigned int>(properties->Get(levelKey)) == level ? INCLUDE : EXCLUDE;
  }
  else if (hasIndex && !hasLevel)
  {
    return static_cast<unsigned int>(properties->Get(indexKey)) == index ? INCLUDE : EXCLUDE;
  }
  else
  {
    assert(hasIndex && hasLevel);
    return static_cast<unsigned int>(properties->Get(levelKey)) == level &&
        static_cast<unsigned int>(properties->Get(indexKey)) == index
      ? INCLUDE
      : EXCLUDE;
  }
}

//----------------------------------------------------------------------------
vtkSelector::SelectionMode vtkSelector::GetBlockSelection(unsigned int compositeIndex)
{
  auto properties = this->Node->GetProperties();
  auto key = vtkSelectionNode::COMPOSITE_INDEX();
  if (properties->Has(key))
  {
    if (static_cast<unsigned int>(properties->Get(key)) == compositeIndex)
    {
      return INCLUDE;
    }
    else
    {
      // this needs some explanation:
      // if `COMPOSITE_INDEX` is present, then the root node is to be treated as
      // excluded unless explicitly selected. This ensures that
      // we only "INCLUDE" the chosen subtree(s).
      // For all other nodes, we will simply return INHERIT, that way the state
      // from the parent is inherited unless overridden.
      return compositeIndex == 0 ? EXCLUDE : INHERIT;
    }
  }
  else
  {
    return INHERIT;
  }
}

//----------------------------------------------------------------------------
// Creates a new insidedness array with the given number of elements.
vtkSmartPointer<vtkSignedCharArray> vtkSelector::CreateInsidednessArray(vtkIdType numElems)
{
  auto darray = vtkSmartPointer<vtkSignedCharArray>::New();
  darray->SetName(this->InsidednessArrayName.c_str());
  darray->SetNumberOfComponents(1);
  darray->SetNumberOfTuples(numElems);
  return darray;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSignedCharArray> vtkSelector::ComputeCellsContainingSelectedPoints(
  vtkDataObject* data, vtkSignedCharArray* selectedPoints)
{
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(data);
  if (!dataset)
  {
    return nullptr;
  }

  const vtkIdType numCells = dataset->GetNumberOfCells();
  auto selectedCells = this->CreateInsidednessArray(numCells);

  if (numCells > 0)
  {
    // call once to make the GetCellPoints call thread safe.
    vtkNew<vtkIdList> cellPts;
    dataset->GetCellPoints(0, cellPts);
  }

  // run through cells and accept those with any point inside
  vtkSMPTools::For(0, numCells, [&](vtkIdType first, vtkIdType last) {
    vtkNew<vtkIdList> cellPts;
    for (vtkIdType cellId = first; cellId < last; ++cellId)
    {
      dataset->GetCellPoints(cellId, cellPts);
      const vtkIdType numCellPts = cellPts->GetNumberOfIds();
      signed char selectedPointFound = 0;
      for (vtkIdType i = 0; i < numCellPts; ++i)
      {
        vtkIdType ptId = cellPts->GetId(i);
        if (selectedPoints->GetValue(ptId) != 0)
        {
          selectedPointFound = 1;
          break;
        }
      }
      selectedCells->SetValue(cellId, selectedPointFound);
    }
  });

  return selectedCells;
}

//----------------------------------------------------------------------------
void vtkSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InsidednessArrayName: " << this->InsidednessArrayName << endl;
}
