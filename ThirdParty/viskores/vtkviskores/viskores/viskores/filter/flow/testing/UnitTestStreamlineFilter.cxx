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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/ParticleAdvection.h>
#include <viskores/filter/flow/PathParticle.h>
#include <viskores/filter/flow/Pathline.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/flow/testing/GenerateTestDataSets.h>
#include <viskores/io/VTKDataSetReader.h>

namespace
{

enum FilterType
{
  PARTICLE_ADVECTION,
  STREAMLINE,
  PATHLINE,
  PATH_PARTICLE
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

void TestStreamline(bool useThreaded)
{
  const viskores::Id3 dims(5, 5, 5);
  const viskores::Bounds bounds(0, 4, 0, 4, 0, 4);
  const viskores::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";

  auto dataSets = viskores::worklet::testing::CreateAllDataSets(bounds, dims, false);
  for (auto& ds : dataSets)
  {
    auto vecField = CreateConstantVectorField(ds.GetNumberOfPoints(), vecX);
    ds.AddPointField(fieldName, vecField);
    viskores::cont::ArrayHandle<viskores::Particle> seedArray =
      viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
                                         viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1),
                                         viskores::Particle(viskores::Vec3f(.2f, 3.0f, .2f), 2) });

    viskores::filter::flow::Streamline streamline;

    streamline.SetUseThreadedAlgorithm(useThreaded);
    streamline.SetStepSize(0.1f);
    streamline.SetNumberOfSteps(20);
    streamline.SetSeeds(seedArray);

    streamline.SetActiveField(fieldName);
    auto output = streamline.Execute(ds);

    //Validate the result is correct.
    VISKORES_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");

    viskores::cont::CoordinateSystem coords = output.GetCoordinateSystem();
    VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == 63, "Wrong number of coordinates");

    viskores::cont::UnknownCellSet dcells = output.GetCellSet();
    VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
  }
}

void TestPathline(bool useThreaded)
{
  const viskores::Id3 dims(5, 5, 5);
  const viskores::Vec3f vecX(1, 0, 0);
  const viskores::Vec3f vecY(0, 1, 0);
  const viskores::Bounds bounds(0, 4, 0, 4, 0, 4);
  std::string var = "vec";

  //test pathline and pathparticle filters.
  for (int fType = 0; fType < 2; fType++)
  {
    auto dataSets1 = viskores::worklet::testing::CreateAllDataSets(bounds, dims, false);
    auto dataSets2 = viskores::worklet::testing::CreateAllDataSets(bounds, dims, false);

    std::size_t numDS = dataSets1.size();
    for (std::size_t i = 0; i < numDS; i++)
    {
      auto ds1 = dataSets1[i];
      auto ds2 = dataSets2[i];

      auto vecField1 = CreateConstantVectorField(ds1.GetNumberOfPoints(), vecX);
      auto vecField2 = CreateConstantVectorField(ds1.GetNumberOfPoints(), vecY);
      ds1.AddPointField(var, vecField1);
      ds2.AddPointField(var, vecField2);

      viskores::cont::ArrayHandle<viskores::Particle> seedArray = viskores::cont::make_ArrayHandle(
        { viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
          viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1),
          viskores::Particle(viskores::Vec3f(.2f, 3.0f, .2f), 2) });

      const viskores::FloatDefault stepSize = .1f;
      const viskores::FloatDefault t0 = 0, t1 = 1;
      const viskores::Id numSteps = 20;

      viskores::cont::DataSet output;
      viskores::Id numExpectedPoints;
      if (fType == 0)
      {
        viskores::filter::flow::Pathline filt;
        filt.SetUseThreadedAlgorithm(useThreaded);
        filt.SetActiveField(var);
        filt.SetStepSize(stepSize);
        filt.SetNumberOfSteps(numSteps);
        filt.SetSeeds(seedArray);
        filt.SetPreviousTime(t0);
        filt.SetNextTime(t1);
        filt.SetNextDataSet(ds2);
        output = filt.Execute(ds1);
        numExpectedPoints = 33;
      }
      else
      {
        viskores::filter::flow::PathParticle filt;
        filt.SetUseThreadedAlgorithm(useThreaded);
        filt.SetActiveField(var);
        filt.SetStepSize(stepSize);
        filt.SetNumberOfSteps(numSteps);
        filt.SetSeeds(seedArray);
        filt.SetPreviousTime(t0);
        filt.SetNextTime(t1);
        filt.SetNextDataSet(ds2);
        output = filt.Execute(ds1);
        numExpectedPoints = 3;
      }

      //Validate the result is correct.
      viskores::cont::CoordinateSystem coords = output.GetCoordinateSystem();

      VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == numExpectedPoints,
                           "Wrong number of coordinates");

      viskores::cont::UnknownCellSet dcells = output.GetCellSet();
      VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
    }
  }
}

void TestAMRStreamline(bool useSL, bool useThreaded)
{
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
    pds.AppendPartition(dsOuter);
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

    if (useSL)
    {
      viskores::filter::flow::Streamline filter;
      filter.SetUseThreadedAlgorithm(useThreaded);
      filter.SetStepSize(0.1f);
      filter.SetNumberOfSteps(100000);
      filter.SetSeeds(seedArray);

      filter.SetActiveField(fieldName);
      auto out = filter.Execute(pds);

      VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 2,
                           "Wrong number of partitions in output");
      auto ds0 = out.GetPartition(0);
      auto ds1 = out.GetPartition(1);

      //validate the outer
      VISKORES_TEST_ASSERT(ds0.GetNumberOfCoordinateSystems() == 1,
                           "Wrong number of coordinate systems in the output dataset");
      auto coords = ds0.GetCoordinateSystem().GetDataAsMultiplexer();
      auto ptPortal = coords.ReadPortal();
      viskores::cont::UnknownCellSet dcells = ds0.GetCellSet();

      VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(), "Wrong cell type.");
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

      //validate the inner
      VISKORES_TEST_ASSERT(ds1.GetNumberOfCoordinateSystems() == 1,
                           "Wrong number of coordinate systems in the output dataset");
      coords = ds1.GetCoordinateSystem().GetDataAsMultiplexer();
      ptPortal = coords.ReadPortal();
      dcells = ds1.GetCellSet();

      VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(), "Wrong cell type.");
      VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 1, "Wrong number of cells.");
      explicitCells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();

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
    else
    {
      viskores::filter::flow::ParticleAdvection filter;
      filter.SetUseThreadedAlgorithm(useThreaded);
      filter.SetStepSize(0.1f);
      filter.SetNumberOfSteps(100000);
      filter.SetSeeds(seedArray);

      filter.SetActiveField(fieldName);
      auto out = filter.Execute(pds);

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

void TestPartitionedDataSet(viskores::Id num, bool useGhost, FilterType fType, bool useThreaded)
{
  viskores::Id numDims = 5;
  viskores::FloatDefault x0 = 0;
  viskores::FloatDefault x1 = x0 + static_cast<viskores::FloatDefault>(numDims - 1);
  viskores::FloatDefault dx = x1 - x0;
  viskores::FloatDefault y0 = 0, y1 = static_cast<viskores::FloatDefault>(numDims - 1);
  viskores::FloatDefault z0 = 0, z1 = static_cast<viskores::FloatDefault>(numDims - 1);

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

  std::vector<viskores::Bounds> bounds;
  for (viskores::Id i = 0; i < num; i++)
  {
    bounds.push_back(viskores::Bounds(x0, x1, y0, y1, z0, z1));
    x0 += dx;
    x1 += dx;
  }

  std::vector<viskores::cont::PartitionedDataSet> allPDs, allPDs2;
  const viskores::Id3 dims(numDims, numDims, numDims);
  allPDs = viskores::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);
  if (fType == FilterType::PATHLINE || fType == FilterType::PATH_PARTICLE)
    allPDs2 = viskores::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);

  viskores::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";
  for (std::size_t idx = 0; idx < allPDs.size(); idx++)
  {
    auto pds = allPDs[idx];
    AddVectorFields(pds, fieldName, vecX);

    viskores::cont::ArrayHandle<viskores::Particle> seedArray;
    seedArray =
      viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
                                         viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1) });
    viskores::Id numSeeds = seedArray.GetNumberOfValues();
    if (fType == FilterType::STREAMLINE || fType == FilterType::PATHLINE)
    {
      viskores::cont::PartitionedDataSet out;
      if (fType == FilterType::STREAMLINE)
      {
        viskores::filter::flow::Streamline streamline;
        streamline.SetUseThreadedAlgorithm(useThreaded);
        streamline.SetStepSize(0.1f);
        streamline.SetNumberOfSteps(100000);
        streamline.SetSeeds(seedArray);
        streamline.SetActiveField(fieldName);
        out = streamline.Execute(pds);
      }
      else
      {
        auto pds2 = allPDs2[idx];
        AddVectorFields(pds2, fieldName, vecX);

        viskores::filter::flow::Pathline pathline;
        pathline.SetUseThreadedAlgorithm(useThreaded);
        pathline.SetPreviousTime(0);
        pathline.SetNextTime(1000);
        pathline.SetNextDataSet(pds2);
        pathline.SetStepSize(0.1f);
        pathline.SetNumberOfSteps(100000);
        pathline.SetSeeds(seedArray);

        pathline.SetActiveField(fieldName);
        out = pathline.Execute(pds);
      }

      for (viskores::Id i = 0; i < num; i++)
      {
        auto outputDS = out.GetPartition(i);
        VISKORES_TEST_ASSERT(outputDS.GetNumberOfCoordinateSystems() == 1,
                             "Wrong number of coordinate systems in the output dataset");

        viskores::cont::UnknownCellSet dcells = outputDS.GetCellSet();
        VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");

        auto coords = outputDS.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();

        viskores::cont::CellSetExplicit<> explicitCells;

        VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(),
                             "Wrong cell type.");
        explicitCells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();

        viskores::FloatDefault xMax =
          static_cast<viskores::FloatDefault>(bounds[static_cast<std::size_t>(i)].X.Max);
        if (useGhost)
          xMax = xMax - 1;
        viskores::Range xMaxRange(xMax, xMax + static_cast<viskores::FloatDefault>(.5));

        for (viskores::Id j = 0; j < numSeeds; j++)
        {
          viskores::cont::ArrayHandle<viskores::Id> indices;
          explicitCells.GetIndices(j, indices);
          viskores::Id nPts = indices.GetNumberOfValues();
          auto iPortal = indices.ReadPortal();
          viskores::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));
          VISKORES_TEST_ASSERT(xMaxRange.Contains(lastPt[0]), "Wrong end point for seed");
        }
      }
    }
    else if (fType == FilterType::PARTICLE_ADVECTION || fType == FilterType::PATH_PARTICLE)
    {
      viskores::cont::PartitionedDataSet out;
      if (fType == FilterType::PARTICLE_ADVECTION)
      {
        viskores::filter::flow::ParticleAdvection particleAdvection;

        particleAdvection.SetStepSize(0.1f);
        particleAdvection.SetNumberOfSteps(100000);
        particleAdvection.SetSeeds(seedArray);

        particleAdvection.SetActiveField(fieldName);
        out = particleAdvection.Execute(pds);
      }
      else
      {
        auto pds2 = allPDs2[idx];
        AddVectorFields(pds2, fieldName, vecX);

        viskores::filter::flow::PathParticle pathParticle;
        pathParticle.SetPreviousTime(0);
        pathParticle.SetNextTime(1000);
        pathParticle.SetNextDataSet(pds2);
        pathParticle.SetStepSize(0.1f);
        pathParticle.SetNumberOfSteps(100000);
        pathParticle.SetSeeds(seedArray);

        pathParticle.SetActiveField(fieldName);
        out = pathParticle.Execute(pds);
      }


      VISKORES_TEST_ASSERT(out.GetNumberOfPartitions() == 1,
                           "Wrong number of partitions in output");
      auto ds = out.GetPartition(0);
      //Validate the result is correct.
      VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                           "Wrong number of coordinate systems in the output dataset");

      viskores::FloatDefault xMax =
        static_cast<viskores::FloatDefault>(bounds[bounds.size() - 1].X.Max);
      if (useGhost)
        xMax = xMax - 1;
      viskores::Range xMaxRange(xMax, xMax + static_cast<viskores::FloatDefault>(.5));

      auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
      VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
      auto ptPortal = coords.ReadPortal();
      for (viskores::Id i = 0; i < numSeeds; i++)
        VISKORES_TEST_ASSERT(xMaxRange.Contains(ptPortal.Get(i)[0]), "Wrong end point for seed");

      viskores::cont::UnknownCellSet dcells = ds.GetCellSet();
      VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
    }
  }
}

template <typename CellSetType, typename CoordsType>
void ValidateEndPoints(const CellSetType& cellSet,
                       const CoordsType& coords,
                       viskores::Id numPoints,
                       const std::vector<viskores::Vec3f>& endPts)
{
  const viskores::FloatDefault eps = static_cast<viskores::FloatDefault>(1e-3);
  auto cPortal = coords.ReadPortal();

  for (viskores::Id i = 0; i < numPoints; i++)
  {
    viskores::Id numPts = cellSet.GetNumberOfPointsInCell(i);
    std::vector<viskores::Id> ids(static_cast<std::size_t>(numPts));
    cellSet.GetCellPointIds(i, ids.data());

    viskores::Vec3f e = endPts[static_cast<std::size_t>(i)];
    viskores::Vec3f pt = cPortal.Get(ids[ids.size() - 1]);
    VISKORES_TEST_ASSERT(viskores::Magnitude(pt - e) <= eps, "Particle advection point is wrong");
  }
}

void TestStreamlineFile(const std::string& fileName,
                        const std::string& fieldName,
                        const std::vector<viskores::Vec3f>& pts,
                        viskores::FloatDefault stepSize,
                        viskores::Id maxSteps,
                        const std::vector<viskores::Vec3f>& endPts,
                        bool useSL)
{
  viskores::io::VTKDataSetReader reader(fileName);
  viskores::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
    VISKORES_TEST_ASSERT(ds.HasField(fieldName));
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fileName;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }
  viskores::Id numPoints = static_cast<viskores::Id>(pts.size());

  std::vector<viskores::Particle> seeds;
  for (viskores::Id i = 0; i < numPoints; i++)
    seeds.push_back(viskores::Particle(pts[static_cast<std::size_t>(i)], i));
  auto seedArray = viskores::cont::make_ArrayHandle(seeds, viskores::CopyFlag::Off);

  viskores::cont::DataSet output;
  if (useSL)
  {
    viskores::filter::flow::Streamline streamline;
    streamline.SetStepSize(stepSize);
    streamline.SetNumberOfSteps(maxSteps);
    streamline.SetSeeds(seedArray);
    streamline.SetActiveField(fieldName);
    output = streamline.Execute(ds);
  }
  else
  {
    viskores::filter::flow::ParticleAdvection particleAdvection;
    particleAdvection.SetStepSize(stepSize);
    particleAdvection.SetNumberOfSteps(maxSteps);
    particleAdvection.SetSeeds(seedArray);
    particleAdvection.SetActiveField(fieldName);
    output = particleAdvection.Execute(ds);
  }

  auto coords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  viskores::cont::UnknownCellSet dcells = output.GetCellSet();
  VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == numPoints, "Wrong number of cells");

  if (useSL)
  {
    VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetExplicit<>>(), "Wrong cell type");
    auto cells = dcells.AsCellSet<viskores::cont::CellSetExplicit<>>();
    ValidateEndPoints(cells, coords, numPoints, endPts);
  }
  else
  {
    VISKORES_TEST_ASSERT(dcells.IsType<viskores::cont::CellSetSingleType<>>(), "Wrong cell type");
    auto cells = dcells.AsCellSet<viskores::cont::CellSetSingleType<>>();
    ValidateEndPoints(cells, coords, numPoints, endPts);
  }
}

void TestStreamlineFilters()
{
  std::vector<bool> flags = { true, false };
  std::vector<FilterType> fTypes = { FilterType::PARTICLE_ADVECTION,
                                     FilterType::STREAMLINE,
                                     FilterType::PATHLINE,
                                     FilterType::PATH_PARTICLE };
  for (int n = 1; n < 3; n++)
  {
    for (auto useGhost : flags)
      for (auto ft : fTypes)
        TestPartitionedDataSet(n, useGhost, ft, false);
  }

  for (auto useThreaded : flags)
  {
    TestStreamline(useThreaded);
    TestPathline(useThreaded);
  }
  for (auto useSL : flags)
    TestAMRStreamline(useSL, false);

  {
    //Rotate test.
    std::vector<viskores::Vec3f> startPoints, endPoints;
    startPoints.push_back(viskores::Vec3f(0.4f, 0.3f, -0.2f));
    startPoints.push_back(viskores::Vec3f(-0.4f, 0.0f, -0.84f));
    startPoints.push_back(viskores::Vec3f(0.0f, 0.0f, 0.41f));
    //End point values were generated in VisIt.
    endPoints.push_back(viskores::Vec3f(-0.341196f, 0.474331f, 0.142614f));
    endPoints.push_back(viskores::Vec3f(-0.342764f, -0.713572f, -0.746209f));
    endPoints.push_back(viskores::Vec3f(-0.617492f, -0.0167f, 0.104733f));
    viskores::FloatDefault stepSize = 0.1f;
    std::string file = viskores::cont::testing::Testing::DataPath("uniform/rotate-vectors.vtk");

    for (auto useSL : flags)
    {
      TestStreamlineFile(file, "rotate", startPoints, stepSize, 1000, endPoints, useSL);
    }
  }

  {
    //Kitchen test.
    std::vector<viskores::Vec3f> startPoints, endPoints;
    startPoints.push_back(viskores::Vec3f(6.0f, 1.0f, 2.0f));
    startPoints.push_back(viskores::Vec3f(1.3f, 2.4f, 1.3f));
    startPoints.push_back(viskores::Vec3f(1.0f, 3.0f, 2.0f));
    //End point values were generated in VisIt.
    endPoints.push_back(viskores::Vec3f(4.42419f, 0.956935f, 1.89111f));
    endPoints.push_back(viskores::Vec3f(0.217019f, 3.65243f, 2.49638f));
    endPoints.push_back(viskores::Vec3f(0.753178f, 0.410568f, 1.11006f));
    viskores::FloatDefault stepSize = 0.2f;
    std::string file = viskores::cont::testing::Testing::DataPath("curvilinear/kitchen.vtk");

    for (auto useSL : flags)
    {
      TestStreamlineFile(file, "velocity", startPoints, stepSize, 2000, endPoints, useSL);
    }
  }

  {
    //ARMWind corner case of particle near boundary.
    std::string file =
      viskores::cont::testing::Testing::DataPath("rectilinear/amr_wind_flowfield.vtk");
    viskores::FloatDefault stepSize = 0.001f;
    std::vector<viskores::Vec3f> startPoints, endPoints;

    startPoints.push_back(
      viskores::Vec3f(0.053217993470017745f, 0.034506499099396459f, 0.057097713925011492f));
    endPoints.push_back(viskores::Vec3f(0.05712112784f, 0.03450008854f, 0.02076501213f));

    for (auto useSL : flags)
    {
      TestStreamlineFile(file, "vec", startPoints, stepSize, 10000, endPoints, useSL);
    }
  }
}
}

int UnitTestStreamlineFilter(int argc, char* argv[])
{
  // Setup MPI environment: This test is not intendent to be run in parallel
  // but filter does make MPI calls
  viskoresdiy::mpi::environment env(argc, argv);
  return viskores::cont::testing::Testing::Run(TestStreamlineFilters, argc, argv);
}
