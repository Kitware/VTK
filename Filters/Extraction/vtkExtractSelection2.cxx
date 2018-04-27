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

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
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
#include "vtkSelectionOperator.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkValueSelector.h"

#include <map>
#include <set>

namespace
{
// This helps us handle either calling `vtkSelectionOperator::ComputeSelectedElements`
// or handling block selection conveniently.
class SelectionEvaluator
{
protected:
  vtkSmartPointer<vtkSelectionNode> Node;

public:
  SelectionEvaluator(vtkSelectionNode* node)
    : Node(node)
  {
  }
  virtual ~SelectionEvaluator() {}

  // ComputeSelectedElements to generate a insidedness array. May return nullptr
  // to indicate nothing is "inside".
  virtual vtkSmartPointer<vtkSignedCharArray> ComputeSelectedElements(vtkDataObject* block,
    unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex) = 0;

protected:
  // Subclasses can call this to check if the block should be skipped.
  bool SkipBlock(unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex)
  {
    auto propertes = this->Node->GetProperties();
    if (propertes->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
      propertes->Get(vtkSelectionNode::COMPOSITE_INDEX()) != compositeIndex)
    {
      return true;
    }

    if (propertes->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      static_cast<unsigned int>(propertes->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())) != amrLevel)
    {
      return true;
    }

    if (propertes->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
      static_cast<unsigned int>(propertes->Get(vtkSelectionNode::HIERARCHICAL_INDEX())) != amrIndex)
    {
      return true;
    }
    return false;
  }

  // Creates a new insideness array with the given number of elements.
  vtkSmartPointer<vtkSignedCharArray> CreateInsidenessArray(vtkIdType numElems)
  {
    auto darray = vtkSmartPointer<vtkSignedCharArray>::New();
    darray->SetName("vtkInsidedness");
    darray->SetNumberOfComponents(1);
    darray->SetNumberOfTuples(numElems);
    return darray;
  }
};

// A standard SelectionEvaluator that uses a vtkSelectionOperator to generate
// the insidedness array.
class SelectionEvaluatorWithOperator : public SelectionEvaluator
{
  vtkSmartPointer<vtkSelectionOperator> Operator;

public:
  SelectionEvaluatorWithOperator(vtkSelectionOperator* op, vtkSelectionNode* node)
    : SelectionEvaluator(node)
    , Operator(op)
  {
  }

  vtkSmartPointer<vtkSignedCharArray> ComputeSelectedElements(vtkDataObject* block,
    unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex) override
  {
    const int assoc =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());
    vtkIdType numElems = block->GetNumberOfElements(assoc);
    if (numElems == 0)
    {
      return nullptr;
    }

    auto insidednessArray = this->CreateInsidenessArray(numElems);
    if (this->SkipBlock(compositeIndex, amrLevel, amrIndex) == true ||
      this->Operator->ComputeSelectedElements(block, insidednessArray) == false)
    {
      insidednessArray->FillValue(0);
    }

    // if selecting cells containing points, we need to map the selected points
    // to selected cells.
    // NOTE: is this correct behavior when "inverse" is true?
    auto selProps = this->Node->GetProperties();
    if (assoc == vtkDataObject::POINT && selProps->Has(vtkSelectionNode::CONTAINING_CELLS()) &&
      selProps->Get(vtkSelectionNode::CONTAINING_CELLS()) == 1)
    {
      return this->ComputeCellsContainingSelectedPoints(block, insidednessArray);
    }
    else
    {
      return insidednessArray;
    }
  }

private:
  vtkSmartPointer<vtkSignedCharArray> ComputeCellsContainingSelectedPoints(
    vtkDataObject* data, vtkSignedCharArray* selectedPoints)
  {
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(data);
    if (!dataset)
    {
      return nullptr;
    }

    const vtkIdType numCells = dataset->GetNumberOfCells();
    auto selectedCells = this->CreateInsidenessArray(numCells);

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
};

// SelectionEvaluator for blocks since the vtkSelectionOperator API doesn't
// really support extracting blocks.
class SelectionEvaluatorForBlocks : public SelectionEvaluator
{
  // this functor are only needed for vtkArrayDispatch to correctly fill it up.
  // otherwise, it'd simply be a set.
  class CompositeIdsT : public std::set<unsigned int>
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      VTK_ASSUME(array->GetNumberOfComponents() == 1);
      vtkDataArrayAccessor<ArrayType> accessor(array);
      for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
      {
        this->insert(static_cast<unsigned int>(accessor.Get(cc, 0)));
      }
    }
  };

  // this functor are only needed for vtkArrayDispatch to correctly fill it up.
  // otherwise, it'd simply be a set.
  class AMRIdsT : public std::set<std::pair<unsigned int, unsigned int> >
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      VTK_ASSUME(array->GetNumberOfComponents() == 2);
      vtkDataArrayAccessor<ArrayType> accessor(array);
      for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
      {
        this->insert(
          std::pair<unsigned int, unsigned int>(static_cast<unsigned int>(accessor.Get(cc, 0)),
            static_cast<unsigned int>(accessor.Get(cc, 1))));
      }
    }
  };

  CompositeIdsT CompositeIds;
  AMRIdsT AMRIds;

public:
  SelectionEvaluatorForBlocks(vtkSelectionNode* node)
    : SelectionEvaluator(node)
  {
    assert(node->GetContentType() == vtkSelectionNode::BLOCKS);
    vtkDataArray* selectionList = vtkDataArray::SafeDownCast(node->GetSelectionList());
    if (selectionList->GetNumberOfComponents() == 2)
    {
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            selectionList, this->AMRIds))
      {
        vtkGenericWarningMacro("SelectionList of unexpected type!");
      }
    }
    else if (selectionList->GetNumberOfComponents() == 1)
    {
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            selectionList, this->CompositeIds))
      {
        vtkGenericWarningMacro("SelectionList of unexpected type!");
      }
    }
  }

  vtkSmartPointer<vtkSignedCharArray> ComputeSelectedElements(vtkDataObject* block,
    unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex) override
  {
    const int assoc =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());
    vtkIdType numElems = block->GetNumberOfElements(assoc);
    if (numElems == 0)
    {
      return nullptr;
    }

    auto insidednessArray = this->CreateInsidenessArray(numElems);

    bool is_selected = (this->CompositeIds.find(compositeIndex) != this->CompositeIds.end()) ||
      (this->AMRIds.find(std::pair<unsigned int, unsigned int>(amrLevel, amrIndex)) !=
                         this->AMRIds.end());
    if (this->SkipBlock(compositeIndex, amrLevel, amrIndex) == true || !is_selected)
    {
      insidednessArray->FillValue(0);
    }
    else
    {
      insidednessArray->FillValue(1);
    }
    return insidednessArray;
  }
};

class MaoOfSelectionEvaluators : public std::map<std::string, std::shared_ptr<SelectionEvaluator> >
{
public:
  vtkSmartPointer<vtkSignedCharArray> ComputeSelectedElements(vtkSelection* selection,
    vtkDataObject* block, unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex)
  {
    std::map<std::string, vtkSmartPointer<vtkSignedCharArray> > arrays;
    for (auto& pair : *this)
    {
      arrays[pair.first] =
        pair.second->ComputeSelectedElements(block, compositeIndex, amrLevel, amrIndex);
    }
    return selection->Evaluate(arrays);
  }
};
}

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
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
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

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PreserveTopology)
  {
    // when PreserveTopology is ON, we're preserve input data type.
    if (outputDO == nullptr || !outputDO - IsA(inputDO->GetClassName()))
    {
      outputDO = inputDO->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(inputDO))
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

  if (vtkTable* inputT = vtkTable::SafeDownCast(inputDO))
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

  if (vtkDataSet* inputDS = vtkDataSet::SafeDownCast(inputDO))
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

  vtkErrorMacro("Not sure what type of output to create!");
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

  // If no selection, quietly select nothing
  if (!selection)
  {
    return 1;
  }

  // check for select FieldType consistency right here and return failure
  // if they are not consistent.
  bool sane;
  const auto assoc = this->GetAttributeTypeOfSelection(selection, sane);
  if (!sane)
  {
    vtkErrorMacro("Selection has selection nodes with inconsistent field types.");
    return 0;
  }

  // create operators for each of vtkSelectionNode instances and initialize
  // them.
  MaoOfSelectionEvaluators operators;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    auto name = selection->GetNodeNameAtIndex(cc);
    if (auto anOperator = this->NewSelectionOperator(
          static_cast<vtkSelectionNode::SelectionContent>(node->GetContentType())))
    {
      anOperator->Initialize(node);
      operators[name] = std::make_shared<SelectionEvaluatorWithOperator>(anOperator, node);
    }
    else if (node->GetContentType() == vtkSelectionNode::BLOCKS)
    {
      operators[name] = std::make_shared<SelectionEvaluatorForBlocks>(node);
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

    auto inIterAMR = vtkUniformGridAMRDataIterator::SafeDownCast(inIter);
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      vtkDataObject* blockDO = inIter->GetCurrentDataObject();
      assert(blockDO != nullptr);

      unsigned int compositeIndex = inIter->GetCurrentFlatIndex();
      unsigned int amrLevel = inIterAMR ? inIterAMR->GetCurrentLevel() : VTK_UNSIGNED_INT_MAX;
      unsigned int amrIndex = inIterAMR ? inIterAMR->GetCurrentIndex() : VTK_UNSIGNED_INT_MAX;

      auto insidednessArray =
        operators.ComputeSelectedElements(selection, blockDO, compositeIndex, amrLevel, amrIndex);
      auto resultDO =
        insidednessArray ? this->ExtractElements(blockDO, assoc, insidednessArray) : nullptr;
      if (resultDO && resultDO->GetNumberOfElements(assoc) > 0)
      {
        // purge empty datasets/tables from the output.
        outputCD->SetDataSet(inIter, resultDO);
      }
    }
  }
  else
  {
    auto insidednessArray = operators.ComputeSelectedElements(
      selection, input, VTK_UNSIGNED_INT_MAX, VTK_UNSIGNED_INT_MAX, VTK_UNSIGNED_INT_MAX);
    if (insidednessArray)
    {
      auto resultDO = this->ExtractElements(input, assoc, insidednessArray);
      output->ShallowCopy(resultDO);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSelectionOperator> vtkExtractSelection2::NewSelectionOperator(
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
      return nullptr;
      break;

    case vtkSelectionNode::USER:
      return nullptr;

    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExtractSelection2::ExtractElements(
  vtkDataObject* block, vtkDataObject::AttributeTypes type, vtkSignedCharArray* insidednessArray)
{
  if (this->PreserveTopology)
  {
    auto output = block->NewInstance();
    output->ShallowCopy(block);
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
