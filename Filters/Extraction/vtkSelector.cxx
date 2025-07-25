// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSelector.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExpandMarkedElements.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRIterator.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkSelector::vtkSelector() = default;

//------------------------------------------------------------------------------
vtkSelector::~vtkSelector() = default;

//------------------------------------------------------------------------------
void vtkSelector::Initialize(vtkSelectionNode* node)
{
  this->Node = node;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkSelector::Execute(vtkDataObject* input, vtkDataObject* output)
{
  if (auto cd = vtkCompositeDataSet::SafeDownCast(input))
  {
    assert(vtkCompositeDataSet::SafeDownCast(output) != nullptr);

    // Populate SubsetCompositeIds if selector expressions are provided in
    // vtkSelectionNode's properties.
    this->ProcessSelectors(cd);

    vtkDataObjectTree* outputDOT = vtkDataObjectTree::SafeDownCast(output);
    if (outputDOT)
    {
      auto inputAMR = vtkUniformGridAMR::SafeDownCast(input);
      auto inputDOT = vtkDataObjectTree::SafeDownCast(input);
      if (inputAMR)
      {
        this->ProcessAMR(inputAMR, outputDOT);
      }
      else if (inputDOT)
      {
        this->ProcessDataObjectTree(inputDOT, outputDOT, this->GetBlockSelection(0), 0);
      }
    }
  }
  else
  {
    this->ProcessBlock(input, output, false);
  }

  // handle expanding to connected elements.
  this->ExpandToConnectedElements(output);
}

//------------------------------------------------------------------------------
void vtkSelector::ExpandToConnectedElements(vtkDataObject* output)
{
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
    const bool removeSeed =
      selectionProperties->Has(vtkSelectionNode::CONNECTED_LAYERS_REMOVE_SEED())
      ? selectionProperties->Get(vtkSelectionNode::CONNECTED_LAYERS_REMOVE_SEED()) == 1
      : false;
    const bool removeIntermediateLayers =
      selectionProperties->Has(vtkSelectionNode::CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS())
      ? selectionProperties->Get(vtkSelectionNode::CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS()) ==
        1
      : false;
    if (layers >= 1 && (association == vtkDataObject::POINT || association == vtkDataObject::CELL))
    {
      vtkNew<vtkExpandMarkedElements> expander;
      expander->SetInputArrayToProcess(0, 0, 0, association, this->InsidednessArrayName.c_str());
      expander->SetNumberOfLayers(layers);
      expander->SetRemoveSeed(removeSeed);
      expander->SetRemoveIntermediateLayers(removeIntermediateLayers);
      expander->SetInputDataObject(output);
      expander->Update();
      output->ShallowCopy(expander->GetOutputDataObject(0));
    }
  }
}

//------------------------------------------------------------------------------
void vtkSelector::ProcessSelectors(vtkCompositeDataSet* input)
{
  this->SubsetCompositeIds.clear();

  auto properties = this->Node->GetProperties();
  if (properties->Has(vtkSelectionNode::ASSEMBLY_NAME()) &&
    properties->Has(vtkSelectionNode::SELECTORS()))
  {
    if (auto assembly = vtkDataAssemblyUtilities::GetDataAssembly(
          properties->Get(vtkSelectionNode::ASSEMBLY_NAME()), input))
    {
      std::vector<std::string> selectors(properties->Length(vtkSelectionNode::SELECTORS()));
      for (int cc = 0; cc < static_cast<int>(selectors.size()); ++cc)
      {
        selectors[cc] = properties->Get(vtkSelectionNode::SELECTORS(), cc);
      }

      auto cids = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
        selectors, assembly, vtkPartitionedDataSetCollection::SafeDownCast(input));
      this->SubsetCompositeIds.insert(cids.begin(), cids.end());
    }
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkSelector::ProcessAMR(vtkUniformGridAMR* input, vtkDataObjectTree* output)
{
  auto iter = vtkUniformGridAMRIterator::SafeDownCast(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto blockMode = this->GetBlockSelection(iter->GetCurrentFlatIndex(), false);
    auto inputDS = iter->GetCurrentDataObject();
    auto outputDS = output->GetDataSet(iter);
    if (inputDS && outputDS)
    {
      this->ProcessBlock(inputDS, outputDS, blockMode == EXCLUDE);
    }
  }

  iter->Delete();
}

//------------------------------------------------------------------------------
vtkSelector::SelectionMode vtkSelector::GetBlockSelection(
  unsigned int compositeIndex, bool isDataObjectTree)
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
      if (isDataObjectTree)
      {
        // this needs some explanation:
        // if `COMPOSITE_INDEX` is present, then the root node is to be treated as
        // excluded unless explicitly selected. This ensures that
        // we only "INCLUDE" the chosen subtree(s).
        // For all other nodes, we will simply return INHERIT, that way the state
        // from the parent is inherited unless overridden.
        return compositeIndex == 0 ? EXCLUDE : INHERIT;
      }
      else
      {
        return EXCLUDE;
      }
    }
  }
  else if (properties->Has(vtkSelectionNode::SELECTORS()) &&
    properties->Has(vtkSelectionNode::ASSEMBLY_NAME()))
  {
    if (this->SubsetCompositeIds.find(compositeIndex) != this->SubsetCompositeIds.end())
    {
      return INCLUDE;
    }
    else
    {
      if (isDataObjectTree)
      {
        // see earlier explanation for why this is done for root node.
        return compositeIndex == 0 ? EXCLUDE : INHERIT;
      }
      else
      {
        return EXCLUDE;
      }
    }
  }
  else
  {
    return INHERIT;
  }
}

//------------------------------------------------------------------------------
// Creates a new insidedness array with the given number of elements.
vtkSmartPointer<vtkSignedCharArray> vtkSelector::CreateInsidednessArray(vtkIdType numElems)
{
  auto darray = vtkSmartPointer<vtkSignedCharArray>::New();
  darray->SetName(this->InsidednessArrayName.c_str());
  darray->SetNumberOfComponents(1);
  darray->SetNumberOfTuples(numElems);
  return darray;
}

//------------------------------------------------------------------------------
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
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType first, vtkIdType last)
    {
      vtkNew<vtkIdList> cellPts;
      vtkIdType numCellPts;
      const vtkIdType* pts;
      signed char selectedPointFound;
      for (vtkIdType cellId = first; cellId < last; ++cellId)
      {
        dataset->GetCellPoints(cellId, numCellPts, pts, cellPts);
        selectedPointFound = 0;
        for (vtkIdType i = 0; i < numCellPts; ++i)
        {
          if (selectedPoints->GetValue(pts[i]) != 0)
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

//------------------------------------------------------------------------------
void vtkSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InsidednessArrayName: " << this->InsidednessArrayName << endl;
}
VTK_ABI_NAMESPACE_END
