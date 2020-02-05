/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelection.h"

#include "vtkAssume.h"
#include "vtkBlockSelector.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkExtractCells.h"
#include "vtkFrustumSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLocationSelector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelector.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkValueSelector.h"

#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkExtractSelection);
//----------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection() {}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkExtractSelection::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PreserveTopology)
  {
    // when PreserveTopology is ON, we're preserve input data type.
    if (outputDO == nullptr || !outputDO->IsA(inputDO->GetClassName()))
    {
      outputDO = inputDO->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a vtkMultiBlockDataSet as output;
    if (vtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkTable::SafeDownCast(inputDO))
  {
    // vtkTable input stays as vtkTable.
    if (vtkTable::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkTable::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkDataSet::SafeDownCast(inputDO))
  {
    // vtkDataSet becomes a vtkUnstructuredGrid.
    if (vtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (!outputDO || !outputDO->IsTypeOf(inputDO->GetClassName()))
  {
    outputDO = inputDO->NewInstance();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
    outputDO->Delete();
    return 1;
  }

  vtkErrorMacro("Not sure what type of output to create!");
  return 0;
}

//----------------------------------------------------------------------------
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
void InvertSelection(vtkSignedCharArray* array)
{
  const vtkIdType n = array->GetNumberOfTuples();
  vtkSMPTools::For(0, n, [&array](vtkIdType start, vtkIdType end) {
    for (vtkIdType i = start; i < end; ++i)
    {
      array->SetValue(i, static_cast<signed char>(array->GetValue(i) * -1 + 1));
    }
  });
}
}

//----------------------------------------------------------------------------
int vtkExtractSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

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
  std::map<std::string, vtkSmartPointer<vtkSelector> > selectors;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    auto name = selection->GetNodeNameAtIndex(cc);

    if (auto anOperator = this->NewSelectionOperator(
          static_cast<vtkSelectionNode::SelectionContent>(node->GetContentType())))
    {
      anOperator->SetInsidednessArrayName(name.c_str());
      anOperator->Initialize(node);
      selectors[name] = anOperator;
    }
    else
    {
      vtkWarningMacro("Unhandled selection node with content type : " << node->GetContentType());
    }
  }

  auto evaluate = [&selectors, assoc, &selection](vtkDataObject* dobj) {
    auto fieldData = dobj->GetAttributes(assoc);
    if (!fieldData)
    {
      return;
    }

    // Iterate over operators and set up a map from selection node name to insidedness
    // array.
    std::map<std::string, vtkSignedCharArray*> arrayMap;
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto name = nodeIter->first;
      auto insidednessArray = vtkSignedCharArray::SafeDownCast(fieldData->GetArray(name.c_str()));
      auto node = selection->GetNode(name.c_str());
      if (insidednessArray != nullptr && node->GetProperties()->Has(vtkSelectionNode::INVERSE()) &&
        node->GetProperties()->Get(vtkSelectionNode::INVERSE()))
      {
        ::InvertSelection(insidednessArray);
      }
      arrayMap[name] = insidednessArray;
    }

    // Evaluate the map of insidedness arrays
    auto blockInsidedness = selection->Evaluate(arrayMap);
    blockInsidedness->SetName("__vtkInsidedness__");
    fieldData->AddArray(blockInsidedness);
  };

  auto extract = [&assoc, this](
                   vtkDataObject* inpDO, vtkDataObject* opDO) -> vtkSmartPointer<vtkDataObject> {
    auto fd = opDO->GetAttributes(assoc);
    auto array =
      fd ? vtkSignedCharArray::SafeDownCast(fd->GetArray("__vtkInsidedness__")) : nullptr;
    auto resultDO = array ? this->ExtractElements(inpDO, assoc, array) : nullptr;
    return (resultDO && resultDO->GetNumberOfElements(assoc) > 0) ? resultDO : nullptr;
  };

  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputCD = vtkCompositeDataSet::SafeDownCast(output);
    assert(outputCD != nullptr);
    outputCD->CopyStructure(inputCD);

    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCD->NewIterator());

    // Initialize the output composite dataset to have blocks with the same type
    // as the input.
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      auto blockInput = inIter->GetCurrentDataObject();
      if (blockInput)
      {
        auto clone = blockInput->NewInstance();
        clone->ShallowCopy(blockInput);
        outputCD->SetDataSet(inIter, clone);
        clone->FastDelete();
      }
    }

    // Evaluate the operators.
    vtkLogStartScope(TRACE, "execute selectors");
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto selector = nodeIter->second;
      selector->Execute(inputCD, outputCD);
    }
    vtkLogEndScope("execute selectors");

    vtkLogStartScope(TRACE, "evaluate expression");
    // Now iterate again over the composite dataset and evaluate the expression to
    // combine all the insidedness arrays.
    vtkSmartPointer<vtkCompositeDataIterator> outIter;
    outIter.TakeReference(outputCD->NewIterator());
    for (outIter->GoToFirstItem(); !outIter->IsDoneWithTraversal(); outIter->GoToNextItem())
    {
      auto outputBlock = outIter->GetCurrentDataObject();
      assert(outputBlock != nullptr);
      // Evaluate the expression.
      evaluate(outputBlock);
    }
    vtkLogEndScope("evaluate expression");

    vtkLogStartScope(TRACE, "extract output");
    for (outIter->GoToFirstItem(); !outIter->IsDoneWithTraversal(); outIter->GoToNextItem())
    {
      outputCD->SetDataSet(
        outIter, extract(inputCD->GetDataSet(outIter), outIter->GetCurrentDataObject()));
    }
    vtkLogEndScope("extract output");
  }
  else
  {
    assert(output != nullptr);

    vtkSmartPointer<vtkDataObject> clone;
    clone.TakeReference(input->NewInstance());
    clone->ShallowCopy(input);

    // Evaluate the operators.
    vtkLogStartScope(TRACE, "execute selectors");
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto selector = nodeIter->second;
      selector->Execute(input, clone);
    }
    vtkLogEndScope("execute selectors");

    vtkLogStartScope(TRACE, "evaluate expression");
    evaluate(clone);
    vtkLogEndScope("evaluate expression");

    vtkLogStartScope(TRACE, "extract output");
    if (auto result = extract(input, clone))
    {
      output->ShallowCopy(result);
    }
    vtkLogEndScope("extract output");
  }

  return 1;
}

//----------------------------------------------------------------------------
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
      return vtkSmartPointer<vtkBlockSelector>::New();

    case vtkSelectionNode::USER:
    case vtkSelectionNode::SELECTIONS:
    case vtkSelectionNode::QUERY:
    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExtractSelection::ExtractElements(
  vtkDataObject* block, vtkDataObject::AttributeTypes type, vtkSignedCharArray* insidednessArray)
{
  if (this->PreserveTopology)
  {
    auto output = block->NewInstance();
    output->ShallowCopy(block);
    insidednessArray->SetName("vtkInsidedness");
    output->GetAttributesAsFieldData(type)->AddArray(insidednessArray);
    return vtkSmartPointer<vtkDataObject>::Take(output);
  }
  if (type == vtkDataObject::POINT)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    auto output = vtkUnstructuredGrid::New();
    this->ExtractSelectedPoints(input, output, insidednessArray);
    return vtkSmartPointer<vtkDataSet>::Take(output);
  }
  else if (type == vtkDataObject::CELL)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    auto output = vtkUnstructuredGrid::New();
    this->ExtractSelectedCells(input, output, insidednessArray);
    return vtkSmartPointer<vtkDataSet>::Take(output);
  }
  else if (type == vtkDataObject::ROW)
  {
    vtkTable* input = vtkTable::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    vtkTable* output = vtkTable::New();
    this->ExtractSelectedRows(input, output, insidednessArray);
    return vtkSmartPointer<vtkTable>::Take(output);
  }

  vtkDataObject* output = block->NewInstance();
  return vtkSmartPointer<vtkDataObject>::Take(output);
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedCells(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* cellInside)
{
  vtkLogScopeF(TRACE, "ExtractSelectedCells");
  const vtkIdType numPts = input->GetNumberOfPoints();
  const vtkIdType numCells = input->GetNumberOfCells();

  if (!cellInside || cellInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  assert(cellInside->GetNumberOfTuples() == numCells);

  const auto range = cellInside->GetValueRange(0);
  if (range[0] == 0 && range[1] == 0)
  {
    // all elements are being masked out, nothing to do.
    return;
  }

  // The "input" is a shallow copy of the input to this filter and hence we can
  // modify it. We add original cell ids and point ids arrays.
  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  originalPointIds->SetNumberOfTuples(numPts);
  std::iota(originalPointIds->GetPointer(0), originalPointIds->GetPointer(0) + numPts, 0);
  input->GetPointData()->AddArray(originalPointIds);

  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetName("vtkOriginalCellIds");
  originalCellIds->SetNumberOfTuples(numCells);
  std::iota(originalCellIds->GetPointer(0), originalCellIds->GetPointer(0) + numCells, 0);
  input->GetCellData()->AddArray(originalCellIds);

  vtkNew<vtkExtractCells> extractor;
  if (range[0] == 1 && range[1] == 1)
  {
    // all elements are selected, pass all data.
    // we still use the extractor since it does the data conversion, if needed
    extractor->SetExtractAllCells(true);
  }
  else
  {
    // convert insideness array to cell ids to extract.
    std::vector<vtkIdType> ids;
    ids.reserve(numCells);
    for (vtkIdType cc = 0; cc < numCells; ++cc)
    {
      if (cellInside->GetValue(cc) != 0)
      {
        ids.push_back(cc);
      }
    }
    extractor->SetAssumeSortedAndUniqueIds(true);
    extractor->SetCellIds(&ids.front(), static_cast<vtkIdType>(ids.size()));
  }

  extractor->SetInputDataObject(input);
  extractor->Update();
  output->ShallowCopy(extractor->GetOutput());
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedPoints(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* pointInside)
{
  if (!pointInside || pointInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkPointData* pd = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();

  // To copy points in a type agnostic way later
  auto pointSet = vtkPointSet::SafeDownCast(input);

  vtkNew<vtkPoints> newPts;
  if (pointSet)
  {
    newPts->SetDataType(pointSet->GetPoints()->GetDataType());
  }
  newPts->Allocate(numPts / 4, numPts);

  vtkNew<vtkIdList> newCellPts;
  newCellPts->Allocate(VTK_CELL_SIZE);

  outputPD->SetCopyGlobalIds(1);
  outputPD->CopyFieldOff("vtkOriginalPointIds");
  outputPD->CopyAllocate(pd);

  double x[3];

  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  outputPD->AddArray(originalPointIds);

  for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    signed char isInside;
    assert(ptId < pointInside->GetNumberOfValues());
    pointInside->GetTypedTuple(ptId, &isInside);
    if (isInside)
    {
      vtkIdType newPointId = -1;
      if (pointSet)
      {
        newPointId = newPts->GetNumberOfPoints();
        newPts->InsertPoints(newPointId, 1, ptId, pointSet->GetPoints());
      }
      else
      {
        input->GetPoint(ptId, x);
        newPointId = newPts->InsertNextPoint(x);
      }
      assert(newPointId >= 0);
      outputPD->CopyData(pd, ptId, newPointId);
      originalPointIds->InsertNextValue(ptId);
    }
  }

  // produce a new vtk_vertex cell for each accepted point
  for (vtkIdType ptId = 0; ptId < newPts->GetNumberOfPoints(); ++ptId)
  {
    newCellPts->Reset();
    newCellPts->InsertId(0, ptId);
    output->InsertNextCell(VTK_VERTEX, newCellPts);
  }
  output->SetPoints(newPts);
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedRows(
  vtkTable* input, vtkTable* output, vtkSignedCharArray* rowsInside)
{
  const vtkIdType numRows = input->GetNumberOfRows();
  vtkNew<vtkIdTypeArray> originalRowIds;
  originalRowIds->SetName("vtkOriginalRowIds");

  output->GetRowData()->CopyFieldOff("vtkOriginalRowIds");
  output->GetRowData()->CopyStructure(input->GetRowData());

  for (vtkIdType rowId = 0; rowId < numRows; ++rowId)
  {
    signed char isInside;
    rowsInside->GetTypedTuple(rowId, &isInside);
    if (isInside)
    {
      output->InsertNextRow(input->GetRow(rowId));
      originalRowIds->InsertNextValue(rowId);
    }
  }
  output->AddColumn(originalRowIds);
}

//----------------------------------------------------------------------------
void vtkExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
