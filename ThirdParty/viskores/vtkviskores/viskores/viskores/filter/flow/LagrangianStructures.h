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

#ifndef viskores_filter_flow_LagrangianStructures_h
#define viskores_filter_flow_LagrangianStructures_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

/// @brief Compute the finite time Lyapunov exponent (FTLE) of a vector field.
///
/// The FTLE is computed by advecting particles throughout the vector field and analyizing
/// where they diverge or converge. By default, the points of the input `viskores::cont::DataSet`
/// are all advected for this computation unless an auxiliary grid is established.
///
class VISKORES_FILTER_FLOW_EXPORT LagrangianStructures : public viskores::filter::Filter
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
  void SetStepSize(viskores::FloatDefault s) { this->StepSize = s; }
  /// @copydoc SetStepSize
  viskores::FloatDefault GetStepSize() { return this->StepSize; }

  /// @brief Specify the maximum number of steps each particle is allowed to traverse.
  ///
  /// This can limit the total length of displacements used when computing the FTLE.
  void SetNumberOfSteps(viskores::Id n) { this->NumberOfSteps = n; }
  /// @copydoc SetNumberOfSteps
  viskores::Id GetNumberOfSteps() { return this->NumberOfSteps; }

  /// @brief Specify the time interval for the advection.
  ///
  /// The FTLE works by advecting all points a finite distance, and this parameter
  /// specifies how far to advect.
  void SetAdvectionTime(viskores::FloatDefault advectionTime)
  {
    this->AdvectionTime = advectionTime;
  }
  /// @copydoc SetAdvectionTime
  viskores::FloatDefault GetAdvectionTime() { return this->AdvectionTime; }

  /// @brief Specify whether to use an auxiliary grid.
  ///
  /// When this flag is off (the default), then the points of the mesh representing the vector
  /// field are advected and used for computing the FTLE. However, if the mesh is too coarse,
  /// the FTLE will likely be inaccurate. Or if the mesh is unstructured the FTLE may be less
  /// efficient to compute. When this flag is on, an auxiliary grid of uniformly spaced points
  /// is used for the FTLE computation.
  void SetUseAuxiliaryGrid(bool useAuxiliaryGrid) { this->UseAuxiliaryGrid = useAuxiliaryGrid; }
  /// @copydoc SetUseAuxiliaryGrid
  bool GetUseAuxiliaryGrid() { return this->UseAuxiliaryGrid; }

  /// @brief Specify the dimensions of the auxiliary grid for FTLE calculation.
  ///
  /// Seeds for advection will be placed along the points of this auxiliary grid.
  /// This option has no effect unless the UseAuxiliaryGrid option is on.
  void SetAuxiliaryGridDimensions(viskores::Id3 auxiliaryDims)
  {
    this->AuxiliaryDims = auxiliaryDims;
  }
  /// @copydoc SetAuxiliaryGridDimensions
  viskores::Id3 GetAuxiliaryGridDimensions() { return this->AuxiliaryDims; }

  /// @brief Specify whether to use flow maps instead of advection.
  ///
  /// If the start and end points for FTLE calculation are known already, advection is
  /// an unnecessary step. This flag allows users to bypass advection, and instead use
  /// a precalculated flow map. By default this option is off.
  void SetUseFlowMapOutput(bool useFlowMapOutput) { this->UseFlowMapOutput = useFlowMapOutput; }
  /// @copydoc SetUseFlowMapOutput
  bool GetUseFlowMapOutput() { return this->UseFlowMapOutput; }

  /// @brief Specify the name of the output field in the data set returned.
  ///
  /// By default, the field will be named `FTLE`.
  void SetOutputFieldName(std::string outputFieldName) { this->OutputFieldName = outputFieldName; }
  /// @copydoc SetOutputFieldName
  std::string GetOutputFieldName() { return this->OutputFieldName; }

  /// @brief Specify the array representing the flow map output to be used for FTLE calculation.
  inline void SetFlowMapOutput(viskores::cont::ArrayHandle<viskores::Vec3f>& flowMap)
  {
    this->FlowMapOutput = flowMap;
  }
  /// @copydoc SetFlowMapOutput
  inline viskores::cont::ArrayHandle<viskores::Vec3f> GetFlowMapOutput()
  {
    return this->FlowMapOutput;
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;

  viskores::FloatDefault AdvectionTime;
  viskores::Id3 AuxiliaryDims;
  viskores::cont::ArrayHandle<viskores::Vec3f> FlowMapOutput;
  std::string OutputFieldName = "FTLE";
  viskores::FloatDefault StepSize = 1.0f;
  viskores::Id NumberOfSteps = 0;
  bool UseAuxiliaryGrid = false;
  bool UseFlowMapOutput = false;
};

}
}
} // namespace viskores

#endif // viskores_filter_flow_LagrangianStructures_h
