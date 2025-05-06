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

#include <viskores/Particle.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/flow/LagrangianStructures.h>

#include <viskores/filter/flow/worklet/Analysis.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/LagrangianStructures.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>
#include <viskores/filter/flow/worklet/Termination.h>

namespace
{

VISKORES_CONT void MapField(viskores::cont::DataSet& dataset, const viskores::cont::Field& field)
{
  if (field.IsWholeDataSetField())
  {
    dataset.AddField(field);
  }
  else
  {
    // Do not currently support other types of fields.
  }
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace flow
{

namespace detail
{
class ExtractParticlePosition : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, FieldOut position);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::Particle& particle, viskores::Vec3f& pt) const
  {
    pt = particle.GetPosition();
  }
};

class MakeParticles : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn seed, FieldOut particle);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::Id index,
                                const viskores::Vec3f& seed,
                                viskores::Particle& particle) const
  {
    particle.SetID(index);
    particle.SetPosition(seed);
  }
};

} //detail


VISKORES_CONT viskores::cont::DataSet LagrangianStructures::DoExecute(
  const viskores::cont::DataSet& input)
{
  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;

  using FieldHandle = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::VelocityField<FieldHandle>;
  using GridEvaluator = viskores::worklet::flow::GridEvaluator<FieldType>;
  using IntegratorType = viskores::worklet::flow::RK4Integrator<GridEvaluator>;
  using Stepper = viskores::worklet::flow::Stepper<IntegratorType, GridEvaluator>;

  viskores::FloatDefault stepSize = this->GetStepSize();
  viskores::Id numberOfSteps = this->GetNumberOfSteps();

  viskores::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  viskores::cont::UnknownCellSet cellset = input.GetCellSet();

  viskores::cont::DataSet lcsInput;
  if (this->GetUseAuxiliaryGrid())
  {
    viskores::Id3 lcsGridDims = this->GetAuxiliaryGridDimensions();
    viskores::Bounds inputBounds = coordinates.GetBounds();
    viskores::Vec3f origin(static_cast<viskores::FloatDefault>(inputBounds.X.Min),
                           static_cast<viskores::FloatDefault>(inputBounds.Y.Min),
                           static_cast<viskores::FloatDefault>(inputBounds.Z.Min));
    viskores::Vec3f spacing;
    spacing[0] = static_cast<viskores::FloatDefault>(inputBounds.X.Length()) /
      static_cast<viskores::FloatDefault>(lcsGridDims[0] - 1);
    spacing[1] = static_cast<viskores::FloatDefault>(inputBounds.Y.Length()) /
      static_cast<viskores::FloatDefault>(lcsGridDims[1] - 1);
    spacing[2] = static_cast<viskores::FloatDefault>(inputBounds.Z.Length()) /
      static_cast<viskores::FloatDefault>(lcsGridDims[2] - 1);
    viskores::cont::DataSetBuilderUniform uniformDatasetBuilder;
    lcsInput = uniformDatasetBuilder.Create(lcsGridDims, origin, spacing);
  }
  else
  {
    // Check if input dataset is structured.
    // If not, we cannot proceed.
    if (!(cellset.IsType<Structured2DType>() || cellset.IsType<Structured3DType>()))
      throw viskores::cont::ErrorFilterExecution(
        "Provided data is not structured, provide parameters for an auxiliary grid.");
    lcsInput = input;
  }
  viskores::cont::ArrayHandle<viskores::Vec3f> lcsInputPoints, lcsOutputPoints;
  viskores::cont::ArrayCopy(lcsInput.GetCoordinateSystem().GetData(), lcsInputPoints);
  if (this->GetUseFlowMapOutput())
  {
    // Check if there is a 1:1 correspondense between the flow map
    // and the input points
    lcsOutputPoints = this->GetFlowMapOutput();
    if (lcsInputPoints.GetNumberOfValues() != lcsOutputPoints.GetNumberOfValues())
      throw viskores::cont::ErrorFilterExecution(
        "Provided flow map does not correspond to the input points for LCS filter.");
  }
  else
  {
    const auto field = input.GetField(this->GetActiveFieldName());

    FieldType velocities(
      field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>(),
      field.GetAssociation());
    GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), velocities);
    Stepper integrator(evaluator, stepSize);
    viskores::worklet::flow::ParticleAdvection particles;
    viskores::worklet::flow::NormalTermination termination(numberOfSteps);
    viskores::worklet::flow::NoAnalysis<viskores::Particle> analysis;
    viskores::cont::ArrayHandle<viskores::Particle> advectionPoints;

    viskores::cont::Invoker invoke;
    invoke(detail::MakeParticles{}, lcsInputPoints, advectionPoints);
    particles.Run(integrator, advectionPoints, termination, analysis);
    invoke(detail::ExtractParticlePosition{}, analysis.Particles, lcsOutputPoints);
  }
  // FTLE output field
  viskores::cont::ArrayHandle<viskores::FloatDefault> outputField;
  viskores::FloatDefault advectionTime = this->GetAdvectionTime();

  viskores::cont::UnknownCellSet lcsCellSet = lcsInput.GetCellSet();
  if (lcsCellSet.IsType<Structured2DType>())
  {
    using AnalysisType = viskores::worklet::flow::LagrangianStructures<2>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    viskores::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }
  else if (lcsCellSet.IsType<Structured3DType>())
  {
    using AnalysisType = viskores::worklet::flow::LagrangianStructures<3>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    viskores::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }


  auto fieldmapper = [&](viskores::cont::DataSet& dataset, const viskores::cont::Field& field)
  { MapField(dataset, field); };
  viskores::cont::DataSet output = this->CreateResultCoordinateSystem(
    input, lcsInput.GetCellSet(), lcsInput.GetCoordinateSystem(), fieldmapper);
  output.AddPointField(this->GetOutputFieldName(), outputField);
  return output;
}

}
}
} // namespace viskores::filter::flow
