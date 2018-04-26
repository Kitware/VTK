/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelection2.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFrustumSelector.h"
#include "vtkGraph.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionOperator.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"
#include "vtkValueSelector.h"

#include <map>

vtkStandardNewMacro(vtkExtractSelection2);

//----------------------------------------------------------------------------
vtkExtractSelection2::vtkExtractSelection2()
  : PreserveTopology(false)
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelection2::~vtkExtractSelection2()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelection2::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
  {
    // Can work with composite datasets.
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
// Needed because parent class sets output type to input type
// and we sometimes want to change it to make an UnstructuredGrid regardless of
// input type
int vtkExtractSelection2::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataSet *input = vtkDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    int passThrough = this->PreserveTopology? 1 : 0;

    vtkDataSet *output = vtkDataSet::GetData(outInfo);
    if (!output ||
      (passThrough && !output->IsA(input->GetClassName())) ||
      (!passThrough && !output->IsA("vtkUnstructuredGrid")))
    {
      vtkDataSet* newOutput = nullptr;
      if (!passThrough)
      {
        // The mesh will be modified.
        newOutput = vtkUnstructuredGrid::New();
      }
      else
      {
        // The mesh will not be modified.
        newOutput = input->NewInstance();
      }
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }

  vtkGraph *graphInput = vtkGraph::GetData(inInfo);
  if (graphInput)
  {
    // Accept graph input, but we don't produce the correct extracted
    // graph as output yet.
    return 1;
  }

  vtkTable *tableInput = vtkTable::GetData(inInfo);
  if (tableInput)
  {
    vtkTable *output = vtkTable::GetData(outInfo);
    if (!output)
    {
      output = vtkTable::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject::AttributeTypes vtkExtractSelection2::GetAttributeTypeOfSelection(vtkSelection* sel, bool& sane)
{
  vtkDataObject::AttributeTypes result = vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
  sane = true;
  for (unsigned int n = 0; n < sel->GetNumberOfNodes(); n++)
  {
    vtkSelectionNode* node = sel->GetNode(n);
    vtkInformation* nodeProperties = node->GetProperties();
    int fieldType = nodeProperties->Get(vtkSelectionNode::FIELD_TYPE());
    if (fieldType == vtkSelectionNode::CELL)
    {
      if (result == vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
      {
        result = vtkDataObject::CELL;
      }
      else if (result != vtkDataObject::CELL)
      {
        vtkErrorMacro("Selection contains mismatched attribute types!");
        sane = false;
      }
    }
    else if (fieldType == vtkSelectionNode::POINT)
    {
      int withCells = nodeProperties->Get(vtkSelectionNode::CONTAINING_CELLS());
      if (withCells)
      {
        if (result == vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
        {
          result = vtkDataObject::CELL;
        }
        else if (result != vtkDataObject::CELL)
        {
          vtkErrorMacro("Selection contains mismatched attribute types!");
          sane = false;
        }
      }
      else
      {
        if (result == vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
        {
          result = vtkDataObject::POINT;
        }
        else if (result != vtkDataObject::POINT)
        {
          vtkErrorMacro("Selection contains mismatched attribute types!");
          sane = false;
        }
      }
    }
    else if (fieldType == vtkSelectionNode::ROW)
    {
      if (result == vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
      {
        result = vtkDataObject::ROW;
      }
      else if (result != vtkDataObject::ROW)
      {
        vtkErrorMacro("Selection contains mismatched attribute types!");
        sane = false;
      }
    }
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkExtractSelection2::RequestData(
  vtkInformation* request,
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

  // If the input is a graph, don't try to handle it
  if ( vtkGraph::SafeDownCast(input) )
  {
    return 1;
  }

  // If no selection, quietly select nothing
  if (!selection)
  {
    return 1;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::SafeDownCast(output);
    cdOutput->CopyStructure(cdInput);

    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    vtkHierarchicalBoxDataIterator* hbIter =
      vtkHierarchicalBoxDataIterator::SafeDownCast(iter);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
    {
      vtkDataObject* block = iter->GetCurrentDataObject();
      if (!block)
      {
        // TODO error message? some default?
        // current filters would fail in this case anyway.
        continue;
      }
      vtkIdType flatIndex = iter->GetCurrentFlatIndex();
      vtkIdType level = -1, hbIndex = -1;
      if (hbIter)
      {
        level = hbIter->GetCurrentLevel();
        hbIndex = hbIter->GetCurrentIndex();
      }
      auto outputBlock = vtkSmartPointer<vtkDataObject>::Take(
        this->ExtractFromBlock(block, flatIndex, level, hbIndex, selection));
      if (outputBlock)
      {
        vtkDataSet* subDS = vtkDataSet::SafeDownCast(outputBlock);
        vtkTable* subTable = vtkTable::SafeDownCast(outputBlock);
        // purge empty datasets/tables from the output.
        if ((subDS  && subDS->GetNumberOfPoints() > 0) ||
          (subTable && subTable->GetNumberOfRows() > 0) ||
          (!subDS && !subTable))
        {
          cdOutput->SetDataSet(iter, outputBlock);
        }
      }
    }
  }
  else
  {
    auto outputDO = vtkSmartPointer<vtkDataObject>::Take(
      this->ExtractFromBlock(input, -1, -1, -1, selection));
    output->ShallowCopy(outputDO);
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkSelectionOperator* vtkExtractSelection2::GetOperatorForNode(vtkSelectionNode* node)
{
  vtkSelectionOperator* op = nullptr;
  int seltype = node->GetContentType();
  switch (seltype)
  {
  case vtkSelectionNode::GLOBALIDS:
  case vtkSelectionNode::PEDIGREEIDS:
  case vtkSelectionNode::VALUES:
  case vtkSelectionNode::INDICES:
    op = vtkValueSelector::New();
    break;

  case vtkSelectionNode::FRUSTUM:
    op = vtkFrustumSelector::New();
    break;

  case vtkSelectionNode::LOCATIONS:
    break;

  case vtkSelectionNode::THRESHOLDS:
    break;

  case vtkSelectionNode::BLOCKS:
    break;

  case vtkSelectionNode::USER:
      vtkErrorMacro("User-supplied, application-specific selections are not supported.");
      return nullptr;

  default:
      vtkErrorMacro("Unrecognized CONTENT_TYPE: " << seltype);
      return nullptr;
  }
  op->Initialize(node);
  return op;
}

//----------------------------------------------------------------------------
void vtkExtractSelection2::ComputeCellsContainingSelectedPoints(
    vtkDataObject* data, vtkSignedCharArray* selectedPoints, vtkSignedCharArray* selectedCells)
{
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(data);
  // TODO not 100% sure about this part... does this type of selection apply to non-datasets?
  if (!dataset)
  {
    selectedCells->FillValue(0);
    return;
  }

  vtkIdType numCells = dataset->GetNumberOfCells();

  //run through cells and accept those with any point inside
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    vtkCell* cell = dataset->GetCell(cellId);
    vtkIdList* cellPts = cell->GetPointIds();
    vtkIdType numCellPts = cell->GetNumberOfPoints();

    signed char selectedPointFound = 0;
    for (vtkIdType i = 0; i < numCellPts; ++i)
    {
      vtkIdType ptId = cellPts->GetId(i);
      signed char isInside;
      selectedPoints->GetTypedTuple(ptId, &isInside);
      if ( isInside )
      {
        selectedPointFound = 1;
        break;
      }
    }
    selectedCells->SetValue(cellId, selectedPointFound);
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSignedCharArray> vtkExtractSelection2::ComputeSelectedElements(vtkDataObject* data,
                                                              vtkIdType flatIndex,
                                                              vtkIdType level,
                                                              vtkIdType hbIndex,
                                                              vtkSelection* selection)
{
  bool typeIsConsistent;
  vtkDataObject::AttributeTypes type = this->GetAttributeTypeOfSelection(selection, typeIsConsistent);
  vtkIdType numPts = data->GetNumberOfElements(type);

  std::map<std::string, vtkSmartPointer<vtkSignedCharArray>> arrays;

  for (unsigned int n = 0; n < selection->GetNumberOfNodes(); n++)
  {
    auto inSelection = vtkSmartPointer<vtkSignedCharArray>::New();
    arrays[selection->GetNodeNameAtIndex(n)] = inSelection;
    inSelection->SetNumberOfTuples(numPts);

    vtkSelectionNode* node = selection->GetNode(n);
    vtkInformation* nodeProperties = node->GetProperties();

    if (nodeProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
        nodeProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()) != flatIndex)
    {
      inSelection->FillValue(0);
    }
    else if ((nodeProperties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
        nodeProperties->Has(vtkSelectionNode::HIERARCHICAL_INDEX())) &&
        (nodeProperties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL()) != level ||
        nodeProperties->Get(vtkSelectionNode::HIERARCHICAL_INDEX()) != hbIndex))
    {
      inSelection->FillValue(0);
    }
    else
    {
      auto op = vtkSmartPointer<vtkSelectionOperator>::Take(this->GetOperatorForNode(node));
      if (nodeProperties->Get(vtkSelectionNode::FIELD_TYPE()) == vtkSelectionNode::POINT &&
          nodeProperties->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
      {
        vtkNew<vtkSignedCharArray> pointSelection;
        pointSelection->SetNumberOfTuples(data->GetNumberOfElements(vtkDataObject::POINT));
        if (!op->ComputeSelectedElements(data, pointSelection))
        {
          // skip selecting cells if no points were selected due to error
          inSelection->FillValue(0);
        }
        else
        {
          this->ComputeCellsContainingSelectedPoints(data, pointSelection, inSelection);
        }
      }
      else
      {
        if (!op->ComputeSelectedElements(data, inSelection))
        {
          // operator cannot evaluate input
          inSelection->FillValue(0);
        }
      }
    }
  }
  return selection->Evaluate(arrays);
}


//----------------------------------------------------------------------------
vtkDataObject* vtkExtractSelection2::ExtractFromBlock(vtkDataObject* block,
                                            vtkIdType flatIndex,
                                            vtkIdType level,
                                            vtkIdType hbIndex,
                                            vtkSelection* selection)
{
  bool typeIsConsistent;
  vtkDataObject::AttributeTypes type = this->GetAttributeTypeOfSelection(selection, typeIsConsistent);
  // If the expression doesn't have a consistent association, we can't make sense of
  // the selection.
  if (!typeIsConsistent)
  {
    return nullptr;
  }
  auto insidednessArray = this->ComputeSelectedElements(block, flatIndex, level, hbIndex, selection);
  if (this->PreserveTopology)
  {
    auto output = block->NewInstance();
    output->ShallowCopy(block);
    output->GetAttributesAsFieldData(type)->AddArray(insidednessArray);
    return output;
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
    return output;
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
    return output;
  }
  else if (type == vtkDataObject::ROW)
  {
    // TODO
  }
  else
  {
    // TODO unknown type
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkExtractSelection2::ExtractSelectedCells(vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* cellInside)
{
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
void vtkExtractSelection2::ExtractSelectedPoints(vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* pointInside)
{
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  vtkNew<vtkPoints> newPts;
  newPts->Allocate(numPts/4,numPts);

  vtkNew<vtkIdList> newCellPts;
  newCellPts->Allocate(VTK_CELL_SIZE);

  outputPD->SetCopyGlobalIds(1);
  outputPD->CopyFieldOff("vtkOriginalPointIds");
  outputPD->CopyAllocate(pd);

  double x[3];

  // The new point id for each point (-1 for not in selection)
  std::vector<vtkIdType> pointMap;
  pointMap.resize(numPts);
  std::fill(pointMap.begin(), pointMap.end(), -1);

  vtkNew<vtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("vtkOriginalPointIds");
  outputPD->AddArray(originalPointIds);

  for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    pointMap[ptId] = -1;

    signed char isInside;
    pointInside->GetTypedTuple(ptId, &isInside);
    if (isInside)
    {
      input->GetPoint(ptId, x);
      vtkIdType newPointId = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,newPointId);
      originalPointIds->InsertNextValue(ptId);
      pointMap[ptId] = newPointId;
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
void vtkExtractSelection2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
