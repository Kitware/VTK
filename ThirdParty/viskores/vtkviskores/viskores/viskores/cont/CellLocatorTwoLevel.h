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
#ifndef viskores_cont_CellLocatorTwoLevel_h
#define viskores_cont_CellLocatorTwoLevel_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetList.h>

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorMultiplexer.h>
#include <viskores/exec/CellLocatorTwoLevel.h>


namespace viskores
{
namespace cont
{

/// \brief A locator that uses 2 nested levels of grids.
///
/// `CellLocatorTwoLevel` creates a cell search structure using two levels of structured
///  grids. The first level is a coarse grid that covers the entire region of the data.
/// It is expected that the distributions of dataset cells in this coarse grid will be
/// very uneven. Within each bin of the coarse grid, a second level grid is defined within
/// the spatial bounds of the coarse bin. The size of this second level grid is proportional
/// to the number of cells in the first level. In this way, the second level grids adapt
/// to the distribution of cells being located. The adaption is not perfect, but it is
/// has very good space efficiency and is fast to generate and use.
///
/// The algorithm used in `CellLocatorTwoLevel` is described in the following publication:
///
/// Javor Kalojanov, Markus Billeter, and Philipp Slusallek.
/// "Two-Level Grids for Ray Tracing on GPUs."
/// _Computer Graphics Forum_, 2011, pages 307-314. DOI 10.1111/j.1467-8659.2011.01862.x
///
class VISKORES_CONT_EXPORT CellLocatorTwoLevel : public viskores::cont::CellLocatorBase
{
  template <typename CellSetCont>
  using CellSetContToExec =
    typename CellSetCont::template ExecConnectivityType<viskores::TopologyElementTagCell,
                                                        viskores::TopologyElementTagPoint>;

public:
  using SupportedCellSets = VISKORES_DEFAULT_CELL_SET_LIST;

  using CellExecObjectList = viskores::ListTransform<SupportedCellSets, CellSetContToExec>;
  using CellLocatorExecList =
    viskores::ListTransform<CellExecObjectList, viskores::exec::CellLocatorTwoLevel>;

  using ExecObjType =
    viskores::ListApply<CellLocatorExecList, viskores::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  CellLocatorTwoLevel()
    : DensityL1(32.0f)
    , DensityL2(2.0f)
  {
  }

  /// @brief Specify the desired approximate number of cells per level 1 bin.
  ///
  /// The default value is 32.
  void SetDensityL1(viskores::FloatDefault val)
  {
    this->DensityL1 = val;
    this->SetModified();
  }
  /// @copydoc SetDensityL1
  viskores::FloatDefault GetDensityL1() const { return this->DensityL1; }

  /// @brief Specify the desired approximate number of cells per level 2 bin.
  ///
  /// This value should be relatively small as it is close to the average number
  /// of cells that must be checked for each find.
  /// The default value is 2.
  void SetDensityL2(viskores::FloatDefault val)
  {
    this->DensityL2 = val;
    this->SetModified();
  }
  /// @copydoc SetDensityL2
  viskores::FloatDefault GetDensityL2() const { return this->DensityL2; }

  /// Print a summary of the state of this locator.
  void PrintSummary(std::ostream& out) const;

  ExecObjType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                  viskores::cont::Token& token) const;

private:
  VISKORES_CONT void Build() override;

  viskores::FloatDefault DensityL1, DensityL2;

  viskores::internal::cl_uniform_bins::Grid TopLevel;
  viskores::cont::ArrayHandle<viskores::internal::cl_uniform_bins::DimVec3> LeafDimensions;
  viskores::cont::ArrayHandle<viskores::Id> LeafStartIndex;
  viskores::cont::ArrayHandle<viskores::Id> CellStartIndex;
  viskores::cont::ArrayHandle<viskores::Id> CellCount;
  viskores::cont::ArrayHandle<viskores::Id> CellIds;

  struct MakeExecObject;
};

}
} // viskores::cont

#endif // viskores_cont_CellLocatorTwoLevel_h
