// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFastSplatter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkDataArrayRange.h"
#include "vtkGraph.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFastSplatter);

//------------------------------------------------------------------------------

vtkFastSplatter::vtkFastSplatter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = 0;
  this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = -1;

  this->OutputDimensions[0] = 100;
  this->OutputDimensions[1] = 100;
  this->OutputDimensions[2] = 1;

  this->LimitMode = NoneLimit;
  this->MinValue = 0.0;
  this->MaxValue = 1.0;

  this->Buckets = vtkImageData::New();

  this->NumberOfPointsSplatted = 0;

  this->LastDataMinValue = 0.0;
  this->LastDataMaxValue = 1.0;
}

vtkFastSplatter::~vtkFastSplatter()
{
  this->Buckets->Delete();
}

void vtkFastSplatter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ModelBounds: " << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ", "
     << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ", " << this->ModelBounds[4] << ", "
     << this->ModelBounds[5] << endl;
  os << indent << "OutputDimensions: " << this->OutputDimensions[0] << ", "
     << this->OutputDimensions[1] << ", " << this->OutputDimensions[2] << endl;
  os << indent << "LimitMode: " << this->LimitMode << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "MaxValue: " << this->MaxValue << endl;
  os << indent << "NumberOfPointsSplatted: " << this->NumberOfPointsSplatted << endl;
}

//------------------------------------------------------------------------------

int vtkFastSplatter::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
      break;
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
      break;
  }
  return 1;
}

//------------------------------------------------------------------------------

// For those familiar with the old pipeline, this is equivalent to the
// ExecuteInformation method.
int vtkFastSplatter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // use model bounds if set
  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Origin[2] = 0;
  if (((this->ModelBounds[0] < this->ModelBounds[1]) || (this->OutputDimensions[0] == 1)) &&
    ((this->ModelBounds[2] < this->ModelBounds[3]) || (this->OutputDimensions[1] == 1)) &&
    ((this->ModelBounds[4] < this->ModelBounds[5]) || (this->OutputDimensions[2] == 1)))
  {
    this->Origin[0] = this->ModelBounds[0];
    this->Origin[1] = this->ModelBounds[2];
    this->Origin[2] = this->ModelBounds[4];
  }

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  int i;
  for (i = 0; i < 3; i++)
  {
    if (this->OutputDimensions[i] > 1)
    {
      this->Spacing[i] = ((this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) /
        (this->OutputDimensions[i] - 1));
    }
    else
    {
      this->Spacing[i] = 1.0;
    }
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->OutputDimensions[0] - 1,
    0, this->OutputDimensions[1] - 1, 0, this->OutputDimensions[2] - 1);
  vtkInformation* splatInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData::SetScalarType(vtkImageData::GetScalarType(splatInfo), outInfo);
  // if (splatInfo->Has(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS()))
  //   {
  //   outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),
  //                splatInfo->Get(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS()));
  //   }

  return 1;
}

//------------------------------------------------------------------------------
int vtkFastSplatter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* splatInfo = inputVector[1]->GetInformationObject(0);

  splatInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    splatInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  int numPieces = 1;
  int piece = 0;
  int ghostLevel = 0;
  // Use the output piece request to break up the input.
  // If not specified, use defaults.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);

  vtkDataObject* data = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (data->GetExtentType() == VTK_3D_EXTENT)
  {
    int* inWholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inWholeExtent, 6);
  }

  return 1;
}

//------------------------------------------------------------------------------
struct vtkFastSplatterBucketPointsWorker
{
  template <class TArray>
  void operator()(TArray* pointArray, vtkIdType numPoints, unsigned int* buckets,
    const int dimensions[3], const double origin[3], const double spacing[3])
  {
    // Clear out the buckets.
    std::fill_n(buckets, dimensions[0] * dimensions[1] * dimensions[2], 0);
    auto points = vtk::DataArrayTupleRange<3>(pointArray);

    // Iterate over all the points.
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      auto p = points[i];

      // Find the bucket.
      vtkIdType loc[3];
      loc[0] = static_cast<vtkIdType>(((p[0] - origin[0]) / spacing[0]) + 0.5);
      loc[1] = static_cast<vtkIdType>(((p[1] - origin[1]) / spacing[1]) + 0.5);
      loc[2] = static_cast<vtkIdType>(((p[2] - origin[2]) / spacing[2]) + 0.5);
      if ((loc[0] < 0) || (loc[0] >= dimensions[0]) || (loc[1] < 0) || (loc[1] >= dimensions[1]) ||
        (loc[2] < 0) || (loc[2] >= dimensions[2]))
      {
        // Point outside of splatting region.
        continue;
      }
      vtkIdType bucketId =
        (loc[2] * dimensions[0] * dimensions[1] + loc[1] * dimensions[0] + loc[0]);

      // Increment the bucket.
      buckets[bucketId]++;
    }
  }
};

//------------------------------------------------------------------------------
struct vtkFastSplatterConvolveWorker
{
  template <class TInArray, class TOutArray>
  void operator()(TInArray* splatArray, TOutArray* outputArray, const int splatDims[3],
    unsigned int* buckets, int* numPointsSplatted, const int imageDims[3])
  {
    using T = vtk::GetAPIType<TInArray>;
    int numPoints = 0;

    // First, clear out the output image.
    outputArray->Fill(0);
    auto splat = vtk::DataArrayValueRange<1>(splatArray);
    auto output = vtk::DataArrayValueRange<1>(outputArray);

    int splatCenter[3];
    splatCenter[0] = splatDims[0] / 2;
    splatCenter[1] = splatDims[1] / 2;
    splatCenter[2] = splatDims[2] / 2;

    // Iterate over all entries in buckets and splat anything that is nonzero.
    unsigned int* b = buckets;
    for (int k = 0; k < imageDims[2]; k++)
    {
      // Figure out how splat projects on image in this slab, taking into
      // account overlap.
      int splatProjMinZ = k - splatCenter[2];
      int splatProjMaxZ = splatProjMinZ + splatDims[2];
      splatProjMinZ = std::max(splatProjMinZ, 0);
      splatProjMaxZ = std::min(splatProjMaxZ, imageDims[2]);

      for (int j = 0; j < imageDims[1]; j++)
      {
        // Figure out how splat projects on image in this slab, taking into
        // account overlap.
        int splatProjMinY = j - splatCenter[1];
        int splatProjMaxY = splatProjMinY + splatDims[1];
        splatProjMinY = std::max(splatProjMinY, 0);
        splatProjMaxY = std::min(splatProjMaxY, imageDims[1]);

        for (int i = 0; i < imageDims[0]; i++)
        {
          // No need to splat 0.
          if (*b == 0)
          {
            b++;
            continue;
          }

          T value = static_cast<T>(*b);
          numPoints += static_cast<int>(*b);
          b++;

          // Figure out how splat projects on image in this pixel, taking into
          // account overlap.
          int splatProjMinX = i - splatCenter[0];
          int splatProjMaxX = splatProjMinX + splatDims[0];
          splatProjMinX = std::max(splatProjMinX, 0);
          splatProjMaxX = std::min(splatProjMaxX, imageDims[0]);

          // Do the splat.
          for (int imageZ = splatProjMinZ; imageZ < splatProjMaxZ; imageZ++)
          {
            int imageZOffset = imageZ * imageDims[0] * imageDims[1];
            int splatZ = imageZ - k + splatCenter[2];
            int splatZOffset = splatZ * splatDims[0] * splatDims[1];
            for (int imageY = splatProjMinY; imageY < splatProjMaxY; imageY++)
            {
              int imageYOffset = imageZOffset + imageY * imageDims[0];
              int splatY = imageY - j + splatCenter[1];
              int splatYOffset = splatZOffset + splatY * splatDims[0];
              for (int imageX = splatProjMinX; imageX < splatProjMaxX; imageX++)
              {
                int imageOffset = imageYOffset + imageX;
                int splatX = imageX - i + splatCenter[0];
                int splatOffset = splatYOffset + splatX;
                output[imageOffset] += value * splat[splatOffset];
              }
            }
          }
        }
      }
    }
    *numPointsSplatted = numPoints;
  }
};

//------------------------------------------------------------------------------
struct vtkFastSplatterClampWorker
{
  template <class TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* array, T minValue, T maxValue)
  {
    auto a = vtk::DataArrayValueRange(array);
    for (vtkIdType i = 0; i < array->GetNumberOfValues(); i++)
    {
      a[i] = std::clamp<T>(a[i], minValue, maxValue);
    }
  }
};

//-----------------------------------------------------------------------------
struct vtkFastSplatterFrozenScaleWorker
{
  template <class TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* scalarArray, T minValue, T maxValue, double min, double max)
  {
    auto array = vtk::DataArrayValueRange(scalarArray).begin();
    decltype(array) a;

    int numComponents = scalarArray->GetNumberOfComponents();
    vtkIdType numTuples = scalarArray->GetNumberOfTuples();
    vtkIdType t;
    for (int c = 0; c < numComponents; c++)
    {
      // Bias everything so that 0 is really the minimum.
      if (min != 0)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a -= static_cast<T>(min);
        }
      }

      // Scale the values.
      if (max != min)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a = static_cast<T>(((maxValue - minValue) * (*a)) / (max - min));
        }
      }

      // Bias everything again so that it lies in the correct range.
      if (minValue != 0)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a += minValue;
        }
      }
    }
  }
};

//-----------------------------------------------------------------------------
struct vtkFastSplatterScaleWorker
{
  template <class TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(
    TArray* scalarArray, T minValue, T maxValue, double* dataMinValue, double* dataMaxValue)
  {
    auto array = vtk::DataArrayValueRange(scalarArray).begin();
    T min, max;
    *dataMinValue = 0;
    *dataMaxValue = 0;
    vtkIdType t;
    int numComponents = scalarArray->GetNumberOfComponents();
    vtkIdType numTuples = scalarArray->GetNumberOfTuples();
    for (int c = 0; c < numComponents; c++)
    {
      // Find the min and max values in the array.
      auto a = array + c;
      min = max = *a;
      a += numComponents;
      for (t = 1; t < numTuples; t++, a += numComponents)
      {
        if (min > *a)
          min = *a;
        if (max < *a)
          max = *a;
      }

      // Bias everything so that 0 is really the minimum.
      if (min != 0)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a -= min;
        }
      }

      // Scale the values.
      if (max != min)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a = ((maxValue - minValue) * (*a)) / (max - min);
        }
      }

      // Bias everything again so that it lies in the correct range.
      if (minValue != 0)
      {
        for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
        {
          *a += minValue;
        }
      }
      if (c == 0)
      {
        *dataMinValue = min;
        *dataMaxValue = max;
      }
    }
  }
};

//------------------------------------------------------------------------------
// For those of you familiar with the old pipeline, this is equivalent to the
// Execute method.
int vtkFastSplatter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->NumberOfPointsSplatted = 0;

  // Get the input and output objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPoints* points = nullptr;
  if (vtkPointSet* const input =
        vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
  {
    points = input->GetPoints();
  }
  else if (vtkGraph* const graph =
             vtkGraph::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
  {
    points = graph->GetPoints();
  }

  vtkInformation* splatInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData* splatImage =
    vtkImageData::SafeDownCast(splatInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Figure out the real bounds to use.
  const double* bounds;
  if (((this->ModelBounds[0] < this->ModelBounds[1]) || (this->OutputDimensions[0] == 1)) &&
    ((this->ModelBounds[2] < this->ModelBounds[3]) || (this->OutputDimensions[1] == 1)) &&
    ((this->ModelBounds[4] < this->ModelBounds[5]) || (this->OutputDimensions[2] == 1)))
  {
    bounds = this->ModelBounds;
  }
  else
  {
    bounds = points->GetBounds();
  }

  // Compute origin and spacing from bounds
  for (int i = 0; i < 3; i++)
  {
    this->Origin[i] = bounds[2 * i];
    if (this->OutputDimensions[i] > 1)
    {
      this->Spacing[i] = ((bounds[2 * i + 1] - bounds[2 * i]) / (this->OutputDimensions[i] - 1));
    }
    else
    {
      this->Spacing[i] = 2.0 * (bounds[2 * i + 1] - bounds[2 * i]);
    }
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }

  // Set up output.
  output->SetDimensions(this->OutputDimensions);
  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
  output->SetOrigin(this->Origin);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
  output->SetSpacing(this->Spacing);
  output->SetExtent(0, this->OutputDimensions[0] - 1, 0, this->OutputDimensions[1] - 1, 0,
    this->OutputDimensions[2] - 1);
  output->AllocateScalars(splatImage->GetScalarType(), splatImage->GetNumberOfScalarComponents());

  // Set up intermediate buckets image.
  this->Buckets->SetDimensions(this->OutputDimensions);
  this->Buckets->SetOrigin(this->Origin);
  this->Buckets->SetSpacing(this->Spacing);
  this->Buckets->SetExtent(0, this->OutputDimensions[0] - 1, 0, this->OutputDimensions[1] - 1, 0,
    this->OutputDimensions[2] - 1);
  this->Buckets->AllocateScalars(VTK_UNSIGNED_INT, 1);

  // Get array for buckets.
  unsigned int* buckets =
    vtkAOSDataArrayTemplate<unsigned int>::FastDownCast(this->Buckets->GetPointData()->GetScalars())
      ->GetPointer(0);

  // Count how many points in the input lie in each pixel of the output image.
  vtkFastSplatterBucketPointsWorker pointsWorker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllPointArrays>::Execute(
        points->GetData(), pointsWorker, points->GetNumberOfPoints(), buckets,
        this->OutputDimensions, this->Origin, this->Spacing))
  {
    pointsWorker(points->GetData(), points->GetNumberOfPoints(), buckets, this->OutputDimensions,
      this->Origin, this->Spacing);
  }

  // Now convolve the splat image with the bucket image.
  auto splatArray = splatImage->GetPointData()->GetScalars();
  auto outputArray = output->GetPointData()->GetScalars();
  vtkFastSplatterConvolveWorker convolveWorker;
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(splatArray, outputArray, convolveWorker,
        splatImage->GetDimensions(), buckets, &this->NumberOfPointsSplatted,
        this->OutputDimensions))
  {
    convolveWorker(splatArray, outputArray, splatImage->GetDimensions(), buckets,
      &this->NumberOfPointsSplatted, this->OutputDimensions);
  }

  // Do any appropriate limiting.
  switch (this->LimitMode)
  {
    case NoneLimit:
      break;
    case ClampLimit:
    {
      vtkFastSplatterClampWorker clampWorker;
      if (!vtkArrayDispatch::Dispatch::Execute(
            outputArray, clampWorker, this->MinValue, this->MaxValue))
      {
        clampWorker(outputArray, this->MinValue, this->MaxValue);
      }
      break;
    }
    case FreezeScaleLimit:
    {
      vtkFastSplatterFrozenScaleWorker frozenScaleWorker;
      if (!vtkArrayDispatch::Dispatch::Execute(outputArray, frozenScaleWorker, this->MinValue,
            this->MaxValue, this->LastDataMinValue, this->LastDataMaxValue))
      {
        frozenScaleWorker(outputArray, this->MinValue, this->MaxValue, this->LastDataMinValue,
          this->LastDataMaxValue);
      }
      break;
    }

    case ScaleLimit:
    {
      vtkFastSplatterScaleWorker scaleWorker;
      if (!vtkArrayDispatch::Dispatch::Execute(outputArray, scaleWorker, this->MinValue,
            this->MaxValue, &this->LastDataMinValue, &this->LastDataMaxValue))
      {
        scaleWorker(outputArray, this->MinValue, this->MaxValue, &this->LastDataMinValue,
          &this->LastDataMaxValue);
      }
      break;
    }
  }

  return 1;
}

void vtkFastSplatter::SetSplatConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}
VTK_ABI_NAMESPACE_END
