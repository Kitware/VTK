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

#include <viskores/exec/CellDerivative.h>
#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/exec/internal/ErrorMessageBuffer.h>

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

// Establish simple mapping between world and parametric coordinates.
// Actual world/parametric coordinates are in a different test.
template <typename T>
viskores::Vec<T, 3> ParametricToWorld(const viskores::Vec<T, 3>& pcoord)
{
  return T(2) * pcoord - viskores::Vec<T, 3>(0.25f);
}
template <typename T>
viskores::Vec<T, 3> WorldToParametric(const viskores::Vec<T, 3>& wcoord)
{
  return T(0.5) * (wcoord + viskores::Vec<T, 3>(0.25f));
}

/// Simple structure describing a linear field.  Has a convenience class
/// for getting values.
template <typename FieldType>
struct LinearField
{
  viskores::Vec<FieldType, 3> Gradient;
  FieldType OriginValue;

  template <typename T>
  FieldType GetValue(viskores::Vec<T, 3> coordinates) const
  {
    return static_cast<FieldType>((coordinates[0] * this->Gradient[0] +
                                   coordinates[1] * this->Gradient[1] +
                                   coordinates[2] * this->Gradient[2]) +
                                  this->OriginValue);
  }
};

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

template <typename FieldType>
struct TestDerivativeFunctor
{
  template <typename CellShapeTag, typename WCoordsVecType>
  void DoTestWithWCoords(CellShapeTag shape,
                         const WCoordsVecType worldCoordinates,
                         LinearField<FieldType> field,
                         viskores::Vec<FieldType, 3> expectedGradient) const
  {
    viskores::IdComponent numPoints = worldCoordinates.GetNumberOfComponents();

    viskores::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Vec3f wcoords = worldCoordinates[pointIndex];
      FieldType value = static_cast<FieldType>(field.GetValue(wcoords));
      fieldValues.Append(value);
    }

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

      viskores::Vec<FieldType, 3> computedGradient;
      CHECK_CALL(viskores::exec::CellDerivative(
        fieldValues, worldCoordinates, pcoords, shape, computedGradient));

      // Note that some gradients (particularly those near the center of
      // polygons with 5 or more points) are not very precise. Thus the
      // tolarance of the test_equal is raised.
      VISKORES_TEST_ASSERT(test_equal(computedGradient, expectedGradient, 0.01),
                           "Gradient is not as expected.");
    }
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              viskores::IdComponent numPoints,
              LinearField<FieldType> field,
              viskores::Vec<FieldType, 3> expectedGradient) const
  {
    viskores::VecVariable<viskores::Vec3f, MAX_POINTS> worldCoordinates;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Vec3f pcoords;
      CHECK_CALL(viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pcoords));
      viskores::Vec3f wcoords = ParametricToWorld(pcoords);
      VISKORES_TEST_ASSERT(test_equal(pcoords, WorldToParametric(wcoords)),
                           "Test world/parametric conversion broken.");
      worldCoordinates.Append(wcoords);
    }

    this->DoTestWithWCoords(shape, worldCoordinates, field, expectedGradient);
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              viskores::IdComponent numPoints,
              viskores::IdComponent topDim) const
  {
    LinearField<FieldType> field;
    viskores::Vec<FieldType, 3> expectedGradient;

    using FieldTraits = viskores::VecTraits<FieldType>;
    using FieldComponentType = typename FieldTraits::ComponentType;

    // Correct topDim for polygons with fewer than 3 points.
    if (topDim > numPoints - 1)
    {
      topDim = numPoints - 1;
    }

    for (viskores::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, 0.0);
    }
    field.Gradient = viskores::make_Vec(FieldType(1.0f), FieldType(1.0f), FieldType(1.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    for (viskores::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(-7.0f));
    }
    field.Gradient = viskores::make_Vec(FieldType(0.25f), FieldType(14.0f), FieldType(11.125f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    for (viskores::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(5.0f));
    }
    field.Gradient = viskores::make_Vec(FieldType(-11.125f), FieldType(-0.25f), FieldType(14.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::uniform_real_distribution<FieldComponentType> randomDist(-20.0f, 20.0f);
    for (viskores::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[0], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[1], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[2], fieldComponent, randomDist(g_RandomGenerator));
    }
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);
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
      this->DoTest(
        CellShapeTag(), numPoints, viskores::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    viskores::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (viskores::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(
        genericShape, numPoints, viskores::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }
  }

  void operator()(viskores::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No derivative." << std::endl;
  }
};

void TestDerivative()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<viskores::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<viskores::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<viskores::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<viskores::Vec3f_64>());

  std::uniform_real_distribution<viskores::Float64> randomDist(-20.0, 20.0);
  viskores::Vec3f origin = viskores::Vec3f(0.25f, 0.25f, 0.25f);
  viskores::Vec3f spacing = viskores::Vec3f(2.0f, 2.0f, 2.0f);

  LinearField<viskores::Float64> scalarField;
  scalarField.OriginValue = randomDist(g_RandomGenerator);
  scalarField.Gradient = viskores::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  viskores::Vec3f_64 expectedScalarGradient = scalarField.Gradient;

  TestDerivativeFunctor<viskores::Float64> testFunctorScalar;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorScalar.DoTestWithWCoords(viskores::CellShapeTagHexahedron(),
                                      viskores::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedScalarGradient[2] = 0.0;
  testFunctorScalar.DoTestWithWCoords(viskores::CellShapeTagQuad(),
                                      viskores::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedScalarGradient[1] = 0.0;
  testFunctorScalar.DoTestWithWCoords(viskores::CellShapeTagLine(),
                                      viskores::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);

  LinearField<viskores::Vec3f_64> vectorField;
  vectorField.OriginValue = viskores::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  vectorField.Gradient = viskores::make_Vec(
    viskores::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    viskores::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    viskores::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)));
  viskores::Vec<viskores::Vec3f_64, 3> expectedVectorGradient = vectorField.Gradient;

  TestDerivativeFunctor<viskores::Vec3f_64> testFunctorVector;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorVector.DoTestWithWCoords(viskores::CellShapeTagHexahedron(),
                                      viskores::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedVectorGradient[2] = viskores::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(viskores::CellShapeTagQuad(),
                                      viskores::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedVectorGradient[1] = viskores::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(viskores::CellShapeTagLine(),
                                      viskores::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
}

} // anonymous namespace

int UnitTestCellDerivative(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDerivative, argc, argv);
}
