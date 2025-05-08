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
#ifndef viskores_m_worklet_ExtractPoints_h
#define viskores_m_worklet_ExtractPoints_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Invoker.h>

#include <viskores/ImplicitFunction.h>

#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

class ExtractPoints
{
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify points within volume of interest
  class ExtractPointsByVOI : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint coordinates,
                                  ExecObject function,
                                  FieldOutPoint passFlags);
    using ExecutionSignature = _4(_2, _3);

    VISKORES_CONT
    explicit ExtractPointsByVOI(bool extractInside)
      : passValue(extractInside)
      , failValue(!extractInside)
    {
    }

    template <typename ImplicitFunction>
    VISKORES_EXEC bool operator()(const viskores::Vec3f_64& coordinate,
                                  const ImplicitFunction& function) const
    {
      bool pass = passValue;
      viskores::Float64 value = function.Value(coordinate);
      if (value > 0)
      {
        pass = failValue;
      }
      return pass;
    }

  private:
    bool passValue;
    bool failValue;
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by id creates new cellset of vertex cells
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                          const viskores::cont::ArrayHandle<viskores::Id>& pointIds)
  {
    viskores::cont::ArrayCopy(pointIds, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    viskores::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), viskores::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by implicit function
  template <typename CellSetType, typename CoordinateType, typename ImplicitFunction>
  viskores::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                          const CoordinateType& coordinates,
                                          const ImplicitFunction& implicitFunction,
                                          bool extractInside)
  {
    // Worklet output will be a boolean passFlag array
    viskores::cont::ArrayHandle<bool> passFlags;

    ExtractPointsByVOI worklet(extractInside);
    viskores::cont::Invoker invoke;
    invoke(worklet, cellSet, coordinates, implicitFunction, passFlags);

    viskores::cont::ArrayHandleCounting<viskores::Id> indices =
      viskores::cont::make_ArrayHandleCounting(
        viskores::Id(0), viskores::Id(1), passFlags.GetNumberOfValues());
    viskores::cont::Algorithm::CopyIf(indices, passFlags, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    viskores::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), viskores::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

private:
  viskores::cont::ArrayHandle<viskores::Id> ValidPointIds;
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_ExtractPoints_h
