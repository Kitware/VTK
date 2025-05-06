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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/flow/StreamSurface.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>
#include <viskores/filter/flow/worklet/StreamSurface.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT viskores::cont::DataSet StreamSurface::DoExecute(const viskores::cont::DataSet& input)
{
  //Validate inputs.
  if (this->GetUseCoordinateSystemAsField())
    throw viskores::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw viskores::cont::ErrorFilterExecution("No seeds provided.");
  if (!this->Seeds.IsBaseComponentType<viskores::Particle>() &&
      this->Seeds.IsBaseComponentType<viskores::ChargedParticle>())
    throw viskores::cont::ErrorFilterExecution("Unsupported particle type in seed array.");
  if (this->NumberOfSteps == 0)
    throw viskores::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw viskores::cont::ErrorFilterExecution("Step size not specified.");
  if (this->NumberOfSteps < 0)
    throw viskores::cont::ErrorFilterExecution("NumberOfSteps cannot be negative");
  if (this->StepSize < 0)
    throw viskores::cont::ErrorFilterExecution("StepSize cannot be negative");

  if (!this->Seeds.IsBaseComponentType<viskores::Particle>())
    throw viskores::cont::ErrorFilterExecution("Unsupported seed type in StreamSurface filter.");

  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const viskores::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using FieldHandle = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::VelocityField<FieldHandle>;
  using GridEvalType = viskores::worklet::flow::GridEvaluator<FieldType>;
  using RK4Type = viskores::worklet::flow::RK4Integrator<GridEvalType>;
  using Stepper = viskores::worklet::flow::Stepper<RK4Type, GridEvalType>;

  //compute streamlines
  const auto& field = input.GetField(this->GetActiveFieldName());
  FieldHandle arr;
  viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), arr);
  FieldType velocities(arr, field.GetAssociation());
  GridEvalType eval(coords, cells, velocities);
  Stepper rk4(eval, this->StepSize);

  using ParticleArray = viskores::cont::ArrayHandle<viskores::Particle>;
  viskores::cont::ArrayHandle<viskores::Particle> seedArray;
  viskores::cont::ArrayCopy(this->Seeds.AsArrayHandle<ParticleArray>(), seedArray);

  viskores::worklet::flow::ParticleAdvection worklet;
  viskores::worklet::flow::NormalTermination termination(this->NumberOfSteps);
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle> analysis(this->NumberOfSteps);

  worklet.Run(rk4, seedArray, termination, analysis);

  //compute surface from streamlines
  viskores::worklet::flow::StreamSurface streamSurface;
  viskores::cont::ArrayHandle<viskores::Vec3f> srfPoints;
  viskores::cont::CellSetSingleType<> srfCells;
  viskores::cont::CoordinateSystem slCoords("coordinates", analysis.Streams);
  streamSurface.Run(slCoords, analysis.PolyLines, srfPoints, srfCells);

  viskores::cont::DataSet outData;
  viskores::cont::CoordinateSystem outputCoords("coordinates", srfPoints);
  outData.AddCoordinateSystem(outputCoords);
  outData.SetCellSet(srfCells);

  return outData;
}

}
}
} // namespace viskores::filter::flow
