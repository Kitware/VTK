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

#ifndef viskores_filter_flow_worklet_GridEvaluatorStatus_h
#define viskores_filter_flow_worklet_GridEvaluatorStatus_h

#include <viskores/Bitset.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

class GridEvaluatorStatus : public viskores::Bitset<viskores::UInt8>
{
public:
  VISKORES_EXEC_CONT GridEvaluatorStatus(){};
  VISKORES_EXEC_CONT GridEvaluatorStatus(bool ok, bool spatial, bool temporal, bool inGhost)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
    this->set(this->IN_GHOST_CELL_BIT, inGhost);
  };

  VISKORES_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VISKORES_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VISKORES_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VISKORES_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VISKORES_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT bool CheckSpatialBounds() const
  {
    return this->test(this->SPATIAL_BOUNDS_BIT);
  }

  VISKORES_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT bool CheckTemporalBounds() const
  {
    return this->test(this->TEMPORAL_BOUNDS_BIT);
  }

  VISKORES_EXEC_CONT void SetInGhostCell() { this->set(this->IN_GHOST_CELL_BIT); }
  VISKORES_EXEC_CONT bool CheckInGhostCell() const { return this->test(this->IN_GHOST_CELL_BIT); }

private:
  static constexpr viskores::Id SUCCESS_BIT = 0;
  static constexpr viskores::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr viskores::Id TEMPORAL_BOUNDS_BIT = 2;
  static constexpr viskores::Id IN_GHOST_CELL_BIT = 3;
};

}
}
}

#endif // viskores_filter_flow_worklet_GridEvaluatorStatus_h
