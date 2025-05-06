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
#ifndef viskores_m_worklet_Threshold_h
#define viskores_m_worklet_Threshold_h

#include <viskores/worklet/CellDeepCopy.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/UnaryPredicates.h>

namespace viskores
{
namespace worklet
{

class Threshold
{
public:
  template <typename UnaryPredicate>
  class ThresholdByPointField : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutCell passFlags);

    using ExecutionSignature = _3(_2, PointCount);

    VISKORES_CONT
    ThresholdByPointField()
      : Predicate()
      , AllPointsMustPass()
    {
    }

    VISKORES_CONT
    explicit ThresholdByPointField(const UnaryPredicate& predicate, bool allPointsMustPass)
      : Predicate(predicate)
      , AllPointsMustPass(allPointsMustPass)
    {
    }

    template <typename ScalarsVecType>
    VISKORES_EXEC bool operator()(const ScalarsVecType& scalars, viskores::Id count) const
    {
      bool pass = this->AllPointsMustPass ? true : false;
      for (viskores::IdComponent i = 0; i < count; ++i)
      {
        if (this->AllPointsMustPass)
        {
          pass &= this->Predicate(scalars[i]);
        }
        else
        {
          pass |= this->Predicate(scalars[i]);
        }
      }

      return pass;
    }

  private:
    UnaryPredicate Predicate;
    bool AllPointsMustPass;
  };

  template <typename CellSetType, typename ValueType, typename StorageType, typename UnaryPredicate>
  viskores::cont::CellSetPermutation<CellSetType> RunImpl(
    const CellSetType& cellSet,
    const viskores::cont::ArrayHandle<ValueType, StorageType>& field,
    viskores::cont::Field::Association fieldType,
    const UnaryPredicate& predicate,
    bool allPointsMustPass,
    bool invert)
  {
    using OutputType = viskores::cont::CellSetPermutation<CellSetType>;

    viskores::cont::ArrayHandle<bool> passFlags;
    switch (fieldType)
    {
      case viskores::cont::Field::Association::Points:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate, allPointsMustPass);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }
      case viskores::cont::Field::Association::Cells:
      {
        viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleTransform(field, predicate),
                                        passFlags);
        break;
      }
      default:
        throw viskores::cont::ErrorBadValue("Expecting point or cell field.");
    }

    if (invert)
    {
      viskores::cont::Algorithm::Copy(
        viskores::cont::make_ArrayHandleTransform(passFlags, viskores::LogicalNot{}), passFlags);
    }

    viskores::cont::Algorithm::CopyIf(
      viskores::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()),
      passFlags,
      this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
  }

  template <typename ValueType, typename StorageType, typename UnaryPredicate>
  viskores::cont::UnknownCellSet Run(
    const viskores::cont::UnknownCellSet& cellSet,
    const viskores::cont::ArrayHandle<ValueType, StorageType>& field,
    viskores::cont::Field::Association fieldType,
    const UnaryPredicate& predicate,
    bool allPointsMustPass = false, // only considered when field association is `Points`
    bool invert = false)
  {
    viskores::cont::UnknownCellSet output;
    CastAndCall(cellSet,
                [&](auto concrete)
                {
                  output = viskores::worklet::CellDeepCopy::Run(this->RunImpl(
                    concrete, field, fieldType, predicate, allPointsMustPass, invert));
                });
    return output;
  }

  viskores::cont::ArrayHandle<viskores::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  viskores::cont::ArrayHandle<viskores::Id> ValidCellIds;
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_Threshold_h
