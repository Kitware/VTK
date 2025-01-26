// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResampleToImage.h"

#include "vtkBoundingBox.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkResampleToImage);

//------------------------------------------------------------------------------
vtkResampleToImage::vtkResampleToImage()
  : UseInputBounds(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->SamplingBounds[0] = this->SamplingBounds[2] = this->SamplingBounds[4] = 0;
  this->SamplingBounds[1] = this->SamplingBounds[3] = this->SamplingBounds[5] = 1;
  this->SamplingDimensions[0] = this->SamplingDimensions[1] = this->SamplingDimensions[2] = 10;
}

//------------------------------------------------------------------------------
vtkResampleToImage::~vtkResampleToImage() = default;

//------------------------------------------------------------------------------
void vtkResampleToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseInputBounds " << this->UseInputBounds << endl;
  os << indent << "SamplingBounds [" << this->SamplingBounds[0] << ", " << this->SamplingBounds[1]
     << ", " << this->SamplingBounds[2] << ", " << this->SamplingBounds[3] << ", "
     << this->SamplingBounds[4] << ", " << this->SamplingBounds[5] << "]" << endl;
  os << indent << "SamplingDimensions " << this->SamplingDimensions[0] << " x "
     << this->SamplingDimensions[1] << " x " << this->SamplingDimensions[2] << endl;
}

//------------------------------------------------------------------------------
vtkImageData* vtkResampleToImage::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
vtkTypeBool vtkResampleToImage::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // propagate update extent
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkResampleToImage::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  int wholeExtent[6] = { 0, this->SamplingDimensions[0] - 1, 0, this->SamplingDimensions[1] - 1, 0,
    this->SamplingDimensions[2] - 1 };

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleToImage::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // This filter always asks for whole extent downstream. To resample
  // a subset of a structured input, you need to use ExtractVOI.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleToImage::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkResampleToImage::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
const char* vtkResampleToImage::GetMaskArrayName() const
{
  return "vtkValidPointMask";
}

//------------------------------------------------------------------------------
namespace
{

inline void ComputeBoundingExtent(
  const double origin[3], const double spacing[3], const double bounds[6], int extent[6])
{
  for (int i = 0; i < 3; ++i)
  {
    if (spacing[i] != 0.0)
    {
      extent[2 * i] = vtkMath::Floor((bounds[2 * i] - origin[i]) / spacing[i]);
      extent[2 * i + 1] = vtkMath::Ceil((bounds[2 * i + 1] - origin[i]) / spacing[i]);
    }
    else
    {
      extent[2 * i] = extent[2 * i + 1] = 0;
    }
  }
}

} // anonymous namespace

void vtkResampleToImage::PerformResampling(vtkDataObject* input, const double samplingBounds[6],
  bool computeProbingExtent, const double inputBounds[6], vtkImageData* output)
{
  if (this->SamplingDimensions[0] <= 0 || this->SamplingDimensions[1] <= 0 ||
    this->SamplingDimensions[2] <= 0)
  {
    return;
  }

  // compute bounds and extent where probing should be performed
  double origin[3] = { samplingBounds[0], samplingBounds[2], samplingBounds[4] };
  double spacing[3];
  for (int i = 0; i < 3; ++i)
  {
    spacing[i] = (this->SamplingDimensions[i] == 1)
      ? 0
      : ((samplingBounds[i * 2 + 1] - samplingBounds[i * 2]) /
          static_cast<double>(this->SamplingDimensions[i] - 1));
  }

  int* updateExtent = this->GetUpdateExtent();
  int probingExtent[6];
  if (computeProbingExtent)
  {
    ComputeBoundingExtent(origin, spacing, inputBounds, probingExtent);
    for (int i = 0; i < 3; ++i)
    {
      probingExtent[2 * i] = vtkMath::Max(probingExtent[2 * i], updateExtent[2 * i]);
      probingExtent[2 * i + 1] = vtkMath::Min(probingExtent[2 * i + 1], updateExtent[2 * i + 1]);
      if (probingExtent[2 * i] > probingExtent[2 * i + 1]) // no overlap
      {
        probingExtent[0] = probingExtent[2] = probingExtent[4] = 0;
        probingExtent[1] = probingExtent[3] = probingExtent[5] = -1;
        break;
      }
    }
  }
  else
  {
    std::copy(updateExtent, updateExtent + 6, probingExtent);
  }

  // perform probing
  vtkNew<vtkImageData> structure;
  structure->SetOrigin(origin);
  structure->SetSpacing(spacing);
  structure->SetExtent(probingExtent);

  vtkNew<vtkCompositeDataProbeFilter> prober;
  prober->SetContainerAlgorithm(this);
  prober->SetInputData(structure);
  prober->SetSourceData(input);
  prober->Update();

  output->ShallowCopy(prober->GetOutput());
  output->GetFieldData()->PassData(input->GetFieldData());
}

//------------------------------------------------------------------------------
namespace
{

class MarkHiddenPoints
{
public:
  MarkHiddenPoints(
    char* maskArray, vtkUnsignedCharArray* pointGhostArray, vtkResampleToImage* filter)
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
  vtkResampleToImage* Filter;
};

class MarkHiddenCells
{
public:
  MarkHiddenCells(vtkImageData* data, char* maskArray, vtkUnsignedCharArray* cellGhostArray,
    vtkResampleToImage* filter)
    : Data(data)
    , MaskArray(maskArray)
    , CellGhostArray(cellGhostArray)
    , Filter(filter)
  {
    this->Data->GetDimensions(this->PointDim);
    this->PointSliceSize = this->PointDim[0] * this->PointDim[1];

    this->CellDim[0] = vtkMath::Max(1, this->PointDim[0] - 1);
    this->CellDim[1] = vtkMath::Max(1, this->PointDim[1] - 1);
    this->CellDim[2] = vtkMath::Max(1, this->PointDim[2] - 1);
    this->CellSliceSize = this->CellDim[0] * this->CellDim[1];
    this->Dim[0] = (this->PointDim[0] > 1) ? 1 : 0;
    this->Dim[1] = (this->PointDim[1] > 1) ? 1 : 0;
    this->Dim[2] = (this->PointDim[2] > 1) ? 1 : 0;
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      int ptijk[3];
      ptijk[2] = cellId / this->CellSliceSize;
      ptijk[1] = (cellId % CellSliceSize) / this->CellDim[0];
      ptijk[0] = (cellId % CellSliceSize) % this->CellDim[0];

      vtkIdType ptid = ptijk[0] + this->PointDim[0] * ptijk[1] + this->PointSliceSize * ptijk[2];

      bool validCell = true;
      for (int k = 0; k <= this->Dim[2]; ++k)
      {
        for (int j = 0; j <= this->Dim[1]; ++j)
        {
          for (int i = 0; i <= this->Dim[0]; ++i)
          {
            validCell &= (0 !=
              this->MaskArray[ptid + i + (j * this->PointDim[0]) + (k * this->PointSliceSize)]);
          }
        }
      }

      if (!validCell)
      {
        this->CellGhostArray->SetValue(
          cellId, this->CellGhostArray->GetValue(cellId) | vtkDataSetAttributes::HIDDENCELL);
      }
    }
  }

private:
  vtkImageData* Data;
  char* MaskArray;
  vtkUnsignedCharArray* CellGhostArray;

  int PointDim[3];
  vtkIdType PointSliceSize;
  int CellDim[3];
  vtkIdType CellSliceSize;
  int Dim[3];
  vtkResampleToImage* Filter;
};

} // anonymous namespace

void vtkResampleToImage::SetBlankPointsAndCells(vtkImageData* data)
{
  if (data->GetNumberOfPoints() <= 0)
  {
    return;
  }

  vtkPointData* pd = data->GetPointData();
  vtkCharArray* maskArray = vtkArrayDownCast<vtkCharArray>(pd->GetArray(this->GetMaskArrayName()));
  char* mask = maskArray->GetPointer(0);

  data->AllocatePointGhostArray();
  vtkUnsignedCharArray* pointGhostArray = data->GetPointGhostArray();

  vtkIdType numPoints = data->GetNumberOfPoints();
  MarkHiddenPoints pointWorklet(mask, pointGhostArray, this);
  vtkSMPTools::For(0, numPoints, pointWorklet);

  data->AllocateCellGhostArray();
  vtkUnsignedCharArray* cellGhostArray = data->GetCellGhostArray();

  vtkIdType numCells = data->GetNumberOfCells();
  MarkHiddenCells cellWorklet(data, mask, cellGhostArray, this);
  vtkSMPTools::For(0, numCells, cellWorklet);
}

//------------------------------------------------------------------------------
int vtkResampleToImage::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double samplingBounds[6];
  if (this->UseInputBounds)
  {
    ComputeDataBounds(input, samplingBounds);

    // To avoid accidentally sampling outside the dataset due to floating point roundoff,
    // nudge the bounds inward by epsilon.
    vtkBoundingBox bbox(samplingBounds);
    const double epsilon = 1.0e-6;
    bbox.ScaleAboutCenter(1.0 - epsilon);
    bbox.GetBounds(samplingBounds);
  }
  else
  {
    std::copy(this->SamplingBounds, this->SamplingBounds + 6, samplingBounds);
  }

  this->PerformResampling(input, samplingBounds, false, nullptr, output);
  this->SetBlankPointsAndCells(output);

  return 1;
}

//------------------------------------------------------------------------------
void vtkResampleToImage::ComputeDataBounds(vtkDataObject* data, double bounds[6])
{
  if (vtkDataSet::SafeDownCast(data))
  {
    vtkDataSet::SafeDownCast(data)->GetBounds(bounds);
  }
  else
  {
    vtkCompositeDataSet* cdata = vtkCompositeDataSet::SafeDownCast(data);
    bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;

    using Opts = vtk::CompositeDataSetOptions;
    for (vtkDataObject* dObj : vtk::Range(cdata, Opts::SkipEmptyNodes))
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(dObj);
      if (!ds)
      {
        vtkGenericWarningMacro("vtkCompositeDataSet leaf not vtkDataSet. Skipping.");
        continue;
      }
      double b[6];
      ds->GetBounds(b);
      for (int i = 0; i < 3; ++i)
      {
        bounds[2 * i] = vtkMath::Min(bounds[2 * i], b[2 * i]);
        bounds[2 * i + 1] = vtkMath::Max(bounds[2 * i + 1], b[2 * i + 1]);
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
