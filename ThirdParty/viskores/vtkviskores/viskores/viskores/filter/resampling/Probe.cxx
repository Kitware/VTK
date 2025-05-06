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
#include <viskores/cont/internal/CastInvalidValue.h>

#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/resampling/Probe.h>
#include <viskores/filter/resampling/worklet/Probe.h>

namespace viskores
{
namespace filter
{
namespace resampling
{

namespace
{

bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                const viskores::worklet::Probe& worklet,
                viskores::Float64 invalidValue)
{
  if (field.IsPointField())
  {
    viskores::cont::UnknownArrayHandle inArray = field.GetData();
    viskores::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

    bool called = false;
    auto tryType = [&](auto t)
    {
      using T = std::decay_t<decltype(t)>;
      if (!called && inArray.IsBaseComponentType<T>())
      {
        called = true;
        viskores::IdComponent numComponents = inArray.GetNumberOfComponentsFlat();
        VISKORES_ASSERT(numComponents == outArray.GetNumberOfComponentsFlat());

        for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
        {
          worklet.ProcessPointField(inArray.ExtractComponent<T>(cIndex),
                                    outArray.ExtractComponent<T>(cIndex, viskores::CopyFlag::Off),
                                    viskores::cont::internal::CastInvalidValue<T>(invalidValue));
        }
      }
    };
    viskores::ListForEach(tryType, viskores::TypeListScalarAll{});
    if (!called)
    {
      VISKORES_LOG_CAST_FAIL(worklet, viskores::TypeListScalarAll);
      return false;
    }

    result.AddPointField(field.GetName(), outArray);
    return true;
  }
  else if (field.IsCellField())
  {
    viskores::cont::Field outField;
    if (viskores::filter::MapFieldPermutation(field, worklet.GetCellIds(), outField, invalidValue))
    {
      // output field should be associated with points
      outField = viskores::cont::Field(
        field.GetName(), viskores::cont::Field::Association::Points, outField.GetData());
      result.AddField(outField);
      return true;
    }
    return false;
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

viskores::cont::DataSet Probe::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::worklet::Probe worklet;
  worklet.Run(input.GetCellSet(),
              input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
              this->Geometry.GetCoordinateSystem().GetData());

  auto mapper = [&](auto& outDataSet, const auto& f)
  { DoMapField(outDataSet, f, worklet, this->InvalidValue); };
  auto output = this->CreateResultCoordinateSystem(
    input, this->Geometry.GetCellSet(), this->Geometry.GetCoordinateSystem(), mapper);
  output.AddField(viskores::cont::make_FieldPoint("HIDDEN", worklet.GetHiddenPointsField()));
  output.AddField(
    viskores::cont::make_FieldCell("HIDDEN", worklet.GetHiddenCellsField(output.GetCellSet())));

  return output;
}

} // namespace resampling
} // namespace filter
} // namespace viskores
