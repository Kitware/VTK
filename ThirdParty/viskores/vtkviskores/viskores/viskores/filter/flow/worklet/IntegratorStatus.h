//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef viskores_filter_flow_worklet_IntegratorStatus_h
#define viskores_filter_flow_worklet_IntegratorStatus_h

#include <iomanip>
#include <limits>

#include <viskores/Bitset.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/filter/flow/worklet/GridEvaluatorStatus.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

class IntegratorStatus : public viskores::Bitset<viskores::UInt8>
{
public:
  VISKORES_EXEC_CONT IntegratorStatus() {}

  VISKORES_EXEC_CONT IntegratorStatus(const bool& ok,
                                      const bool& spatial,
                                      const bool& temporal,
                                      const bool& inGhost,
                                      const bool& isZero)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
    this->set(this->IN_GHOST_CELL_BIT, inGhost);
    this->set(this->ZERO_VELOCITY_BIT, isZero);
  }

  VISKORES_EXEC_CONT IntegratorStatus(const GridEvaluatorStatus& es, bool isZero)
    : IntegratorStatus(es.CheckOk(),
                       es.CheckSpatialBounds(),
                       es.CheckTemporalBounds(),
                       es.CheckInGhostCell(),
                       isZero)
  {
  }

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
  VISKORES_EXEC_CONT void SetZeroVelocity() { this->set(this->ZERO_VELOCITY_BIT); }
  VISKORES_EXEC_CONT bool CheckZeroVelocity() const { return this->test(this->ZERO_VELOCITY_BIT); }

private:
  static constexpr viskores::Id SUCCESS_BIT = 0;
  static constexpr viskores::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr viskores::Id TEMPORAL_BOUNDS_BIT = 2;
  static constexpr viskores::Id IN_GHOST_CELL_BIT = 3;
  static constexpr viskores::Id ZERO_VELOCITY_BIT = 4;
};

inline VISKORES_CONT std::ostream& operator<<(std::ostream& s, const IntegratorStatus& status)
{
  s << "[ok= " << status.CheckOk() << " sp= " << status.CheckSpatialBounds()
    << " tm= " << status.CheckTemporalBounds() << " gc= " << status.CheckInGhostCell()
    << "zero= " << status.CheckZeroVelocity() << " ]";
  return s;
}

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_IntegratorStatus_h
