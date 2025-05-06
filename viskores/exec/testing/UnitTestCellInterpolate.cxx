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

#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/exec/internal/ErrorMessageBuffer.h>

#include <viskores/CellTraits.h>
#include <viskores/StaticAssert.h>
#include <viskores/VecAxisAlignedPointCoordinates.h>
#include <viskores/VecVariable.h>

#include <viskores/cont/testing/Testing.h>

#define CHECK_CALL(call) \
  VISKORES_TEST_ASSERT((call) == viskores::ErrorCode::Success, "Call resulted in error.")

namespace
{

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
struct TestInterpolateFunctor
{
  using ComponentType = typename viskores::VecTraits<FieldType>::ComponentType;

  template <typename CellShapeTag, typename FieldVecType>
  void DoTestWithField(CellShapeTag shape, const FieldVecType& fieldValues) const
  {
    viskores::IdComponent numPoints = fieldValues.GetNumberOfComponents();
    if (numPoints < 1)
    {
      return;
    }

    FieldType averageValue = viskores::TypeTraits<FieldType>::ZeroInitialization();
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      averageValue = averageValue + fieldValues[pointIndex];
    }
    averageValue = static_cast<ComponentType>(1.0 / numPoints) * averageValue;

    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Vec3f pcoord;
      CHECK_CALL(viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pcoord));
      FieldType interpolatedValue;
      CHECK_CALL(viskores::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue));

      VISKORES_TEST_ASSERT(test_equal(fieldValues[pointIndex], interpolatedValue),
                           "Interpolation at point not point value.");
    }

    viskores::Vec3f pcoord;
    CHECK_CALL(viskores::exec::ParametricCoordinatesCenter(numPoints, shape, pcoord));
    FieldType interpolatedValue;
    CHECK_CALL(viskores::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue));

    VISKORES_TEST_ASSERT(test_equal(averageValue, interpolatedValue),
                         "Interpolation at center not average value.");
  }

  template <typename CellShapeTag, typename IndexVecType, typename FieldPortalType>
  void DoTestWithIndices(CellShapeTag shape,
                         const IndexVecType& pointIndices,
                         const FieldPortalType& fieldValues) const
  {
    viskores::IdComponent numPoints = pointIndices.GetNumberOfComponents();
    if (numPoints < 1)
    {
      return;
    }

    FieldType averageValue = viskores::TypeTraits<FieldType>::ZeroInitialization();
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      averageValue = averageValue + fieldValues.Get(pointIndices[pointIndex]);
    }
    averageValue = static_cast<ComponentType>(1.0 / numPoints) * averageValue;

    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Vec3f pcoord;
      CHECK_CALL(viskores::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pcoord));
      FieldType interpolatedValue;
      CHECK_CALL(viskores::exec::CellInterpolate(
        pointIndices, fieldValues, pcoord, shape, interpolatedValue));

      VISKORES_TEST_ASSERT(test_equal(fieldValues.Get(pointIndices[pointIndex]), interpolatedValue),
                           "Interpolation at point not point value.");
    }

    if (shape.Id != viskores::CELL_SHAPE_POLY_LINE)
    {
      viskores::Vec3f pcoord;
      CHECK_CALL(viskores::exec::ParametricCoordinatesCenter(numPoints, shape, pcoord));
      FieldType interpolatedValue;
      CHECK_CALL(viskores::exec::CellInterpolate(
        pointIndices, fieldValues, pcoord, shape, interpolatedValue));

      VISKORES_TEST_ASSERT(test_equal(averageValue, interpolatedValue),
                           "Interpolation at center not average value.");
    }
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape, viskores::IdComponent numPoints) const
  {
    viskores::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      FieldType value = TestValue(pointIndex + 1, FieldType());
      fieldValues.Append(value);
    }

    this->DoTestWithField(shape, fieldValues);

    viskores::cont::ArrayHandle<FieldType> fieldArray;
    fieldArray.Allocate(41);
    SetPortal(fieldArray.WritePortal());

    viskores::VecVariable<viskores::Id, MAX_POINTS> pointIndices;
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      viskores::Id globalIndex = (7 + (13 * pointIndex)) % 41;
      pointIndices.Append(globalIndex);
    }

    this->DoTestWithIndices(shape, pointIndices, fieldArray.ReadPortal());
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

    for (viskores::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(CellShapeTag(), numPoints);
    }

    viskores::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (viskores::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(genericShape, numPoints);
    }
  }
};

void TestInterpolate()
{
  std::cout << "======== Float32 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<viskores::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<viskores::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<viskores::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  viskores::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<viskores::Vec3f_64>());

  TestInterpolateFunctor<viskores::Vec3f> testFunctor;
  viskores::Vec3f origin = TestValue(0, viskores::Vec3f());
  viskores::Vec3f spacing = TestValue(1, viskores::Vec3f());
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  testFunctor.DoTestWithField(viskores::CellShapeTagLine(),
                              viskores::VecAxisAlignedPointCoordinates<1>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  testFunctor.DoTestWithField(viskores::CellShapeTagQuad(),
                              viskores::VecAxisAlignedPointCoordinates<2>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctor.DoTestWithField(viskores::CellShapeTagHexahedron(),
                              viskores::VecAxisAlignedPointCoordinates<3>(origin, spacing));
}

} // anonymous namespace

int UnitTestCellInterpolate(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestInterpolate, argc, argv);
}
