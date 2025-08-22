// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisAlignedReflectionFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkFieldData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridScales.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkReflectionUtilities.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformHyperTreeGrid.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkAxisAlignedReflectionFilter);

namespace
{
//------------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> CreatePartitionedDataSet(vtkDataObject* dataObject)
{
  vtkNew<vtkPartitionedDataSet> parts;
  parts->SetNumberOfPartitions(1);
  parts->SetPartition(0, dataObject);
  return parts;
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);

  if (inputDS)
  {
    inputDS->GetBounds(bounds);
  }
  else if (inputCD)
  {
    inputCD->GetBounds(bounds);
  }
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::AddPartitionedDataSet(
  vtkPartitionedDataSetCollection* outputPDSC, vtkDataObject* dObj, vtkInformation* inputMetadata,
  const int nodeId, const bool isParentMultiblock, const bool isInputCopy)
{
  vtkDataAssembly* outputHierarchy = outputPDSC->GetDataAssembly();
  outputPDSC->SetPartitionedDataSet(this->PartitionIndex, ::CreatePartitionedDataSet(dObj));
  std::string nodeName;
  int& count = isInputCopy ? this->InputCount : this->ReflectionCount;
  const std::string nodeNamePrefix = isInputCopy ? "Input_" : "Reflection_";
  const std::string nodeNamePrefixMeta = isInputCopy ? "" : "Reflection_";
  if (inputMetadata && inputMetadata->Has(vtkCompositeDataSet::NAME()))
  {
    nodeName = nodeNamePrefixMeta +
      outputHierarchy->MakeValidNodeName(inputMetadata->Get(vtkCompositeDataSet::NAME()));
  }
  else
  {
    nodeName = nodeNamePrefix + std::to_string(count++);
  }

  outputPDSC->GetMetaData(this->PartitionIndex)->Set(vtkCompositeDataSet::NAME(), nodeName.c_str());
  if (isParentMultiblock)
  {
    int dsNodeId = outputHierarchy->AddNode(nodeName.c_str(), nodeId);
    outputHierarchy->AddDataSetIndex(dsNodeId, this->PartitionIndex);
  }
  else
  {
    outputHierarchy->AddDataSetIndex(nodeId, this->PartitionIndex);
  }
  this->PartitionIndex++;
}

//------------------------------------------------------------------------------
bool vtkAxisAlignedReflectionFilter::ProcessComposite(vtkPartitionedDataSetCollection* outputPDSC,
  vtkCompositeDataSet* inputCD, double bounds[6], int inputNodeId, int reflectionNodeId)
{

  vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(inputCD);
  if (!inputTree)
  {
    vtkErrorMacro("Failed convert composite to data object tree.");
    return false;
  }

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(inputCD);

  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(inputTree->NewTreeIterator());
  iter->SetVisitOnlyLeaves(false);
  iter->SetTraverseSubTree(false);
  iter->SetSkipEmptyNodes(false);

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (this->CheckAbort())
    {
      break;
    }

    vtkInformation* inputMetadata =
      inputTree->HasMetaData(iter) ? inputTree->GetMetaData(iter) : nullptr;

    vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (cds)
    {
      vtkDataAssembly* outputHierarchy = outputPDSC->GetDataAssembly();
      std::string compositeNodeName = "Composite";
      if (inputMetadata && inputMetadata->Has(vtkCompositeDataSet::NAME()))
      {
        compositeNodeName =
          outputHierarchy->MakeValidNodeName(inputMetadata->Get(vtkCompositeDataSet::NAME()));
      }

      int compositeInputNodeId = -1;
      if (this->CopyInput)
      {
        compositeInputNodeId = outputHierarchy->AddNode(compositeNodeName.c_str(), inputNodeId);
      }
      int compositeReflectionNodeId =
        outputHierarchy->AddNode(compositeNodeName.c_str(), reflectionNodeId);

      if (!ProcessComposite(
            outputPDSC, cds, bounds, compositeInputNodeId, compositeReflectionNodeId))
      {
        vtkErrorMacro("Failed to process composite dataset " << cds->GetClassName());
        return false;
      }
      continue;
    }

    // Handle empty nodes
    if (iter->GetCurrentDataObject() == nullptr)
    {
      if (this->CopyInput)
      {
        this->AddPartitionedDataSet(
          outputPDSC, nullptr, inputMetadata, inputNodeId, mb != nullptr, true);
      }
      this->AddPartitionedDataSet(
        outputPDSC, nullptr, inputMetadata, reflectionNodeId, mb != nullptr, false);
      continue;
    }

    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (!ds && !htg)
    {
      vtkErrorMacro("Unhandled data type.");
      return false;
    }

    vtkDataObject* dObj = vtkDataObject::SafeDownCast(iter->GetCurrentDataObject());
    vtkSmartPointer<vtkDataObject> outputObj = dObj->NewInstance();

    if (this->CopyInput)
    {
      vtkSmartPointer<vtkDataObject> inputCopy = dObj->NewInstance();
      inputCopy->ShallowCopy(dObj);
      this->AddPartitionedDataSet(
        outputPDSC, inputCopy, inputMetadata, inputNodeId, mb != nullptr, true);
      inputCopy->Delete();
    }

    if (!this->ProcessLeaf(dObj, outputObj, bounds))
    {
      outputObj->Delete();
      vtkErrorMacro("Failed to process data object " << dObj->GetClassName());
      return false;
    }

    this->AddPartitionedDataSet(
      outputPDSC, outputObj, inputMetadata, reflectionNodeId, mb != nullptr, false);
    outputObj->Delete();
  }
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkAxisAlignedReflectionFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ReflectionPlane != nullptr)
  {
    vtkMTimeType planeMTime = this->ReflectionPlane->GetMTime();
    return (planeMTime > mTime ? planeMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
int vtkAxisAlignedReflectionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPartitionedDataSetCollection* outputPDSC =
    vtkPartitionedDataSetCollection::GetData(outputVector, 0);

  vtkNew<vtkDataAssembly> outputHierarchy;
  outputPDSC->SetDataAssembly(outputHierarchy);
  int rootId = outputHierarchy->GetRootNode();

  outputHierarchy->SetRootNodeName("Root");

  int inputNodeId = -1;
  if (this->CopyInput)
  {
    inputNodeId = outputHierarchy->AddNode("Input", rootId);
    if (inputNodeId == -1)
    {
      vtkErrorMacro("Unable to a add new child node for node " + std::to_string(rootId));
      return 0;
    }
  }

  int reflectionNodeId = outputHierarchy->AddNode("Reflection", rootId);
  if (reflectionNodeId == -1)
  {
    vtkErrorMacro("Unable to a add new child node for node " + std::to_string(rootId));
    return 0;
  }

  vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkHyperTreeGrid* inputHtg = vtkHyperTreeGrid::GetData(inputVector[0], 0);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);

  if (inputDS || inputHtg)
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    if (this->CopyInput)
    {
      vtkSmartPointer<vtkDataObject> inputCopy = inputDO->NewInstance();
      inputCopy->ShallowCopy(inputDO);
      outputPDSC->SetPartitionedDataSet(
        this->PartitionIndex, ::CreatePartitionedDataSet(inputCopy));
      outputPDSC->GetMetaData(this->PartitionIndex)->Set(vtkCompositeDataSet::NAME(), "Input");
      outputHierarchy->AddDataSetIndex(inputNodeId, this->PartitionIndex);
      inputCopy->Delete();
      this->PartitionIndex++;
    }

    double bounds[6];
    this->ComputeBounds(inputDO, bounds);
    vtkSmartPointer<vtkDataObject> outputDO = inputDO->NewInstance();
    if (this->ProcessLeaf(inputDO, outputDO, bounds))
    {
      outputPDSC->SetPartitionedDataSet(this->PartitionIndex, ::CreatePartitionedDataSet(outputDO));
      outputPDSC->GetMetaData(this->PartitionIndex)->Set(vtkCompositeDataSet::NAME(), "Reflection");
      outputHierarchy->AddDataSetIndex(reflectionNodeId, this->PartitionIndex);
      outputDO->Delete();
    }
    else
    {
      outputDO->Delete();
      vtkErrorMacro("Failed to process data object " << inputDO->GetClassName());
      return 0;
    }
  }
  else if (inputCD)
  {
    double bounds[6];
    this->ComputeBounds(inputCD, bounds);

    if (!ProcessComposite(outputPDSC, inputCD, bounds, inputNodeId, reflectionNodeId))
    {
      vtkErrorMacro("Failed to process composite dataset " << inputCD->GetClassName());
      return 0;
    }
  }
  else
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    vtkErrorMacro("Unhandled data type: " << inputDO->GetClassName());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::FindAndReflectArrays(vtkDataSet* input, vtkDataSet* output,
  int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9])
{
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  int numPts = input->GetNumberOfPoints();
  int numCells = input->GetNumberOfCells();

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inPD, this->ReflectAllInputArrays);
  for (vtkIdType i = 0; i < numPts; i++)
  {
    vtkReflectionUtilities::ReflectReflectableArrays(
      reflectableArrays, inPD, outPD, i, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir, i);
  }
  reflectableArrays.clear();
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inCD, this->ReflectAllInputArrays);
  for (vtkIdType i = 0; i < numCells; i++)
  {
    vtkReflectionUtilities::ReflectReflectableArrays(
      reflectableArrays, inCD, outCD, i, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir, i);
  }
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessImageData(vtkImageData* input, vtkImageData* output,
  double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9])
{
  output->DeepCopy(input);
  int extent[6];
  input->GetExtent(extent);
  int dims[3];
  input->GetDimensions(dims);

  output->SetOrigin(input->GetOrigin()[0] + constant[0], input->GetOrigin()[1] + constant[1],
    input->GetOrigin()[2] + constant[2]);

  output->SetDirectionMatrix(mirrorDir[0], 0, 0, 0, mirrorDir[1], 0, 0, 0, mirrorDir[2]);

  this->FindAndReflectArrays(input, output, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessRectilinearGrid(vtkRectilinearGrid* input,
  vtkRectilinearGrid* output, double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6],
  int mirrorTensorDir[9])
{
  output->SetExtent(input->GetExtent());
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  outPD->CopyAllOn();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllOn();
  outCD->CopyAllocate(inCD);

  int dims[3];
  input->GetDimensions(dims);
  output->SetDimensions(dims);
  vtkDataArray* inXArray = input->GetXCoordinates();
  vtkDataArray* inYArray = input->GetYCoordinates();
  vtkDataArray* inZArray = input->GetZCoordinates();
  vtkDataArray* outXArray = output->GetXCoordinates();
  vtkDataArray* outYArray = output->GetYCoordinates();
  vtkDataArray* outZArray = output->GetZCoordinates();
  outXArray->SetNumberOfTuples(inXArray->GetNumberOfTuples());
  outYArray->SetNumberOfTuples(inYArray->GetNumberOfTuples());
  outZArray->SetNumberOfTuples(inZArray->GetNumberOfTuples());
  for (int i = 0; i < dims[0]; i++)
  {
    double* tp = inXArray->GetTuple(i);
    double newTp = tp[0] * mirrorDir[0] + constant[0];
    outXArray->SetTuple(dims[0] - i - 1, &newTp);
  }

  for (int i = 0; i < dims[1]; i++)
  {
    double* tp = inYArray->GetTuple(i);
    double newTp = tp[0] * mirrorDir[1] + constant[1];
    outYArray->SetTuple(dims[1] - i - 1, &newTp);
  }

  for (int i = 0; i < dims[2]; i++)
  {
    double* tp = inZArray->GetTuple(i);
    double newTp = tp[0] * mirrorDir[2] + constant[2];
    outZArray->SetTuple(dims[2] - i - 1, &newTp);
  }

  // The copy of the data could be avoided by using an implicit array
  int numPts = input->GetNumberOfPoints();
  int numCells = input->GetNumberOfCells();

  for (int i = 0; i < numPts; i++)
  {
    outPD->CopyData(inPD, i, numPts - i - 1);
  }
  for (int i = 0; i < numCells; i++)
  {
    outCD->CopyData(inCD, i, numCells - i - 1);
  }

  this->FindAndReflectArrays(input, output, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessExplicitStructuredGrid(vtkExplicitStructuredGrid* input,
  vtkExplicitStructuredGrid* output, double constant[3], int mirrorDir[3],
  int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9])
{
  output->SetExtent(input->GetExtent());

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  outPoints->Allocate(numPts);
  outPD->CopyAllOn();
  outPD->CopyAllocate(inPD);

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inPD, this->ReflectAllInputArrays);

  for (vtkIdType i = 0; i < numPts; i++)
  {
    double point[3];
    if (this->CheckAbort())
    {
      break;
    }
    input->GetPoint(i, point);
    vtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inPD, outPD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, ptId);
  }
  output->SetPoints(outPoints);

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkCellArray> outCells = vtkSmartPointer<vtkCellArray>::New();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCells->Allocate(numCells);
  outCD->CopyAllOn();
  outCD->CopyAllocate(inCD);

  reflectableArrays.clear();
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inCD, this->ReflectAllInputArrays);

  vtkNew<vtkIdList> cellPts;
  for (vtkIdType i = 0; i < numCells; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    input->GetCellPoints(i, cellPts);
    vtkIdType newCellPts[8] = { cellPts->GetId(3), cellPts->GetId(2), cellPts->GetId(1),
      cellPts->GetId(0), cellPts->GetId(7), cellPts->GetId(6), cellPts->GetId(5),
      cellPts->GetId(4) };
    vtkIdType outputCellId = outCells->InsertNextCell(8, newCellPts);

    outCD->CopyData(inCD, i, outputCellId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inCD, outCD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, outputCellId);
  }

  output->SetCells(outCells);

  output->ComputeFacesConnectivityFlagsArray();
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessStructuredGrid(vtkStructuredGrid* input,
  vtkStructuredGrid* output, double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6],
  int mirrorTensorDir[9])
{
  output->SetExtent(input->GetExtent());

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  outPoints->Allocate(numPts);
  outPD->CopyAllOn();
  outPD->CopyAllocate(inPD);

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inPD, this->ReflectAllInputArrays);

  for (vtkIdType i = numPts - 1; i >= 0; i--)
  {
    if (this->CheckAbort())
    {
      break;
    }

    double point[3];
    input->GetPoint(i, point);
    vtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inPD, outPD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, ptId);
  }

  output->SetPoints(outPoints);
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessPolyData(vtkPolyData* input, vtkPolyData* output,
  double constant[3], int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9])
{
  output->ShallowCopy(input);

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  vtkSmartPointer<vtkCellArray> outVerts = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> outLines = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> outPolys = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> outStrips = vtkSmartPointer<vtkCellArray>::New();
  outVerts->Allocate(input->GetNumberOfVerts());
  outLines->Allocate(input->GetNumberOfLines());
  outPolys->Allocate(input->GetNumberOfPolys());
  outStrips->Allocate(input->GetNumberOfStrips());

  outPoints->Allocate(numPts);
  outPD->CopyAllOn();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllOn();
  outCD->CopyAllocate(inCD);

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inPD, this->ReflectAllInputArrays);

  for (vtkIdType i = 0; i < numPts; i++)
  {
    double point[3];
    if (this->CheckAbort())
    {
      break;
    }
    input->GetPoint(i, point);
    vtkIdType ptId = outPoints->InsertNextPoint(mirrorDir[0] * point[0] + constant[0],
      mirrorDir[1] * point[1] + constant[1], mirrorDir[2] * point[2] + constant[2]);
    outPD->CopyData(inPD, i, ptId);

    vtkReflectionUtilities::ReflectReflectableArrays(reflectableArrays, inPD, outPD, i, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, ptId);
  }

  output->SetPoints(outPoints);

  vtkNew<vtkIdList> cellPts;
  for (vtkIdType i = 0; i < numCells; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    int cellType = input->GetCellType(i);
    input->GetCellPoints(i, cellPts);
    int numCellPts = cellPts->GetNumberOfIds();
    vtkIdType newCellId;

    if (cellType == VTK_TRIANGLE_STRIP && numCellPts % 2 == 0)
    {
      // Triangle strips with even number of triangles have
      // to be handled specially. A degenerate triangle is
      // introduced to reflect all the triangles properly.
      input->GetCellPoints(i, cellPts);
      numCellPts++;
      std::vector<vtkIdType> newCellPts(numCellPts);
      vtkIdType pointIdOffset = 0;
      newCellPts[0] = cellPts->GetId(0) + pointIdOffset;
      newCellPts[1] = cellPts->GetId(2) + pointIdOffset;
      newCellPts[2] = cellPts->GetId(1) + pointIdOffset;
      newCellPts[3] = cellPts->GetId(2) + pointIdOffset;
      for (int j = 4; j < numCellPts; j++)
      {
        newCellPts[j] = cellPts->GetId(j - 1) + pointIdOffset;
      }
      newCellId = outStrips->InsertNextCell(numCellPts, newCellPts.data());
    }
    else
    {
      input->GetCellPoints(i, cellPts);
      int nbPts = cellPts->GetNumberOfIds();
      std::vector<vtkIdType> newCellPts(nbPts);
      for (int j = 0; j != nbPts; j++)
      {
        // Indexing in this way ensures proper reflection of quad triangulation
        newCellPts[(nbPts - j) % nbPts] = cellPts->GetId(j);
      }
      switch (cellType)
      {
        case VTK_VERTEX:
          newCellId = outVerts->InsertNextCell(numCellPts, newCellPts.data());
          break;
        case VTK_LINE:
          newCellId = outLines->InsertNextCell(numCellPts, newCellPts.data());
          break;
        default:
          newCellId = outPolys->InsertNextCell(numCellPts, newCellPts.data());
      }
    }
    outCD->CopyData(inCD, i, newCellId);
  }

  output->SetVerts(outVerts);
  output->SetLines(outLines);
  output->SetPolys(outPolys);
  output->SetStrips(outStrips);

  reflectableArrays.clear();
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inCD, this->ReflectAllInputArrays);

  for (vtkIdType i = 0; i < numCells; i++)
  {
    vtkReflectionUtilities::ReflectReflectableArrays(
      reflectableArrays, inCD, outCD, i, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir, i);
  }
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::ProcessHtg(vtkHyperTreeGrid* input, vtkHyperTreeGrid* output,
  int mirrorDir[3], int mirrorSymmetricTensorDir[6], int mirrorTensorDir[9])
{
  // Skip empty inputs
  if (input->GetNumberOfLeaves() == 0)
  {
    return;
  }

  // Shallow copy structure of input into output
  output->CopyStructure(input);

  // Shallow copy data of input into output
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->PassData(inCD);

  std::vector<std::pair<vtkIdType, int>> reflectableArrays;
  vtkReflectionUtilities::FindAllReflectableArrays(
    reflectableArrays, inCD, this->ReflectAllInputArrays);

  for (size_t i = 0; i < reflectableArrays.size(); i++)
  {
    vtkAbstractArray* inArr = inCD->GetAbstractArray(reflectableArrays[i].first);
    vtkSmartPointer<vtkAbstractArray> newArr = inArr->NewInstance();
    newArr->SetName(inArr->GetName());
    newArr->SetNumberOfComponents(inArr->GetNumberOfComponents());
    newArr->SetNumberOfTuples(inArr->GetNumberOfTuples());
    outCD->AddArray(newArr);
    newArr->Delete();
  }

  for (int i = 0; i < input->GetNumberOfCells(); i++)
  {
    vtkReflectionUtilities::ReflectReflectableArrays(
      reflectableArrays, inCD, outCD, i, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir, i);
  }

  // Retrieve reflection direction and coordinates to be reflected
  unsigned int direction = this->PlaneAxisInternal;
  double offset = 0.;
  vtkUniformHyperTreeGrid* inputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(input);
  vtkUniformHyperTreeGrid* outputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(output);
  if (inputUHTG)
  {
    assert(outputUHTG);
    double origin[3];
    inputUHTG->GetOrigin(origin);
    double scale[3];
    inputUHTG->GetGridScale(scale);

    unsigned int size = inputUHTG->GetCellDims()[direction];

    switch (this->PlaneMode)
    {
      case PLANE:
      {
        offset = 2 * this->PlaneOriginInternal[direction];
        break;
      }
      case X_MIN:
      case Y_MIN:
      case Z_MIN:
      {
        double u = origin[direction];
        double v = origin[direction] + size * scale[0];
        offset = u < v ? 2. * u : 2. * v;
        break;
      }
      case X_MAX:
      case Y_MAX:
      case Z_MAX:
      {
        double u = origin[direction];
        double v = origin[direction] + size * scale[0];
        offset = u > v ? 2. * u : 2. * v;
        break;
      }
    }

    // Create array for reflected coordinates
    // Reflect point coordinate
    // Assign new coordinates to appropriate axis
    origin[direction] = offset - origin[direction];
    scale[direction] = -scale[direction];

    outputUHTG->SetOrigin(origin);
    outputUHTG->SetGridScale(scale);
  }
  else
  {
    vtkDataArray* inCoords = nullptr;
    if (this->PlaneAxisInternal == X_PLANE)
    {
      inCoords = input->GetXCoordinates();
    }
    else if (this->PlaneAxisInternal == Y_PLANE)
    {
      inCoords = input->GetYCoordinates();
    }
    else
    {
      inCoords = input->GetZCoordinates();
    }

    // Retrieve size of reflected coordinates array
    unsigned int size = input->GetCellDims()[direction];

    switch (this->PlaneMode)
    {
      case PLANE:
      {
        offset = 2 * this->PlaneOriginInternal[direction];
        break;
      }
      case X_MIN:
      case Y_MIN:
      case Z_MIN:
      {
        double u = inCoords->GetTuple1(0);
        double v = inCoords->GetTuple1(size);
        offset = u < v ? 2. * u : 2. * v;
        break;
      }
      case X_MAX:
      case Y_MAX:
      case Z_MAX:
      {
        double u = inCoords->GetTuple1(0);
        double v = inCoords->GetTuple1(size);
        offset = u > v ? 2. * u : 2. * v;
        break;
      }
    }

    // Create array for reflected coordinates
    ++size;
    vtkDoubleArray* outCoords = vtkDoubleArray::New();
    outCoords->SetNumberOfTuples(size);

    // Reflect point coordinate
    double coord;
    for (unsigned int i = 0; i < size; ++i)
    {
      coord = inCoords->GetTuple1(i);
      outCoords->SetTuple1(i, offset - coord);
    } // i

    // Assign new coordinates to appropriate axis
    switch (direction)
    {
      case 0:
        output->SetXCoordinates(outCoords);
        break;
      case 1:
        output->SetYCoordinates(outCoords);
        break;
      case 2:
        output->SetZCoordinates(outCoords);
    } // switch ( direction )

    // Clean up
    outCoords->Delete();
  }

  // Retrieve interface arrays if available
  vtkDataArray* inNormals = nullptr;
  vtkDataArray* inIntercepts = nullptr;
  bool hasInterface = input->GetHasInterface();
  if (hasInterface)
  {
    inNormals = inCD->GetArray(input->GetInterfaceNormalsName());
    inIntercepts = inCD->GetArray(input->GetInterfaceInterceptsName());

    if (!inNormals || !inIntercepts)
    {
      vtkWarningMacro(<< "Incomplete material interface data; ignoring it.");
      hasInterface = false;
    }
  }

  // Create arrays for reflected interface if present
  vtkDoubleArray* outNormals = nullptr;
  vtkDoubleArray* outIntercepts = nullptr;
  if (hasInterface)
  {
    vtkIdType nTuples = inNormals->GetNumberOfTuples();
    outNormals = vtkDoubleArray::New();
    outNormals->SetNumberOfComponents(3);
    outNormals->SetNumberOfTuples(nTuples);
    outNormals->SetName("outNormals");
    output->SetInterfaceNormalsName(outNormals->GetName());

    outIntercepts = vtkDoubleArray::New();
    outIntercepts->SetNumberOfComponents(3);
    outIntercepts->SetNumberOfTuples(nTuples);
    outIntercepts->SetName("outIntercepts");
    output->SetInterfaceInterceptsName(outIntercepts->GetName());

    // Reflect interface normals if present
    // Iterate over all cells
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      // Compute and stored reflected normal
      double norm[3];
      memcpy(norm, inNormals->GetTuple3(i), 3 * sizeof(double));
      norm[direction] = -norm[direction];
      outNormals->SetTuple3(i, norm[0], norm[1], norm[2]);

      // Compute and store reflected intercept
      double* inter = inIntercepts->GetTuple3(i);
      const double diff = offset * norm[direction];

      // Reflect necessary planes depending on the interface type (simple, double)
      if (inter[2] == -1 || inter[2] == 0)
      {
        inter[0] -= diff;
      }
      if (inter[2] == 1 || inter[2] == 0)
      {
        inter[1] -= diff;
      }

      outIntercepts->SetTuple3(i, inter[0], inter[1], inter[2]);
    } // i

    // Assign new interface arrays if available
    outCD->SetVectors(outNormals);
    outCD->AddArray(outIntercepts);
  } // if ( hasInterface )

  // Clean up
  if (hasInterface)
  {
    outNormals->Delete();
    outIntercepts->Delete();
  }

  // Update HTGs scale
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  output->InitializeTreeIterator(it);
  vtkHyperTree* tree = nullptr;
  vtkIdType index;
  while ((tree = it.GetNextTree(index)))
  {
    if (this->CheckAbort())
    {
      break;
    }
    assert(tree->GetTreeIndex() == index);
    double origin[3];
    double scale[3];
    output->GetLevelZeroOriginAndSizeFromIndex(index, origin, scale);
    tree->SetScales(std::make_shared<vtkHyperTreeGridScales>(output->GetBranchFactor(), scale));
  }
}

//------------------------------------------------------------------------------
bool vtkAxisAlignedReflectionFilter::ProcessLeaf(
  vtkDataObject* inputDataObject, vtkDataObject* outputDataObject, double bounds[6])
{
  double constant[3] = { 0.0, 0.0, 0.0 };
  int mirrorDir[3] = { 1, 1, 1 };
  int mirrorSymmetricTensorDir[6] = { 1, 1, 1, 1, 1, 1 };
  int mirrorTensorDir[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

  if (this->PlaneMode == PLANE)
  {
    if (!this->ReflectionPlane->GetAxisAligned())
    {
      vtkErrorMacro("Unable to retrieve valid axis-aligned implicit function to reflect with.");
      return false;
    }
    double* normal = this->ReflectionPlane->GetNormal();
    double* origin = this->ReflectionPlane->GetOrigin();
    double offset = this->ReflectionPlane->GetOffset();
    this->PlaneAxisInternal = X_PLANE;
    if (normal[1] > normal[0])
    {
      this->PlaneAxisInternal = Y_PLANE;
    }
    if (normal[2] > normal[0])
    {
      this->PlaneAxisInternal = Z_PLANE;
    }
    this->PlaneOriginInternal[0] = origin[0] + offset;
    this->PlaneOriginInternal[1] = origin[1] + offset;
    this->PlaneOriginInternal[2] = origin[2] + offset;
  }
  else
  {
    switch (this->PlaneMode)
    {
      case X_MIN:
        this->PlaneAxisInternal = X_PLANE;
        this->PlaneOriginInternal[0] = bounds[0];
        break;
      case Y_MIN:
        this->PlaneAxisInternal = Y_PLANE;
        this->PlaneOriginInternal[1] = bounds[2];
        break;
      case Z_MIN:
        this->PlaneAxisInternal = Z_PLANE;
        this->PlaneOriginInternal[2] = bounds[4];
        break;
      case X_MAX:
        this->PlaneAxisInternal = X_PLANE;
        this->PlaneOriginInternal[0] = bounds[1];
        break;
      case Y_MAX:
        this->PlaneAxisInternal = Y_PLANE;
        this->PlaneOriginInternal[1] = bounds[3];
        break;
      case Z_MAX:
        this->PlaneAxisInternal = Z_PLANE;
        this->PlaneOriginInternal[2] = bounds[5];
        break;
    }
  }

  switch (this->PlaneAxisInternal)
  {
    case X_PLANE:
      constant[0] = 2 * this->PlaneOriginInternal[0];
      break;
    case Y_PLANE:
      constant[1] = 2 * this->PlaneOriginInternal[1];
      break;
    case Z_PLANE:
      constant[2] = 2 * this->PlaneOriginInternal[2];
      break;
  }

  switch (this->PlaneAxisInternal)
  {
    case X_PLANE:
      mirrorDir[0] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
    case Y_PLANE:
      mirrorDir[1] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      break;
    case Z_PLANE:
      mirrorDir[2] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
  }
  vtkMath::TensorFromSymmetricTensor(mirrorSymmetricTensorDir, mirrorTensorDir);

  if (auto ug = vtkUnstructuredGrid::SafeDownCast(inputDataObject))
  {
    auto output = vtkUnstructuredGrid::SafeDownCast(outputDataObject);
    vtkReflectionUtilities::ProcessUnstructuredGrid(ug, output, constant, mirrorDir,
      mirrorSymmetricTensorDir, mirrorTensorDir, false, this->ReflectAllInputArrays, this);
  }
  else if (auto imgData = vtkImageData::SafeDownCast(inputDataObject))
  {
    auto output = vtkImageData::SafeDownCast(outputDataObject);
    ProcessImageData(
      imgData, output, constant, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else if (auto rg = vtkRectilinearGrid::SafeDownCast(inputDataObject))
  {
    auto output = vtkRectilinearGrid::SafeDownCast(outputDataObject);
    ProcessRectilinearGrid(
      rg, output, constant, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else if (auto esg = vtkExplicitStructuredGrid::SafeDownCast(inputDataObject))
  {
    auto output = vtkExplicitStructuredGrid::SafeDownCast(outputDataObject);
    ProcessExplicitStructuredGrid(
      esg, output, constant, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else if (auto sg = vtkStructuredGrid::SafeDownCast(inputDataObject))
  {
    auto output = vtkStructuredGrid::SafeDownCast(outputDataObject);
    ProcessStructuredGrid(
      sg, output, constant, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else if (auto pd = vtkPolyData::SafeDownCast(inputDataObject))
  {
    auto output = vtkPolyData::SafeDownCast(outputDataObject);
    ProcessPolyData(pd, output, constant, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else if (auto htg = vtkHyperTreeGrid::SafeDownCast(inputDataObject))
  {
    auto output = vtkHyperTreeGrid::SafeDownCast(outputDataObject);
    ProcessHtg(htg, output, mirrorDir, mirrorSymmetricTensorDir, mirrorTensorDir);
  }
  else
  {
    vtkErrorMacro("AxisAlignedReflectionFilter: Unhandled type of DataSet ("
      << inputDataObject->GetClassName() << ")");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkAxisAlignedReflectionFilter::FillInputPortInformation(int, vtkInformation* info)
{
  // Input can be a dataset, a htg or a composite of datasets.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAxisAlignedReflectionFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  if (input)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outInfo);
    if (!output)
    {
      vtkPartitionedDataSetCollection* newOutput = vtkPartitionedDataSetCollection::New();
      outInfo->Set(vtkDataSet::DATA_OBJECT(), newOutput);
      newOutput->FastDelete();
    }
  }
  else
  {
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedReflectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
  os << indent << "CopyInput: " << (this->CopyInput ? "On" : "Off") << endl;
  os << indent << "ReflectAllInputArrays: " << (this->ReflectAllInputArrays ? "On" : "Off") << endl;
  os << indent << "PlaneMode: " << this->PlaneMode << endl;
  this->ReflectionPlane->PrintSelf(os, indent.GetNextIndent());
}

VTK_ABI_NAMESPACE_END
