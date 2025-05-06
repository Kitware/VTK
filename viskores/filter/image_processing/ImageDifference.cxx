//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/UncertainCellSet.h>

#include <viskores/filter/image_processing/ImageDifference.h>
#include <viskores/filter/image_processing/worklet/ImageDifference.h>
#include <viskores/worklet/AveragePointNeighborhood.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{
namespace
{
struct GreaterThanThreshold
{
  GreaterThanThreshold(const viskores::FloatDefault& thresholdError)
    : ThresholdError(thresholdError)
  {
  }
  VISKORES_EXEC_CONT bool operator()(const viskores::FloatDefault& x) const
  {
    return x > ThresholdError;
  }
  viskores::FloatDefault ThresholdError;
};
} // anonymous namespace

VISKORES_CONT ImageDifference::ImageDifference()
{
  this->SetPrimaryField("image-1");
  this->SetSecondaryField("image-2");
  this->SetOutputFieldName("image-diff");
}

VISKORES_CONT viskores::cont::DataSet ImageDifference::DoExecute(
  const viskores::cont::DataSet& input)
{
  this->ImageDiffWithinThreshold = true;

  const auto& primaryField = this->GetFieldFromDataSet(input);
  if (!primaryField.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Performing Image Difference");

  auto inputCellSet =
    input.GetCellSet().ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>();

  const auto& secondaryField = this->GetFieldFromDataSet(1, input);

  viskores::cont::UnknownArrayHandle diffOutput;
  viskores::cont::ArrayHandle<viskores::FloatDefault> thresholdOutput;

  auto resolveType = [&](const auto& primaryArray)
  {
    // use std::decay to remove const ref from the decltype of primaryArray.
    using T = typename std::decay_t<decltype(primaryArray)>::ValueType;
    viskores::cont::ArrayHandle<T> secondaryArray;
    viskores::cont::ArrayCopyShallowIfPossible(secondaryField.GetData(), secondaryArray);

    viskores::cont::ArrayHandle<T> primaryOutput;
    viskores::cont::ArrayHandle<T> secondaryOutput;
    if (this->AverageRadius > 0)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Performing Average with radius: " << this->AverageRadius);
      auto averageWorklet = viskores::worklet::AveragePointNeighborhood(this->AverageRadius);
      this->Invoke(averageWorklet, inputCellSet, primaryArray, primaryOutput);
      this->Invoke(averageWorklet, inputCellSet, secondaryArray, secondaryOutput);
    }
    else
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Not performing average");
      viskores::cont::ArrayCopyShallowIfPossible(primaryArray, primaryOutput);
      secondaryOutput = secondaryArray;
    }

    viskores::cont::ArrayHandle<T> diffArray;
    if (this->PixelShiftRadius > 0)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Diffing image in Neighborhood");
      this->Invoke(viskores::worklet::ImageDifferenceNeighborhood(this->PixelShiftRadius,
                                                                  this->PixelDiffThreshold),
                   inputCellSet,
                   primaryOutput,
                   secondaryOutput,
                   diffArray,
                   thresholdOutput);
    }
    else
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Diffing image directly");
      this->Invoke(viskores::worklet::ImageDifference(),
                   primaryOutput,
                   secondaryOutput,
                   diffArray,
                   thresholdOutput);
    }
    diffOutput = diffArray;
  };
  this->CastAndCallVecField<4>(primaryField, resolveType);

  viskores::cont::ArrayHandle<viskores::FloatDefault> errorPixels;
  viskores::cont::Algorithm::CopyIf(
    thresholdOutput, thresholdOutput, errorPixels, GreaterThanThreshold(this->PixelDiffThreshold));
  if (errorPixels.GetNumberOfValues() >
      thresholdOutput.GetNumberOfValues() * this->AllowedPixelErrorRatio)
  {
    this->ImageDiffWithinThreshold = false;
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Difference within threshold: "
                   << this->ImageDiffWithinThreshold
                   << ", for pixels outside threshold: " << errorPixels.GetNumberOfValues()
                   << ", with a total number of pixels: " << thresholdOutput.GetNumberOfValues()
                   << ", and an allowable pixel error ratio: " << this->AllowedPixelErrorRatio
                   << ", with a total summed threshold error: "
                   << viskores::cont::Algorithm::Reduce(errorPixels, static_cast<FloatDefault>(0)));

  auto outputDataSet = this->CreateResultFieldPoint(input, this->GetOutputFieldName(), diffOutput);
  outputDataSet.AddPointField(this->GetThresholdFieldName(), thresholdOutput);
  return outputDataSet;
}
}
} // namespace filter
} // namespace viskores
