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
#ifndef viskores_m_worklet_ThresholdPoints_h
#define viskores_m_worklet_ThresholdPoints_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

class ThresholdPoints
{
public:
  template <typename UnaryPredicate>
  class ThresholdPointField : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutPoint passFlags);
    using ExecutionSignature = _3(_2);

    VISKORES_CONT
    ThresholdPointField()
      : Predicate()
    {
    }

    VISKORES_CONT
    explicit ThresholdPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarType>
    VISKORES_EXEC bool operator()(const ScalarType& scalar) const
    {
      return this->Predicate(scalar);
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType, typename ScalarsArrayHandle, typename UnaryPredicate>
  viskores::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                          const ScalarsArrayHandle& scalars,
                                          const UnaryPredicate& predicate)
  {
    viskores::cont::ArrayHandle<bool> passFlags;

    using ThresholdWorklet = ThresholdPointField<UnaryPredicate>;

    ThresholdWorklet worklet(predicate);
    viskores::cont::Invoker invoker;
    invoker(worklet, cellSet, scalars, passFlags);

    viskores::cont::ArrayHandle<viskores::Id> pointIds;
    viskores::cont::ArrayHandleCounting<viskores::Id> indices =
      viskores::cont::make_ArrayHandleCounting(
        viskores::Id(0), viskores::Id(1), passFlags.GetNumberOfValues());
    viskores::cont::Algorithm::CopyIf(indices, passFlags, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    viskores::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(cellSet.GetNumberOfPoints(), viskores::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_ThresholdPoints_h
