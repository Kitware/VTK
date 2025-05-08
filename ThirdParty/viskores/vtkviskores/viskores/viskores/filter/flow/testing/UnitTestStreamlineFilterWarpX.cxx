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

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/WarpXStreamline.h>
#include <viskores/io/VTKDataSetReader.h>

namespace
{

void GenerateChargedParticles(const viskores::cont::ArrayHandle<viskores::Vec3f>& pos,
                              const viskores::cont::ArrayHandle<viskores::Vec3f>& mom,
                              const viskores::cont::ArrayHandle<viskores::Float64>& mass,
                              const viskores::cont::ArrayHandle<viskores::Float64>& charge,
                              const viskores::cont::ArrayHandle<viskores::Float64>& weight,
                              viskores::cont::ArrayHandle<viskores::ChargedParticle>& seeds)
{
  auto pPortal = pos.ReadPortal();
  auto uPortal = mom.ReadPortal();
  auto mPortal = mass.ReadPortal();
  auto qPortal = charge.ReadPortal();
  auto wPortal = weight.ReadPortal();

  auto numValues = pos.GetNumberOfValues();

  seeds.Allocate(numValues);
  auto sPortal = seeds.WritePortal();

  for (viskores::Id i = 0; i < numValues; i++)
  {
    viskores::ChargedParticle electron(
      pPortal.Get(i), i, mPortal.Get(i), qPortal.Get(i), wPortal.Get(i), uPortal.Get(i));
    sPortal.Set(i, electron);
  }
}

void TestFilters()
{
  std::string particleFile = viskores::cont::testing::Testing::DataPath("misc/warpXparticles.vtk");
  std::string fieldFile = viskores::cont::testing::Testing::DataPath("misc/warpXfields.vtk");

  using SeedsType = viskores::cont::ArrayHandle<viskores::ChargedParticle>;

  SeedsType seeds;
  viskores::io::VTKDataSetReader seedsReader(particleFile);
  viskores::cont::DataSet seedsData = seedsReader.ReadDataSet();
  viskores::cont::ArrayHandle<viskores::Vec3f> pos, mom;
  viskores::cont::ArrayHandle<viskores::Float64> mass, charge, w;

  seedsData.GetCoordinateSystem().GetDataAsDefaultFloat().AsArrayHandle(pos);
  seedsData.GetField("Momentum").GetDataAsDefaultFloat().AsArrayHandle(mom);
  seedsData.GetField("Mass").GetData().AsArrayHandle(mass);
  seedsData.GetField("Charge").GetData().AsArrayHandle(charge);
  seedsData.GetField("Weighting").GetData().AsArrayHandle(w);

  GenerateChargedParticles(pos, mom, mass, charge, w, seeds);

  viskores::io::VTKDataSetReader dataReader(fieldFile);
  viskores::cont::DataSet dataset = dataReader.ReadDataSet();
  viskores::cont::UnknownCellSet cells = dataset.GetCellSet();
  viskores::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();

  auto bounds = coords.GetBounds();
  std::cout << "Bounds : " << bounds << std::endl;
  using Structured3DType = viskores::cont::CellSetStructured<3>;
  Structured3DType castedCells;
  cells.AsCellSet(castedCells);
  auto dims = castedCells.GetSchedulingRange(viskores::TopologyElementTagPoint());
  viskores::Vec3f spacing = {
    static_cast<viskores::FloatDefault>(bounds.X.Length()) / (dims[0] - 1),
    static_cast<viskores::FloatDefault>(bounds.Y.Length()) / (dims[1] - 1),
    static_cast<viskores::FloatDefault>(bounds.Z.Length()) / (dims[2] - 1)
  };
  std::cout << spacing << std::endl;
  constexpr static viskores::FloatDefault SPEED_OF_LIGHT =
    static_cast<viskores::FloatDefault>(2.99792458e8);
  spacing = spacing * spacing;

  viskores::Id steps = 50;
  viskores::FloatDefault length = static_cast<viskores::FloatDefault>(
    1.0 / (SPEED_OF_LIGHT * viskores::Sqrt(1. / spacing[0] + 1. / spacing[1] + 1. / spacing[2])));
  std::cout << "CFL length : " << length << std::endl;

  viskores::filter::flow::WarpXStreamline streamline;
  streamline.SetStepSize(length);
  streamline.SetNumberOfSteps(steps);
  streamline.SetSeeds(seeds);
  streamline.SetEField("E");
  streamline.SetBField("B");

  auto output = streamline.Execute(dataset);

  VISKORES_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
  VISKORES_TEST_ASSERT(output.GetCoordinateSystem().GetNumberOfPoints() == 2550,
                       "Wrong number of coordinates");
  VISKORES_TEST_ASSERT(output.GetCellSet().GetNumberOfCells() == 50, "Wrong number of cells");
}
}

int UnitTestStreamlineFilterWarpX(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFilters, argc, argv);
}
