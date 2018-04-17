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
#include "vtkGraph.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractSelection2);

//----------------------------------------------------------------------------
vtkExtractSelection2::vtkExtractSelection2()
  : PreserveTopology(false)
{
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
}

vtkSignedCharArray* vtkExtractSelection2::ComputePointsInside(vtkDataObject* block,
                                                              vtkIdType flatIndex,
                                                              vtkIdType level,
                                                              vtkIdType hbIndex,
                                                              vtkSelection* selection)
{
  return nullptr;
}

vtkSignedCharArray* vtkExtractSelection2::ComputeCellsInside(vtkDataObject* block,
                                                              vtkIdType flatIndex,
                                                              vtkIdType level,
                                                              vtkIdType hbIndex,
                                                              vtkSelection* selection)
{
  return nullptr;
}

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
  if (type == vtkDataObject::POINT)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(block);
    if (!input)
    {
      vtkErrorMacro("Point-based selection requested but input is not a vtkDataSet");
    }
    auto insidednessArray = this->ComputePointsInside(input, flatIndex, level, hbIndex, selection);
    if (this->PreserveTopology)
    {
      auto output = input->NewInstance();
      output->ShallowCopy(input);
      output->GetPointData()->AddArray(insidednessArray);
      return output;
    }
    else
    {
      bool withCells = true; // TODO
      auto output = vtkUnstructuredGrid::New();
      this->ExtractSelectedPoints(input, output, insidednessArray, withCells);
      return output;
    }
  }
  else if (type == vtkDataObject::CELL)
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(block);
    if (!input)
    {
      vtkErrorMacro("Cell-based selection requested but input is not a vtkDataSet");
    }
    auto insidednessArray = this->ComputeCellsInside(input, flatIndex, level, hbIndex, selection);
    if (this->PreserveTopology)
    {
      auto output = input->NewInstance();
      output->ShallowCopy(input);
      output->GetCellData()->AddArray(insidednessArray);
      return output;
    }
    else
    {
      auto output = vtkUnstructuredGrid::New();
      this->ExtractSelectedCells(input, output, insidednessArray);
      return output;
    }
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
void vtkExtractSelection2::ExtractSelectedPoints(vtkDataSet* input, vtkUnstructuredGrid* output, vtkSignedCharArray* pointInside, bool extractWithContainingCells)
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

  if (extractWithContainingCells)
  {
    outputCD->SetCopyGlobalIds(1);
    outputCD->CopyFieldOff("vtkOriginalCellIds");
    outputCD->CopyAllocate(cd);

    vtkNew<vtkIdTypeArray> originalCellIds;
    originalCellIds->SetNumberOfComponents(1);
    originalCellIds->SetName("vtkOriginalCellIds");
    outputCD->AddArray(originalCellIds);

    //run through cells and accept those with any point inside
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      vtkCell* cell = input->GetCell(cellId);
      vtkIdList* cellPts = cell->GetPointIds();
      vtkIdType numCellPts = cell->GetNumberOfPoints();
      newCellPts->Reset();

      bool isect = false;
      for (vtkIdType i = 0; i < numCellPts; ++i)
      {
        vtkIdType ptId = cellPts->GetId(i);
        signed char isInside;
        pointInside->GetTypedTuple(ptId, &isInside);
        if ( isInside )
        {
          isect = true;
          break;
        }
      }
      if (isect)
      {
        for (vtkIdType i = 0; i < numCellPts; ++i)
        {
          vtkIdType ptId = cellPts->GetId(i);
          vtkIdType newPointId = pointMap[ptId];
          if (newPointId < 0)
          {
            //we haven't encountered it before, add it and remember
            input->GetPoint(ptId,x);
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
  }
  else
  {
    //produce a new vtk_vertex cell for each accepted point
    for (vtkIdType ptId = 0; ptId < newPts->GetNumberOfPoints(); ++ptId)
    {
      newCellPts->Reset();
      newCellPts->InsertId(0, ptId);
      output->InsertNextCell(VTK_VERTEX, newCellPts);
    }
  }
  output->SetPoints(newPts);
}

//----------------------------------------------------------------------------
void vtkExtractSelection2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
