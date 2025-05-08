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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ErrorFilterExecution.h>

#include <viskores/filter/flow/Lagrangian.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>

#include <viskores/filter/flow/worklet/Analysis.h>
#include <viskores/filter/flow/worklet/Termination.h>
//#include <viskores/worklet/WorkletMapField.h>

//#include <cstring>
//#include <sstream>
//#include <string.h>

namespace viskores
{
namespace filter
{
namespace flow
{

namespace
{
class ValidityCheck : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  ValidityCheck(viskores::Bounds b)
    : bounds(b)
  {
  }

  template <typename ValidityType>
  VISKORES_EXEC void operator()(const viskores::Particle& end_point, ValidityType& res) const
  {
    viskores::Id steps = end_point.GetNumberOfSteps();
    if (steps > 0 && res == 1)
    {
      if (bounds.Contains(end_point.GetPosition()))
      {
        res = 1;
      }
      else
      {
        res = 0;
      }
    }
    else
    {
      res = 0;
    }
  }

private:
  viskores::Bounds bounds;
};

class DisplacementCalculation : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldIn start_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename DisplacementType>
  VISKORES_EXEC void operator()(const viskores::Particle& end_point,
                                const viskores::Particle& start_point,
                                DisplacementType& res) const
  {
    res[0] = end_point.GetPosition()[0] - start_point.GetPosition()[0];
    res[1] = end_point.GetPosition()[1] - start_point.GetPosition()[1];
    res[2] = end_point.GetPosition()[2] - start_point.GetPosition()[2];
  }
};

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

//-----------------------------------------------------------------------------
void Lagrangian::UpdateSeedResolution(const viskores::cont::DataSet input)
{
  viskores::cont::UnknownCellSet cell_set = input.GetCellSet();

  if (cell_set.CanConvert<viskores::cont::CellSetStructured<1>>())
  {
    viskores::cont::CellSetStructured<1> cell_set1 =
      cell_set.AsCellSet<viskores::cont::CellSetStructured<1>>();
    viskores::Id dims1 = cell_set1.GetPointDimensions();
    this->SeedRes[0] = dims1;
    if (this->CustRes)
    {
      this->SeedRes[0] = dims1 / this->ResX;
    }
  }
  else if (cell_set.CanConvert<viskores::cont::CellSetStructured<2>>())
  {
    viskores::cont::CellSetStructured<2> cell_set2 =
      cell_set.AsCellSet<viskores::cont::CellSetStructured<2>>();
    viskores::Id2 dims2 = cell_set2.GetPointDimensions();
    this->SeedRes[0] = dims2[0];
    this->SeedRes[1] = dims2[1];
    if (this->CustRes)
    {
      this->SeedRes[0] = dims2[0] / this->ResX;
      this->SeedRes[1] = dims2[1] / this->ResY;
    }
  }
  else if (cell_set.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    viskores::cont::CellSetStructured<3> cell_set3 =
      cell_set.AsCellSet<viskores::cont::CellSetStructured<3>>();
    viskores::Id3 dims3 = cell_set3.GetPointDimensions();
    this->SeedRes[0] = dims3[0];
    this->SeedRes[1] = dims3[1];
    this->SeedRes[2] = dims3[2];
    if (this->CustRes)
    {
      this->SeedRes[0] = dims3[0] / this->ResX;
      this->SeedRes[1] = dims3[1] / this->ResY;
      this->SeedRes[2] = dims3[2] / this->ResZ;
    }
  }
}


//-----------------------------------------------------------------------------
void Lagrangian::InitializeSeedPositions(const viskores::cont::DataSet& input)
{
  viskores::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  Lagrangian::UpdateSeedResolution(input);

  viskores::Float64 x_spacing = 0.0, y_spacing = 0.0, z_spacing = 0.0;
  if (this->SeedRes[0] > 1)
    x_spacing = (double)(bounds.X.Max - bounds.X.Min) / (double)(this->SeedRes[0] - 1);
  if (this->SeedRes[1] > 1)
    y_spacing = (double)(bounds.Y.Max - bounds.Y.Min) / (double)(this->SeedRes[1] - 1);
  if (this->SeedRes[2] > 1)
    z_spacing = (double)(bounds.Z.Max - bounds.Z.Min) / (double)(this->SeedRes[2] - 1);
  // Divide by zero handling for 2D data set. How is this handled

  this->BasisParticles.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
  this->BasisParticlesValidity.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);

  auto portal1 = this->BasisParticles.WritePortal();
  auto portal2 = this->BasisParticlesValidity.WritePortal();

  viskores::Id id = 0;
  for (int z = 0; z < this->SeedRes[2]; z++)
  {
    viskores::FloatDefault zi = static_cast<viskores::FloatDefault>(z * z_spacing);
    for (int y = 0; y < this->SeedRes[1]; y++)
    {
      viskores::FloatDefault yi = static_cast<viskores::FloatDefault>(y * y_spacing);
      for (int x = 0; x < this->SeedRes[0]; x++)
      {
        viskores::FloatDefault xi = static_cast<viskores::FloatDefault>(x * x_spacing);
        portal1.Set(
          id,
          viskores::Particle(Vec3f(static_cast<viskores::FloatDefault>(bounds.X.Min) + xi,
                                   static_cast<viskores::FloatDefault>(bounds.Y.Min) + yi,
                                   static_cast<viskores::FloatDefault>(bounds.Z.Min) + zi),
                             id));
        portal2.Set(id, 1);
        id++;
      }
    }
  }
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet Lagrangian::DoExecute(const viskores::cont::DataSet& input)
{
  if (this->Cycle == 0)
  {
    this->InitializeSeedPositions(input);
    viskores::cont::ArrayCopy(this->BasisParticles, this->BasisParticlesOriginal);
  }

  if (this->WriteFrequency == 0)
  {
    throw viskores::cont::ErrorFilterExecution(
      "Write frequency can not be 0. Use SetWriteFrequency().");
  }
  viskores::cont::ArrayHandle<viskores::Particle> basisParticleArray;
  viskores::cont::ArrayCopy(this->BasisParticles, basisParticleArray);

  this->Cycle += 1;
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const viskores::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  viskores::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  using FieldHandle = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::VelocityField<FieldHandle>;
  using GridEvalType = viskores::worklet::flow::GridEvaluator<FieldType>;
  using RK4Type = viskores::worklet::flow::RK4Integrator<GridEvalType>;
  using Stepper = viskores::worklet::flow::Stepper<RK4Type, GridEvalType>;

  viskores::worklet::flow::ParticleAdvection particleadvection;

  const auto field = input.GetField(this->GetActiveFieldName());
  FieldType velocities(
    field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>(),
    field.GetAssociation());

  GridEvalType gridEval(coords, cells, velocities);
  Stepper rk4(gridEval, static_cast<viskores::Float32>(this->StepSize));
  viskores::worklet::flow::NormalTermination termination(1);
  viskores::worklet::flow::NoAnalysis<viskores::Particle> analysis;
  particleadvection.Run(rk4, basisParticleArray, termination, analysis); // Taking a single step
  auto particles = analysis.Particles;

  viskores::cont::DataSet outputData;

  if (this->Cycle % this->WriteFrequency == 0)
  {
    /* Steps to create a structured dataset */
    UpdateSeedResolution(input);
    viskores::cont::ArrayHandle<viskores::Vec3f> basisParticlesDisplacement;
    basisParticlesDisplacement.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
    DisplacementCalculation displacement;
    this->Invoke(displacement, particles, this->BasisParticlesOriginal, basisParticlesDisplacement);
    viskores::Vec3f origin(0);
    viskores::Vec3f spacing(0);
    if (this->SeedRes[0] > 1)
    {
      spacing[0] = static_cast<viskores::FloatDefault>(bounds.X.Length() / (this->SeedRes[0] - 1));
    }
    if (this->SeedRes[1] > 1)
    {
      spacing[1] = static_cast<viskores::FloatDefault>(bounds.Y.Length() / (this->SeedRes[1] - 1));
    }
    if (this->SeedRes[2] > 1)
    {
      spacing[2] = static_cast<viskores::FloatDefault>(bounds.Z.Length() / (this->SeedRes[2] - 1));
    }
    viskores::cont::CoordinateSystem outCoords("coords", this->SeedRes, origin, spacing);
    viskores::cont::CellSetStructured<3> outCellSet;
    outCellSet.SetPointDimensions(this->SeedRes);
    auto fieldmapper =
      [&](viskores::cont::DataSet& dataset, const viskores::cont::Field& fieldToPass)
    { MapField(dataset, fieldToPass); };
    outputData = this->CreateResultCoordinateSystem(input, outCellSet, outCoords, fieldmapper);
    outputData.AddPointField("valid", this->BasisParticlesValidity);
    outputData.AddPointField("displacement", basisParticlesDisplacement);

    if (this->ResetParticles)
    {
      this->InitializeSeedPositions(input);
      viskores::cont::ArrayCopy(this->BasisParticles, this->BasisParticlesOriginal);
    }
    else
    {
      viskores::cont::ArrayCopy(particles, this->BasisParticles);
    }
  }
  else
  {
    ValidityCheck check(bounds);
    this->Invoke(check, particles, this->BasisParticlesValidity);
    viskores::cont::ArrayCopy(particles, this->BasisParticles);
  }

  return outputData;
}

}
}
} //viskores::filter::flow
