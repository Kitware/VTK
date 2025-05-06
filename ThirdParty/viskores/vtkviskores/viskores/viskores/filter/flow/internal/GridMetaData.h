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
#ifndef viskores_filter_flow_internal_GridMetaData_h
#define viskores_filter_flow_internal_GridMetaData_h

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

class GridMetaData
{
public:
  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;

  VISKORES_CONT
  GridMetaData(const viskores::cont::UnknownCellSet cellSet)
  {
    if (cellSet.CanConvert<Structured2DType>())
    {
      this->cellSet2D = true;
      viskores::Id2 dims = cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(
        viskores::TopologyElementTagPoint());
      this->Dims = viskores::Id3(dims[0], dims[1], 1);
    }
    else
    {
      this->cellSet2D = false;
      this->Dims = cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(
        viskores::TopologyElementTagPoint());
    }
    this->PlaneSize = Dims[0] * Dims[1];
    this->RowSize = Dims[0];
  }

  VISKORES_EXEC
  bool IsCellSet2D() const { return this->cellSet2D; }

  VISKORES_EXEC
  void GetLogicalIndex(const viskores::Id index, viskores::Id3& logicalIndex) const
  {
    logicalIndex[0] = index % Dims[0];
    logicalIndex[1] = (index / Dims[0]) % Dims[1];
    if (this->cellSet2D)
      logicalIndex[2] = 0;
    else
      logicalIndex[2] = index / (Dims[0] * Dims[1]);
  }

  VISKORES_EXEC
  const viskores::Vec<viskores::Id, 6> GetNeighborIndices(const viskores::Id index) const
  {
    viskores::Vec<viskores::Id, 6> indices;
    viskores::Id3 logicalIndex;
    GetLogicalIndex(index, logicalIndex);

    // For differentials w.r.t delta in x
    indices[0] = (logicalIndex[0] == 0) ? index : index - 1;
    indices[1] = (logicalIndex[0] == Dims[0] - 1) ? index : index + 1;
    // For differentials w.r.t delta in y
    indices[2] = (logicalIndex[1] == 0) ? index : index - RowSize;
    indices[3] = (logicalIndex[1] == Dims[1] - 1) ? index : index + RowSize;
    if (!this->cellSet2D)
    {
      // For differentials w.r.t delta in z
      indices[4] = (logicalIndex[2] == 0) ? index : index - PlaneSize;
      indices[5] = (logicalIndex[2] == Dims[2] - 1) ? index : index + PlaneSize;
    }
    return indices;
  }

private:
  bool cellSet2D = false;
  viskores::Id3 Dims;
  viskores::Id PlaneSize;
  viskores::Id RowSize;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_GridMetaData_h
