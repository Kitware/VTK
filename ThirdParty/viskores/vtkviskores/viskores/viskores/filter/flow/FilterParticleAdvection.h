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

#ifndef viskores_filter_flow_FilterParticleAdvection_h
#define viskores_filter_flow_FilterParticleAdvection_h

#include <viskores/Deprecated.h>
#include <viskores/Particle.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.

class VISKORES_FILTER_FLOW_EXPORT FilterParticleAdvection : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  bool CanThread() const override { return false; }

  /// @brief Specifies the step size used for the numerical integrator.
  ///
  /// The numerical integrators operate by advancing each particle by a finite amount.
  /// This parameter defines the distance to advance each time. Smaller values are
  /// more accurate but take longer to integrate. An appropriate step size is usually
  /// around the size of each cell.
  VISKORES_CONT void SetStepSize(viskores::FloatDefault s) { this->StepSize = s; }

  /// @brief Specifies the maximum number of integration steps for each particle.
  ///
  /// Some particle paths may loop and continue indefinitely. This parameter sets an upper
  /// limit on the total length of advection.
  VISKORES_CONT void SetNumberOfSteps(viskores::Id n) { this->NumberOfSteps = n; }

  /// @brief Specify the seed locations for the particle advection.
  ///
  /// Each seed represents one particle that is advected by the vector field.
  /// The particles are represented by a `viskores::Particle` object or similar
  /// type of object (such as `viskores::ChargedParticle`).
  template <typename ParticleType>
  VISKORES_CONT void SetSeeds(viskores::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->Seeds = seeds;
  }

  /// @copydoc SetSeeds
  template <typename ParticleType>
  VISKORES_CONT void SetSeeds(const std::vector<ParticleType>& seeds,
                              viskores::CopyFlag copyFlag = viskores::CopyFlag::On)
  {
    this->Seeds = viskores::cont::make_ArrayHandle(seeds, copyFlag);
  }

  VISKORES_CONT void SetBlockIDs(const std::vector<viskores::Id>& blockIds)
  {
    this->BlockIds = blockIds;
    this->BlockIdsSet = true;
  }

  VISKORES_CONT
  void SetSolverRK4()
  {
    this->SolverType = viskores::filter::flow::IntegrationSolverType::RK4_TYPE;
  }

  VISKORES_CONT
  void SetSolverEuler()
  {
    this->SolverType = viskores::filter::flow::IntegrationSolverType::EULER_TYPE;
  }

  VISKORES_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VISKORES_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

  VISKORES_DEPRECATED(2.2, "All communication is asynchronous now.")
  VISKORES_CONT
  void SetUseAsynchronousCommunication() {}

  VISKORES_DEPRECATED(2.2, "All communication is asynchronous now.")
  VISKORES_CONT
  bool GetUseAsynchronousCommunication() { return true; }

  VISKORES_DEPRECATED(2.2, "All communication is asynchronous now.")
  VISKORES_CONT
  void SetUseSynchronousCommunication() {}

  VISKORES_DEPRECATED(2.2, "All communication is asynchronous now.")
  VISKORES_CONT
  bool GetUseSynchronousCommunication() { return false; }


protected:
  VISKORES_CONT virtual void ValidateOptions() const;

  bool BlockIdsSet = false;
  std::vector<viskores::Id> BlockIds;
  viskores::filter::flow::internal::BoundsMap BoundsMap;
  viskores::Id NumberOfSteps = 0;
  viskores::cont::UnknownArrayHandle Seeds;
  viskores::filter::flow::IntegrationSolverType SolverType =
    viskores::filter::flow::IntegrationSolverType::RK4_TYPE;
  viskores::FloatDefault StepSize = 0;
  bool UseThreadedAlgorithm = false;
  viskores::filter::flow::VectorFieldType VecFieldType =
    viskores::filter::flow::VectorFieldType::VELOCITY_FIELD_TYPE;

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_FilterParticleAdvection_h
