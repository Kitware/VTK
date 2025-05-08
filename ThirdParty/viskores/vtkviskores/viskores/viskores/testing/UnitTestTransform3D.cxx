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

#include <viskores/Transform3D.h>

#include <viskores/testing/Testing.h>

#include <ctime>
#include <random>

namespace
{

std::mt19937 g_RandomGenerator;

template <typename T>
struct TransformTests
{
  std::uniform_real_distribution<T> RandomDistribution;
  TransformTests()
    : RandomDistribution(0.0f, 1.0f)
  {
  }

  T RandomNum() { return this->RandomDistribution(g_RandomGenerator); }

  using Vec = viskores::Vec<T, 3>;
  using Transform = viskores::Matrix<T, 4, 4>;

  Vec RandomVector()
  {
    Vec vec(this->RandomNum(), this->RandomNum(), this->RandomNum());
    return T(2) * vec - Vec(1);
  }

  void CheckTranslate()
  {
    std::cout << "--- Checking translate" << std::endl;

    Vec startPoint = this->RandomVector();
    std::cout << " Starting point: " << startPoint << std::endl;

    Vec translateAmount = this->RandomVector();
    std::cout << " Translation amount: " << translateAmount << std::endl;

    Transform translate = viskores::Transform3DTranslate(translateAmount);

    Vec translated1 = viskores::Transform3DPoint(translate, startPoint);
    std::cout << " First translation: " << translated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(translated1, startPoint + translateAmount), "Bad translation.");

    Vec translated2 = viskores::Transform3DPoint(translate, translated1);
    std::cout << " Second translation: " << translated2 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(translated2, startPoint + T(2) * translateAmount),
                         "Bad translation.");

    // Vectors should be invariant to translation.
    translated1 = viskores::Transform3DVector(translate, startPoint);
    std::cout << " Translated vector: " << translated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(translated1, startPoint), "Bad translation.");
  }

  void CheckScale()
  {
    std::cout << "--- Checking scale" << std::endl;

    Vec startPoint = this->RandomVector();
    std::cout << " Starting point: " << startPoint << std::endl;

    Vec scaleAmount = this->RandomVector();
    std::cout << " Scale amount: " << scaleAmount << std::endl;

    Transform scale = viskores::Transform3DScale(scaleAmount);

    Vec scaled1 = viskores::Transform3DPoint(scale, startPoint);
    std::cout << " First scale: " << scaled1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(scaled1, startPoint * scaleAmount), "Bad scale.");

    Vec scaled2 = viskores::Transform3DPoint(scale, scaled1);
    std::cout << " Second scale: " << scaled2 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(scaled2, startPoint * scaleAmount * scaleAmount), "Bad scale.");

    // Vectors should scale the same as points.
    scaled1 = viskores::Transform3DVector(scale, startPoint);
    std::cout << " Scaled vector: " << scaled1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(scaled1, startPoint * scaleAmount), "Bad scale.");
  }

  void CheckRotate()
  {
    std::cout << "--- Checking rotate" << std::endl;

    Vec startPoint = this->RandomVector();
    std::cout << " Starting point: " << startPoint << std::endl;

    const T ninetyDegrees = T(90);

    std::cout << "--Rotate 90 degrees around X" << std::endl;
    Transform rotateX = viskores::Transform3DRotateX(ninetyDegrees);

    Vec rotated1 = viskores::Transform3DPoint(rotateX, startPoint);
    std::cout << " First rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(startPoint[0], -startPoint[2], startPoint[1])),
                         "Bad rotate.");

    Vec rotated2 = viskores::Transform3DPoint(rotateX, rotated1);
    std::cout << " Second rotate: " << rotated2 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated2, Vec(startPoint[0], -startPoint[1], -startPoint[2])),
                         "Bad rotate.");

    // Vectors should rotate the same as points.
    rotated1 = viskores::Transform3DVector(rotateX, startPoint);
    std::cout << " Vector rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(startPoint[0], -startPoint[2], startPoint[1])),
                         "Bad rotate.");

    std::cout << "--Rotate 90 degrees around Y" << std::endl;
    Transform rotateY = viskores::Transform3DRotateY(ninetyDegrees);

    rotated1 = viskores::Transform3DPoint(rotateY, startPoint);
    std::cout << " First rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(startPoint[2], startPoint[1], -startPoint[0])),
                         "Bad rotate.");

    rotated2 = viskores::Transform3DPoint(rotateY, rotated1);
    std::cout << " Second rotate: " << rotated2 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated2, Vec(-startPoint[0], startPoint[1], -startPoint[2])),
                         "Bad rotate.");

    // Vectors should rotate the same as points.
    rotated1 = viskores::Transform3DVector(rotateY, startPoint);
    std::cout << " Vector rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(startPoint[2], startPoint[1], -startPoint[0])),
                         "Bad rotate.");

    std::cout << "--Rotate 90 degrees around Z" << std::endl;
    Transform rotateZ = viskores::Transform3DRotateZ(ninetyDegrees);

    rotated1 = viskores::Transform3DPoint(rotateZ, startPoint);
    std::cout << " First rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(-startPoint[1], startPoint[0], startPoint[2])),
                         "Bad rotate.");

    rotated2 = viskores::Transform3DPoint(rotateZ, rotated1);
    std::cout << " Second rotate: " << rotated2 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated2, Vec(-startPoint[0], -startPoint[1], startPoint[2])),
                         "Bad rotate.");

    // Vectors should rotate the same as points.
    rotated1 = viskores::Transform3DVector(rotateZ, startPoint);
    std::cout << " Vector rotate: " << rotated1 << std::endl;
    VISKORES_TEST_ASSERT(test_equal(rotated1, Vec(-startPoint[1], startPoint[0], startPoint[2])),
                         "Bad rotate.");
  }

  void CheckPerspective()
  {
    std::cout << "--- Checking Perspective" << std::endl;

    Vec startPoint = this->RandomVector();
    std::cout << " Starting point: " << startPoint << std::endl;

    Transform perspective(0);
    perspective(0, 0) = 1;
    perspective(1, 1) = 1;
    perspective(2, 2) = 1;
    perspective(3, 2) = 1;

    Vec projected = viskores::Transform3DPointPerspective(perspective, startPoint);
    std::cout << " Projected: " << projected << std::endl;
    VISKORES_TEST_ASSERT(test_equal(projected, startPoint / startPoint[2]), "Bad perspective.");
  }
};

struct TryTransformsFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TransformTests<T> tests;
    tests.CheckTranslate();
    tests.CheckScale();
    tests.CheckRotate();
  }
};

void TestTransforms()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  viskores::testing::Testing::TryTypes(TryTransformsFunctor(), viskores::TypeListFieldScalar());
}

} // anonymous namespace

int UnitTestTransform3D(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestTransforms, argc, argv);
}
