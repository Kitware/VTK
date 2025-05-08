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

#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/CellTraits.h>
#include <viskores/StaticAssert.h>
#include <viskores/VecVariable.h>

#include <viskores/cont/testing/Testing.h>

#include <ctime>
#include <random>

#define CHECK_CALL(call) \
  VISKORES_TEST_ASSERT((call) == viskores::ErrorCode::Success, "Call resulted in error.")

namespace
{

std::mt19937 g_RandomGenerator;

static constexpr viskores::IdComponent MAX_POINTS = 8;

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     viskores::CellTraitsTagSizeFixed,
                     viskores::IdComponent& minPoints,
                     viskores::IdComponent& maxPoints)
{
  // If this line fails, then MAX_POINTS is not large enough to support all
  // cell shapes.
  VISKORES_STATIC_ASSERT((viskores::CellTraits<CellShapeTag>::NUM_POINTS <= MAX_POINTS));
  minPoints = maxPoints = viskores::CellTraits<CellShapeTag>::NUM_POINTS;
}

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     viskores::CellTraitsTagSizeVariable,
                     viskores::IdComponent& minPoints,
                     viskores::IdComponent& maxPoints)
{
  minPoints = 1;
  maxPoints = MAX_POINTS;
}

template <typename PointWCoordsType, typename T, typename CellShapeTag>
static void CompareCoordinates(const PointWCoordsType& pointWCoords,
                               viskores::Vec<T, 3> truePCoords,
                               viskores::Vec<T, 3> trueWCoords,
                               CellShapeTag shape)
{
  using Vector3 = viskores::Vec<T, 3>;

  Vector3 computedWCoords;
  CHECK_CALL(viskores::exec::ParametricCoordinatesToWorldCoordinates(
    pointWCoords, truePCoords, shape, computedWCoords));
  VISKORES_TEST_ASSERT(test_equal(computedWCoords, trueWCoords, 0.01),
                       "Computed wrong world coords from parametric coords.");

  Vector3 computedPCoords;
  CHECK_CALL(viskores::exec::WorldCoordinatesToParametricCoordinates(
    pointWCoords, trueWCoords, shape, computedPCoords));
  VISKORES_TEST_ASSERT(test_equal(computedPCoords, truePCoords, 0.01),
                       "Computed wrong parametric coords from world coords.");
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSpecial(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;
  using T = typename Vector3::ComponentType;

  const viskores::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

  for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    Vector3 pcoords;
    CHECK_CALL(viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pcoords));
    Vector3 wcoords = pointWCoords[pointIndex];
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }

  {
    Vector3 wcoords = pointWCoords[0];
    for (viskores::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
    {
      wcoords = wcoords + pointWCoords[pointIndex];
    }
    wcoords = wcoords / Vector3(T(numPoints));

    Vector3 pcoords;
    CHECK_CALL(viskores::exec::ParametricCoordinatesCenter(numPoints, shape, pcoords));
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSample(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;

  const viskores::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

  std::uniform_real_distribution<viskores::FloatDefault> randomDist;

  for (viskores::IdComponent trial = 0; trial < 5; trial++)
  {
    // Generate a random pcoords that we know is in the cell.
    viskores::Vec3f pcoords(0);
    viskores::FloatDefault totalWeight = 0;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Vec3f pointPcoords;
      CHECK_CALL(
        viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pointPcoords));
      viskores::FloatDefault weight = randomDist(g_RandomGenerator);
      pcoords = pcoords + weight * pointPcoords;
      totalWeight += weight;
    }
    pcoords = (1 / totalWeight) * pcoords;

    // If you convert to world coordinates and back, you should get the
    // same value.
    Vector3 wcoords;
    CHECK_CALL(viskores::exec::ParametricCoordinatesToWorldCoordinates(
      pointWCoords, pcoords, shape, wcoords));
    Vector3 computedPCoords;
    CHECK_CALL(viskores::exec::WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, shape, computedPCoords));

    VISKORES_TEST_ASSERT(test_equal(pcoords, computedPCoords, 0.05),
                         "pcoord/wcoord transform not symmetrical");
  }
}

template <typename PointWCoordsType, typename CellShellTag>
static void TestPCoords(const PointWCoordsType& pointWCoords, CellShellTag shape)
{
  TestPCoordsSpecial(pointWCoords, shape);
  TestPCoordsSample(pointWCoords, shape);
}

template <typename T>
struct TestPCoordsFunctor
{
  using Vector3 = viskores::Vec<T, 3>;
  using PointWCoordType = viskores::VecVariable<Vector3, MAX_POINTS>;

  template <typename CellShapeTag>
  PointWCoordType MakePointWCoords(CellShapeTag, viskores::IdComponent numPoints) const
  {
    std::uniform_real_distribution<T> randomDist(-1, 1);

    Vector3 sheerVec(randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), 0);

    PointWCoordType pointWCoords;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      Vector3 pcoords;
      CHECK_CALL(
        viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, CellShapeTag(), pcoords));

      Vector3 wCoords =
        Vector3(pcoords[0], pcoords[1], pcoords[2] + viskores::Dot(pcoords, sheerVec));
      pointWCoords.Append(wCoords);
    }

    return pointWCoords;
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    viskores::IdComponent minPoints;
    viskores::IdComponent maxPoints;
    GetMinMaxPoints(CellShapeTag(),
                    typename viskores::CellTraits<CellShapeTag>::IsSizeFixed(),
                    minPoints,
                    maxPoints);

    std::cout << "--- Test shape tag directly" << std::endl;
    for (viskores::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), CellShapeTag());
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    viskores::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (viskores::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), genericShape);
    }
  }

  void operator()(viskores::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No points." << std::endl;
  }
};

void TestAllPCoords()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<viskores::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<viskores::Float64>());

  std::cout << "======== Rectilinear Shapes ===============" << std::endl;
  std::uniform_real_distribution<viskores::FloatDefault> randomDist(0.01f, 1.0f);
  viskores::Vec3f origin(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  viskores::Vec3f spacing(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));

  TestPCoords(viskores::VecAxisAlignedPointCoordinates<3>(origin, spacing),
              viskores::CellShapeTagHexahedron());
  TestPCoords(viskores::VecAxisAlignedPointCoordinates<2>(origin, spacing),
              viskores::CellShapeTagQuad());
  TestPCoords(viskores::VecAxisAlignedPointCoordinates<1>(origin, spacing),
              viskores::CellShapeTagLine());
}

} // Anonymous namespace

int UnitTestParametricCoordinates(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAllPCoords, argc, argv);
}
