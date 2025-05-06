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
#ifndef viskores_m_worklet_Tetrahedralize_h
#define viskores_m_worklet_Tetrahedralize_h

#include <viskores/filter/geometry_refinement/worklet/tetrahedralize/TetrahedralizeExplicit.h>
#include <viskores/filter/geometry_refinement/worklet/tetrahedralize/TetrahedralizeStructured.h>

namespace viskores
{
namespace worklet
{

class Tetrahedralize
{
public:
  //
  // Distribute multiple copies of cell data depending on cells create from original
  //
  struct DistributeCellData : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
    using ExecutionSignature = void(_1, _2);

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename CountArrayType>
    VISKORES_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    template <typename T>
    VISKORES_EXEC void operator()(T inputIndex, T& outputIndex) const
    {
      outputIndex = inputIndex;
    }
  };

  Tetrahedralize()
    : OutCellScatter(viskores::cont::ArrayHandle<viskores::IdComponent>{})
  {
  }

  // Tetrahedralize explicit data set, save number of tetra cells per input
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TetrahedralizeExplicit worklet;
    viskores::cont::ArrayHandle<viskores::IdComponent> outCellsPerCell;
    viskores::cont::CellSetSingleType<> result = worklet.Run(cellSet, outCellsPerCell);
    this->OutCellScatter = DistributeCellData::MakeScatter(outCellsPerCell);
    return result;
  }

  // Tetrahedralize structured data set, save number of tetra cells per input
  viskores::cont::CellSetSingleType<> Run(const viskores::cont::CellSetStructured<3>& cellSet)
  {
    TetrahedralizeStructured worklet;
    viskores::cont::ArrayHandle<viskores::IdComponent> outCellsPerCell;
    viskores::cont::CellSetSingleType<> result = worklet.Run(cellSet, outCellsPerCell);
    this->OutCellScatter = DistributeCellData::MakeScatter(outCellsPerCell);
    return result;
  }

  viskores::cont::CellSetSingleType<> Run(const viskores::cont::CellSetStructured<2>&)
  {
    throw viskores::cont::ErrorBadType("CellSetStructured<2> can't be tetrahedralized");
  }

  viskores::cont::CellSetSingleType<> Run(const viskores::cont::CellSetStructured<1>&)
  {
    throw viskores::cont::ErrorBadType("CellSetStructured<1> can't be tetrahedralized");
  }

  DistributeCellData::ScatterType GetOutCellScatter() const { return this->OutCellScatter; }

private:
  DistributeCellData::ScatterType OutCellScatter;
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_Tetrahedralize_h
