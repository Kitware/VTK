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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/source/Tangle.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace source
{
namespace tangle
{
class TangleField : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  const viskores::Vec3f CellDimsf;
  const viskores::Vec3f Mins;
  const viskores::Vec3f Maxs;

  VISKORES_CONT
  TangleField(const viskores::Id3& cdims, const viskores::Vec3f& mins, const viskores::Vec3f& maxs)
    : CellDimsf(static_cast<viskores::FloatDefault>(cdims[0]),
                static_cast<viskores::FloatDefault>(cdims[1]),
                static_cast<viskores::FloatDefault>(cdims[2]))
    , Mins(mins)
    , Maxs(maxs)
  {
  }

  template <typename ThreadIndexType>
  VISKORES_EXEC void operator()(const ThreadIndexType& threadIndex, viskores::Float32& v) const
  {
    //We are operating on a 3d structured grid. This means that the threadIndex has
    //efficiently computed the i,j,k of the point current point for us
    const viskores::Id3 ijk = threadIndex.GetInputIndex3D();
    const viskores::Vec3f xyzf = static_cast<viskores::Vec3f>(ijk) / this->CellDimsf;

    const viskores::Vec3f_32 values = 3.0f * viskores::Vec3f_32(Mins + (Maxs - Mins) * xyzf);
    const viskores::Float32& xx = values[0];
    const viskores::Float32& yy = values[1];
    const viskores::Float32& zz = values[2];

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};
} // namespace tangle

viskores::cont::DataSet Tangle::DoExecute() const
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  viskores::cont::DataSet dataSet;

  const viskores::Vec3f mins = { -1.0f, -1.0f, -1.0f };
  const viskores::Vec3f maxs = { 1.0f, 1.0f, 1.0f };

  viskores::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(this->PointDimensions);
  dataSet.SetCellSet(cellSet);

  viskores::Id3 cellDims = this->GetCellDimensions();

  viskores::cont::ArrayHandle<viskores::Float32> pointFieldArray;
  this->Invoke(tangle::TangleField{ cellDims, mins, maxs }, cellSet, pointFieldArray);

  const viskores::Vec3f origin(0.0f, 0.0f, 0.0f);
  const viskores::Vec3f spacing(1.0f / static_cast<viskores::FloatDefault>(cellDims[0]),
                                1.0f / static_cast<viskores::FloatDefault>(cellDims[1]),
                                1.0f / static_cast<viskores::FloatDefault>(cellDims[2]));

  viskores::cont::ArrayHandleUniformPointCoordinates coordinates(
    this->PointDimensions, origin, spacing);
  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", coordinates));
  dataSet.AddField(viskores::cont::make_FieldPoint("tangle", pointFieldArray));

  return dataSet;
}

} // namespace source
} // namespace viskores
