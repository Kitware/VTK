// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMergeBlocks.h"

#include "vtkAppendDataSets.h"
#include "vtkCleanArrays.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{
vtkSmartPointer<vtkDataSet> CreateOutput(vtkMergeBlocks* self)
{
  if (self->GetOutputDataSetType() == VTK_POLY_DATA)
  {
    return vtkSmartPointer<vtkPolyData>::New();
  }
  else
  {
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }
}

void Merge(vtkDataObject* input, vtkDataSet* output, vtkMergeBlocks* self)
{
  if (output->IsA(input->GetClassName()))
  {
    output->ShallowCopy(input);
  }
  else if (auto ds = vtkDataSet::SafeDownCast(input))
  {
    vtkNew<vtkAppendDataSets> appender;
    appender->SetMergePoints(self->GetMergePoints() ? true : false);
    appender->SetOutputDataSetType(self->GetOutputDataSetType());
    appender->SetTolerance(self->GetTolerance());
    appender->SetToleranceIsAbsolute(self->GetToleranceIsAbsolute());
    appender->AddInputDataObject(ds);
    appender->Update();
    output->ShallowCopy(appender->GetOutputDataObject(0));
  }
  else if (auto tree = vtkDataObjectTree::SafeDownCast(input))
  {
    vtkNew<vtkAppendDataSets> appender;
    appender->SetMergePoints(self->GetMergePoints() ? true : false);
    appender->SetOutputDataSetType(self->GetOutputDataSetType());
    appender->SetTolerance(self->GetTolerance());
    appender->SetToleranceIsAbsolute(self->GetToleranceIsAbsolute());
    using Opts = vtk::DataObjectTreeOptions;
    for (auto child :
      vtk::Range(tree, Opts::TraverseSubTree | Opts::SkipEmptyNodes | Opts::VisitOnlyLeaves))
    {
      if (auto childDS = vtkDataSet::SafeDownCast(child))
      {
        appender->AddInputDataObject(childDS);
      }
    }
    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      output->ShallowCopy(appender->GetOutputDataObject(0));
    }
  }

  // remove partial arrays.
  vtkNew<vtkCleanArrays> cleaner;
  cleaner->SetInputData(output);
  cleaner->Update();
  output->ShallowCopy(cleaner->GetOutput());
}

void RecursiveMerge(
  vtkMultiBlockDataSet* inputMB, vtkMultiBlockDataSet* outputMB, vtkMergeBlocks* self)
{
  const auto numBlocks = inputMB->GetNumberOfBlocks();
  outputMB->SetNumberOfBlocks(numBlocks);
  for (unsigned int cc = 0; cc < numBlocks; ++cc)
  {
    if (inputMB->HasMetaData(cc))
    {
      outputMB->GetMetaData(cc)->Copy(inputMB->GetMetaData(cc));
    }
    auto inputBlock = inputMB->GetBlock(cc);
    if (auto inputBlockPD = vtkPartitionedDataSet::SafeDownCast(inputBlock))
    {
      auto ds = CreateOutput(self);
      Merge(inputBlockPD, ds, self);
      outputMB->SetBlock(cc, ds);
    }
    else if (auto inputBlockMB = vtkMultiBlockDataSet::SafeDownCast(inputBlock))
    {
      vtkNew<vtkMultiBlockDataSet> outputBlock;
      RecursiveMerge(inputBlockMB, outputBlock, self);
      outputMB->SetBlock(cc, outputBlock);
    }
    else
    {
      outputMB->SetBlock(cc, inputBlock);
    }
  }
}

void HandleFieldData(vtkDataObject* inputDO, vtkDataObject* outputDO)
{
  // Handle root-node filed data. All array in root's field data not already
  // present in the output are passed.
  auto inFD = inputDO->GetFieldData();
  auto outFD = outputDO->GetFieldData();
  for (int cc = 0, max = inFD->GetNumberOfArrays(); cc < max; ++cc)
  {
    auto aname = inFD->GetArrayName(cc);
    if (aname && !outFD->HasArray(aname))
    {
      outFD->AddArray(inFD->GetAbstractArray(cc));
    }
  }
}
}

vtkStandardNewMacro(vtkMergeBlocks);
//----------------------------------------------------------------------------
vtkMergeBlocks::vtkMergeBlocks()
  : MergePoints(true)
  , Tolerance(0.0)
  , MergePartitionsOnly(false)
  , ToleranceIsAbsolute(false)
  , OutputDataSetType(VTK_UNSTRUCTURED_GRID)
{
}

//----------------------------------------------------------------------------
vtkMergeBlocks::~vtkMergeBlocks() = default;

//----------------------------------------------------------------------------
int vtkMergeBlocks::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  if (vtkDataSet::SafeDownCast(inputDO) != nullptr || this->MergePartitionsOnly == false ||
    (this->MergePartitionsOnly && vtkPartitionedDataSet::SafeDownCast(inputDO) != nullptr))
  {
    auto outputDS = vtkDataSet::SafeDownCast(outputDO);
    assert(outputDS != nullptr);
    ::Merge(inputDO, outputDS, this);
    ::HandleFieldData(inputDO, outputDO);
    return 1;
  }

  auto inputTree = vtkDataObjectTree::SafeDownCast(inputDO);
  assert(this->MergePartitionsOnly == true);
  assert(inputTree != nullptr);
  (void)inputTree;

  if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(outputDO);
    assert(outputPDC != nullptr);

    outputPDC->SetNumberOfPartitionedDataSets(inputPDC->GetNumberOfPartitionedDataSets());
    for (unsigned int cc = 0, max = inputPDC->GetNumberOfPartitionedDataSets(); cc < max; ++cc)
    {
      if (inputPDC->HasMetaData(cc))
      {
        outputPDC->GetMetaData(cc)->Copy(inputPDC->GetMetaData(cc));
      }
      if (auto inputPD = inputPDC->GetPartitionedDataSet(cc))
      {
        auto ds = ::CreateOutput(this);
        ::Merge(inputPD, ds, this);
        vtkNew<vtkPartitionedDataSet> outputPD;
        outputPD->SetPartition(0, ds);
        outputPDC->SetPartitionedDataSet(cc, outputPD);
      }
    }
    ::HandleFieldData(inputDO, outputDO);
    return 1;
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    ::RecursiveMerge(mb, vtkMultiBlockDataSet::SafeDownCast(outputDO), this);
    ::HandleFieldData(inputDO, outputDO);
    return 1;
  }

  vtkErrorMacro("Unsupported input type: " << inputDO->GetClassName());
  return 0;
}

//----------------------------------------------------------------------------
int vtkMergeBlocks::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkMergeBlocks::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkDataObject::GetData(outputVector, 0);
  if (this->MergePartitionsOnly &&
    (vtkMultiBlockDataSet::SafeDownCast(input) ||
      vtkPartitionedDataSetCollection::SafeDownCast(input)))
  {
    if (output == nullptr || !output->IsA(input->GetClassName()))
    {
      output = input->NewInstance();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    if (this->OutputDataSetType == VTK_UNSTRUCTURED_GRID &&
      vtkUnstructuredGrid::SafeDownCast(output) == nullptr)
    {
      output = vtkUnstructuredGrid::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    else if (this->OutputDataSetType == VTK_POLY_DATA &&
      vtkPolyData::SafeDownCast(output) == nullptr)
    {
      output = vtkPolyData::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkMergeBlocks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints: " << this->MergePoints << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "ToleranceIsAbsolute: " << this->ToleranceIsAbsolute << "\n";
  os << indent << "MergePartitionsOnly: " << this->MergePartitionsOnly << endl;
  os << indent
     << "OutputDataSetType: " << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
     << endl;
}
VTK_ABI_NAMESPACE_END
