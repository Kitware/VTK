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

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkBlockSelector.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSet.h"
#include "vtkFrustumSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLocationSelector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelector.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkValueSelector.h"

#include <map>
#include <memory>
#include <set>

vtkStandardNewMacro(vtkExtractSelection);
//----------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
  : PreserveTopology(false)
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelection::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
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
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
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

  if (!outputDO || ! outputDO->IsTypeOf(inputDO->GetClassName()))
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
vtkDataObject::AttributeTypes vtkExtractSelection::GetAttributeTypeOfSelection(vtkSelection* sel, bool& sane)
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
    if (!array)
    {
      return;
    }

    const int n = array->GetNumberOfTuples();
    for (int i = 0; i < n; ++i)
    {
      array->SetValue(i, array->GetValue(i) * -1 + 1);
    }
  }
}

//----------------------------------------------------------------------------
int vtkExtractSelection::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  // If no input, error
  if (!input)
  {
    vtkErrorMacro(<<"No input specified");
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
  std::map< std::string, vtkSmartPointer< vtkSelector > > selectors;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    auto name = selection->GetNodeNameAtIndex(cc);

    if (auto anOperator = this->NewSelectionOperator(
          static_cast<vtkSelectionNode::SelectionContent>(node->GetContentType())))
    {
      anOperator->Initialize(node, name.c_str());
      selectors[name] = anOperator;
    }
    else
    {
      vtkWarningMacro("Unhandled selection node with content type : " << node->GetContentType());
    }
  }

  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputCD = vtkCompositeDataSet::SafeDownCast(output);
    assert(outputCD != nullptr);
    outputCD->CopyStructure(inputCD);

    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCD->NewIterator());

    // Initialize the output composite dataset to have blocks with the same type
    // as the input. We don't need to copy points or cell information, we just need
    // a convenient place to store the insidedness array.
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      auto blockInput = inIter->GetCurrentDataObject();
      if (blockInput)
      {
        outputCD->SetDataSet(inIter, blockInput->NewInstance());
      }
    }

    // Evaluate the operators.
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto name = nodeIter->first;
      auto selector = nodeIter->second;
      selector->ComputeSelectedElements(inputCD, outputCD);
    }

    // Now iterate again over the composite dataset and evaluate the expression to
    // combine all the insidedness arrays.
    for (inIter->GoToFirstItem(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      vtkDataObject* inputBlock = inputCD->GetDataSet(inIter);
      vtkDataObject* outputBlock = outputCD->GetDataSet(inIter);
      assert(outputBlock != nullptr);

      // Iterate over operators and set up a map from selection node name to insidedness
      // array.
      std::map<std::string, vtkSignedCharArray*> arrayMap;
      for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
      {
        auto name = nodeIter->first;
        auto fieldData = outputBlock->GetAttributes(assoc);
        if (!fieldData)
        {
          arrayMap[name] = nullptr;
          continue;
        }
        auto array = fieldData->GetArray(name.c_str());
        auto insidednessArray = vtkSignedCharArray::SafeDownCast(array);

        auto node = selection->GetNode(name.c_str());
        if (node->GetProperties()->Has(vtkSelectionNode::INVERSE()) &&
            node->GetProperties()->Get(vtkSelectionNode::INVERSE()))
        {
          InvertSelection(insidednessArray);
        }
        arrayMap[name] = insidednessArray;
      }

      // Evaluate the map of insidedness arrays
      auto blockInsidedness = selection->Evaluate(arrayMap);
      auto resultDO = this->ExtractElements(inputBlock, assoc, blockInsidedness);
      outputCD->GetDataSet(inIter)->Delete();
      if (resultDO && resultDO->GetNumberOfElements(assoc) > 0)
      {
        outputCD->SetDataSet(inIter, resultDO);
      }
      else
      {
        outputCD->SetDataSet(inIter, nullptr);
      }
    }
  }
  else
  {
    std::map<std::string, vtkSignedCharArray*> arrayMap;
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto name = nodeIter->first;
      auto selector = nodeIter->second;
      selector->ComputeSelectedElements(input, output);

      // Set up a map from selection node name to insidedness array.
      auto array = output->GetAttributes(assoc)->GetArray(name.c_str());
      auto insidednessArray = vtkSignedCharArray::SafeDownCast(array);

      auto node = selection->GetNode(name.c_str());
      if (node->GetProperties()->Has(vtkSelectionNode::INVERSE()) &&
          node->GetProperties()->Get(vtkSelectionNode::INVERSE()))
      {
        InvertSelection(insidednessArray);
      }
      arrayMap[name] = insidednessArray;
    }

    // Evaluate the map of insidedness arrays
    auto insidedness = selection->Evaluate(arrayMap);
    auto result = this->ExtractElements(input, assoc, insidedness);
    output->ShallowCopy(result);
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
      return nullptr;

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
  vtkDataSet* input,
  vtkUnstructuredGrid* output,
  vtkSignedCharArray* cellInside)
{
  if (!cellInside || cellInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  vtkNew<vtkPoints> newPts;
  newPts->Allocate(numPts/4,numPts);

  outputPD->SetCopyGlobalIds(1);
  outputPD->CopyFieldOff("vtkOriginalPointIds");
  outputPD->CopyAllocate(pd);

  outputCD->SetCopyGlobalIds(1);
  outputCD->CopyFieldOff("vtkOriginalCellIds");
  outputCD->CopyAllocate(cd);

  double x[3];

  vtkNew<vtkIdList> newCellPts;
  newCellPts->Allocate(VTK_CELL_SIZE);

  // The new point id for each point (-1 for not in selection)
  std::vector<vtkIdType> pointMap;
  pointMap.resize(numPts);
  std::fill(pointMap.begin(), pointMap.end(), -1);

  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  outputPD->AddArray(originalPointIds);

  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetName("vtkOriginalCellIds");
  outputCD->AddArray(originalCellIds);

  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    // 1 means selected, 0 means not selected
    signed char isInside;
    assert(cellId < cellInside->GetNumberOfValues());
    cellInside->GetTypedTuple(cellId, &isInside);
    if (isInside)
    {
      vtkCell* cell = input->GetCell(cellId);
      vtkIdList* cellPts = cell->GetPointIds();
      vtkIdType numCellPts = cell->GetNumberOfPoints();
      newCellPts->Reset();
        for (vtkIdType i = 0; i < numCellPts; ++i)
        {
          vtkIdType ptId = cellPts->GetId(i);
          vtkIdType newPointId = pointMap[ptId];
          if (newPointId < 0)
          {
            input->GetPoint(ptId, x);
            newPointId = newPts->InsertNextPoint(x);
            outputPD->CopyData(pd,ptId,newPointId);
            originalPointIds->InsertNextValue(ptId);
            pointMap[ptId] = newPointId;
          }
          newCellPts->InsertId(i,newPointId);
        }
        // special handling for polyhedron cells
        if (vtkUnstructuredGrid::SafeDownCast(input) &&
            cell->GetCellType() == VTK_POLYHEDRON)
        {
          newCellPts->Reset();
          vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newCellPts);
          vtkUnstructuredGrid::ConvertFaceStreamPointIds(newCellPts, &pointMap[0]);
        }
        vtkIdType newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
        outputCD->CopyData(cd,cellId,newCellId);
        originalCellIds->InsertNextValue(cellId);
    }
  }
  output->SetPoints(newPts);
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedPoints(
  vtkDataSet* input,
  vtkUnstructuredGrid* output,
  vtkSignedCharArray* pointInside)
{
  if (!pointInside || pointInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkPointData *pd = input->GetPointData();
  vtkPointData *outputPD = output->GetPointData();

  vtkNew<vtkPoints> newPts;
  newPts->Allocate(numPts/4,numPts);

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
      input->GetPoint(ptId, x);
      vtkIdType newPointId = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,newPointId);
      originalPointIds->InsertNextValue(ptId);
    }
  }

  //produce a new vtk_vertex cell for each accepted point
  for (vtkIdType ptId = 0; ptId < newPts->GetNumberOfPoints(); ++ptId)
  {
    newCellPts->Reset();
    newCellPts->InsertId(0, ptId);
    output->InsertNextCell(VTK_VERTEX, newCellPts);
  }
  output->SetPoints(newPts);
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ExtractSelectedRows(vtkTable* input, vtkTable* output, vtkSignedCharArray* rowsInside)
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

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkExtractSelection::SetShowBounds(bool)
{
  VTK_LEGACY_BODY(vtkExtractSelection::SetShowBounds, "VTK 8.2");
}
//----------------------------------------------------------------------------
bool vtkExtractSelection::GetShowBounds()
{
  VTK_LEGACY_BODY(vtkExtractSelection::GetShowBounds, "VTK 8.2");
  return false;
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ShowBoundsOn()
{
  VTK_LEGACY_BODY(vtkExtractSelection::ShowBoundsOn, "VTK 8.2");
}

//----------------------------------------------------------------------------
void vtkExtractSelection::ShowBoundsOff()
{
  VTK_LEGACY_BODY(vtkExtractSelection::ShowBoundsOff, "VTK 8.2");
}

//----------------------------------------------------------------------------
void vtkExtractSelection::SetUseProbeForLocations(bool)
{
  VTK_LEGACY_BODY(vtkExtractSelection::SetUseProbeForLocations, "VTK 8.2");
}

//----------------------------------------------------------------------------
bool vtkExtractSelection::GetUseProbeForLocations()
{
  VTK_LEGACY_BODY(vtkExtractSelection::GetUseProbeForLocations, "VTK 8.2");
  return false;
}

//----------------------------------------------------------------------------
void vtkExtractSelection::UseProbeForLocationsOn()
{
  VTK_LEGACY_BODY(vtkExtractSelection::UseProbeForLocationsOn, "VTK 8.2");
}

//----------------------------------------------------------------------------
void vtkExtractSelection::UseProbeForLocationsOff()
{
  VTK_LEGACY_BODY(vtkExtractSelection::UseProbeForLocationsOff, "VTK 8.2");
}
#endif
