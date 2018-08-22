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

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkUniformGridAMRDataIterator.h"

//----------------------------------------------------------------------------
vtkSelector::vtkSelector()
{
}

//----------------------------------------------------------------------------
vtkSelector::~vtkSelector()
{
}

//----------------------------------------------------------------------------
void vtkSelector::Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName)
{
  this->Node = node;
  this->InsidednessArrayName = insidednessArrayName;
}

//--------------------------------------------------------------------------
bool vtkSelector::ComputeSelectedElements(vtkDataObject* input, vtkDataObject* output)
{
  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputCD = vtkCompositeDataSet::SafeDownCast(output);
    return this->ComputeSelectedElementsForCompositeDataSet(inputCD, outputCD);
  }
  else
  {
    int association =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(
        this->Node->GetFieldType());
    vtkIdType numElements = input->GetNumberOfElements(association);
    auto insidednessArray = this->CreateInsidednessArray(numElements);
    insidednessArray->SetName(this->InsidednessArrayName.c_str());

    bool computed = this->ComputeSelectedElementsForBlock(input, insidednessArray,
      VTK_UNSIGNED_INT_MAX, VTK_UNSIGNED_INT_MAX, VTK_UNSIGNED_INT_MAX);
    if (!computed)
    {
      insidednessArray->Fill(0);
    }

    // If selecting cells containing points, we need to map the selected points
    // to selected cells.
    auto selectionProperties = this->Node->GetProperties();
    if (association == vtkDataObject::POINT &&
        selectionProperties->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
        selectionProperties->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
    {
      // insidednessArray going in is associated with points. Returned, it is
      // associated with cells.
      insidednessArray = this->ComputeCellsContainingSelectedPoints(input, insidednessArray);
      insidednessArray->SetName(this->InsidednessArrayName.c_str());
      association = vtkDataObject::CELL;
    }

    if (auto dsa = output->GetAttributes(association))
    {
      dsa->AddArray(insidednessArray);
    }
    return computed;
  }
}

//----------------------------------------------------------------------------
bool vtkSelector::ComputeSelectedElementsForCompositeDataSet(
  vtkCompositeDataSet* inputCD, vtkCompositeDataSet* outputCD)
{
  assert(inputCD != nullptr);
  assert(outputCD != nullptr);
  vtkSmartPointer<vtkCompositeDataIterator> inIter;
  inIter.TakeReference(inputCD->NewIterator());
  auto inIterAMR = vtkUniformGridAMRDataIterator::SafeDownCast(inIter);
  for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
  {
    vtkDataObject* inputBlock = inIter->GetCurrentDataObject();
    vtkDataObject* outputBlock = outputCD->GetDataSet(inIter);
    assert(inputBlock != nullptr);
    assert(outputBlock != nullptr);

    int association =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(
        this->Node->GetFieldType());
    vtkIdType numElements = inputBlock->GetNumberOfElements(association);
    auto insidednessArray = this->CreateInsidednessArray(numElements);
    insidednessArray->SetName(this->InsidednessArrayName.c_str());

    unsigned int compositeIndex = inIter->GetCurrentFlatIndex();
    unsigned int amrLevel = inIterAMR ? inIterAMR->GetCurrentLevel() : VTK_UNSIGNED_INT_MAX;
    unsigned int amrIndex = inIterAMR ? inIterAMR->GetCurrentIndex() : VTK_UNSIGNED_INT_MAX;
    if (this->SkipBlock(compositeIndex, amrLevel, amrIndex) ||
      !this->ComputeSelectedElementsForBlock(inputBlock, insidednessArray, compositeIndex, amrLevel, amrIndex))
    {
      insidednessArray->Fill(0);
    }

    // If selecting cells containing points, we need to map the selected points
    // to selected cells.
    auto selectionProperties = this->Node->GetProperties();
    if (association == vtkDataObject::POINT &&
        selectionProperties->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
        selectionProperties->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
    {
      // insidednessArray going in is associated with points. Returned, it is
      // associated with cells.
      insidednessArray = this->ComputeCellsContainingSelectedPoints(inputBlock, insidednessArray);
      insidednessArray->SetName(this->InsidednessArrayName.c_str());
      association = vtkDataObject::CELL;
    }
    auto fieldData = outputBlock->GetAttributes(association);
    if (fieldData)
    {
      fieldData->AddArray(insidednessArray);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSelector::SkipBlock(unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex)
{
  auto properties = this->Node->GetProperties();
  if (properties->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
    static_cast<unsigned int>(properties->Get(vtkSelectionNode::COMPOSITE_INDEX())) != compositeIndex)
  {
    return true;
  }

   if (properties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
    static_cast<unsigned int>(properties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())) != amrLevel)
  {
    return true;
  }

   if (properties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
    static_cast<unsigned int>(properties->Get(vtkSelectionNode::HIERARCHICAL_INDEX())) != amrIndex)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
// Creates a new insidedness array with the given number of elements.
vtkSmartPointer<vtkSignedCharArray> vtkSelector::CreateInsidednessArray(vtkIdType numElems)
{
  auto darray = vtkSmartPointer<vtkSignedCharArray>::New();
  darray->SetName("vtkInsidedness");
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
  vtkNew<vtkIdList> cellPts;
  // run through cells and accept those with any point inside
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    dataset->GetCellPoints(cellId, cellPts);
    vtkIdType numCellPts = cellPts->GetNumberOfIds();
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
  return selectedCells;
}

//----------------------------------------------------------------------------
void vtkSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
