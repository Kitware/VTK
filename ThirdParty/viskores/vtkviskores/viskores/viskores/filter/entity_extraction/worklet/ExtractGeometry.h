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
#ifndef viskores_m_worklet_ExtractGeometry_h
#define viskores_m_worklet_ExtractGeometry_h

#include <viskores/worklet/CellDeepCopy.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/ImplicitFunction.h>

namespace viskores
{
namespace worklet
{

class ExtractGeometry
{
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify cells within volume of interest
  class ExtractCellsByVOI : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn coordinates,
                                  ExecObject implicitFunction,
                                  FieldOutCell passFlags);
    using ExecutionSignature = _4(PointCount, PointIndices, _2, _3);

    VISKORES_CONT
    ExtractCellsByVOI(bool extractInside, bool extractBoundaryCells, bool extractOnlyBoundaryCells)
      : ExtractInside(extractInside)
      , ExtractBoundaryCells(extractBoundaryCells)
      , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
    {
    }

    template <typename ConnectivityInVec, typename InVecFieldPortalType, typename ImplicitFunction>
    VISKORES_EXEC bool operator()(viskores::Id numIndices,
                                  const ConnectivityInVec& connectivityIn,
                                  const InVecFieldPortalType& coordinates,
                                  const ImplicitFunction& function) const
    {
      // Count points inside/outside volume of interest
      viskores::IdComponent inCnt = 0;
      viskores::IdComponent outCnt = 0;
      viskores::Id indx;
      for (indx = 0; indx < numIndices; indx++)
      {
        viskores::Id ptId = connectivityIn[static_cast<viskores::IdComponent>(indx)];
        viskores::Vec<FloatDefault, 3> coordinate = coordinates.Get(ptId);
        viskores::FloatDefault value = function.Value(coordinate);
        if (value <= 0)
          inCnt++;
        if (value >= 0)
          outCnt++;
      }

      // Decide if cell is extracted
      bool passFlag = false;
      if (inCnt == numIndices && ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (outCnt == numIndices && !ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (inCnt > 0 && outCnt > 0 && (ExtractBoundaryCells || ExtractOnlyBoundaryCells))
      {
        passFlag = true;
      }
      return passFlag;
    }

  private:
    bool ExtractInside;
    bool ExtractBoundaryCells;
    bool ExtractOnlyBoundaryCells;
  };

  class AddPermutationCellSet
  {
    viskores::cont::UnknownCellSet* Output;
    viskores::cont::ArrayHandle<viskores::Id>* ValidIds;

  public:
    AddPermutationCellSet(viskores::cont::UnknownCellSet& cellOut,
                          viskores::cont::ArrayHandle<viskores::Id>& validIds)
      : Output(&cellOut)
      , ValidIds(&validIds)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellset) const
    {
      viskores::cont::CellSetPermutation<CellSetType> permCellSet(*this->ValidIds, cellset);
      *this->Output = permCellSet;
    }
  };

  template <typename CellSetType, typename ImplicitFunction>
  viskores::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                        const viskores::cont::CoordinateSystem& coordinates,
                                        const ImplicitFunction& implicitFunction,
                                        bool extractInside,
                                        bool extractBoundaryCells,
                                        bool extractOnlyBoundaryCells)
  {
    // Worklet output will be a boolean passFlag array
    viskores::cont::ArrayHandle<bool> passFlags;

    ExtractCellsByVOI worklet(extractInside, extractBoundaryCells, extractOnlyBoundaryCells);
    viskores::cont::Invoker invoke;
    invoke(worklet, cellSet, coordinates, implicitFunction, passFlags);

    viskores::cont::ArrayHandleCounting<viskores::Id> indices =
      viskores::cont::make_ArrayHandleCounting(
        viskores::Id(0), viskores::Id(1), passFlags.GetNumberOfValues());
    viskores::cont::Algorithm::CopyIf(indices, passFlags, this->ValidCellIds);

    // generate the cellset
    viskores::cont::CellSetPermutation<CellSetType> permutedCellSet(this->ValidCellIds, cellSet);

    viskores::cont::CellSetExplicit<> outputCells;
    return viskores::worklet::CellDeepCopy::Run(permutedCellSet);
  }

  viskores::cont::ArrayHandle<viskores::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  viskores::cont::ArrayHandle<viskores::Id> ValidCellIds;
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_ExtractGeometry_h
