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

#include <viskores/Types.h>

#include <viskoresstd/is_trivial.h>

#include <viskores/testing/Testing.h>

namespace
{

void CheckTypeSizes()
{
  std::cout << "Checking sizes of base types." << std::endl;
  VISKORES_TEST_ASSERT(sizeof(viskores::Int8) == 1, "Int8 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::UInt8) == 1, "UInt8 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::Int16) == 2, "Int16 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::UInt16) == 2, "UInt16 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::Int32) == 4, "Int32 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::UInt32) == 4, "UInt32 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::Int64) == 8, "Int64 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::UInt64) == 8, "UInt64 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::Float32) == 4, "Float32 wrong size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::Float64) == 8, "Float64 wrong size.");
}

// This part of the test has to be broken out of GeneralVecTypeTest because
// the negate operation is only supported on vectors of signed types.
template <typename ComponentType, viskores::IdComponent Size>
void DoGeneralVecTypeTestNegate(const viskores::Vec<ComponentType, Size>&)
{
  using VectorType = viskores::Vec<ComponentType, Size>;
  for (viskores::Id valueIndex = 0; valueIndex < 10; valueIndex++)
  {
    VectorType original = TestValue(valueIndex, VectorType());
    VectorType negative = -original;

    for (viskores::IdComponent componentIndex = 0; componentIndex < Size; componentIndex++)
    {
      VISKORES_TEST_ASSERT(test_equal(-(original[componentIndex]), negative[componentIndex]),
                           "Vec did not negate correctly.");
    }

    VISKORES_TEST_ASSERT(test_equal(original, -negative), "Double Vec negative is not positive.");
  }
}

template <typename ComponentType, viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<ComponentType, Size>&)
{
  // Do not test the negate operator unless it is a negatable type.
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Int8, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Int16, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Int32, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Int64, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Float32, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

template <viskores::IdComponent Size>
void GeneralVecTypeTestNegate(const viskores::Vec<viskores::Float64, Size>& x)
{
  DoGeneralVecTypeTestNegate(x);
}

//general type test for VecC
template <typename ComponentType, viskores::IdComponent Size>
void GeneralVecCTypeTest(const viskores::Vec<ComponentType, Size>&)
{
  std::cout << "Checking VecC functionality" << std::endl;

  using T = viskores::VecC<ComponentType>;
  using VecT = viskores::Vec<ComponentType, Size>;

  //grab the number of elements of T
  VecT aSrc, bSrc, cSrc;
  T a(aSrc), b(bSrc), c(cSrc);

  VISKORES_TEST_ASSERT(a.GetNumberOfComponents() == Size,
                       "GetNumberOfComponents returns wrong size.");

  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    a[i] = ComponentType((i + 1) * 2);
    b[i] = ComponentType(i + 1);
  }

  c = a;
  VISKORES_TEST_ASSERT(test_equal(a, c), "Copy does not work.");

  //verify prefix and postfix increment and decrement
  ++c[Size - 1];
  c[Size - 1]++;
  VISKORES_TEST_ASSERT(test_equal(c[Size - 1], a[Size - 1] + 2), "Bad increment on component.");
  --c[Size - 1];
  c[Size - 1]--;
  VISKORES_TEST_ASSERT(test_equal(c[Size - 1], a[Size - 1]), "Bad decrement on component.");

  c = a;
  c += b;
  VISKORES_TEST_ASSERT(test_equal(c, aSrc + bSrc), "Bad +=");
  c -= b;
  VISKORES_TEST_ASSERT(test_equal(c, a), "Bad -=");
  c *= b;
  VISKORES_TEST_ASSERT(test_equal(c, aSrc * bSrc), "Bad *=");
  c /= b;
  VISKORES_TEST_ASSERT(test_equal(c, a), "Bad /=");

  //make c nearly alike a to verify == and != are correct.
  c = a;
  c[Size - 1] = ComponentType(c[Size - 1] - 1);

  VecT correct_plus;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_plus[i] = ComponentType(a[i] + b[i]);
  }
  VecT plus = a + bSrc;
  VISKORES_TEST_ASSERT(test_equal(plus, correct_plus), "Tuples not added correctly.");
  plus = aSrc + b;
  VISKORES_TEST_ASSERT(test_equal(plus, correct_plus), "Tuples not added correctly.");

  VecT correct_minus;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_minus[i] = ComponentType(a[i] - b[i]);
  }
  VecT minus = a - bSrc;
  VISKORES_TEST_ASSERT(test_equal(minus, correct_minus), "Tuples not subtracted correctly.");
  minus = aSrc - b;
  VISKORES_TEST_ASSERT(test_equal(minus, correct_minus), "Tuples not subtracted correctly.");

  VecT correct_mult;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_mult[i] = ComponentType(a[i] * b[i]);
  }
  VecT mult = a * bSrc;
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult), "Tuples not multiplied correctly.");
  mult = aSrc * b;
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult), "Tuples not multiplied correctly.");

  VecT correct_div;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_div[i] = ComponentType(a[i] / b[i]);
  }
  VecT div = a / bSrc;
  VISKORES_TEST_ASSERT(test_equal(div, correct_div), "Tuples not divided correctly.");
  div = aSrc / b;
  VISKORES_TEST_ASSERT(test_equal(div, correct_div), "Tuples not divided correctly.");

  ComponentType d = static_cast<ComponentType>(viskores::Dot(a, b));
  ComponentType correct_d = 0;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_d = ComponentType(correct_d + a[i] * b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(d, correct_d), "Dot(Tuple) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");
}

//general type test for VecC
template <typename ComponentType, viskores::IdComponent Size>
void GeneralVecCConstTypeTest(const viskores::Vec<ComponentType, Size>&)
{
  std::cout << "Checking VecCConst functionality" << std::endl;

  using T = viskores::VecCConst<ComponentType>;
  using VecT = viskores::Vec<ComponentType, Size>;

  //grab the number of elements of T
  VecT aSrc, bSrc, cSrc;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    aSrc[i] = ComponentType((i + 1) * 2);
    bSrc[i] = ComponentType(i + 1);
  }
  cSrc = aSrc;

  T a(aSrc), b(bSrc), c(cSrc);

  VISKORES_TEST_ASSERT(a.GetNumberOfComponents() == Size,
                       "GetNumberOfComponents returns wrong size.");

  VISKORES_TEST_ASSERT(test_equal(a, c), "Comparison not working.");

  //make c nearly alike a to verify == and != are correct.
  cSrc = aSrc;
  cSrc[Size - 1] = ComponentType(cSrc[Size - 1] - 1);

  VecT correct_plus;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_plus[i] = ComponentType(a[i] + b[i]);
  }
  VecT plus = a + bSrc;
  VISKORES_TEST_ASSERT(test_equal(plus, correct_plus), "Tuples not added correctly.");
  plus = aSrc + b;
  VISKORES_TEST_ASSERT(test_equal(plus, correct_plus), "Tuples not added correctly.");

  VecT correct_minus;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_minus[i] = ComponentType(a[i] - b[i]);
  }
  VecT minus = a - bSrc;
  VISKORES_TEST_ASSERT(test_equal(minus, correct_minus), "Tuples not subtracted correctly.");
  minus = aSrc - b;
  VISKORES_TEST_ASSERT(test_equal(minus, correct_minus), "Tuples not subtracted correctly.");

  VecT correct_mult;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_mult[i] = ComponentType(a[i] * b[i]);
  }
  VecT mult = a * bSrc;
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult), "Tuples not multiplied correctly.");
  mult = aSrc * b;
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult), "Tuples not multiplied correctly.");

  VecT correct_div;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_div[i] = ComponentType(a[i] / b[i]);
  }
  VecT div = a / bSrc;
  VISKORES_TEST_ASSERT(test_equal(div, correct_div), "Tuples not divided correctly.");
  div = aSrc / b;
  VISKORES_TEST_ASSERT(test_equal(div, correct_div), "Tuples not divided correctly.");

  ComponentType d = static_cast<ComponentType>(viskores::Dot(a, b));
  ComponentType correct_d = 0;
  for (viskores::IdComponent i = 0; i < Size; ++i)
  {
    correct_d = ComponentType(correct_d + a[i] * b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(d, correct_d), "Dot(Tuple) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");
}

//general type test for Vec
template <typename ComponentType, viskores::IdComponent Size>
void GeneralVecTypeTest(const viskores::Vec<ComponentType, Size>&)
{
  std::cout << "Checking general Vec functionality." << std::endl;

  using T = viskores::Vec<ComponentType, Size>;

  // Vector types should preserve the trivial properties of their components.
  // This insures that algorithms like std::copy will optimize fully.
  VISKORES_TEST_ASSERT(viskoresstd::is_trivial<ComponentType>::value ==
                         viskoresstd::is_trivial<T>::value,
                       "VectorType's triviality differs from ComponentType.");

  VISKORES_TEST_ASSERT(T::NUM_COMPONENTS == Size, "NUM_COMPONENTS is wrong size.");

  //grab the number of elements of T
  T a, b, c;
  ComponentType s(5);

  VISKORES_TEST_ASSERT(a.GetNumberOfComponents() == Size,
                       "GetNumberOfComponents returns wrong size.");

  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    a[i] = ComponentType((i + 1) * 2);
    b[i] = ComponentType(i + 1);
  }

  a.CopyInto(c);
  VISKORES_TEST_ASSERT(test_equal(a, c), "CopyInto does not work.");

  //verify prefix and postfix increment and decrement
  ++c[T::NUM_COMPONENTS - 1];
  c[T::NUM_COMPONENTS - 1]++;
  VISKORES_TEST_ASSERT(test_equal(c[T::NUM_COMPONENTS - 1], a[T::NUM_COMPONENTS - 1] + 2),
                       "Bad increment on component.");
  --c[T::NUM_COMPONENTS - 1];
  c[T::NUM_COMPONENTS - 1]--;
  VISKORES_TEST_ASSERT(test_equal(c[T::NUM_COMPONENTS - 1], a[T::NUM_COMPONENTS - 1]),
                       "Bad decrement on component.");

  //make c nearly like a to verify == and != are correct.
  c[T::NUM_COMPONENTS - 1] = ComponentType(c[T::NUM_COMPONENTS - 1] - 1);

  T plus = a + b;
  T correct_plus;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_plus[i] = ComponentType(a[i] + b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(plus, correct_plus), "Tuples not added correctly.");

  T minus = a - b;
  T correct_minus;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_minus[i] = ComponentType(a[i] - b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(minus, correct_minus), "Tuples not subtracted correctly.");

  T mult = a * b;
  T correct_mult;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_mult[i] = ComponentType(a[i] * b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult), "Tuples not multiplied correctly.");

  T div = a / b;
  T correct_div;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_div[i] = ComponentType(a[i] / b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(div, correct_div), "Tuples not divided correctly.");

  mult = s * a;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_mult[i] = ComponentType(s * a[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult),
                       "Scalar and Tuple did not multiply correctly.");

  mult = a * s;
  VISKORES_TEST_ASSERT(test_equal(mult, correct_mult),
                       "Tuple and Scalar to not multiply correctly.");

  div = a / ComponentType(2);
  VISKORES_TEST_ASSERT(test_equal(div, b), "Tuple does not divide by Scalar correctly.");

  ComponentType d = static_cast<ComponentType>(viskores::Dot(a, b));
  ComponentType correct_d = 0;
  for (viskores::IdComponent i = 0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_d = ComponentType(correct_d + a[i] * b[i]);
  }
  VISKORES_TEST_ASSERT(test_equal(d, correct_d), "Dot(Tuple) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");

  GeneralVecTypeTestNegate(T());
  GeneralVecCTypeTest(T());
  GeneralVecCConstTypeTest(T());
}

template <typename ComponentType, viskores::IdComponent Size>
void TypeTest(const viskores::Vec<ComponentType, Size>&)
{
  GeneralVecTypeTest(viskores::Vec<ComponentType, Size>());
}

template <typename Scalar>
void TypeTest(const viskores::Vec<Scalar, 1>&)
{
  using Vector = viskores::Vec<Scalar, 1>;
  std::cout << "Checking constexpr construction for Vec1." << std::endl;

  constexpr Vector constExprVec1(Scalar(1));
  constexpr Vector constExprVec2 = { Scalar(1) };
  constexpr Vector madeVec = viskores::make_Vec(Scalar(1));
  VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec), "constexpr Vec1 failed equality test.");
  VISKORES_TEST_ASSERT(test_equal(constExprVec2, madeVec), "constexpr Vec1 failed equality test.");
}

template <typename Scalar>
void TypeTest(const viskores::Vec<Scalar, 2>&)
{
  using Vector = viskores::Vec<Scalar, 2>;

  GeneralVecTypeTest(Vector());

  Vector a{ 2, 4 };
  Vector b = { 1, 2 };
  Scalar s = 5;

  VISKORES_TEST_ASSERT(a == viskores::make_Vec(Scalar(2), Scalar(4)),
                       "make_Vec creates different object.");
  VISKORES_TEST_ASSERT((a == viskores::Vec<Scalar, 2>{ Scalar(2), Scalar(4) }),
                       "Construct with initializer list creates different object.");

  Vector plus = a + b;
  VISKORES_TEST_ASSERT(test_equal(plus, viskores::make_Vec(3, 6)), "Vectors do not add correctly.");

  Vector minus = a - b;
  VISKORES_TEST_ASSERT(test_equal(minus, viskores::make_Vec(1, 2)),
                       "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(2, 8)),
                       "Vectors to not multiply correctly.");

  Vector div = a / b;
  VISKORES_TEST_ASSERT(test_equal(div, viskores::make_Vec(2, 2)),
                       "Vectors to not divide correctly.");

  mult = s * a;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20)),
                       "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20)),
                       "Vector and scalar to not multiply correctly.");

  div = a / Scalar(2);
  VISKORES_TEST_ASSERT(test_equal(div, viskores::make_Vec(1, 2)),
                       "Vector does not divide by Scalar correctly.");

  Scalar d = static_cast<Scalar>(viskores::Dot(a, b));
  VISKORES_TEST_ASSERT(test_equal(d, Scalar(10)), "Dot(Vector2) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const Vector c(2, 3);
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");

  std::cout << "Checking constexpr construction for Vec2." << std::endl;
  constexpr Vector constExprVec1(Scalar(1), Scalar(2));
  constexpr Vector constExprVec2 = { Scalar(1), Scalar(2) };
  constexpr Vector madeVec = viskores::make_Vec(Scalar(1), Scalar(2));
  VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec), "constexpr Vec2 failed equality test.");
  VISKORES_TEST_ASSERT(test_equal(constExprVec2, madeVec), "constexpr Vec2 failed equality test.");

  // Check fill constructor.
  Vector fillVec1 = { Scalar(8) };
  Vector fillVec2(Scalar(8), Scalar(8));
  VISKORES_TEST_ASSERT(test_equal(fillVec1, fillVec2), "fill ctor Vec2 failed equality test.");
}

template <typename Scalar>
void TypeTest(const viskores::Vec<Scalar, 3>&)
{
  using Vector = viskores::Vec<Scalar, 3>;

  GeneralVecTypeTest(Vector());

  Vector a = { 2, 4, 6 };
  Vector b{ 1, 2, 3 };
  Scalar s = 5;

  VISKORES_TEST_ASSERT(a == viskores::make_Vec(Scalar(2), Scalar(4), Scalar(6)),
                       "make_Vec creates different object.");
  VISKORES_TEST_ASSERT((a == viskores::Vec<Scalar, 3>{ Scalar(2), Scalar(4), Scalar(6) }),
                       "Construct with initializer list creates different object.");

  Vector plus = a + b;
  VISKORES_TEST_ASSERT(test_equal(plus, viskores::make_Vec(3, 6, 9)),
                       "Vectors do not add correctly.");

  Vector minus = a - b;
  VISKORES_TEST_ASSERT(test_equal(minus, viskores::make_Vec(1, 2, 3)),
                       "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(2, 8, 18)),
                       "Vectors to not multiply correctly.");

  Vector div = a / b;
  VISKORES_TEST_ASSERT(test_equal(div, viskores::make_Vec(2, 2, 2)),
                       "Vectors to not divide correctly.");

  mult = s * a;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20, 30)),
                       "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20, 30)),
                       "Vector and scalar to not multiply correctly.");

  div = a / Scalar(2);
  VISKORES_TEST_ASSERT(test_equal(div, b), "Vector does not divide by Scalar correctly.");

  Scalar d = static_cast<Scalar>(viskores::Dot(a, b));
  VISKORES_TEST_ASSERT(test_equal(d, Scalar(28)), "Dot(Vector3) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const Vector c(2, 4, 5);
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");

  std::cout << "Checking constexpr construction for Vec3." << std::endl;
  constexpr Vector constExprVec1(Scalar(1), Scalar(2), Scalar(3));
  constexpr Vector constExprVec2 = { Scalar(1), Scalar(2), Scalar(3) };
  constexpr Vector madeVec = viskores::make_Vec(Scalar(1), Scalar(2), Scalar(3));
  VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec), "constexpr Vec3 failed equality test.");
  VISKORES_TEST_ASSERT(test_equal(constExprVec2, madeVec), "constexpr Vec3 failed equality test.");

  // Check fill constructor.
  Vector fillVec1 = { Scalar(8) };
  Vector fillVec2(Scalar(8), Scalar(8), Scalar(8));
  VISKORES_TEST_ASSERT(test_equal(fillVec1, fillVec2), "fill ctor Vec3 failed equality test.");
}

template <typename Scalar>
void TypeTest(const viskores::Vec<Scalar, 4>&)
{
  using Vector = viskores::Vec<Scalar, 4>;

  GeneralVecTypeTest(Vector());

  Vector a{ 2, 4, 6, 8 };
  Vector b = { 1, 2, 3, 4 };
  Scalar s = 5;

  VISKORES_TEST_ASSERT(a == viskores::make_Vec(Scalar(2), Scalar(4), Scalar(6), Scalar(8)),
                       "make_Vec creates different object.");
  VISKORES_TEST_ASSERT(
    (a == viskores::Vec<Scalar, 4>{ Scalar(2), Scalar(4), Scalar(6), Scalar(8) }),
    "Construct with initializer list creates different object.");

  Vector plus = a + b;
  VISKORES_TEST_ASSERT(test_equal(plus, viskores::make_Vec(3, 6, 9, 12)),
                       "Vectors do not add correctly.");

  Vector minus = a - b;
  VISKORES_TEST_ASSERT(test_equal(minus, viskores::make_Vec(1, 2, 3, 4)),
                       "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(2, 8, 18, 32)),
                       "Vectors to not multiply correctly.");

  Vector div = a / b;
  VISKORES_TEST_ASSERT(test_equal(div, viskores::make_Vec(2, 2, 2, 2)),
                       "Vectors to not divide correctly.");

  mult = s * a;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20, 30, 40)),
                       "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VISKORES_TEST_ASSERT(test_equal(mult, viskores::make_Vec(10, 20, 30, 40)),
                       "Vector and scalar to not multiply correctly.");

  div = a / Scalar(2);
  VISKORES_TEST_ASSERT(test_equal(div, b), "Vector does not divide by Scalar correctly.");

  Scalar d = static_cast<Scalar>(viskores::Dot(a, b));
  VISKORES_TEST_ASSERT(test_equal(d, Scalar(60)), "Dot(Vector4) wrong");

  VISKORES_TEST_ASSERT(!(a < b), "operator< wrong");
  VISKORES_TEST_ASSERT((b < a), "operator< wrong");
  VISKORES_TEST_ASSERT(!(a < a), "operator< wrong");
  VISKORES_TEST_ASSERT((a < plus), "operator< wrong");
  VISKORES_TEST_ASSERT((minus < plus), "operator< wrong");

  VISKORES_TEST_ASSERT(!(a == b), "operator== wrong");
  VISKORES_TEST_ASSERT((a == a), "operator== wrong");

  VISKORES_TEST_ASSERT((a != b), "operator!= wrong");
  VISKORES_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const Vector c(2, 4, 6, 7);
  VISKORES_TEST_ASSERT((c < a), "operator< wrong");

  VISKORES_TEST_ASSERT(!(c == a), "operator == wrong");
  VISKORES_TEST_ASSERT(!(a == c), "operator == wrong");

  VISKORES_TEST_ASSERT((c != a), "operator != wrong");
  VISKORES_TEST_ASSERT((a != c), "operator != wrong");

  std::cout << "Checking constexpr construction for Vec4." << std::endl;
  constexpr Vector constExprVec1(Scalar(1), Scalar(2), Scalar(3), Scalar(4));
  constexpr Vector constExprVec2 = { Scalar(1), Scalar(2), Scalar(3), Scalar(4) };
  constexpr Vector madeVec = viskores::make_Vec(Scalar(1), Scalar(2), Scalar(3), Scalar(4));
  VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec), "constexpr Vec4 failed equality test.");
  VISKORES_TEST_ASSERT(test_equal(constExprVec2, madeVec), "constexpr Vec4 failed equality test.");

  // Check fill constructor.
  Vector fillVec1 = { Scalar(8) };
  Vector fillVec2(Scalar(8), Scalar(8), Scalar(8), Scalar(8));
  VISKORES_TEST_ASSERT(test_equal(fillVec1, fillVec2), "fill ctor Vec4 failed equality test.");

  Scalar values[4] = { Scalar(1), Scalar(1), Scalar(1), Scalar(1) };
  Vector lvalVec1 = viskores::make_Vec(values[0], values[1], values[2], values[3]);
  Vector lvalVec2 = Vector(values[0], values[1], values[2], values[3]);
  VISKORES_TEST_ASSERT(test_equal(lvalVec1, lvalVec2), "lvalue ctor Vec4 failed equality test.");
}

template <typename Scalar>
void TypeTest(const viskores::Vec<Scalar, 6>&)
{
  using Vector = viskores::Vec<Scalar, 6>;
  std::cout << "Checking constexpr construction for Vec6." << std::endl;
  constexpr Vector constExprVec1(Scalar(1), Scalar(2), Scalar(3), Scalar(4), Scalar(5), Scalar(6));
  Vector braceVec = { Scalar(1), Scalar(2), Scalar(3), Scalar(4), Scalar(5), Scalar(6) };
  constexpr Vector madeVec =
    viskores::make_Vec(Scalar(1), Scalar(2), Scalar(3), Scalar(4), Scalar(5), Scalar(6));
  VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec), "constexpr Vec6 failed equality test.");
  VISKORES_TEST_ASSERT(test_equal(braceVec, madeVec), "constexpr Vec6 failed equality test.");

  // Check fill constructor.
  Vector fillVec1 = { Scalar(8) };
  Vector fillVec2 = Vector(Scalar(8), Scalar(8), Scalar(8), Scalar(8), Scalar(8), Scalar(8));
  VISKORES_TEST_ASSERT(test_equal(fillVec1, fillVec2), "fill ctor Vec6 failed equality test.");
}

template <typename Scalar>
void TypeTest(Scalar)
{
  std::cout << "Test functionality of scalar type." << std::endl;

  Scalar a = 4;
  Scalar b = 2;

  Scalar plus = Scalar(a + b);
  if (plus != 6)
  {
    VISKORES_TEST_FAIL("Scalars do not add correctly.");
  }

  Scalar minus = Scalar(a - b);
  if (minus != 2)
  {
    VISKORES_TEST_FAIL("Scalars to not subtract correctly.");
  }

  Scalar mult = Scalar(a * b);
  if (mult != 8)
  {
    VISKORES_TEST_FAIL("Scalars to not multiply correctly.");
  }

  Scalar div = Scalar(a / b);
  if (div != 2)
  {
    VISKORES_TEST_FAIL("Scalars to not divide correctly.");
  }

  if (a == b)
  {
    VISKORES_TEST_FAIL("operator== wrong");
  }
  if (!(a != b))
  {
    VISKORES_TEST_FAIL("operator!= wrong");
  }

  if (viskores::Dot(a, b) != 8)
  {
    VISKORES_TEST_FAIL("Dot(Scalar) wrong");
  }

  //verify we don't roll over
  Scalar c = 128;
  Scalar d = 32;
  auto r = viskores::Dot(c, d);
  VISKORES_TEST_ASSERT((sizeof(r) >= sizeof(int)),
                       "Dot(Scalar) didn't promote smaller than 32bit types");
  if (r != 4096)
  {
    VISKORES_TEST_FAIL("Dot(Scalar) wrong");
  }
}

template <typename Scalar>
void TypeTest(viskores::Vec<viskores::Vec<Scalar, 2>, 3>)
{
  using Vector = viskores::Vec<viskores::Vec<Scalar, 2>, 3>;

  {
    Vector vec = { { 0, 1 }, { 2, 3 }, { 4, 5 } };
    std::cout << "Initialize completely " << vec << std::endl;
    VISKORES_TEST_ASSERT(test_equal(vec[0][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[0][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][0], 2), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][1], 3), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][0], 4), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][1], 5), "Vec of vec initializer list wrong.");
  }

  {
    Vector vec = { viskores::make_Vec(Scalar(0), Scalar(1)) };
    std::cout << "Initialize inner " << vec << std::endl;
    VISKORES_TEST_ASSERT(test_equal(vec[0][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[0][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][1], 1), "Vec of vec initializer list wrong.");
  }

  {
    Vector vec = { { 0, 1 } };
    std::cout << "Initialize inner " << vec << std::endl;
    VISKORES_TEST_ASSERT(test_equal(vec[0][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[0][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][1], 1), "Vec of vec initializer list wrong.");
  }

  {
    Vector vec = { { 0 }, { 1 }, { 2 } };
    std::cout << "Initialize outer " << vec << std::endl;
    VISKORES_TEST_ASSERT(test_equal(vec[0][0], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[0][1], 0), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][0], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[1][1], 1), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][0], 2), "Vec of vec initializer list wrong.");
    VISKORES_TEST_ASSERT(test_equal(vec[2][1], 2), "Vec of vec initializer list wrong.");
  }

  {
    // Both of these constructors are disallowed.
    //Vector vec1 = { 0, 1, 2 };
    //Vector vec2 = { 0, 1 };
  }

  {
    std::cout << "Checking constexpr construction for Vec3<Vec2>." << std::endl;
    constexpr Vector constExprVec1(viskores::Vec<Scalar, 2>(1, 2),
                                   viskores::Vec<Scalar, 2>(1, 2),
                                   viskores::Vec<Scalar, 2>(1, 2));
    constexpr Vector constExprVec2 = { { 1, 2 }, { 1, 2 }, { 1, 2 } };
    constexpr Vector madeVec = viskores::make_Vec(viskores::make_Vec(Scalar(1), Scalar(2)),
                                                  viskores::make_Vec(Scalar(1), Scalar(2)),
                                                  viskores::make_Vec(Scalar(1), Scalar(2)));

    VISKORES_TEST_ASSERT(test_equal(constExprVec1, madeVec),
                         "constexpr Vec3<Vec2> failed equality test.");
    VISKORES_TEST_ASSERT(test_equal(constExprVec2, madeVec),
                         "constexpr Vec3<Vec2> failed equality test.");

    // Check fill constructor.
    Vector fillVec1 = { { Scalar(1), Scalar(2) } };
    Vector fillVec2(viskores::Vec<Scalar, 2>(1, 2),
                    viskores::Vec<Scalar, 2>(1, 2),
                    viskores::Vec<Scalar, 2>(1, 2));
    VISKORES_TEST_ASSERT(test_equal(fillVec1, fillVec2),
                         "fill ctor Vec3ofVec2 failed equality test.");
  }
}

template <typename Scalar>
void TypeTest(viskores::Vec<viskores::Vec<Scalar, 2>, 5>)
{
  using Vector = viskores::Vec<viskores::Vec<Scalar, 2>, 5>;
  Vector braceVec = { { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } };
  constexpr Vector constExprVec = viskores::make_Vec(viskores::make_Vec(Scalar(1), Scalar(1)),
                                                     viskores::make_Vec(Scalar(2), Scalar(2)),
                                                     viskores::make_Vec(Scalar(3), Scalar(3)),
                                                     viskores::make_Vec(Scalar(4), Scalar(4)),
                                                     viskores::make_Vec(Scalar(5), Scalar(5)));
  VISKORES_TEST_ASSERT(test_equal(constExprVec, braceVec), "Vec5<Vec2> failed equality test.");
}

struct TypeTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    TypeTest(T());
  }
};

using TypesToTest = viskores::ListAppend<viskores::testing::Testing::TypeListExemplarTypes,
                                         viskores::List<viskores::Vec<viskores::FloatDefault, 6>,
                                                        viskores::Id4,
                                                        viskores::Vec<unsigned char, 4>,
                                                        viskores::Vec<viskores::Id, 1>,
                                                        viskores::Id2,
                                                        viskores::Vec<viskores::Float64, 1>,
                                                        viskores::Vec<viskores::Id2, 3>,
                                                        viskores::Vec<viskores::Vec2f_32, 3>,
                                                        viskores::Vec<viskores::Vec2f_32, 5>>>;

void TestTypes()
{
  CheckTypeSizes();

  viskores::testing::Testing::TryTypes(TypeTestFunctor(), TypesToTest());
}

} // anonymous namespace

int UnitTestTypes(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestTypes, argc, argv);
}
