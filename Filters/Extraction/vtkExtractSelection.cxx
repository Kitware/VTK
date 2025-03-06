// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#include "vtkExtractSelection.h"

#include "vtkAppendSelection.h"
#include "vtkBitArray.h"
#include "vtkBlockSelector.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkExtractCells.h"
#include "vtkFrustumSelector.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridToUnstructuredGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLocationSelector.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelector.h"
#include "vtkSignedCharArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkValueSelector.h"

#include <array>
#include <cassert>
#include <map>
#include <vector>

namespace
{

// Recursive check for visible cells and propagate up visibility to parents
bool SanitizeHTGMask(vtkHyperTreeGridNonOrientedCursor* cursor)
{
  if (!cursor->IsLeaf())
  {
    bool isMasked = cursor->IsMasked();
    if (isMasked)
    {
      cursor->SetMask(false);
    }
    bool checkLowerLevelVisible = false;
    for (vtkIdType iChild = 0; iChild < cursor->GetNumberOfChildren(); ++iChild)
    {
      cursor->ToChild(iChild);
      checkLowerLevelVisible = SanitizeHTGMask(cursor) || checkLowerLevelVisible;
      cursor->ToParent();
    }
    if (!checkLowerLevelVisible && isMasked)
    {
      cursor->SetMask(true);
    }
  }
  return !cursor->IsMasked();
}

}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractSelection);
//------------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection() = default;

//------------------------------------------------------------------------------
int vtkExtractSelection::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractSelection::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  if (!inputDO)
  {
    return 0;
  }

  const int inputType = inputDO->GetDataObjectType();
  int outputType = -1;

  if (this->PreserveTopology)
  {
    // when PreserveTopology is ON, we preserve input data type.
    outputType = inputType;
  }
  else if (vtkDataObjectTree::SafeDownCast(inputDO))
  {
    // For DataObjectTree, preserve the type.
    outputType = inputType;
  }
  else if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For other composite datasets, we create a vtkPartitionedDataSetCollection as output;
    outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  }
  else if (vtkDataSet::SafeDownCast(inputDO) ||
    (this->HyperTreeGridToUnstructuredGrid && vtkHyperTreeGrid::SafeDownCast(inputDO)))
  {
    // vtkDataSet becomes a vtkUnstructuredGrid.
    outputType = VTK_UNSTRUCTURED_GRID;
  }
  else
  {
    // preserve input type for the rest e.g. vtkTable, vtkGraph etc.
    outputType = inputType;
  }

  auto outInfo = outputVector->GetInformationObject(0);
  if (outputType != -1 &&
    vtkDataObjectAlgorithm::SetOutputDataObject(outputType, outInfo, /*exact=*/true))
  {
    return 1;
  }

  vtkErrorMacro("Not sure what type of output to create!");
  return 0;
}

//------------------------------------------------------------------------------
vtkDataObject::AttributeTypes vtkExtractSelection::GetAttributeTypeOfSelection(
  vtkSelection* sel, bool& sane)
{
  sane = true;
  int fieldType = -1;
  for (unsigned int n = 0; n < sel->GetNumberOfNodes(); n++)
  {
    vtkSelectionNode* node = sel->GetNode(n);

    int nodeFieldType = node->GetFieldType();

    if (nodeFieldType == vtkSelectionNode::POINT &&
      node->GetProperties()->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
      node->GetProperties()->Get(vtkSelectionNode::CONTAINING_CELLS()))
    {
      // we're really selecting cells, not points.
      nodeFieldType = vtkSelectionNode::CELL;
    }

    if (n != 0 && fieldType != nodeFieldType)
    {
      sane = false;
      vtkErrorMacro("Selection contains mismatched attribute types!");
      return vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
    }

    fieldType = nodeFieldType;
  }

  return fieldType == -1 ? vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES
                         : static_cast<vtkDataObject::AttributeTypes>(
                             vtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType));
}

namespace
{
//----------------------------------------------------------------------------
void InvertSelection(vtkSignedCharArray* array)
{
  const vtkIdType n = array->GetNumberOfTuples();
  vtkSMPTools::For(0, n,
    [&array](vtkIdType start, vtkIdType end)
    {
      for (vtkIdType i = start; i < end; ++i)
      {
        array->SetValue(i, static_cast<signed char>(array->GetValue(i) * -1 + 1));
      }
    });
}

//----------------------------------------------------------------------------
// Remove all selection nodes that their propId = vtkSelectionNode::PROCESS_ID()
// is not the same as the processId = vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER().
void vtkTrimSelection(vtkSelection* input, int processId)
{
  if (input)
  {
    unsigned int numNodes = input->GetNumberOfNodes();
    for (unsigned int cc = 0; cc < numNodes; cc++)
    {
      vtkSelectionNode* node = input->GetNode(cc);
      int propId = (node->GetProperties()->Has(vtkSelectionNode::PROCESS_ID()))
        ? node->GetProperties()->Get(vtkSelectionNode::PROCESS_ID())
        : -1;
      if (propId != -1 && processId != -1 && propId != processId)
      {
        input->RemoveNode(node);
      }
    }
  }
}
}

//------------------------------------------------------------------------------
int vtkExtractSelection::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // If no input, error
  if (!input)
  {
    vtkErrorMacro(<< "No input specified");
    return 0;
  }

  // If no selection, quietly select nothing
  if (!selection)
  {
    return 1;
  }

  // preserve only nodes that their processId matches the current processId
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    int processId = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    vtkTrimSelection(selection, processId);
  }

  // check for empty selection
  if (selection->GetNumberOfNodes() == 0)
  {
    return 1;
  }

  // check for select FieldType consistency right here and return failure
  // if they are not consistent.
  bool sane;
  const auto assoc = this->GetAttributeTypeOfSelection(selection, sane);
  if (!sane || assoc == vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    vtkErrorMacro("Selection has selection nodes with inconsistent field types.");
    return 0;
  }

  // Create operators for each of vtkSelectionNode instances and initialize them.
  std::map<std::string, vtkSmartPointer<vtkSelector>> selectors;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    auto name = selection->GetNodeNameAtIndex(cc);

    if (auto anOperator = this->NewSelectionOperator(
          static_cast<vtkSelectionNode::SelectionContent>(node->GetContentType())))
    {
      anOperator->SetInsidednessArrayName(name);
      anOperator->Initialize(node);
      selectors[name] = anOperator;
    }
    else
    {
      vtkWarningMacro("Unhandled selection node with content type : " << node->GetContentType());
    }
  }

  // Check if vtkSelector::ExpandToConnectedElements will be used.
  // This is useful because we can omit shallow copy of the input data.
  bool expandToConnectedElements = false;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    int association = vtkSelectionNode::ConvertSelectionFieldToAttributeType(node->GetFieldType());
    const int layers = node->GetProperties()->Get(vtkSelectionNode::CONNECTED_LAYERS());

    if (layers >= 1 && (association == vtkDataObject::POINT || association == vtkDataObject::CELL))
    {
      expandToConnectedElements = true;
      break;
    }
  }

  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputCD = vtkCompositeDataSet::SafeDownCast(output);
    assert(outputCD != nullptr);
    outputCD->CopyStructure(inputCD);

    auto inIter = vtk::TakeSmartPointer(inputCD->NewIterator());

    // Initialize the output composite dataset to have blocks with the same type
    // as the input.
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (this->CheckAbort())
      {
        break;
      }
      auto blockInput = inIter->GetCurrentDataObject();
      if (blockInput)
      {
        vtkSmartPointer<vtkDataObject> clone;
        if (expandToConnectedElements || this->PreserveTopology ||
          vtkHyperTreeGrid::SafeDownCast(blockInput))
        {
          clone.TakeReference(blockInput->NewInstance());
          clone->ShallowCopy(blockInput);
        }
        else
        {
          if (assoc != vtkDataObject::ROW)
          {
            clone.TakeReference(vtkUnstructuredGrid::New());
          }
          else
          {
            clone.TakeReference(vtkTable::New());
          }
        }
        outputCD->SetDataSet(inIter, clone);
      }
    }

    // Evaluate the operators.
    vtkLogStartScope(TRACE, "execute selectors");
    for (const auto& nodeSelector : selectors)
    {
      if (this->CheckAbort())
      {
        break;
      }
      nodeSelector.second->Execute(inputCD, outputCD);
    }
    vtkLogEndScope("execute selectors");

    vtkLogStartScope(TRACE, "evaluate expression and extract output");
    // Now iterate again over the composite dataset and evaluate the expression to
    // combine all the insidedness arrays and then extract the elements.
    bool globalEvaluationResult = true;
    // we use the input iterator instead of the output one, because if inputCD is subclass of
    // vtkUniformGridAMR, GetDataSet requires the iterator to be vtkUniformGridAMRDataIterator
    for (inIter->GoToFirstItem(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (this->CheckAbort())
      {
        break;
      }
      auto inBlock = inIter->GetCurrentDataObject();
      auto outBlock = outputCD->GetDataSet(inIter);
      if (inBlock && outBlock)
      {
        // Evaluate the expression.
        auto evaluationResult = this->EvaluateSelection(outBlock, assoc, selection, selectors);
        if (evaluationResult != EvaluationResult::INVALID)
        {
          vtkSmartPointer<vtkUnsignedCharArray> colorArray =
            this->EvaluateColorArrayInSelection(outBlock, assoc, selection);

          // Extract the elements.
          auto extractResult = this->ExtractElements(inBlock, assoc, evaluationResult, outBlock);

          this->AddColorArrayOnObject(extractResult, colorArray);
          outputCD->SetDataSet(inIter, extractResult);
        }
        else
        {
          globalEvaluationResult = false;
          break;
        }
      }
    }
    vtkLogEndScope("evaluate expression and extract output");
    // check for evaluate result errors
    if (!globalEvaluationResult)
    {
      // If the expression evaluation failed, then we need to set all the blocks to null.
      for (inIter->GoToFirstItem(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
      {
        outputCD->SetDataSet(inIter, nullptr);
      }
      return 0;
    }
  }
  else
  {
    assert(output != nullptr);

    vtkSmartPointer<vtkDataObject> clone;
    if (expandToConnectedElements || this->PreserveTopology ||
      vtkHyperTreeGrid::SafeDownCast(input))
    {
      clone.TakeReference(input->NewInstance());
      clone->ShallowCopy(input);
    }
    else
    {
      if (assoc != vtkDataObject::ROW)
      {
        clone.TakeReference(vtkUnstructuredGrid::New());
      }
      else
      {
        clone.TakeReference(vtkTable::New());
      }
    }

    // Evaluate the operators.
    vtkLogStartScope(TRACE, "execute selectors");
    for (const auto& nodeSelector : selectors)
    {
      if (this->CheckAbort())
      {
        break;
      }
      nodeSelector.second->Execute(input, clone);
    }
    vtkLogEndScope("execute selectors");

    vtkLogStartScope(TRACE, "evaluate expression");
    auto evaluateResult = this->EvaluateSelection(clone, assoc, selection, selectors);
    vtkLogEndScope("evaluate expression");
    // check for evaluate result errors
    if (evaluateResult == EvaluationResult::INVALID)
    {
      output->Initialize();
      return 0;
    }

    vtkSmartPointer<vtkUnsignedCharArray> colorArray =
      this->EvaluateColorArrayInSelection(clone, assoc, selection);

    vtkLogStartScope(TRACE, "extract output");
    if (auto extractResult = this->ExtractElements(input, assoc, evaluateResult, clone))
    {
      output->ShallowCopy(extractResult);
    }
    vtkLogEndScope("extract output");

    this->AddColorArrayOnObject(output, colorArray);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkSelector> vtkExtractSelection::NewSelectionOperator(
  vtkSelectionNode::SelectionContent contentType)
{
  switch (contentType)
  {
    case vtkSelectionNode::GLOBALIDS:
    case vtkSelectionNode::PEDIGREEIDS:
    case vtkSelectionNode::VALUES:
    case vtkSelectionNode::INDICES:
    case vtkSelectionNode::THRESHOLDS:
      return vtkSmartPointer<vtkValueSelector>::New();

    case vtkSelectionNode::FRUSTUM:
      return vtkSmartPointer<vtkFrustumSelector>::New();

    case vtkSelectionNode::LOCATIONS:
      return vtkSmartPointer<vtkLocationSelector>::New();

    case vtkSelectionNode::BLOCKS:
    case vtkSelectionNode::BLOCK_SELECTORS:
      return vtkSmartPointer<vtkBlockSelector>::New();

    case vtkSelectionNode::USER:
    case vtkSelectionNode::QUERY:
    default:
      return nullptr;
  }
}

//------------------------------------------------------------------------------
vtkExtractSelection::EvaluationResult vtkExtractSelection::EvaluateSelection(
  vtkDataObject* dataObject, vtkDataObject::AttributeTypes association, vtkSelection* selection,
  std::map<std::string, vtkSmartPointer<vtkSelector>>& selectors)
{
  auto fieldData = dataObject->GetAttributes(association);
  if (!fieldData)
  {
    return EvaluationResult::NONE;
  }

  // Iterate over operators and set up a map from selection node name to insidedness
  // array.
  std::map<std::string, vtkSignedCharArray*> arrayMap;
  for (const auto& nodeSelector : selectors)
  {
    auto name = nodeSelector.first;
    auto insidednessArray = vtkSignedCharArray::SafeDownCast(fieldData->GetArray(name.c_str()));
    auto node = selection->GetNode(name);
    if (insidednessArray != nullptr && node->GetProperties()->Has(vtkSelectionNode::INVERSE()) &&
      node->GetProperties()->Get(vtkSelectionNode::INVERSE()))
    {
      ::InvertSelection(insidednessArray);
    }
    arrayMap[name] = insidednessArray;
  }

  // Evaluate the map of insidedness arrays
  std::array<signed char, 2> range;
  auto blockInsidedness = selection->Evaluate(arrayMap, range);
  if (blockInsidedness)
  {
    blockInsidedness->SetName("__vtkInsidedness__");
    if (range[0] == 0 && range[1] == 0)
    {
      return EvaluationResult::NONE;
    }
    else if (range[0] == 1 && range[1] == 1)
    {
      fieldData->AddArray(blockInsidedness);
      return EvaluationResult::ALL;
    }
    else
    {
      fieldData->AddArray(blockInsidedness);
      return EvaluationResult::MIXED;
    }
  }
  else
  {
    return EvaluationResult::INVALID;
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnsignedCharArray> vtkExtractSelection::EvaluateColorArrayInSelection(
  vtkDataObject* dataObject, vtkDataObject::AttributeTypes association, vtkSelection* selection)
{
  vtkSmartPointer<vtkUnsignedCharArray> outputArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  auto fieldData = dataObject->GetAttributes(association);
  if (!fieldData)
  {
    return nullptr;
  }

  // Map for each selectionNode the corresponding insidednessArray which take in account if the
  // selection is inverted
  std::string expression = selection->GetExpression();
  std::map<std::string, vtkSignedCharArray*> arrayMap;
  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); i++)
  {
    std::string name = selection->GetNodeNameAtIndex(i);
    if (name.empty())
    {
      continue;
    }

    auto* insidednessArray = vtkSignedCharArray::SafeDownCast(fieldData->GetArray(name.c_str()));
    if (!insidednessArray)
    {
      continue;
    }

    std::size_t pos = expression.find(name);
    if (pos == std::string::npos)
    {
      continue;
    }
    // Corner case for the first selectionNode in the expression, in this case the selection cannot
    // be inverted
    else if (pos <= 1)
    {
      arrayMap[name] = insidednessArray;
      continue;
    }

    std::string operand = expression.substr(pos - 2, 1);
    if (operand == "!")
    {
      ::InvertSelection(insidednessArray);
    }
    arrayMap[name] = insidednessArray;
  }

  vtkSignedCharArray* insidednessArray = nullptr;

  std::vector<vtkSignedCharArray*> selArrays;
  for (int i = 0; i < fieldData->GetNumberOfArrays(); i++)
  {
    auto* selArray = vtkSignedCharArray::SafeDownCast(fieldData->GetArray(i));
    if (!selArray)
    {
      continue;
    }
    // Internal array added by the EvaluateSelection method, __vtkInsidedness__ is used to know
    // which cell/point from the dataSet are inside a selection, it is useful here to follow the
    // expression set by the user and color the selection as the user expect.
    if (strcmp(selArray->GetName(), "__vtkInsidedness__") == 0)
    {
      insidednessArray = selArray;
    }
    else
    {
      selArrays.push_back(selArray);
    }
  }

  const char* colorArrayName = vtkAppendSelection::GetColorArrayName();

  // Find the associated color for each selection
  std::vector<std::array<double, 3>> colorArrays;
  for (unsigned int selIdx = 0; selIdx < selection->GetNumberOfNodes(); selIdx++)
  {
    auto* selectionNode = selection->GetNode(selIdx);
    auto* colorArray = vtkUnsignedCharArray::SafeDownCast(
      selectionNode->GetSelectionData()->GetArray(colorArrayName));
    if (!colorArray)
    {
      continue;
    }

    bool findColor = false;
    for (int i = 0; i < colorArray->GetNumberOfTuples(); i++)
    {
      double* currColor = colorArray->GetTuple3(i);
      if (currColor[0] != 0 || currColor[1] != 0 || currColor[2] != 0)
      {
        colorArrays.emplace_back(std::array<double, 3>{ currColor[0], currColor[1], currColor[2] });
        findColor = true;
        break;
      }
    }

    if (!findColor)
    {
      colorArrays.emplace_back(std::array<double, 3>{ 0, 0, 0 });
    }
  }

  if (selArrays.size() != colorArrays.size())
  {
    // Silently do nothing as a color array for a selection isn't required
    return nullptr;
  }

  if (!insidednessArray)
  {
    return nullptr;
  }

  unsigned int numberOfElement = insidednessArray->GetNumberOfTuples();

  std::vector<double*> colors;
  std::vector<unsigned int> indexOffsetPerColorArray(colorArrays.size(), 0);
  // Populate the array
  for (unsigned int i = 0; i < numberOfElement; i++)
  {
    if (insidednessArray->GetValue(i) == 0)
    {
      continue;
    }

    std::size_t selIdx = selArrays.size();
    for (auto it = arrayMap.rbegin(); it != arrayMap.rend() && selIdx-- != 0; ++it)
    {
      if (it->second->GetValue(i) == 0)
      {
        continue;
      }

      colors.emplace_back(colorArrays[selIdx].data());
      indexOffsetPerColorArray[selIdx]++;
      break;
    }
  }

  outputArray->SetName(colorArrayName);
  outputArray->SetNumberOfComponents(3);
  outputArray->SetNumberOfTuples(colors.size());
  for (std::size_t i = 0; i < colors.size(); i++)
  {
    double* color = colors[i];
    outputArray->SetTuple3(i, color[0], color[1], color[2]);
  }

  return outputArray;
}

//------------------------------------------------------------------------------
void vtkExtractSelection::AddColorArrayOnObject(
  vtkDataObject* dataObject, vtkUnsignedCharArray* colorArray)
{
  if (!dataObject || !colorArray)
  {
    return;
  }

  auto* outputFieldData = dataObject->GetAttributes(vtkDataObject::CELL);
  if (outputFieldData && colorArray->GetNumberOfTuples() != 0)
  {
    outputFieldData->SetScalars(colorArray);
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExtractSelection::ExtractElements(vtkDataObject* inputBlock,
  vtkDataObject::AttributeTypes type, EvaluationResult evaluationResult, vtkDataObject* outputBlock)
{
  bool extractAll = evaluationResult == EvaluationResult::ALL;
  bool extractNone = evaluationResult == EvaluationResult::NONE;
  if (extractNone)
  {
    // Assume nothing was selected and return.
    return nullptr;
  }
  auto fd = outputBlock->GetAttributes(type);
  vtkSmartPointer<vtkSignedCharArray> insidednessArray =
    fd ? vtkSignedCharArray::SafeDownCast(fd->GetArray("__vtkInsidedness__")) : nullptr;
  if (!insidednessArray || insidednessArray->GetNumberOfTuples() <= 0)
  {
    // No insidedness array
    return nullptr;
  }
  vtkSmartPointer<vtkDataObject> result;

  if (vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(inputBlock))
  {
    vtkNew<vtkBitArray> mask;
    mask->SetNumberOfComponents(1);
    mask->SetNumberOfTuples(insidednessArray->GetNumberOfTuples());
    auto masking = [&mask, &insidednessArray](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType iMask = begin; iMask < end; ++iMask)
      {
        mask->SetValue(iMask, static_cast<int>(insidednessArray->GetValue(iMask) == 0));
      }
    };
    // lambda method is not thread safe due to vtkBitArray (see issue #18837)
    //---------------------------------------------------------------------
    masking(0, mask->GetNumberOfTuples());
    // vtkSMPTools::For(0, mask->GetNumberOfTuples(), masking);
    //---------------------------------------------------------------------
    if (htg->HasMask())
    {
      auto originalMask = htg->GetMask();
      auto maskOring = [&mask, &originalMask](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType iMask = begin; iMask < end; ++iMask)
        {
          if (originalMask->GetValue(iMask))
          {
            mask->SetValue(iMask, 1);
          }
        }
      };
      maskOring(0, mask->GetNumberOfTuples());
    }
    result.TakeReference(htg->NewInstance());
    vtkHyperTreeGrid* outHTG = vtkHyperTreeGrid::SafeDownCast(result);
    outHTG->ShallowCopy(htg);
    outHTG->SetMask(mask);
    // sanitize the mask
    {
      vtkIdType index = 0;
      vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
      outHTG->InitializeTreeIterator(iterator);
      vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
      while (iterator.GetNextTree(index))
      {
        if (this->CheckAbort())
        {
          break;
        }
        cursor->Initialize(outHTG, index);
        ::SanitizeHTGMask(cursor);
      }
    }
    if (this->HyperTreeGridToUnstructuredGrid)
    {
      vtkNew<vtkHyperTreeGridToUnstructuredGrid> htg2ug;
      htg2ug->SetInputDataObject(outHTG);
      htg2ug->Update();
      result = htg2ug->GetOutput();
    }
    return result;
  }

  if (this->PreserveTopology)
  {
    insidednessArray->SetName("vtkInsidedness");
    outputBlock->GetAttributesAsFieldData(type)->AddArray(insidednessArray);
    result = outputBlock;
    return result;
  }
  else if (type == vtkDataObject::POINT)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(inputBlock);
    if (!input)
    {
      return nullptr;
    }
    // if output is already a vtkUnstructuredGrid, we can use it directly
    vtkSmartPointer<vtkUnstructuredGrid> output;
    if (outputBlock->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
      outputBlock->Initialize();
      output = static_cast<vtkUnstructuredGrid*>(outputBlock);
    }
    else
    {
      output = vtkSmartPointer<vtkUnstructuredGrid>::New();
    }
    this->ExtractSelectedPoints(input, output, insidednessArray, extractAll);
    result = output;
  }
  else if (type == vtkDataObject::CELL)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(inputBlock);
    if (!input)
    {
      return nullptr;
    }
    // if output is already a vtkUnstructuredGrid, we can use it directly
    vtkSmartPointer<vtkUnstructuredGrid> output;
    if (outputBlock->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
      outputBlock->Initialize();
      output = static_cast<vtkUnstructuredGrid*>(outputBlock);
    }
    else
    {
      output = vtkSmartPointer<vtkUnstructuredGrid>::New();
    }
    this->ExtractSelectedCells(input, output, insidednessArray, extractAll);
    result = output;
  }
  else if (type == vtkDataObject::ROW)
  {
    vtkTable* input = vtkTable::SafeDownCast(inputBlock);
    if (!input)
    {
      return nullptr;
    }
    // if output is already a vtkTable, we can use it directly
    vtkSmartPointer<vtkTable> output;
    if (outputBlock->GetDataObjectType() == VTK_TABLE)
    {
      outputBlock->Initialize();
      output = static_cast<vtkTable*>(outputBlock);
    }
    else
    {
      output = vtkSmartPointer<vtkTable>::New();
    }
    this->ExtractSelectedRows(input, output, insidednessArray, extractAll);
    result = output;
  }
  else
  {
    outputBlock->Initialize();
    result = outputBlock;
  }

  return result && result->GetNumberOfElements(type) > 0 ? result : nullptr;
}

//------------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedCells(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* cellInside, bool extractAll)
{
  vtkLogScopeF(TRACE, "ExtractSelectedCells");
  const vtkIdType numPts = input->GetNumberOfPoints();
  const vtkIdType numCells = input->GetNumberOfCells();

  // The "input" is a shallow copy of the input to this filter and hence we can
  // modify it. We add original cell ids and point ids arrays.
  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  originalPointIds->SetNumberOfTuples(numPts);
  vtkSMPTools::For(0, numPts,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType ptId = begin; ptId < end; ++ptId)
      {
        originalPointIds->SetValue(ptId, ptId);
      }
    });
  input->GetPointData()->AddArray(originalPointIds);

  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetName("vtkOriginalCellIds");
  originalCellIds->SetNumberOfTuples(numCells);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        originalCellIds->SetValue(cellId, cellId);
      }
    });
  input->GetCellData()->AddArray(originalCellIds);

  vtkNew<vtkExtractCells> extractor;
  extractor->SetContainerAlgorithm(this);
  if (extractAll)
  {
    // all elements are selected, pass all data.
    // we still use the extractor since it does the data conversion, if needed
    extractor->SetExtractAllCells(true);
  }
  else
  {
    // convert insideness array to cell ids to extract.
    vtkNew<vtkIdList> ids;
    ids->Allocate(numCells);
    vtkUnsignedCharArray* ghostArray = input->GetCellGhostArray();
    for (vtkIdType cc = 0; cc < numCells; ++cc)
    {
      if (ghostArray && ghostArray->GetValue(cc) == vtkDataSetAttributes::HIDDENCELL)
      {
        // skip this cell
        continue;
      }
      if (cellInside->GetValue(cc) != 0)
      {
        ids->InsertNextId(cc);
      }
    }
    extractor->SetAssumeSortedAndUniqueIds(true);
    extractor->SetCellList(ids);
  }

  extractor->SetInputDataObject(input);
  extractor->Update();
  output->ShallowCopy(extractor->GetOutput());
}

//------------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedPoints(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* pointInside, bool extractAll)
{
  vtkIdType numPts = input->GetNumberOfPoints();

  vtkPointData* pd = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();

  // To copy points in a type agnostic way later
  auto pointSet = vtkPointSet::SafeDownCast(input);

  outputPD->SetCopyGlobalIds(1);
  outputPD->CopyFieldOff("vtkOriginalPointIds");
  outputPD->CopyAllocate(pd);

  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  outputPD->AddArray(originalPointIds);

  vtkNew<vtkPoints> newPts;
  if (!extractAll)
  {
    if (pointSet)
    {
      newPts->SetDataType(pointSet->GetPoints()->GetDataType());
    }
    vtkNew<vtkIdList> ids;
    ids->Allocate(numPts);
    vtkUnsignedCharArray* ghostArray = input->GetPointGhostArray();
    for (vtkIdType cc = 0; cc < numPts; ++cc)
    {
      if (ghostArray && ghostArray->GetValue(cc) == vtkDataSetAttributes::HIDDENPOINT)
      {
        // skip this point
        continue;
      }
      if (pointInside->GetValue(cc) != 0)
      {
        ids->InsertNextId(cc);
      }
    }
    const vtkIdType numNewPts = ids->GetNumberOfIds();
    // copy points
    newPts->SetNumberOfPoints(numNewPts);
    vtkSMPTools::For(0, numNewPts,
      [&](vtkIdType begin, vtkIdType end)
      {
        double point[3];
        auto idsPtr = ids->GetPointer(0);
        for (vtkIdType ptId = begin; ptId < end; ++ptId)
        {
          input->GetPoint(idsPtr[ptId], point);
          newPts->SetPoint(ptId, point);
        }
      });
    // copy point data
    outputPD->SetNumberOfTuples(numNewPts);
    outputPD->CopyData(pd, ids);
    // set original point ids
    originalPointIds->SetNumberOfTuples(numNewPts);
    vtkSMPTools::For(0, numNewPts,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto idsPtr = ids->GetPointer(0);
        for (vtkIdType ptId = begin; ptId < end; ++ptId)
        {
          originalPointIds->SetValue(ptId, idsPtr[ptId]);
        }
      });
  }
  else
  {
    // copy points
    if (pointSet)
    {
      newPts->ShallowCopy(pointSet->GetPoints());
    }
    else
    {
      newPts->SetNumberOfPoints(numPts);
      vtkSMPTools::For(0, numPts,
        [&](vtkIdType beginPtId, vtkIdType endPtId)
        {
          double x[3];
          for (vtkIdType ptId = beginPtId; ptId < endPtId; ++ptId)
          {
            input->GetPoint(ptId, x);
            newPts->SetPoint(ptId, x);
          }
        });
    }
    // copy point data
    outputPD->PassData(pd);
    // set original point ids
    originalPointIds->SetNumberOfTuples(numPts);
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType beginPtId, vtkIdType endPtId)
      {
        for (vtkIdType ptId = beginPtId; ptId < endPtId; ++ptId)
        {
          originalPointIds->SetValue(ptId, ptId);
        }
      });
  }
  output->SetPoints(newPts);

  // produce a new vtk_vertex cell for each accepted point
  vtkIdType newNumPts = output->GetNumberOfPoints();
  // create connectivity array
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(newNumPts);
  vtkSMPTools::For(0, newNumPts,
    [&](vtkIdType beginPtId, vtkIdType endPtId)
    {
      for (vtkIdType ptId = beginPtId; ptId < endPtId; ++ptId)
      {
        connectivity->SetValue(ptId, ptId);
      }
    });
  // create offsets array
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(newNumPts + 1);
  vtkSMPTools::For(0, newNumPts + 1,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; i++)
      {
        offsets->SetValue(i, i);
      }
    });
  // create cell array
  vtkNew<vtkCellArray> cells;
  cells->SetData(offsets, connectivity);
  // create cell types
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfValues(newNumPts);
  static constexpr unsigned char cellType = VTK_VERTEX;
  vtkSMPTools::Fill(cellTypes->GetPointer(0), cellTypes->GetPointer(newNumPts), cellType);
  // set cells
  output->SetCells(cellTypes, cells);

  // Copy field data
  output->GetFieldData()->ShallowCopy(input->GetFieldData());
}

//------------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedRows(
  vtkTable* input, vtkTable* output, vtkSignedCharArray* rowsInside, bool extractAll)
{
  const vtkIdType numRows = input->GetNumberOfRows();
  vtkNew<vtkIdTypeArray> originalRowIds;
  originalRowIds->SetName("vtkOriginalRowIds");

  output->GetRowData()->CopyFieldOff("vtkOriginalRowIds");
  output->GetRowData()->CopyStructure(input->GetRowData());

  if (!extractAll)
  {
    for (vtkIdType rowId = 0; rowId < numRows; ++rowId)
    {
      signed char isInside = rowsInside->GetTypedComponent(rowId, 0);
      if (isInside)
      {
        output->InsertNextRow(input->GetRow(rowId));
        originalRowIds->InsertNextValue(rowId);
      }
    }
  }
  else
  {
    output->ShallowCopy(input);
    originalRowIds->SetNumberOfTuples(numRows);
    vtkSMPTools::For(0, numRows,
      [&](vtkIdType beginRowId, vtkIdType endRowId)
      {
        for (vtkIdType rowId = beginRowId; rowId < endRowId; ++rowId)
        {
          originalRowIds->SetValue(rowId, rowId);
        }
      });
  }
  output->AddColumn(originalRowIds);
}

//------------------------------------------------------------------------------
void vtkExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
VTK_ABI_NAMESPACE_END
