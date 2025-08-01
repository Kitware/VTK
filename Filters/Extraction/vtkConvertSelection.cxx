// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConvertSelection.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRIterator.h"
#include "vtkUnsignedIntArray.h"
#include "vtkValueSelector.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkConvertSelection, ArrayNames, vtkStringArray);
vtkCxxSetObjectMacro(vtkConvertSelection, SelectionExtractor, vtkExtractSelection);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkConvertSelection);

//------------------------------------------------------------------------------
vtkConvertSelection::vtkConvertSelection()
{
  this->SetNumberOfInputPorts(2);
  this->OutputType = vtkSelectionNode::INDICES;
  this->ArrayNames = nullptr;
  this->InputFieldType = -1;
  this->MatchAnyValues = false;
  this->AllowMissingArray = false;
  this->SelectionExtractor = nullptr;
}

//------------------------------------------------------------------------------
vtkConvertSelection::~vtkConvertSelection()
{
  this->SetArrayNames(nullptr);
  this->SetSelectionExtractor(nullptr);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::AddArrayName(const char* name)
{
  if (!this->ArrayNames)
  {
    this->ArrayNames = vtkStringArray::New();
  }
  this->ArrayNames->InsertNextValue(name);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::ClearArrayNames()
{
  if (this->ArrayNames)
  {
    this->ArrayNames->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkConvertSelection::SetArrayName(const char* name)
{
  if (!this->ArrayNames)
  {
    this->ArrayNames = vtkStringArray::New();
  }
  this->ArrayNames->Initialize();
  this->ArrayNames->InsertNextValue(name);
}

//------------------------------------------------------------------------------
const char* vtkConvertSelection::GetArrayName()
{
  if (this->ArrayNames && this->ArrayNames->GetNumberOfValues() > 0)
  {
    return this->ArrayNames->GetValue(0).c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::SelectTableFromTable(
  vtkTable* selTable, vtkTable* dataTable, vtkIdTypeArray* indices)
{
  std::set<vtkIdType> matching;
  vtkNew<vtkIdList> list;
  for (vtkIdType row = 0; row < selTable->GetNumberOfRows(); row++)
  {
    matching.clear();
    bool initialized = false;
    for (vtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
    {
      vtkAbstractArray* from = selTable->GetColumn(col);
      vtkAbstractArray* to = dataTable->GetColumnByName(from->GetName());
      if (to)
      {
        to->LookupValue(selTable->GetValue(row, col), list);
        if (!initialized)
        {
          matching.insert(list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds());
          initialized = true;
        }
        else
        {
          std::set<vtkIdType> intersection;
          std::sort(list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds());
          std::set_intersection(matching.begin(), matching.end(), list->GetPointer(0),
            list->GetPointer(0) + list->GetNumberOfIds(),
            std::inserter(intersection, intersection.begin()));
          matching = intersection;
        }
      }
    }
    for (const auto& match : matching)
    {
      indices->InsertNextValue(match);
    }
    if (row % 100 == 0)
    {
      double progress = 0.8 * row / selTable->GetNumberOfRows();
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::ConvertToIndexSelection(
  vtkSelectionNode* input, vtkDataSet* data, vtkSelectionNode* output)
{
  vtkNew<vtkSelection> tempInput;
  tempInput->AddNode(input);

  // Use the extraction filter to create an insidedness array.
  vtkExtractSelection* extract = this->SelectionExtractor;
  extract->PreserveTopologyOn();
  extract->SetInputData(0, data);
  extract->SetInputData(1, tempInput);
  extract->Update();
  vtkDataSet* const extracted = vtkDataSet::SafeDownCast(extract->GetOutput());

  output->SetContentType(vtkSelectionNode::INDICES);
  int type = input->GetFieldType();
  output->SetFieldType(type);
  vtkSignedCharArray* insidedness = nullptr;
  if (type == vtkSelectionNode::CELL)
  {
    insidedness = vtkArrayDownCast<vtkSignedCharArray>(
      extracted->GetCellData()->GetAbstractArray("vtkInsidedness"));
  }
  else if (type == vtkSelectionNode::POINT)
  {
    insidedness = vtkArrayDownCast<vtkSignedCharArray>(
      extracted->GetPointData()->GetAbstractArray("vtkInsidedness"));
  }
  else
  {
    vtkErrorMacro("Unknown field type");
    return 0;
  }

  if (!insidedness)
  {
    // Empty selection
    return 0;
  }

  // Convert the insidedness array into an index input.
  vtkNew<vtkIdTypeArray> indexArray;
  for (vtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
  {
    if (insidedness->GetValue(i) == 1)
    {
      indexArray->InsertNextValue(i);
    }
  }
  output->SetSelectionList(indexArray);
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::ConvertToBlockSelection(
  vtkSelection* input, vtkCompositeDataSet* data, vtkSelection* output)
{
  std::set<unsigned int> indices;
  int fieldType = -1;
  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    vtkSmartPointer<vtkSelectionNode> inputNode = input->GetNode(n);

    // if node has no items in the selection list, it's a clear indication that
    // nothing is selected and the node should simply be ignored.
    if (inputNode->GetSelectionList() == nullptr ||
      inputNode->GetSelectionList()->GetNumberOfTuples() == 0)
    {
      continue;
    }

    if (inputNode->GetContentType() == vtkSelectionNode::GLOBALIDS)
    {
      // global id selection does not have COMPOSITE_INDEX() key, so we convert
      // it to an index base selection, so that we can determine the composite
      // indices.
      vtkNew<vtkSelection> tempSel;
      tempSel->AddNode(inputNode);
      vtkSmartPointer<vtkSelection> tempOutput;
      tempOutput.TakeReference(vtkConvertSelection::ToIndexSelection(tempSel, data));
      inputNode = tempOutput->GetNode(0);
    }

    vtkInformation* properties = inputNode->GetProperties();
    if (properties->Has(vtkSelectionNode::CONTENT_TYPE()) &&
      properties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      indices.insert(
        static_cast<unsigned int>(properties->Get(vtkSelectionNode::COMPOSITE_INDEX())));
    }
    else if (properties->Has(vtkSelectionNode::CONTENT_TYPE()) &&
      properties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
      properties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) && data->IsA("vtkUniformGridAMR"))
    {
      // convert hierarchical index to composite index.
      vtkUniformGridAMR* hbox = vtkUniformGridAMR::SafeDownCast(data);
      indices.insert(hbox->GetAbsoluteBlockIndex(
        static_cast<unsigned int>(properties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())),
        static_cast<unsigned int>(properties->Get(vtkSelectionNode::HIERARCHICAL_INDEX()))));
    }

    // save field type. I am just picking the first one for now.
    fieldType = fieldType == -1 ? inputNode->GetFieldType() : fieldType;
  }

  if (indices.empty())
  {
    // nothing to convert, or converted to empty selection.
    return 1;
  }

  vtkNew<vtkSelectionNode> outputNode;
  outputNode->SetFieldType(fieldType);
  if (this->OutputType == vtkSelectionNode::BLOCKS)
  {
    std::vector<unsigned int> vIndices(indices.size());
    std::copy(indices.begin(), indices.end(), vIndices.begin());

    // get the composite ids from the selectors that correspond to the indices.
    // this is done to avoid selecting pieces/datasets from a partitioned/multi-piece dataset
    // and selecting only partitioned/multi-piece datasets, except if the parent of the index is
    // a multiblock
    auto hierarchy =
      vtkDataAssemblyUtilities::GetDataAssembly(vtkDataAssemblyUtilities::HierarchyName(), data);
    const auto selectorsCompositeIds =
      vtkDataAssemblyUtilities::GetSelectorsCompositeIdsForCompositeIds(vIndices, hierarchy);

    vtkNew<vtkUnsignedIntArray> selectionList;
    selectionList->SetNumberOfTuples(static_cast<vtkIdType>(selectorsCompositeIds.size()));
    vtkIdType cc = 0;
    for (const auto& id : selectorsCompositeIds)
    {
      selectionList->SetValue(cc++, id);
    }
    outputNode->SetContentType(vtkSelectionNode::BLOCKS);
    outputNode->SetSelectionList(selectionList);
  }
  else if (this->OutputType == vtkSelectionNode::BLOCK_SELECTORS)
  {
    // convert ids to selectors.
    std::vector<unsigned int> vIndices(indices.size());
    std::copy(indices.begin(), indices.end(), vIndices.begin());

    auto hierarchy =
      vtkDataAssemblyUtilities::GetDataAssembly(vtkDataAssemblyUtilities::HierarchyName(), data);
    const auto selectors =
      vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(vIndices, hierarchy);

    vtkNew<vtkStringArray> selectionList;
    selectionList->SetName(vtkDataAssemblyUtilities::HierarchyName());
    selectionList->SetNumberOfTuples(selectors.size());
    vtkIdType cc = 0;
    for (const auto& name : selectors)
    {
      selectionList->SetValue(cc++, name);
    }
    outputNode->SetContentType(vtkSelectionNode::BLOCK_SELECTORS);
    outputNode->SetSelectionList(selectionList);
  }

  outputNode->SetFieldType(fieldType);
  output->AddNode(outputNode);
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::ConvertCompositeDataSet(
  vtkSelection* input, vtkCompositeDataSet* data, vtkSelection* output)
{
  // If OutputType is BLOCKS or BLOCK_SELECTORS we just want to create a new
  // selection with the chosen block indices.
  if (this->OutputType == vtkSelectionNode::BLOCKS ||
    this->OutputType == vtkSelectionNode::BLOCK_SELECTORS)
  {
    return this->ConvertToBlockSelection(input, data, output);
  }

  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    vtkSelectionNode* inputNode = input->GetNode(n);

    // *  If input has no composite keys then it implies that it applies to all
    //    nodes in the data. If input has composite keys, output will have
    //    composite keys unless outputContentType == GLOBALIDS.
    //    If input does not have composite keys, then composite
    //    keys are only added for outputContentType == INDICES, FRUSTUM and
    //    PEDIGREEIDS.
    bool has_composite_key =
      inputNode->GetProperties()->Has(vtkSelectionNode::COMPOSITE_INDEX()) != 0;

    unsigned int composite_index = has_composite_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()))
      : 0;

    bool has_hierarchical_key =
      inputNode->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) != 0 &&
      inputNode->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) != 0;

    unsigned int hierarchical_level = has_hierarchical_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_LEVEL()))
      : 0;
    unsigned int hierarchical_index = has_hierarchical_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_INDEX()))
      : 0;

    if ((!has_composite_key && !has_hierarchical_key) &&
      (inputNode->GetContentType() == vtkSelectionNode::BLOCKS ||
        inputNode->GetContentType() == vtkSelectionNode::BLOCK_SELECTORS ||
        inputNode->GetContentType() == vtkSelectionNode::QUERY) &&
      this->OutputType == vtkSelectionNode::INDICES)
    {
      this->ConvertFromQueryAndBlockSelectionNodeCompositeDataSet(inputNode, data, output);
      continue;
    }

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(data->NewIterator());

    vtkUniformGridAMRIterator* hierIter = vtkUniformGridAMRIterator::SafeDownCast(iter);

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (has_hierarchical_key && hierIter &&
        (hierIter->GetCurrentLevel() != hierarchical_level ||
          hierIter->GetCurrentIndex() != hierarchical_index))
      {
        continue;
      }

      if (has_composite_key && iter->GetCurrentFlatIndex() != composite_index)
      {
        continue;
      }

      vtkNew<vtkSelection> outputNodes;
      vtkNew<vtkSelection> tempSel;
      tempSel->AddNode(inputNode);
      if (!this->Convert(tempSel, iter->GetCurrentDataObject(), outputNodes))
      {
        return 0;
      }

      for (unsigned int j = 0; j < outputNodes->GetNumberOfNodes(); ++j)
      {
        vtkSelectionNode* outputNode = outputNodes->GetNode(j);
        if ((has_hierarchical_key || has_composite_key ||
              this->OutputType == vtkSelectionNode::INDICES ||
              this->OutputType == vtkSelectionNode::PEDIGREEIDS ||
              this->OutputType == vtkSelectionNode::FRUSTUM) &&
          this->OutputType != vtkSelectionNode::GLOBALIDS)
        {
          outputNode->GetProperties()->Set(
            vtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());

          if (has_hierarchical_key && hierIter)
          {
            outputNode->GetProperties()->Set(
              vtkSelectionNode::HIERARCHICAL_LEVEL(), hierarchical_level);
            outputNode->GetProperties()->Set(
              vtkSelectionNode::HIERARCHICAL_INDEX(), hierarchical_index);
          }
        }
        output->Union(outputNode);
      } // for each output node
    }   // for each block
  }     // for each input selection node

  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::ConvertFromQueryAndBlockSelectionNodeCompositeDataSet(
  vtkSelectionNode* inputNode, vtkCompositeDataSet* data, vtkSelection* output)
{
  // QUERY/block/block_selectors selection types with composite data input need special handling.
  // The query/block/block_selectors can apply to a composite dataset, so we extract the selection
  // on the entire dataset here and convert it to an index selection.
  vtkNew<vtkSelection> tempSelection;
  tempSelection->AddNode(inputNode);
  vtkExtractSelection* extract = this->SelectionExtractor;
  extract->PreserveTopologyOn();
  extract->SetInputData(0, data);
  extract->SetInputData(1, tempSelection);
  extract->Update();

  if (auto cds = vtkCompositeDataSet::SafeDownCast(extract->GetOutput()))
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());

    vtkUniformGridAMRIterator* hierIter = vtkUniformGridAMRIterator::SafeDownCast(iter);

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* dataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!dataset)
      {
        continue;
      }

      // Create a selection node for the block
      vtkNew<vtkSelectionNode> outputNode;
      outputNode->SetFieldType(inputNode->GetFieldType());
      outputNode->SetContentType(vtkSelectionNode::INDICES);
      auto outputProperties = outputNode->GetProperties();
      outputProperties->Set(vtkSelectionNode::INVERSE(), 0);
      outputProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());

      if (hierIter)
      {
        outputProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), hierIter->GetCurrentLevel());
        outputProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), hierIter->GetCurrentIndex());
      }

      // Create a list of ids to select
      vtkSignedCharArray* insidedness = nullptr;

      int type = inputNode->GetFieldType();
      if (type == vtkSelectionNode::CELL)
      {
        insidedness = vtkArrayDownCast<vtkSignedCharArray>(
          dataset->GetCellData()->GetAbstractArray("vtkInsidedness"));
      }
      else if (type == vtkSelectionNode::POINT)
      {
        insidedness = vtkArrayDownCast<vtkSignedCharArray>(
          dataset->GetPointData()->GetAbstractArray("vtkInsidedness"));
      }
      else
      {
        vtkErrorMacro("Unknown field type");
        return 0;
      }

      assert(insidedness);

      // Convert the insidedness array into an index input.
      vtkNew<vtkIdTypeArray> idList;
      for (vtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
      {
        if (insidedness->GetValue(i) == 1)
        {
          idList->InsertNextValue(i);
        }
      }

      outputNode->SetSelectionList(idList);
      output->Union(outputNode);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::Convert(vtkSelection* input, vtkDataObject* data, vtkSelection* output)
{
  unsigned int checkAbortInterval =
    std::min(input->GetNumberOfNodes() / 10 + 1, (unsigned int)1000);
  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    if (n % checkAbortInterval == 0 && this->CheckAbort())
    {
      break;
    }
    vtkSelectionNode* inputNode = input->GetNode(n);
    vtkNew<vtkSelectionNode> outputNode;

    outputNode->ShallowCopy(inputNode);
    outputNode->SetContentType(this->OutputType);

    // If it is the same type, we are done
    if (inputNode->GetContentType() != vtkSelectionNode::VALUES &&
      inputNode->GetContentType() != vtkSelectionNode::THRESHOLDS &&
      inputNode->GetContentType() == this->OutputType)
    {
      output->Union(outputNode);
      continue;
    }

    // If the input is a values or thresholds selection, we need array names
    // on the selection arrays to perform the selection.
    if (inputNode->GetContentType() == vtkSelectionNode::VALUES ||
      inputNode->GetContentType() == vtkSelectionNode::THRESHOLDS)
    {
      vtkFieldData* selData = inputNode->GetSelectionData();
      for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
        if (!selData->GetAbstractArray(i)->GetName())
        {
          vtkErrorMacro("Array name must be specified for values or thresholds selection.");
          return 0;
        }
      }
    }

    // If the output is a threshold selection, we need exactly one array name.
    if (this->OutputType == vtkSelectionNode::THRESHOLDS &&
      (this->ArrayNames == nullptr || this->ArrayNames->GetNumberOfValues() != 1))
    {
      vtkErrorMacro("One array name must be specified for thresholds selection.");
      return 0;
    }

    // If the output is a values selection, we need at lease one array name.
    if (this->OutputType == vtkSelectionNode::VALUES &&
      (this->ArrayNames == nullptr || this->ArrayNames->GetNumberOfValues() == 0))
    {
      vtkErrorMacro("At least one array name must be specified for values selection.");
      return 0;
    }

    // If we are converting a thresholds or values selection to
    // a selection on the same arrays, we are done.
    if ((inputNode->GetContentType() == vtkSelectionNode::VALUES ||
          inputNode->GetContentType() == vtkSelectionNode::THRESHOLDS) &&
      this->OutputType == inputNode->GetContentType() &&
      this->ArrayNames->GetNumberOfValues() == inputNode->GetSelectionData()->GetNumberOfArrays())
    {
      bool same = true;
      vtkFieldData* selData = inputNode->GetSelectionData();
      for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
        if (selData->GetAbstractArray(i)->GetName() != this->ArrayNames->GetValue(i))
        {
          same = false;
          break;
        }
      }
      if (same)
      {
        output->Union(outputNode);
        continue;
      }
    }

    // Check whether we can do the conversion
    if (this->OutputType != vtkSelectionNode::VALUES &&
      this->OutputType != vtkSelectionNode::GLOBALIDS &&
      this->OutputType != vtkSelectionNode::PEDIGREEIDS &&
      this->OutputType != vtkSelectionNode::INDICES)
    {
      vtkErrorMacro("Cannot convert to type " << this->OutputType << " unless input type matches.");
      return 0;
    }

    // Get the correct field data
    vtkFieldData* fd = nullptr;
    vtkDataSetAttributes* dsa = nullptr;
    if (vtkDataSet::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == vtkSelectionNode::CELL)
      {
        dsa = vtkDataSet::SafeDownCast(data)->GetCellData();
      }
      else if (inputNode->GetFieldType() == vtkSelectionNode::POINT)
      {
        dsa = vtkDataSet::SafeDownCast(data)->GetPointData();
      }
      else if (inputNode->GetFieldType() == vtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        vtkErrorMacro("Inappropriate selection type for a vtkDataSet");
        return 0;
      }
    }
    else if (vtkGraph::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == vtkSelectionNode::EDGE)
      {
        dsa = vtkGraph::SafeDownCast(data)->GetEdgeData();
      }
      else if (inputNode->GetFieldType() == vtkSelectionNode::VERTEX)
      {
        dsa = vtkGraph::SafeDownCast(data)->GetVertexData();
      }
      else if (inputNode->GetFieldType() == vtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        vtkErrorMacro("Inappropriate selection type for a vtkGraph");
        return 0;
      }
    }
    else if (vtkTable::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() != vtkSelectionNode::FIELD)
      {
        dsa = vtkTable::SafeDownCast(data)->GetRowData();
      }
      else
      {
        fd = data->GetFieldData();
      }
    }
    else
    {
      if (!inputNode->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == vtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        vtkErrorMacro("Inappropriate selection type for a non-dataset, non-graph");
        return 0;
      }
    }

    // First, convert the selection to a list of indices

    vtkNew<vtkIdTypeArray> indices;

    if (inputNode->GetContentType() == vtkSelectionNode::FRUSTUM ||
      inputNode->GetContentType() == vtkSelectionNode::LOCATIONS ||
      inputNode->GetContentType() == vtkSelectionNode::QUERY)
    {
      if (!vtkDataSet::SafeDownCast(data))
      {
        vtkErrorMacro(
          "Can only convert from frustum, locations, or query if the input is a vtkDataSet");
        return 0;
      }
      // Use the extract selection filter to create an index selection
      vtkNew<vtkSelectionNode> indexNode;
      this->ConvertToIndexSelection(inputNode, vtkDataSet::SafeDownCast(data), indexNode);
      // TODO: We should shallow copy this, but the method is not defined.
      indices->DeepCopy(indexNode->GetSelectionList());
    }
    else if (inputNode->GetContentType() == vtkSelectionNode::THRESHOLDS)
    {
      vtkDoubleArray* lims = vtkArrayDownCast<vtkDoubleArray>(inputNode->GetSelectionList());
      if (!lims)
      {
        vtkErrorMacro("Thresholds selection requires vtkDoubleArray selection list.");
        return 0;
      }
      vtkDataArray* dataArr = nullptr;
      if (dsa)
      {
        dataArr = vtkArrayDownCast<vtkDataArray>(dsa->GetAbstractArray(lims->GetName()));
      }
      else if (fd)
      {
        dataArr = vtkArrayDownCast<vtkDataArray>(fd->GetAbstractArray(lims->GetName()));
      }
      if (!dataArr)
      {
        if (!this->AllowMissingArray)
        {
          vtkErrorMacro("Could not find vtkDataArray for thresholds selection.");
          return 0;
        }
        else
        {
          return 1;
        }
      }
      // create insidenessArray
      vtkNew<vtkSignedCharArray> insidednessArray;
      insidednessArray->SetName(lims->GetName());
      insidednessArray->SetNumberOfComponents(1);
      insidednessArray->SetNumberOfTuples(dataArr->GetNumberOfTuples());
      // create selector
      vtkNew<vtkValueSelector> valueSelector;
      valueSelector->SetInsidednessArrayName(lims->GetName());
      valueSelector->Initialize(inputNode);
      valueSelector->ComputeSelectedElements(data, insidednessArray);
      valueSelector->Finalize();
      for (vtkIdType id = 0; id < dataArr->GetNumberOfTuples(); id++)
      {
        if (insidednessArray->GetValue(id) == 1)
        {
          indices->InsertNextValue(id);
        }
      }
    }
    else if (inputNode->GetContentType() == vtkSelectionNode::INDICES)
    {
      // TODO: We should shallow copy this, but the method is not defined.
      indices->DeepCopy(inputNode->GetSelectionList());
    }
    else if (inputNode->GetContentType() == vtkSelectionNode::VALUES)
    {
      vtkFieldData* selData = inputNode->GetSelectionData();
      vtkNew<vtkTable> selTable;
      selTable->GetRowData()->ShallowCopy(selData);
      vtkNew<vtkTable> dataTable;
      for (vtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
      {
        vtkAbstractArray* dataArr = nullptr;
        if (dsa)
        {
          dataArr = dsa->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
        else if (fd)
        {
          dataArr = fd->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
        if (dataArr)
        {
          dataTable->AddColumn(dataArr);
        }
      }
      // Select rows matching selTable from the input dataTable
      // and put the matches in the index array.
      this->SelectTableFromTable(selTable, dataTable, indices);
    }
    else if (inputNode->GetContentType() == vtkSelectionNode::PEDIGREEIDS ||
      inputNode->GetContentType() == vtkSelectionNode::GLOBALIDS)
    {
      // Get the appropriate array
      vtkAbstractArray* selArr = inputNode->GetSelectionList();
      vtkAbstractArray* dataArr = nullptr;
      if (dsa && inputNode->GetContentType() == vtkSelectionNode::PEDIGREEIDS)
      {
        dataArr = dsa->GetPedigreeIds();
      }
      else if (dsa && inputNode->GetContentType() == vtkSelectionNode::GLOBALIDS)
      {
        dataArr = dsa->GetGlobalIds();
      }
      else if (fd && selArr->GetName())
      {
        // Since data objects only have field data which does not have attributes,
        // use the array name to try to match the incoming selection's array.
        dataArr = fd->GetAbstractArray(selArr->GetName());
      }
      else
      {
        vtkErrorMacro("Tried to use array name to match global or pedigree ids on data object,"
          << "but name not set on selection array.");
        return 0;
      }

      // Check array compatibility
      if (!dataArr)
      {
        if (!this->AllowMissingArray)
        {
          vtkErrorMacro("Selection array does not exist in input dataset.");
          return 0;
        }
        else
        {
          return 1;
        }
      }

      // Handle the special case where we have a domain array.
      vtkStringArray* domainArr =
        dsa ? vtkArrayDownCast<vtkStringArray>(dsa->GetAbstractArray("domain")) : nullptr;
      if (inputNode->GetContentType() == vtkSelectionNode::PEDIGREEIDS && domainArr &&
        selArr->GetName())
      {
        // Perform the lookup, keeping only those items in the correct domain.
        std::string domain = selArr->GetName();
        vtkIdType numTuples = selArr->GetNumberOfTuples();
        vtkNew<vtkIdList> list;
        for (vtkIdType i = 0; i < numTuples; i++)
        {
          dataArr->LookupValue(selArr->GetVariantValue(i), list);
          vtkIdType numIds = list->GetNumberOfIds();
          for (vtkIdType j = 0; j < numIds; j++)
          {
            if (domainArr->GetValue(list->GetId(j)) == domain)
            {
              indices->InsertNextValue(list->GetId(j));
            }
          }
        }
      }
      // If no domain array, the name of the selection and data arrays
      // must match (if they exist).
      else if (inputNode->GetContentType() != vtkSelectionNode::PEDIGREEIDS || !selArr->GetName() ||
        !dataArr->GetName() || !strcmp(selArr->GetName(), dataArr->GetName()))
      {
        // Perform the lookup
        vtkIdType numTuples = selArr->GetNumberOfTuples();
        vtkNew<vtkIdList> list;
        for (vtkIdType i = 0; i < numTuples; i++)
        {
          dataArr->LookupValue(selArr->GetVariantValue(i), list);
          vtkIdType numIds = list->GetNumberOfIds();
          for (vtkIdType j = 0; j < numIds; j++)
          {
            indices->InsertNextValue(list->GetId(j));
          }
        }
      }
    }

    double progress = 0.8;
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

    // Now that we have the list of indices, convert the selection by indexing
    // values in another array.

    // If it is an index selection, we are done.
    if (this->OutputType == vtkSelectionNode::INDICES)
    {
      outputNode->SetSelectionList(indices);
      output->Union(outputNode);
      continue;
    }

    vtkIdType numOutputArrays = 1;
    if (this->OutputType == vtkSelectionNode::VALUES)
    {
      numOutputArrays = this->ArrayNames->GetNumberOfValues();
    }

    // Handle the special case where we have a pedigree id selection with a domain array.
    vtkStringArray* outputDomainArr =
      dsa ? vtkArrayDownCast<vtkStringArray>(dsa->GetAbstractArray("domain")) : nullptr;
    if (this->OutputType == vtkSelectionNode::PEDIGREEIDS && outputDomainArr)
    {
      vtkAbstractArray* outputDataArr = dsa->GetPedigreeIds();
      // Check array existence.
      if (!outputDataArr)
      {
        if (!this->AllowMissingArray)
        {
          vtkErrorMacro("Output selection array does not exist in input dataset.");
          return 0;
        }
        else
        {
          return 1;
        }
      }

      std::map<std::string, vtkSmartPointer<vtkAbstractArray>> domainArrays;
      vtkIdType numTuples = outputDataArr->GetNumberOfTuples();
      vtkIdType numIndices = indices->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIndices; ++i)
      {
        vtkIdType index = indices->GetValue(i);
        if (index >= numTuples)
        {
          continue;
        }
        std::string domain = outputDomainArr->GetValue(index);
        if (domainArrays.count(domain) == 0)
        {
          domainArrays[domain].TakeReference(
            vtkAbstractArray::CreateArray(outputDataArr->GetDataType()));
          domainArrays[domain]->SetName(domain.c_str());
        }
        vtkAbstractArray* domainArr = domainArrays[domain];
        domainArr->InsertNextTuple(index, outputDataArr);
        if (i % 1000 == 0)
        {
          progress = 0.8 + (0.2 * i / numIndices);
          this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }
      for (auto& domainArray : domainArrays)
      {
        vtkNew<vtkSelectionNode> node;
        node->SetContentType(vtkSelectionNode::PEDIGREEIDS);
        node->SetFieldType(inputNode->GetFieldType());
        node->SetSelectionList(domainArray.second);
        output->Union(node);
      }
      continue;
    }

    vtkNew<vtkDataSetAttributes> outputData;
    for (vtkIdType ind = 0; ind < numOutputArrays; ind++)
    {
      // Find the output array where to get the output selection values.
      vtkAbstractArray* outputDataArr = nullptr;
      if (dsa && this->OutputType == vtkSelectionNode::VALUES)
      {
        outputDataArr = dsa->GetAbstractArray(this->ArrayNames->GetValue(ind).c_str());
      }
      else if (fd && this->OutputType == vtkSelectionNode::VALUES)
      {
        outputDataArr = fd->GetAbstractArray(this->ArrayNames->GetValue(ind).c_str());
      }
      else if (dsa && this->OutputType == vtkSelectionNode::PEDIGREEIDS)
      {
        outputDataArr = dsa->GetPedigreeIds();
      }
      else if (dsa && this->OutputType == vtkSelectionNode::GLOBALIDS)
      {
        outputDataArr = dsa->GetGlobalIds();
      }
      else
      {
        // TODO: Make this error go away.
        vtkErrorMacro(
          "BUG: Currently you can only specify pedigree and global ids on a vtkDataSet.");
        return 0;
      }

      // Check array existence.
      if (outputDataArr)
      {
        // Put the array's values into the selection.
        vtkAbstractArray* outputArr = vtkAbstractArray::CreateArray(outputDataArr->GetDataType());
        outputArr->SetName(outputDataArr->GetName());
        vtkIdType numTuples = outputDataArr->GetNumberOfTuples();
        vtkIdType numIndices = indices->GetNumberOfTuples();
        for (vtkIdType i = 0; i < numIndices; ++i)
        {
          vtkIdType index = indices->GetValue(i);
          if (index < numTuples)
          {
            outputArr->InsertNextTuple(index, outputDataArr);
          }
          if (i % 1000 == 0)
          {
            progress = 0.8 + (0.2 * (ind * numIndices + i)) / (numOutputArrays * numIndices);
            this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
          }
        }

        if (this->MatchAnyValues)
        {
          vtkNew<vtkSelectionNode> outNode;
          outNode->ShallowCopy(inputNode);
          outNode->SetContentType(this->OutputType);
          outNode->SetSelectionList(outputArr);
          output->AddNode(outNode);
        }
        else
        {
          outputData->AddArray(outputArr);
        }

        outputArr->Delete();
      }
    }

    // If there are no output arrays, just add a dummy one so
    // that the selection list is not null.
    if (outputData->GetNumberOfArrays() == 0)
    {
      vtkNew<vtkIdTypeArray> arr;
      arr->SetName("Empty");
      outputData->AddArray(arr);
    }

    outputNode->SetSelectionData(outputData);
    output->Union(outputNode);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvertSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkSelection* origInput = vtkSelection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->SelectionExtractor)
  {
    vtkNew<vtkExtractSelection> se;
    this->SetSelectionExtractor(se);
  }

  vtkNew<vtkSelection> input;
  input->ShallowCopy(origInput);
  if (this->InputFieldType != -1)
  {
    for (unsigned int i = 0; i < input->GetNumberOfNodes(); ++i)
    {
      input->GetNode(i)->SetFieldType(this->InputFieldType);
    }
  }

  vtkInformation* dataInfo = inputVector[1]->GetInformationObject(0);
  vtkDataObject* data = dataInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (data && data->IsA("vtkCompositeDataSet"))
  {
    return this->ConvertCompositeDataSet(input, vtkCompositeDataSet::SafeDownCast(data), output);
  }

  return this->Convert(input, data, output);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::SetDataObjectConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//------------------------------------------------------------------------------
int vtkConvertSelection::FillInputPortInformation(int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  }
  else if (port == 1)
  {
    // Can convert from a vtkCompositeDataSet, vtkDataSet, vtkGraph, or vtkTable
    info->Remove(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedItems(
  vtkSelection* input, vtkDataObject* data, int fieldType, vtkIdTypeArray* indices)
{
  vtkSelection* indexSel =
    vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::INDICES);
  for (unsigned int n = 0; n < indexSel->GetNumberOfNodes(); ++n)
  {
    vtkSelectionNode* node = indexSel->GetNode(n);
    vtkIdTypeArray* list = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
    if (node->GetFieldType() == fieldType && node->GetContentType() == vtkSelectionNode::INDICES &&
      list)
    {
      for (vtkIdType i = 0; i < list->GetNumberOfTuples(); ++i)
      {
        vtkIdType cur = list->GetValue(i);
        if (indices->LookupValue(cur) < 0)
        {
          indices->InsertNextValue(cur);
        }
      }
    }
  }
  indexSel->Delete();
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedVertices(
  vtkSelection* input, vtkGraph* data, vtkIdTypeArray* indices)
{
  vtkConvertSelection::GetSelectedItems(input, data, vtkSelectionNode::VERTEX, indices);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedEdges(
  vtkSelection* input, vtkGraph* data, vtkIdTypeArray* indices)
{
  vtkConvertSelection::GetSelectedItems(input, data, vtkSelectionNode::EDGE, indices);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedPoints(
  vtkSelection* input, vtkDataSet* data, vtkIdTypeArray* indices)
{
  vtkConvertSelection::GetSelectedItems(input, data, vtkSelectionNode::POINT, indices);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedCells(
  vtkSelection* input, vtkDataSet* data, vtkIdTypeArray* indices)
{
  vtkConvertSelection::GetSelectedItems(input, data, vtkSelectionNode::CELL, indices);
}

//------------------------------------------------------------------------------
void vtkConvertSelection::GetSelectedRows(
  vtkSelection* input, vtkTable* data, vtkIdTypeArray* indices)
{
  vtkConvertSelection::GetSelectedItems(input, data, vtkSelectionNode::ROW, indices);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToIndexSelection(vtkSelection* input, vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::INDICES);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToGlobalIdSelection(vtkSelection* input, vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::GLOBALIDS);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToPedigreeIdSelection(vtkSelection* input, vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::PEDIGREEIDS);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToValueSelection(
  vtkSelection* input, vtkDataObject* data, const char* arrayName)
{
  vtkNew<vtkStringArray> names;
  names->InsertNextValue(arrayName);
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::VALUES, names);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToValueSelection(
  vtkSelection* input, vtkDataObject* data, vtkStringArray* arrayNames)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelectionNode::VALUES, arrayNames);
}

//------------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToSelectionType(vtkSelection* input, vtkDataObject* data,
  int type, vtkStringArray* arrayNames, int inputFieldType, bool allowMissingArray)
{
  vtkNew<vtkConvertSelection> convert;
  vtkDataObject* dataCopy = data->NewInstance();
  dataCopy->ShallowCopy(data);
  vtkNew<vtkSelection> inputCopy;
  inputCopy->ShallowCopy(input);
  convert->SetInputData(0, inputCopy);
  convert->SetInputData(1, dataCopy);
  convert->SetOutputType(type);
  convert->SetArrayNames(arrayNames);
  convert->SetInputFieldType(inputFieldType);
  convert->SetAllowMissingArray(allowMissingArray);
  convert->Update();
  vtkSelection* output = convert->GetOutput();
  output->Register(nullptr);
  dataCopy->Delete();
  return output;
}

//------------------------------------------------------------------------------
void vtkConvertSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFieldType: " << this->InputFieldType << endl;
  os << indent << "OutputType: " << this->OutputType << endl;
  os << indent << "SelectionExtractor: " << this->SelectionExtractor << endl;
  os << indent << "MatchAnyValues: " << (this->MatchAnyValues ? "true" : "false") << endl;
  os << indent << "AllowMissingArray: " << (this->AllowMissingArray ? "true" : "false") << endl;
  os << indent << "ArrayNames: " << (this->ArrayNames ? "" : "(null)") << endl;
  if (this->ArrayNames)
  {
    this->ArrayNames->PrintSelf(os, indent.GetNextIndent());
  }
}
VTK_ABI_NAMESPACE_END
