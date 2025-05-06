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
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#include <viskores/filter/entity_extraction/worklet/ThresholdPoints.h>

namespace
{
// Predicate for values less than minimum
class ValuesBelow
{
public:
  VISKORES_CONT
  explicit ValuesBelow(const viskores::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VISKORES_EXEC bool operator()(const ScalarType& value) const
  {
    return value <= static_cast<ScalarType>(this->Value);
  }

private:
  viskores::Float64 Value;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  VISKORES_CONT
  explicit ValuesAbove(const viskores::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VISKORES_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Value);
  }

private:
  viskores::Float64 Value;
};

// Predicate for values between minimum and maximum

class ValuesBetween
{
public:
  VISKORES_CONT
  ValuesBetween(const viskores::Float64& lower, const viskores::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename ScalarType>
  VISKORES_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Lower) &&
      value <= static_cast<ScalarType>(this->Upper);
  }

private:
  viskores::Float64 Lower;
  viskores::Float64 Upper;
};

bool DoMapField(viskores::cont::DataSet& result, const viskores::cont::Field& field)
{
  // point data is copied as is because it was not collapsed
  if (field.IsPointField())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    // cell data does not apply
    return false;
  }
}

} // anonymous namespace

namespace viskores
{
namespace filter
{

namespace entity_extraction
{
//-----------------------------------------------------------------------------
VISKORES_CONT void ThresholdPoints::SetThresholdBelow(const viskores::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_BELOW;
}

VISKORES_CONT void ThresholdPoints::SetThresholdAbove(const viskores::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_ABOVE;
}

VISKORES_CONT void ThresholdPoints::SetThresholdBetween(const viskores::Float64 value1,
                                                        const viskores::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
  this->ThresholdType = THRESHOLD_BETWEEN;
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet ThresholdPoints::DoExecute(
  const viskores::cont::DataSet& input)
{
  // extract the input cell set
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  // field to threshold on must be a point field
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  // run the worklet on the cell set and input field
  viskores::cont::CellSetSingleType<> outCellSet;
  viskores::worklet::ThresholdPoints worklet;

  auto resolveType = [&](const auto& concrete)
  {
    switch (this->ThresholdType)
    {
      case THRESHOLD_BELOW:
      {
        outCellSet = worklet.Run(cells, concrete, ValuesBelow(this->GetLowerThreshold()));
        break;
      }
      case THRESHOLD_ABOVE:
      {
        outCellSet = worklet.Run(cells, concrete, ValuesAbove(this->GetUpperThreshold()));
        break;
      }
      case THRESHOLD_BETWEEN:
      default:
      {
        outCellSet = worklet.Run(
          cells, concrete, ValuesBetween(this->GetLowerThreshold(), this->GetUpperThreshold()));
        break;
      }
    }
  };

  this->CastAndCallScalarField(field, resolveType);

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f); };
  viskores::cont::DataSet output = this->CreateResult(input, outCellSet, mapper);

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    viskores::filter::clean_grid::CleanGrid compactor;
    compactor.SetCompactPointFields(true);
    compactor.SetMergePoints(true);
    return compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

} // namespace entity_extraction
} // namespace filter
} // namespace viskores
