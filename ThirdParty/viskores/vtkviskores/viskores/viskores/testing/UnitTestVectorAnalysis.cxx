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

#include <viskores/VectorAnalysis.h>

#include <viskores/Types.h>
#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

#include <math.h>

namespace
{

namespace internal
{

template <typename VectorType>
typename viskores::VecTraits<VectorType>::ComponentType MyMag(const VectorType& vt)
{
  using Traits = viskores::VecTraits<VectorType>;
  double total = 0.0;
  for (viskores::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    total += Traits::GetComponent(vt, index) * Traits::GetComponent(vt, index);
  }
  return static_cast<typename Traits::ComponentType>(sqrt(total));
}

template <typename VectorType>
VectorType MyNormal(const VectorType& vt)
{
  using Traits = viskores::VecTraits<VectorType>;
  typename Traits::ComponentType mag = internal::MyMag(vt);
  VectorType temp = vt;
  for (viskores::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    Traits::SetComponent(temp, index, Traits::GetComponent(vt, index) / mag);
  }
  return temp;
}

template <typename T, typename W>
T MyLerp(const T& a, const T& b, const W& w)
{
  return (W(1) - w) * a + w * b;
}
}

template <typename VectorType>
void TestVector(const VectorType& vector)
{
  using ComponentType = typename viskores::VecTraits<VectorType>::ComponentType;

  //to do have to implement a norm and normalized call to verify the math ones
  //against
  ComponentType magnitude = viskores::Magnitude(vector);
  ComponentType magnitudeCompare = internal::MyMag(vector);
  VISKORES_TEST_ASSERT(test_equal(magnitude, magnitudeCompare), "Magnitude failed test.");

  ComponentType magnitudeSquared = viskores::MagnitudeSquared(vector);
  VISKORES_TEST_ASSERT(test_equal(magnitude * magnitude, magnitudeSquared),
                       "Magnitude squared test failed.");

  if (magnitudeSquared > 0)
  {
    ComponentType rmagnitude = viskores::RMagnitude(vector);
    VISKORES_TEST_ASSERT(test_equal(1 / magnitude, rmagnitude), "Reciprical magnitude failed.");

    VISKORES_TEST_ASSERT(test_equal(viskores::Normal(vector), internal::MyNormal(vector)),
                         "Normalized vector failed test.");

    VectorType normalizedVector = vector;
    viskores::Normalize(normalizedVector);
    VISKORES_TEST_ASSERT(test_equal(normalizedVector, internal::MyNormal(vector)),
                         "Inplace Normalized vector failed test.");
  }
}

template <typename VectorType>
void TestLerp(const VectorType& a,
              const VectorType& b,
              const VectorType& w,
              const typename viskores::VecTraits<VectorType>::ComponentType& wS)
{
  VectorType viskoresLerp = viskores::Lerp(a, b, w);
  VectorType otherLerp = internal::MyLerp(a, b, w);
  VISKORES_TEST_ASSERT(test_equal(viskoresLerp, otherLerp),
                       "Vectors with Vector weight do not lerp() correctly");

  VectorType lhsS = internal::MyLerp(a, b, wS);
  VectorType rhsS = viskores::Lerp(a, b, wS);
  VISKORES_TEST_ASSERT(test_equal(lhsS, rhsS),
                       "Vectors with Scalar weight do not lerp() correctly");
}

template <typename T>
void TestCross(const viskores::Vec<T, 3>& x, const viskores::Vec<T, 3>& y)
{
  using Vec3 = viskores::Vec<T, 3>;
  Vec3 cross = viskores::Cross(x, y);

  // The cross product result should be perpendicular to input vectors.
  VISKORES_TEST_ASSERT(viskores::Abs(viskores::Dot(cross, x)) <
                         std::numeric_limits<T>::epsilon() * viskores::MagnitudeSquared(x),
                       "Cross product not perpendicular.");
  VISKORES_TEST_ASSERT(viskores::Abs(viskores::Dot(cross, y)) <
                         std::numeric_limits<T>::epsilon() * viskores::MagnitudeSquared(y),
                       "Cross product not perpendicular.");
  // The length of cross product should be the lengths of the input vectors
  // times the sin of the angle between them.
  T sinAngle = viskores::Magnitude(cross) * viskores::RMagnitude(x) * viskores::RMagnitude(y);

  // The dot product is likewise the lengths of the input vectors times the
  // cos of the angle between them.
  T cosAngle = viskores::Dot(x, y) * viskores::RMagnitude(x) * viskores::RMagnitude(y);

  // Test that these are the actual sin and cos of the same angle with a
  // basic trigonometric identity.
  VISKORES_TEST_ASSERT(test_equal(sinAngle * sinAngle + cosAngle * cosAngle, T(1.0)),
                       "Bad cross product length.");

  // Test finding the normal to a triangle (similar to cross product).
  Vec3 normal = viskores::TriangleNormal(x, y, Vec3(0, 0, 0));
  VISKORES_TEST_ASSERT(viskores::Abs(viskores::Dot(normal, x - y)) <
                         std::numeric_limits<T>::epsilon() * viskores::MagnitudeSquared(x),
                       "Triangle normal is not really normal.");
}

template <typename VectorBasisType>
void TestOrthonormalize(const VectorBasisType& inputs, int expectedRank)
{
  VectorBasisType outputs;
  int actualRank = viskores::Orthonormalize(inputs, outputs);
  VISKORES_TEST_ASSERT(test_equal(actualRank, expectedRank), "Orthonormalized rank is unexpected.");
}

struct TestLinearFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    using Traits = viskores::VecTraits<T>;
    const viskores::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;
    using ComponentType = typename Traits::ComponentType;

    T zeroVector = T(ComponentType(0));
    T normalizedVector = T(viskores::RSqrt(ComponentType(NUM_COMPONENTS)));
    T posVec = TestValue(1, T());
    T negVec = -TestValue(2, T());

    TestVector(zeroVector);
    TestVector(normalizedVector);
    TestVector(posVec);
    TestVector(negVec);

    T weight(ComponentType(0.5));
    ComponentType weightS(0.5);
    TestLerp(zeroVector, normalizedVector, weight, weightS);
    TestLerp(zeroVector, posVec, weight, weightS);
    TestLerp(zeroVector, negVec, weight, weightS);

    TestLerp(normalizedVector, zeroVector, weight, weightS);
    TestLerp(normalizedVector, posVec, weight, weightS);
    TestLerp(normalizedVector, negVec, weight, weightS);

    TestLerp(posVec, zeroVector, weight, weightS);
    TestLerp(posVec, normalizedVector, weight, weightS);
    TestLerp(posVec, negVec, weight, weightS);

    TestLerp(negVec, zeroVector, weight, weightS);
    TestLerp(negVec, normalizedVector, weight, weightS);
    TestLerp(negVec, posVec, weight, weightS);
  }
};

struct TestCrossFunctor
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    TestCross(VectorType(1.0f, 0.0f, 0.0f), VectorType(0.0f, 1.0f, 0.0f));
    TestCross(VectorType(1.0f, 2.0f, 3.0f), VectorType(-3.0f, -1.0f, 1.0f));
    TestCross(VectorType(0.0f, 0.0f, 1.0f), VectorType(0.001f, 0.01f, 2.0f));
    // Example from: https://pharr.org/matt/blog/2019/11/03/difference-of-floats.html
    TestCross(VectorType(33962.035f, 41563.4f, 7706.415f),
              VectorType(-24871.969f, -30438.8f, -5643.727f));
  }
};

struct TestVectorFunctor
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    constexpr int NUM_COMPONENTS = VectorType::NUM_COMPONENTS;
    using Traits = viskores::VecTraits<VectorType>;
    using ComponentType = typename Traits::ComponentType;
    viskores::Vec<VectorType, NUM_COMPONENTS> basis;
    VectorType normalizedVector = VectorType(viskores::RSqrt(ComponentType(NUM_COMPONENTS)));
    VectorType zeroVector = VectorType(ComponentType(0));
    // Test with a degenerate set of inputs:
    basis[0] = zeroVector;
    basis[1] = normalizedVector;
    for (int ii = 2; ii < NUM_COMPONENTS; ++ii)
    {
      basis[ii] = zeroVector;
    }
    TestOrthonormalize(basis, 1);
    // Now test with a valid set of inputs:
    for (int ii = 0; ii < NUM_COMPONENTS; ++ii)
    {
      for (int jj = 0; jj < NUM_COMPONENTS; ++jj)
      {
        basis[ii][jj] =
          ComponentType(jj == ii ? 1.0 : 0.0) + ComponentType(0.05) * ComponentType(jj);
      }
    }
    TestOrthonormalize(basis, NUM_COMPONENTS);
  }
};

void TestVectorAnalysis()
{
  viskores::testing::Testing::TryTypes(TestLinearFunctor(), viskores::TypeListField());
  viskores::testing::Testing::TryTypes(TestCrossFunctor(), viskores::TypeListFieldVec3());
  viskores::testing::Testing::TryTypes(TestVectorFunctor(), viskores::TypeListFloatVec());
}

} // anonymous namespace

int UnitTestVectorAnalysis(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestVectorAnalysis, argc, argv);
}
