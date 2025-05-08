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

#include <viskores/source/Wavelet.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{
inline viskores::FloatDefault computeScaleFactor(viskores::Id min, viskores::Id max)
{
  return (min < max) ? (1.f / static_cast<viskores::FloatDefault>(max - min))
                     : static_cast<viskores::FloatDefault>(1.);
}
}
namespace viskores
{
namespace source
{
namespace wavelet
{

struct WaveletField : public viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  using Vec3F = viskores::Vec3f;

  Vec3F Center;
  Vec3F Spacing;
  Vec3F Frequency;
  Vec3F Magnitude;
  Vec3F MinimumPoint;
  Vec3F Scale;
  viskores::Id3 Offset;
  viskores::Id3 Dims;
  viskores::FloatDefault MaximumValue;
  viskores::FloatDefault Temp2;

  WaveletField(const Vec3F& center,
               const Vec3F& spacing,
               const Vec3F& frequency,
               const Vec3F& magnitude,
               const Vec3F& minimumPoint,
               const Vec3F& scale,
               const viskores::Id3& offset,
               const viskores::Id3& dims,
               viskores::FloatDefault maximumValue,
               viskores::FloatDefault temp2)
    : Center(center)
    , Spacing(spacing)
    , Frequency(frequency)
    , Magnitude(magnitude)
    , MinimumPoint(minimumPoint)
    , Scale(scale)
    , Offset(offset)
    , Dims(dims)
    , MaximumValue(maximumValue)
    , Temp2(temp2)
  {
  }

  template <typename ThreadIndexType>
  VISKORES_EXEC void operator()(const ThreadIndexType& threadIndex,
                                viskores::FloatDefault& scalar) const
  {
    const viskores::Id3 ijk = threadIndex.GetInputIndex3D();

    // map ijk to the point location, accounting for spacing:
    const Vec3F loc = Vec3F(ijk + this->Offset) * this->Spacing;

    // Compute the distance from the center of the gaussian:
    const Vec3F scaledLoc = (this->Center - loc) * this->Scale;
    viskores::FloatDefault gaussSum = viskores::Dot(scaledLoc, scaledLoc);

    const Vec3F periodicContribs{
      this->Magnitude[0] * viskores::Sin(this->Frequency[0] * scaledLoc[0]),
      this->Magnitude[1] * viskores::Sin(this->Frequency[1] * scaledLoc[1]),
      this->Magnitude[2] * viskores::Cos(this->Frequency[2] * scaledLoc[2]),
    };

    // The vtkRTAnalyticSource documentation says the periodic contributions
    // should be multiplied in, but the implementation adds them. We'll do as
    // they do, not as they say.
    scalar = this->MaximumValue * viskores::Exp(-gaussSum * this->Temp2) +
      viskores::ReduceSum(periodicContribs);
  }
};
} // namespace wavelet

Wavelet::Wavelet(viskores::Id3 minExtent, viskores::Id3 maxExtent)
  : MinimumExtent(minExtent)
  , MaximumExtent(maxExtent)
{
}

template <viskores::IdComponent Dim>
viskores::cont::DataSet Wavelet::GenerateDataSet(viskores::cont::CoordinateSystem coords) const
{
  // And cells:
  viskores::Vec<viskores::Id, Dim> dims;
  for (unsigned int d = 0; d < Dim; d++)
  {
    dims[d] = this->MaximumExtent[d] - this->MinimumExtent[d] + 1;
  }
  viskores::cont::CellSetStructured<Dim> cellSet;
  cellSet.SetPointDimensions(dims);

  // Compile the dataset:
  viskores::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(coords);
  dataSet.SetCellSet(cellSet);

  // Scalars, too
  viskores::cont::Field field = this->GeneratePointField(cellSet, "RTData");
  dataSet.AddField(field);

  return dataSet;
}

viskores::cont::DataSet Wavelet::DoExecute() const
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  // Create points:
  const viskores::Id3 dims{ this->MaximumExtent - this->MinimumExtent + viskores::Id3{ 1 } };
  viskores::cont::CoordinateSystem coords{ "coordinates", dims, this->GetOrigin(), this->Spacing };

  // Compile the dataset:
  if (this->MaximumExtent[2] - this->MinimumExtent[2] < viskores::Epsilon<viskores::FloatDefault>())
  {
    return this->GenerateDataSet<2>(coords);
  }
  else
  {
    return this->GenerateDataSet<3>(coords);
  }
}

template <viskores::IdComponent Dim>
viskores::cont::Field Wavelet::GeneratePointField(
  const viskores::cont::CellSetStructured<Dim>& cellset,
  const std::string& name) const
{
  const viskores::Id3 dims{ this->MaximumExtent - this->MinimumExtent + viskores::Id3{ 1 } };
  viskores::Vec3f minPt = viskores::Vec3f(this->MinimumExtent) * this->Spacing;
  viskores::FloatDefault temp2 = 1.f / (2.f * this->StandardDeviation * this->StandardDeviation);
  viskores::Vec3f scale{ computeScaleFactor(this->MinimumExtent[0], this->MaximumExtent[0]),
                         computeScaleFactor(this->MinimumExtent[1], this->MaximumExtent[1]),
                         computeScaleFactor(this->MinimumExtent[2], this->MaximumExtent[2]) };

  viskores::cont::ArrayHandle<viskores::FloatDefault> output;
  wavelet::WaveletField worklet{ this->Center,
                                 this->Spacing,
                                 this->Frequency,
                                 this->Magnitude,
                                 minPt,
                                 scale,
                                 this->MinimumExtent,
                                 dims,
                                 this->MaximumValue,
                                 temp2 };
  this->Invoke(worklet, cellset, output);
  return viskores::cont::make_FieldPoint(name, output);
}

} // namespace source
} // namespace viskores
