// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResampleWithDataSet.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkResampleWithDataSet);

//------------------------------------------------------------------------------
vtkResampleWithDataSet::vtkResampleWithDataSet()
  : MarkBlankPointsAndCells(true)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkResampleWithDataSet::~vtkResampleWithDataSet() = default;

//------------------------------------------------------------------------------
void vtkResampleWithDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Prober->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkResampleWithDataSet::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkResampleWithDataSet::SetSourceData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
void vtkResampleWithDataSet::SetCategoricalData(bool arg)
{
  this->Prober->SetCategoricalData(arg);
}

bool vtkResampleWithDataSet::GetCategoricalData()
{
  return this->Prober->GetCategoricalData() != 0;
}

void vtkResampleWithDataSet::SetPassCellArrays(bool arg)
{
  this->Prober->SetPassCellArrays(arg);
}

bool vtkResampleWithDataSet::GetPassCellArrays()
{
  return this->Prober->GetPassCellArrays() != 0;
}

void vtkResampleWithDataSet::SetPassPointArrays(bool arg)
{
  this->Prober->SetPassPointArrays(arg);
}

bool vtkResampleWithDataSet::GetPassPointArrays()
{
  return this->Prober->GetPassPointArrays() != 0;
}

void vtkResampleWithDataSet::SetPassFieldArrays(bool arg)
{
  this->Prober->SetPassFieldArrays(arg);
}

bool vtkResampleWithDataSet::GetPassFieldArrays()
{
  return this->Prober->GetPassFieldArrays() != 0;
}

void vtkResampleWithDataSet::SetPassPartialArrays(bool arg)
{
  this->Prober->SetPassPartialArrays(arg);
}

bool vtkResampleWithDataSet::GetPassPartialArrays()
{
  return this->Prober->GetPassPartialArrays();
}

void vtkResampleWithDataSet::SetCellLocatorPrototype(vtkAbstractCellLocator* locator)
{
  this->Prober->SetCellLocatorPrototype(locator);
}

vtkAbstractCellLocator* vtkResampleWithDataSet::GetCellLocatorPrototype() const
{
  return this->Prober->GetCellLocatorPrototype();
}

//------------------------------------------------------------------------------
void vtkResampleWithDataSet::SetTolerance(double arg)
{
  this->Prober->SetTolerance(arg);
}

double vtkResampleWithDataSet::GetTolerance()
{
  return this->Prober->GetTolerance();
}

void vtkResampleWithDataSet::SetComputeTolerance(bool arg)
{
  this->Prober->SetComputeTolerance(arg);
}

bool vtkResampleWithDataSet::GetComputeTolerance()
{
  return this->Prober->GetComputeTolerance();
}

void vtkResampleWithDataSet::SetSnapToCellWithClosestPoint(bool arg)
{
  this->Prober->SetSnapToCellWithClosestPoint(arg);
}

bool vtkResampleWithDataSet::GetSnapToCellWithClosestPoint()
{
  return this->Prober->GetSnapToCellWithClosestPoint();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkResampleWithDataSet::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Prober->GetMTime());
}

//------------------------------------------------------------------------------
int vtkResampleWithDataSet::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleWithDataSet::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleWithDataSet::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");

  if (port == 1)
  {
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleWithDataSet::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
const char* vtkResampleWithDataSet::GetMaskArrayName() const
{
  return this->Prober->GetValidPointMaskArrayName();
}

//------------------------------------------------------------------------------
namespace
{

class MarkHiddenPoints
{
public:
  MarkHiddenPoints(
    char* maskArray, vtkUnsignedCharArray* pointGhostArray, vtkResampleWithDataSet* filter)
    : MaskArray(maskArray)
    , PointGhostArray(pointGhostArray)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType i = begin; i < end; ++i)
    {
      if (i % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      if (!this->MaskArray[i])
      {
        this->PointGhostArray->SetValue(
          i, this->PointGhostArray->GetValue(i) | vtkDataSetAttributes::HIDDENPOINT);
      }
    }
  }

private:
  char* MaskArray;
  vtkUnsignedCharArray* PointGhostArray;
  vtkResampleWithDataSet* Filter;
};

class MarkHiddenCells
{
public:
  MarkHiddenCells(vtkDataSet* data, char* maskArray, vtkUnsignedCharArray* cellGhostArray,
    vtkResampleWithDataSet* filter)
    : Data(data)
    , MaskArray(maskArray)
    , CellGhostArray(cellGhostArray)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkIdList* cellPoints = this->PointIds.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType i = begin; i < end; ++i)
    {
      if (i % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      this->Data->GetCellPoints(i, cellPoints);
      vtkIdType npts = cellPoints->GetNumberOfIds();
      for (vtkIdType j = 0; j < npts; ++j)
      {
        vtkIdType ptid = cellPoints->GetId(j);
        if (!this->MaskArray[ptid])
        {
          this->CellGhostArray->SetValue(
            i, this->CellGhostArray->GetValue(i) | vtkDataSetAttributes::HIDDENCELL);
          break;
        }
      }
    }
  }

private:
  vtkDataSet* Data;
  char* MaskArray;
  vtkUnsignedCharArray* CellGhostArray;
  vtkResampleWithDataSet* Filter;

  vtkSMPThreadLocalObject<vtkIdList> PointIds;
};

} // anonymous namespace

void vtkResampleWithDataSet::SetBlankPointsAndCells(vtkDataSet* dataset)
{
  if (dataset->GetNumberOfPoints() <= 0)
  {
    return;
  }

  vtkPointData* pd = dataset->GetPointData();
  vtkCharArray* maskArray = vtkArrayDownCast<vtkCharArray>(pd->GetArray(this->GetMaskArrayName()));
  char* mask = maskArray->GetPointer(0);

  dataset->AllocatePointGhostArray();
  vtkUnsignedCharArray* pointGhostArray = dataset->GetPointGhostArray();

  vtkIdType numPoints = dataset->GetNumberOfPoints();
  MarkHiddenPoints pointWorklet(mask, pointGhostArray, this);
  vtkSMPTools::For(0, numPoints, pointWorklet);

  dataset->AllocateCellGhostArray();
  vtkUnsignedCharArray* cellGhostArray = dataset->GetCellGhostArray();

  vtkIdType numCells = dataset->GetNumberOfCells();
  // GetCellPoints needs to be called once from a single thread for safe
  // multi-threaded calls
  vtkNew<vtkIdList> cpts;
  dataset->GetCellPoints(0, cpts);

  MarkHiddenCells cellWorklet(dataset, mask, cellGhostArray, this);
  vtkSMPTools::For(0, numCells, cellWorklet);
}

//------------------------------------------------------------------------------
int vtkResampleWithDataSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* source = sourceInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkDataObject* inDataObject = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* outDataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  this->Prober->SetContainerAlgorithm(this);
  if (inDataObject->IsA("vtkDataSet"))
  {
    vtkDataSet* input = vtkDataSet::SafeDownCast(inDataObject);
    vtkDataSet* output = vtkDataSet::SafeDownCast(outDataObject);

    this->Prober->SetInputData(input);
    this->Prober->SetSourceData(source);
    this->Prober->Update();
    output->ShallowCopy(this->Prober->GetOutput());
    if (this->MarkBlankPointsAndCells)
    {
      this->SetBlankPointsAndCells(output);
    }
  }
  else if (inDataObject->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(inDataObject);
    vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(outDataObject);
    output->CopyStructure(input);

    this->Prober->SetSourceData(source);

    using Opts = vtk::CompositeDataSetOptions;
    for (auto node : vtk::Range(input, Opts::SkipEmptyNodes))
    {
      if (this->CheckAbort())
      {
        break;
      }
      vtkDataSet* ds = static_cast<vtkDataSet*>(node.GetDataObject());
      if (ds)
      {
        this->Prober->SetInputData(ds);
        this->Prober->Update();
        vtkDataSet* result = this->Prober->GetOutput();

        vtkDataSet* block = result->NewInstance();
        block->ShallowCopy(result);
        if (this->MarkBlankPointsAndCells)
        {
          this->SetBlankPointsAndCells(block);
        }
        node.SetDataObject(output, block);
        block->Delete();
      }
    }
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
