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

#ifndef viskores_filter_flow_StreamSurface_h
#define viskores_filter_flow_StreamSurface_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

/// \brief Generate stream surfaces from a vector field.
///
/// This filter takes as input a velocity vector field and seed locations. The seed locations
/// should be arranged in a line or curve. The filter then traces the path each seed point
/// would take if moving at the velocity specified by the field and connects all the lines
/// together into a surface. Mathematically, this is the surface that is tangent to the
/// velocity field everywhere.
///
/// The output of this filter is a `viskores::cont::DataSet` containing a mesh for the created
/// surface.
class VISKORES_FILTER_FLOW_EXPORT StreamSurface : public viskores::filter::Filter
{
public:
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
  /// The particles are represented by a `viskores::Particle` object.
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

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;

  viskores::Id NumberOfSteps = 0;
  viskores::cont::UnknownArrayHandle Seeds;
  viskores::FloatDefault StepSize = 0;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_StreamSurface_h
