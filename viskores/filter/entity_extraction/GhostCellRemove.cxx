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

#include <viskores/RangeId3.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/entity_extraction/ExtractStructured.h>
#include <viskores/filter/entity_extraction/GhostCellRemove.h>
#include <viskores/filter/entity_extraction/worklet/Threshold.h>

namespace
{

template <typename T>
VISKORES_EXEC inline bool ShouldRemove(T value, viskores::UInt8 removeTypes)
{
  return ((value & removeTypes) != 0);
}

class RemoveGhostPredicate
{
public:
  VISKORES_CONT RemoveGhostPredicate()
    : RemoveTypes(0xFF)
  {
  }

  VISKORES_CONT explicit RemoveGhostPredicate(viskores::UInt8 val)
    : RemoveTypes(val)
  {
  }

  VISKORES_EXEC bool operator()(const viskores::UInt8& value) const
  {
    return !ShouldRemove(value, this->RemoveTypes);
  }

private:
  viskores::UInt8 RemoveTypes;
};

template <int DIMS>
VISKORES_EXEC_CONT viskores::Id3 getLogical(const viskores::Id& index,
                                            const viskores::Id3& cellDims);

template <>
VISKORES_EXEC_CONT viskores::Id3 getLogical<3>(const viskores::Id& index,
                                               const viskores::Id3& cellDims)
{
  viskores::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = (index / (cellDims[0])) % (cellDims[1]);
  res[2] = index / ((cellDims[0]) * (cellDims[1]));
  return res;
}

template <>
VISKORES_EXEC_CONT viskores::Id3 getLogical<2>(const viskores::Id& index,
                                               const viskores::Id3& cellDims)
{
  viskores::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = index / cellDims[0];
  return res;
}

template <>
VISKORES_EXEC_CONT viskores::Id3 getLogical<1>(const viskores::Id& index, const viskores::Id3&)
{
  viskores::Id3 res(0, 0, 0);
  res[0] = index;
  return res;
}

template <int DIMS>
class RealMinMax : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  RealMinMax(viskores::Id3 cellDims, viskores::UInt8 removeTypes)
    : CellDims(cellDims)
    , RemoveTypes(removeTypes)
  {
  }

  typedef void ControlSignature(FieldIn, AtomicArrayInOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename Atomic>
  VISKORES_EXEC void Max(Atomic& atom, const viskores::Id& val, const viskores::Id& index) const
  {
    viskores::Id old = atom.Get(index);
    while (old < val)
    {
      atom.CompareExchange(index, &old, val);
    }
  }

  template <typename Atomic>
  VISKORES_EXEC void Min(Atomic& atom, const viskores::Id& val, const viskores::Id& index) const
  {
    viskores::Id old = atom.Get(index);
    while (old > val)
    {
      atom.CompareExchange(index, &old, val);
    }
  }

  template <typename T, typename AtomicType>
  VISKORES_EXEC void operator()(const T& value, const viskores::Id& index, AtomicType& atom) const
  {
    // we are finding the logical min max of valid cells
    if (ShouldRemove(value, this->RemoveTypes))
    {
      return;
    }

    viskores::Id3 logical = getLogical<DIMS>(index, CellDims);

    Min(atom, logical[0], 0);
    Min(atom, logical[1], 1);
    Min(atom, logical[2], 2);

    Max(atom, logical[0], 3);
    Max(atom, logical[1], 4);
    Max(atom, logical[2], 5);
  }

private:
  viskores::Id3 CellDims;
  viskores::UInt8 RemoveTypes;
};

template <int DIMS>
VISKORES_EXEC_CONT bool checkRange(const viskores::RangeId3& range, const viskores::Id3& p);

template <>
VISKORES_EXEC_CONT bool checkRange<1>(const viskores::RangeId3& range, const viskores::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max;
}
template <>
VISKORES_EXEC_CONT bool checkRange<2>(const viskores::RangeId3& range, const viskores::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max;
}
template <>
VISKORES_EXEC_CONT bool checkRange<3>(const viskores::RangeId3& range, const viskores::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max &&
    p[2] >= range.Z.Min && p[2] <= range.Z.Max;
}

template <int DIMS>
class Validate : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  Validate(const viskores::Id3& cellDims,
           viskores::UInt8 removeTypes,
           const viskores::RangeId3& range)
    : CellDims(cellDims)
    , RemoveVals(removeTypes)
    , Range(range)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename T>
  VISKORES_EXEC void operator()(const T& value,
                                const viskores::Id& index,
                                viskores::UInt8& invalid) const
  {
    if (ShouldRemove(value, this->RemoveVals) &&
        checkRange<DIMS>(Range, getLogical<DIMS>(index, CellDims)))
    {
      invalid = static_cast<viskores::UInt8>(1);
    }
    else
    {
      invalid = 0;
    }
  }

private:
  viskores::Id3 CellDims;
  viskores::UInt8 RemoveVals;
  viskores::RangeId3 Range;
};

template <int DIMS, typename T, typename StorageType>
bool CanStrip(const viskores::cont::ArrayHandle<T, StorageType>& ghostField,
              const viskores::cont::Invoker& invoke,
              viskores::UInt8 removeTypes,
              viskores::RangeId3& range,
              const viskores::Id3& cellDims)
{
  viskores::cont::ArrayHandle<viskores::Id> minmax;
  minmax.Allocate(6);
  minmax.WritePortal().Set(0, std::numeric_limits<viskores::Id>::max());
  minmax.WritePortal().Set(1, std::numeric_limits<viskores::Id>::max());
  minmax.WritePortal().Set(2, std::numeric_limits<viskores::Id>::max());
  minmax.WritePortal().Set(3, std::numeric_limits<viskores::Id>::min());
  minmax.WritePortal().Set(4, std::numeric_limits<viskores::Id>::min());
  minmax.WritePortal().Set(5, std::numeric_limits<viskores::Id>::min());

  invoke(RealMinMax<3>(cellDims, removeTypes), ghostField, minmax);

  auto portal = minmax.ReadPortal();
  range = viskores::RangeId3(
    portal.Get(0), portal.Get(3), portal.Get(1), portal.Get(4), portal.Get(2), portal.Get(5));

  viskores::cont::ArrayHandle<viskores::UInt8> invalidFlags;

  invoke(Validate<DIMS>(cellDims, removeTypes, range), ghostField, invalidFlags);

  viskores::UInt8 res =
    viskores::cont::Algorithm::Reduce(invalidFlags, viskores::UInt8(0), viskores::Maximum());
  return res == 0;
}

template <typename T, typename StorageType>
bool CanDoStructuredStrip(const viskores::cont::UnknownCellSet& cells,
                          const viskores::cont::ArrayHandle<T, StorageType>& ghostField,
                          const viskores::cont::Invoker& invoke,
                          viskores::UInt8 removeTypes,
                          viskores::RangeId3& range)
{
  bool canDo = false;
  viskores::Id3 cellDims(1, 1, 1);

  if (cells.CanConvert<viskores::cont::CellSetStructured<1>>())
  {
    auto cells1D = cells.AsCellSet<viskores::cont::CellSetStructured<1>>();
    viskores::Id d = cells1D.GetCellDimensions();
    cellDims[0] = d;
    VISKORES_ASSERT(ghostField.GetNumberOfValues() == cellDims[0]);
    canDo = CanStrip<1>(ghostField, invoke, removeTypes, range, cellDims);
  }
  else if (cells.CanConvert<viskores::cont::CellSetStructured<2>>())
  {
    auto cells2D = cells.AsCellSet<viskores::cont::CellSetStructured<2>>();
    viskores::Id2 d = cells2D.GetCellDimensions();
    cellDims[0] = d[0];
    cellDims[1] = d[1];
    VISKORES_ASSERT(ghostField.GetNumberOfValues() == (cellDims[0] * cellDims[1]));
    canDo = CanStrip<2>(ghostField, invoke, removeTypes, range, cellDims);
  }
  else if (cells.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    auto cells3D = cells.AsCellSet<viskores::cont::CellSetStructured<3>>();
    cellDims = cells3D.GetCellDimensions();
    VISKORES_ASSERT(ghostField.GetNumberOfValues() == (cellDims[0] * cellDims[1] * cellDims[2]));
    canDo = CanStrip<3>(ghostField, invoke, removeTypes, range, cellDims);
  }

  return canDo;
}

bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                const viskores::worklet::Threshold& worklet)
{
  if (field.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
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
} // end anon namespace

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VISKORES_CONT GhostCellRemove::GhostCellRemove()
{
  this->SetActiveField(viskores::cont::GetGlobalGhostCellFieldName());
  this->SetFieldsToPass(viskores::cont::GetGlobalGhostCellFieldName(),
                        viskores::filter::FieldSelection::Mode::Exclude);
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet GhostCellRemove::DoExecute(
  const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const viskores::cont::Field& field =
    (this->GetUseGhostCellsAsField() ? input.GetGhostCellField()
                                     : this->GetFieldFromDataSet(input));

  viskores::cont::ArrayHandle<viskores::UInt8> fieldArray;
  viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), fieldArray);

  //Preserve structured output where possible.
  if (cells.CanConvert<viskores::cont::CellSetStructured<1>>() ||
      cells.CanConvert<viskores::cont::CellSetStructured<2>>() ||
      cells.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    viskores::RangeId3 range;
    if (CanDoStructuredStrip(cells, fieldArray, this->Invoke, this->GetTypesToRemove(), range))
    {
      viskores::filter::entity_extraction::ExtractStructured extract;
      extract.SetInvoker(this->Invoke);
      viskores::RangeId3 erange(
        range.X.Min, range.X.Max + 2, range.Y.Min, range.Y.Max + 2, range.Z.Min, range.Z.Max + 2);
      viskores::Id3 sample(1, 1, 1);
      extract.SetVOI(erange);
      extract.SetSampleRate(sample);
      if (this->GetRemoveGhostField())
        extract.SetFieldsToPass(this->GetActiveFieldName(),
                                viskores::filter::FieldSelection::Mode::Exclude);

      auto output = extract.Execute(input);
      return output;
    }
  }

  viskores::cont::UnknownCellSet cellOut;
  viskores::worklet::Threshold worklet;

  cellOut = worklet.Run(
    cells, fieldArray, field.GetAssociation(), RemoveGhostPredicate(this->GetTypesToRemove()));

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}

}
}
}
