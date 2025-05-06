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
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/entity_extraction/Threshold.h>
#include <viskores/filter/entity_extraction/worklet/Threshold.h>

#include <viskores/cont/Invoker.h>

#include <viskores/BinaryPredicates.h>
#include <viskores/Math.h>

namespace
{
class ThresholdRange
{
public:
  VISKORES_CONT ThresholdRange() = default;

  VISKORES_CONT
  ThresholdRange(const viskores::Float64& lower, const viskores::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  VISKORES_EXEC bool operator()(const T& value) const
  {
    return static_cast<viskores::Float64>(value) >= this->Lower &&
      static_cast<viskores::Float64>(value) <= this->Upper;
  }

private:
  viskores::Float64 Lower;
  viskores::Float64 Upper;
};

bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                const viskores::worklet::Threshold& worklet)
{
  if (field.IsPointField() || field.IsWholeDataSetField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}

template <typename Operator>
class CombinePassFlagsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut, FieldIn);
  using ExecitionSignature = void(_1, _2);

  VISKORES_CONT
  explicit CombinePassFlagsWorklet(const Operator& combine)
    : Combine(combine)
  {
  }

  VISKORES_EXEC void operator()(bool& combined, bool incoming) const
  {
    combined = this->Combine(combined, incoming);
  }

private:
  Operator Combine;
};

class ThresholdPassFlag
{
public:
  VISKORES_CONT ThresholdPassFlag() = default;

  VISKORES_EXEC bool operator()(bool value) const { return value; }
};

} // end anon namespace

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VISKORES_CONT void Threshold::SetThresholdBelow(viskores::Float64 value)
{
  this->SetLowerThreshold(viskores::NegativeInfinity<viskores::Float64>());
  this->SetUpperThreshold(value);
}

VISKORES_CONT void Threshold::SetThresholdAbove(viskores::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(viskores::Infinity<viskores::Float64>());
}

VISKORES_CONT void Threshold::SetThresholdBetween(viskores::Float64 value1,
                                                  viskores::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet Threshold::DoExecute(const viskores::cont::DataSet& input)
{
  //get the cells and coordinates of the dataset
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  viskores::worklet::Threshold worklet;
  viskores::cont::UnknownCellSet cellOut;

  auto callWithArrayBaseComponent = [&](auto baseComp)
  {
    using ComponentType = decltype(baseComp);
    if (!field.GetData().IsBaseComponentType<ComponentType>())
    {
      return;
    }

    if (this->ComponentMode == Component::Selected || field.GetData().GetNumberOfComponents() == 1)
    {
      auto arrayComponent =
        field.GetData().ExtractComponent<ComponentType>(this->SelectedComponent);
      cellOut = worklet.Run(
        cells, arrayComponent, field.GetAssociation(), predicate, this->AllInRange, this->Invert);
    }
    else
    {
      viskores::cont::ArrayHandle<bool> passFlags;
      if (this->ComponentMode == Component::Any)
      {
        auto combineWorklet = CombinePassFlagsWorklet<viskores::LogicalOr>(viskores::LogicalOr{});
        passFlags.AllocateAndFill(field.GetNumberOfValues(), false);
        for (viskores::IdComponent i = 0; i < field.GetData().GetNumberOfComponents(); ++i)
        {
          auto arrayComponent = field.GetData().ExtractComponent<ComponentType>(i);
          auto thresholded = viskores::cont::make_ArrayHandleTransform(arrayComponent, predicate);
          viskores::cont::Invoker()(combineWorklet, passFlags, thresholded);
        }
      }
      else // this->ComponentMode == Component::All
      {
        auto combineWorklet = CombinePassFlagsWorklet<viskores::LogicalAnd>(viskores::LogicalAnd{});
        passFlags.AllocateAndFill(field.GetNumberOfValues(), true);
        for (viskores::IdComponent i = 0; i < field.GetData().GetNumberOfComponents(); ++i)
        {
          auto arrayComponent = field.GetData().ExtractComponent<ComponentType>(i);
          auto thresholded = viskores::cont::make_ArrayHandleTransform(arrayComponent, predicate);
          viskores::cont::Invoker()(combineWorklet, passFlags, thresholded);
        }
      }

      cellOut = worklet.Run(cells,
                            passFlags,
                            field.GetAssociation(),
                            ThresholdPassFlag{},
                            this->AllInRange,
                            this->Invert);
    }
  };

  viskores::ListForEach(callWithArrayBaseComponent, viskores::TypeListScalarAll{});

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}
} // namespace entity_extraction
} // namespace filter
} // namespace viskores
