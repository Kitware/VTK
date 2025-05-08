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

#include <viskores/CellClassification.h>
#include <viskores/Particle.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/ParticleAdvection.h>
#include <viskores/filter/flow/Pathline.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/flow/testing/GenerateTestDataSets.h>
#include <viskores/thirdparty/diy/diy.h>

namespace
{

enum FilterType
{
  PARTICLE_ADVECTION,
  STREAMLINE,
  PATHLINE
};

viskores::cont::ArrayHandle<viskores::Vec3f> CreateConstantVectorField(viskores::Id num,
                                                                       const viskores::Vec3f& vec)
{
  viskores::cont::ArrayHandleConstant<viskores::Vec3f> vecConst;
  vecConst = viskores::cont::make_ArrayHandleConstant(vec, num);

  viskores::cont::ArrayHandle<viskores::Vec3f> vecField;
  viskores::cont::ArrayCopy(vecConst, vecField);
  return vecField;
}

void AddVectorFields(viskores::cont::PartitionedDataSet& pds,
                     const std::string& fieldName,
                     const viskores::Vec3f& vec)
{
  for (auto& ds : pds)
    ds.AddPointField(fieldName, CreateConstantVectorField(ds.GetNumberOfPoints(), vec));
}

template <typename FilterType>
void SetFilter(FilterType& filter,
               viskores::FloatDefault stepSize,
               viskores::Id numSteps,
               const std::string& fieldName,
               viskores::cont::ArrayHandle<viskores::Particle> seedArray,
               bool useThreaded,
               bool useBlockIds,
               const std::vector<viskores::Id>& blockIds)
{
  filter.SetStepSize(stepSize);
  filter.SetNumberOfSteps(numSteps);
  filter.SetSeeds(seedArray);
  filter.SetActiveField(fieldName);
  filter.SetUseThreadedAlgorithm(useThreaded);

  if (useBlockIds)
    filter.SetBlockIDs(blockIds);
}

void TestAMRStreamline(FilterType fType, bool useThreaded)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  if (comm.rank() == 0)
  {
    switch (fType)
    {
      case PARTICLE_ADVECTION:
        std::cout << "Particle advection";
        break;
      case STREAMLINE:
        std::cout << "Streamline";
        break;
      case PATHLINE:
        std::cout << "Pathline";
        break;
    }
    if (useThreaded)
      std::cout << " - using threaded";
    std::cout << " - on an AMR data set" << std::endl;
  }

  if (comm.size() < 2)
    return;

  viskores::Bounds outerBounds(0, 10, 0, 10, 0, 10);
  viskores::Id3 outerDims(11, 11, 11);
  auto outerDataSets = viskores::worklet::testing::CreateAllDataSets(outerBounds, outerDims, false);

  viskores::Bounds innerBounds(3.8, 5.2, 3.8, 5.2, 3.8, 5.2);
  viskores::Bounds innerBoundsNoGhost(4, 5, 4, 5, 4, 5);
  viskores::Id3 innerDims(12, 12, 12);
  auto innerDataSets = viskores::worklet::testing::CreateAllDataSets(innerBounds, innerDims, true);

  std::size_t numDS = outerDataSets.size();
  for (std::size_t d = 0; d < numDS; d++)
  {
    auto dsOuter = outerDataSets[d];
    auto dsInner = innerDataSets[d];

    //Add ghost cells for the outerDataSets.
    //One interior cell is a ghost.
    std::vector<viskores::UInt8> ghosts;
    ghosts.resize(dsOuter.GetCellSet().GetNumberOfCells());
    viskores::Id idx = 0;
    for (viskores::Id i = 0; i < outerDims[0] - 1; i++)
      for (viskores::Id j = 0; j < outerDims[1] - 1; j++)
        for (viskores::Id k = 0; k < outerDims[2] - 1; k++)
        {
          //Mark the inner cell as ghost.
          if (i == 4 && j == 4 && k == 4)
            ghosts[idx] = viskores::CellClassification::Ghost;
          else
            ghosts[idx] = viskores::CellClassification::Normal;
          idx++;
        }
    dsOuter.SetGhostCellField(viskores::cont::make_ArrayHandle(ghosts, viskores::CopyFlag::On));

    //Create a partitioned dataset with 1 inner and 1 outer.
    viskores::cont::PartitionedDataSet pds;
    if (comm.rank() == 0)
      pds.AppendPartition(dsOuter);
    else if (comm.rank() == 1)
      pds.AppendPartition(dsInner);

    std::string fieldName = "vec";
    viskores::Vec3f vecX(1, 0, 0);
    AddVectorFields(pds, fieldName, vecX);

    //seed 0 goes right through the center of the inner
    viskores::Particle p0(viskores::Vec3f(static_cast<viskores::FloatDefault>(1),
                                          static_cast<viskores::FloatDefault>(4.5),
                                          static_cast<viskores::FloatDefault>(4.5)),
                          0);

    //seed 1 goes remains entirely in the outer
    viskores::Particle p1(viskores::Vec3f(static_cast<viskores::FloatDefault>(1),
                                          static_cast<viskores::FloatDefault>(3),
                                          static_cast<viskores::FloatDefault>(3)),
                          1);

    viskores::cont::ArrayHandle<viskores::Particle> seedArray;
    seedArray = viskores::cont::make_ArrayHandle({ p0, p1 });
    viskores::Id numSeeds = seedArray.GetNumberOfValues();

    viskores::FloatDefault stepSize = 0.1f;
    viskores::Id numSteps = 100000;

    if (fType == STREAMLINE || fType == PATHLINE)
    {
      viskores::cont::PartitionedDataSet out;

      if (fType == STREAMLINE)
      {
        viskores::filter::flow::Streamline streamline;
        SetFilter(streamline, stepSize, numSteps, fieldName, seedArray, useThreaded, false, {});
        out = streamline.Execute(pds);
      }
      else if (fType == PATHLINE)
      {
        viskores::filter::flow::Pathline pathline;
        SetFilter(pathline, stepSize, numSteps, fieldName, seedArray, useThreaded, false, {});
        //Create timestep 2
        auto pds2 = viskores::cont::PartitionedDataSet(pds);
        pathline.SetPreviousTime(0);
        pathline.SetNextTime(10);
        pathline.SetNextDataSet(pds2);
        out = pathline.Execute(pds);
      }

      if (comm.rank() <= 1)
        VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 1,
                             "Wrong number of partitions in output");
      else
        continue;

      auto ds = out.GetPartition(0);

      //validate the outer (rank 0)
      if (comm.rank() == 0)
      {
        VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                             "Wrong number of coordinate systems in the output dataset");
        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        viskores::cont::UnknownCellSet dcells = ds.GetCellSet();

        VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(),
                             "Wrong cell type.");
        //The seed that goes through the inner is broken up into two polylines
        //the begining, and then the end.
        VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds + 1, "Wrong number of cells.");
        auto explicitCells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();
        for (viskores::Id j = 0; j < numSeeds; j++)
        {
          viskores::cont::ArrayHandle<viskores::Id> indices;
          explicitCells.GetIndices(j, indices);
          viskores::Id nPts = indices.GetNumberOfValues();
          auto iPortal = indices.ReadPortal();
          viskores::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));

          if (j == 0) //this is the seed that goes THROUGH inner.
          {
            VISKORES_TEST_ASSERT(outerBounds.Contains(lastPt),
                                 "End point is NOT inside the outer bounds.");
            VISKORES_TEST_ASSERT(innerBounds.Contains(lastPt),
                                 "End point is NOT inside the inner bounds.");
          }
          else
          {
            VISKORES_TEST_ASSERT(!outerBounds.Contains(lastPt),
                                 "Seed final location is INSIDE the dataset");
            VISKORES_TEST_ASSERT(lastPt[0] > outerBounds.X.Max,
                                 "Seed final location in wrong location");
          }
        }
      }

      //validate the inner (rank 1)
      else if (comm.rank() == 1)
      {
        VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                             "Wrong number of coordinate systems in the output dataset");
        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        auto dcells = ds.GetCellSet();

        VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(),
                             "Wrong cell type.");
        VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 1, "Wrong number of cells.");
        auto explicitCells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();

        viskores::cont::ArrayHandle<viskores::Id> indices;
        explicitCells.GetIndices(0, indices);
        viskores::Id nPts = indices.GetNumberOfValues();
        auto iPortal = indices.ReadPortal();
        viskores::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));

        //The last point should be OUTSIDE innerBoundsNoGhost but inside innerBounds
        VISKORES_TEST_ASSERT(!innerBoundsNoGhost.Contains(lastPt) && innerBounds.Contains(lastPt),
                             "Seed final location not contained in bounds correctly.");
        VISKORES_TEST_ASSERT(lastPt[0] > innerBoundsNoGhost.X.Max,
                             "Seed final location in wrong location");
      }
    }
    else if (fType == PARTICLE_ADVECTION)
    {
      viskores::filter::flow::ParticleAdvection filter;
      filter.SetUseThreadedAlgorithm(useThreaded);
      filter.SetStepSize(0.1f);
      filter.SetNumberOfSteps(100000);
      filter.SetSeeds(seedArray);

      filter.SetActiveField(fieldName);
      auto out = filter.Execute(pds);

      if (comm.rank() == 0)
      {
        VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 1,
                             "Wrong number of partitions in output");
        auto ds = out.GetPartition(0);
        VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                             "Wrong number of coordinate systems in the output dataset");
        viskores::cont::UnknownCellSet dcells = ds.GetCellSet();
        VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetSingleType<>>(),
                             "Wrong cell type.");

        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");

        for (viskores::Id i = 0; i < numSeeds; i++)
        {
          VISKORES_TEST_ASSERT(!outerBounds.Contains(ptPortal.Get(i)),
                               "Seed final location is INSIDE the dataset");
          VISKORES_TEST_ASSERT(ptPortal.Get(i)[0] > outerBounds.X.Max,
                               "Seed final location in wrong location");
        }
      }
    }
  }
}

void DoTest()
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  if (comm.rank() == 0)
  {
    std::cout << std::endl << "*** TestStreamlineAMRMPI" << std::endl;
  }

  for (auto fType : { PARTICLE_ADVECTION, STREAMLINE, PATHLINE })
  {
    for (auto useThreaded : { true, false })
    {
      TestAMRStreamline(fType, useThreaded);
    }
  }
}

} // anonymous namespace

int UnitTestStreamlineAMRMPI(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
