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

#include "TestingFlow.h"

#include <viskores/CellClassification.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/ParticleAdvection.h>
#include <viskores/filter/flow/Pathline.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/flow/testing/GenerateTestDataSets.h>
#include <viskores/thirdparty/diy/diy.h>

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

std::vector<viskores::cont::PartitionedDataSet> CreateAllDataSetBounds(viskores::Id nPerRank,
                                                                       bool useGhost)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  viskores::Id totNumBlocks = nPerRank * comm.size();
  viskores::Id numDims = 5;

  viskores::FloatDefault x0 = 0;
  viskores::FloatDefault x1 = x0 + static_cast<viskores::FloatDefault>(numDims - 1);
  viskores::FloatDefault dx = x1 - x0;
  viskores::FloatDefault y0 = 0, y1 = numDims - 1, z0 = 0, z1 = numDims - 1;

  if (useGhost)
  {
    numDims = numDims + 2; //add 1 extra on each side
    x0 = x0 - 1;
    x1 = x1 + 1;
    dx = x1 - x0 - 2;
    y0 = y0 - 1;
    y1 = y1 + 1;
    z0 = z0 - 1;
    z1 = z1 + 1;
  }

  //Create ALL of the blocks.
  std::vector<viskores::Bounds> bounds;
  for (viskores::Id i = 0; i < totNumBlocks; i++)
  {
    bounds.push_back(viskores::Bounds(x0, x1, y0, y1, z0, z1));
    x0 += dx;
    x1 += dx;
  }

  const viskores::Id3 dims(numDims, numDims, numDims);
  auto allPDS = viskores::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);

  return allPDS;
}

std::vector<viskores::Range> ExtractMaxXRanges(const viskores::cont::PartitionedDataSet& pds,
                                               bool useGhost)
{
  std::vector<viskores::Range> xMaxRanges;
  for (const auto& ds : pds.GetPartitions())
  {
    auto bounds = ds.GetCoordinateSystem().GetBounds();
    auto xMax = bounds.X.Max;
    if (useGhost)
      xMax = xMax - 1;
    xMaxRanges.push_back(viskores::Range(xMax, xMax + static_cast<viskores::FloatDefault>(.5)));
  }

  return xMaxRanges;
}

void ValidateOutput(const viskores::cont::DataSet& out,
                    viskores::Id numSeeds,
                    const viskores::Range& xMaxRange,
                    FilterType fType,
                    bool checkEndPoint,
                    bool blockDuplication)
{
  //Validate the result is correct.
  VISKORES_TEST_ASSERT(out.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

  viskores::cont::UnknownCellSet dcells = out.GetCellSet();
  viskores::Id numCells = out.GetNumberOfCells();

  if (!blockDuplication)
    VISKORES_TEST_ASSERT(numCells == numSeeds, "Wrong number of cells");

  auto coords = out.GetCoordinateSystem().GetDataAsMultiplexer();
  auto ptPortal = coords.ReadPortal();

  if (fType == STREAMLINE || fType == PATHLINE)
  {
    viskores::cont::CellSetExplicit<> explicitCells;
    VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(), "Wrong cell type.");
    explicitCells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();
    for (viskores::Id j = 0; j < numCells; j++)
    {
      viskores::cont::ArrayHandle<viskores::Id> indices;
      explicitCells.GetIndices(j, indices);
      viskores::Id nPts = indices.GetNumberOfValues();
      auto iPortal = indices.ReadPortal();
      viskores::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));
      if (checkEndPoint)
        VISKORES_TEST_ASSERT(xMaxRange.Contains(lastPt[0]), "Wrong end point for seed");
    }
  }
  else if (fType == PARTICLE_ADVECTION)
  {
    if (!blockDuplication)
      VISKORES_TEST_ASSERT(out.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
    if (checkEndPoint)
    {
      for (viskores::Id i = 0; i < numCells; i++)
        VISKORES_TEST_ASSERT(xMaxRange.Contains(ptPortal.Get(i)[0]), "Wrong end point for seed");
    }
  }
}

void TestPartitionedDataSet(viskores::Id nPerRank,
                            bool useGhost,
                            FilterType fType,
                            bool useThreaded,
                            bool useBlockIds,
                            bool duplicateBlocks)
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
    std::cout << " blocksPerRank= " << nPerRank;
    if (useGhost)
      std::cout << " - using ghost cells";
    if (useThreaded)
      std::cout << " - using threaded";
    if (useBlockIds)
      std::cout << " - using block IDs";
    if (duplicateBlocks)
      std::cout << " - with duplicate blocks";
    std::cout << " - on a partitioned data set" << std::endl;
  }

  std::vector<viskores::Id> blockIds;
  //Uniform assignment.
  for (viskores::Id i = 0; i < nPerRank; i++)
    blockIds.push_back(comm.rank() * nPerRank + i);

  //For block duplication, give everyone the 2nd to last block.
  //We want to keep the last block on the last rank for validation.
  if (duplicateBlocks && blockIds.size() > 1)
  {
    viskores::Id totNumBlocks = comm.size() * nPerRank;
    viskores::Id dupBlock = totNumBlocks - 2;
    for (int r = 0; r < comm.size(); r++)
    {
      if (std::find(blockIds.begin(), blockIds.end(), dupBlock) == blockIds.end())
        blockIds.push_back(dupBlock);
    }
  }

  std::vector<viskores::cont::PartitionedDataSet> allPDS, allPDS2;
  allPDS = CreateAllDataSetBounds(nPerRank, useGhost);
  allPDS2 = CreateAllDataSetBounds(nPerRank, useGhost);
  auto xMaxRanges = ExtractMaxXRanges(allPDS[0], useGhost);

  viskores::FloatDefault time0 = 0;
  viskores::FloatDefault time1 = xMaxRanges[xMaxRanges.size() - 1].Max;

  viskores::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";
  viskores::FloatDefault stepSize = 0.1f;
  viskores::Id numSteps = 100000;
  for (std::size_t n = 0; n < allPDS.size(); n++)
  {
    viskores::cont::PartitionedDataSet pds;
    for (const auto& bid : blockIds)
      pds.AppendPartition(allPDS[n].GetPartition(bid));
    AddVectorFields(pds, fieldName, vecX);

    viskores::cont::ArrayHandle<viskores::Particle> seedArray;
    seedArray =
      viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
                                         viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1) });
    viskores::Id numSeeds = seedArray.GetNumberOfValues();

    if (fType == STREAMLINE)
    {
      viskores::filter::flow::Streamline streamline;
      SetFilter(
        streamline, stepSize, numSteps, fieldName, seedArray, useThreaded, useBlockIds, blockIds);
      auto out = streamline.Execute(pds);

      viskores::Id numOutputs = out.GetNumberOfPartitions();
      bool checkEnds = numOutputs == static_cast<viskores::Id>(blockIds.size());
      for (viskores::Id i = 0; i < numOutputs; i++)
      {
        ValidateOutput(out.GetPartition(i),
                       numSeeds,
                       xMaxRanges[blockIds[i]],
                       fType,
                       checkEnds,
                       duplicateBlocks);
      }
    }
    else if (fType == PARTICLE_ADVECTION)
    {
      viskores::filter::flow::ParticleAdvection particleAdvection;
      SetFilter(particleAdvection,
                stepSize,
                numSteps,
                fieldName,
                seedArray,
                useThreaded,
                useBlockIds,
                blockIds);

      auto out = particleAdvection.Execute(pds);

      //Particles end up in last rank.
      if (comm.rank() == comm.size() - 1)
      {
        bool checkEnds = out.GetNumberOfPartitions() == static_cast<viskores::Id>(blockIds.size());
        auto nP = out.GetNumberOfPartitions();
        if (nP != 1)
          std::cout << comm.rank() << " numPartitions= " << nP << std::endl;
        VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 1,
                             "Wrong number of partitions in output 1");
        ValidateOutput(out.GetPartition(0),
                       numSeeds,
                       xMaxRanges[xMaxRanges.size() - 1],
                       fType,
                       checkEnds,
                       duplicateBlocks);
      }
      else
        VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 0,
                             "Wrong number of partitions in output 0");
    }
    else if (fType == PATHLINE)
    {
      viskores::cont::PartitionedDataSet pds2;
      for (const auto& bid : blockIds)
        pds2.AppendPartition(allPDS2[n].GetPartition(bid));
      AddVectorFields(pds2, fieldName, vecX);

      viskores::filter::flow::Pathline pathline;
      SetFilter(
        pathline, stepSize, numSteps, fieldName, seedArray, useThreaded, useBlockIds, blockIds);

      pathline.SetPreviousTime(time0);
      pathline.SetNextTime(time1);
      pathline.SetNextDataSet(pds2);

      auto out = pathline.Execute(pds);
      viskores::Id numOutputs = out.GetNumberOfPartitions();
      bool checkEnds = numOutputs == static_cast<viskores::Id>(blockIds.size());
      for (viskores::Id i = 0; i < numOutputs; i++)
        ValidateOutput(out.GetPartition(i),
                       numSeeds,
                       xMaxRanges[blockIds[i]],
                       fType,
                       checkEnds,
                       duplicateBlocks);
    }
  }
}
