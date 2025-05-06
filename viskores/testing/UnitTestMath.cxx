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
#include <viskores/Math.h>

#include <viskores/TypeList.h>
#include <viskores/VecTraits.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/Algorithm.h>

#include <viskores/cont/testing/Testing.h>

#include <limits>


//-----------------------------------------------------------------------------
namespace UnitTestMathNamespace
{

class Lists
{
public:
  static constexpr viskores::IdComponent NUM_NUMBERS = 5;

  VISKORES_EXEC_CONT viskores::Float64 NumberList(viskores::Int32 i) const
  {
    viskores::Float64 numberList[NUM_NUMBERS] = { 0.25, 0.5, 1.0, 2.0, 3.75 };
    return numberList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 AngleList(viskores::Int32 i) const
  {
    viskores::Float64 angleList[NUM_NUMBERS] = { 0.643501108793284, // angle for 3, 4, 5 triangle.
                                                 0.78539816339745,  // pi/4
                                                 0.5235987755983,   // pi/6
                                                 1.0471975511966,   // pi/3
                                                 0.0 };
    return angleList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 OppositeList(viskores::Int32 i) const
  {
    viskores::Float64 oppositeList[NUM_NUMBERS] = {
      3.0, 1.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 0.0
    };
    return oppositeList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 AdjacentList(viskores::Int32 i) const
  {
    viskores::Float64 adjacentList[NUM_NUMBERS] = {
      4.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 1.0, 1.0
    };
    return adjacentList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 HypotenuseList(viskores::Int32 i) const
  {
    viskores::Float64 hypotenuseList[NUM_NUMBERS] = {
      5.0, 1.414213562373095 /*sqrt(2)*/, 2.0, 2.0, 1.0
    };
    return hypotenuseList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 NumeratorList(viskores::Int32 i) const
  {
    viskores::Float64 numeratorList[NUM_NUMBERS] = { 6.5, 5.8, 9.3, 77.0, 0.1 };
    return numeratorList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 DenominatorList(viskores::Int32 i) const
  {
    viskores::Float64 denominatorList[NUM_NUMBERS] = { 2.3, 1.6, 3.1, 19.0, 0.4 };
    return denominatorList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 FModRemainderList(viskores::Int32 i) const
  {
    viskores::Float64 fModRemainderList[NUM_NUMBERS] = { 1.9, 1.0, 0.0, 1.0, 0.1 };
    return fModRemainderList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 RemainderList(viskores::Int32 i) const
  {
    viskores::Float64 remainderList[NUM_NUMBERS] = { -0.4, -0.6, 0.0, 1.0, 0.1 };
    return remainderList[i];
  }
  VISKORES_EXEC_CONT viskores::Int64 QuotientList(viskores::Int32 i) const
  {
    viskores::Int64 quotientList[NUM_NUMBERS] = { 3, 4, 3, 4, 0 };
    return quotientList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 XList(viskores::Int32 i) const
  {
    viskores::Float64 xList[NUM_NUMBERS] = { 4.6, 0.1, 73.4, 55.0, 3.75 };
    return xList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 FractionalList(viskores::Int32 i) const
  {
    viskores::Float64 fractionalList[NUM_NUMBERS] = { 0.6, 0.1, 0.4, 0.0, 0.75 };
    return fractionalList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 FloorList(viskores::Int32 i) const
  {
    viskores::Float64 floorList[NUM_NUMBERS] = { 4.0, 0.0, 73.0, 55.0, 3.0 };
    return floorList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 CeilList(viskores::Int32 i) const
  {
    viskores::Float64 ceilList[NUM_NUMBERS] = { 5.0, 1.0, 74.0, 55.0, 4.0 };
    return ceilList[i];
  }
  VISKORES_EXEC_CONT viskores::Float64 RoundList(viskores::Int32 i) const
  {
    viskores::Float64 roundList[NUM_NUMBERS] = { 5.0, 0.0, 73.0, 55.0, 4.0 };
    return roundList[i];
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct ScalarFieldTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void TestPi() const
  {
    //    std::cout << "Testing Pi" << std::endl;
    VISKORES_MATH_ASSERT(test_equal(viskores::Pi(), 3.14159265), "Pi not correct.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Pif(), 3.14159265f), "Pif not correct.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Pi<viskores::Float64>(), 3.14159265),
                         "Pi template function not correct.");
  }

  VISKORES_EXEC
  void TestArcTan2() const
  {
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(0.0), T(1.0)), T(0.0)), "ATan2 x+ axis.");
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(1.0), T(0.0)), T(0.5 * viskores::Pi())),
                         "ATan2 y+ axis.");
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(-1.0), T(0.0)), T(-0.5 * viskores::Pi())),
                         "ATan2 y- axis.");

    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(1.0), T(1.0)), T(0.25 * viskores::Pi())),
                         "ATan2 Quadrant 1");
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(1.0), T(-1.0)), T(0.75 * viskores::Pi())),
                         "ATan2 Quadrant 2");
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(-1.0), T(-1.0)), T(-0.75 * viskores::Pi())),
                         "ATan2 Quadrant 3");
    VISKORES_MATH_ASSERT(test_equal(viskores::ATan2(T(-1.0), T(1.0)), T(-0.25 * viskores::Pi())),
                         "ATan2 Quadrant 4");
  }

  VISKORES_EXEC
  void TestPow() const
  {
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(Lists{}.NumberList(index));
      T powx = viskores::Pow(x, static_cast<T>(2.0));
      T sqrx = x * x;
      VISKORES_MATH_ASSERT(test_equal(powx, sqrx), "Power gave wrong result.");
    }
  }

  VISKORES_EXEC
  void TestLog2() const
  {
    VISKORES_MATH_ASSERT(test_equal(viskores::Log2(T(0.25)), T(-2.0)), "Bad value from Log2");
    VISKORES_MATH_ASSERT(test_equal(viskores::Log2(viskores::Vec<T, 4>(0.5, 1.0, 2.0, 4.0)),
                                    viskores::Vec<T, 4>(-1.0, 0.0, 1.0, 2.0)),
                         "Bad value from Log2");
  }

  VISKORES_EXEC
  void TestNonFinites() const
  {
    T zero = 0.0;
    T finite = 1.0;
    T nan = viskores::Nan<T>();
    T inf = viskores::Infinity<T>();
    T neginf = viskores::NegativeInfinity<T>();
    T epsilon = viskores::Epsilon<T>();

    // General behavior.
    VISKORES_MATH_ASSERT(nan != viskores::Nan<T>(), "Nan not equal itself.");
    // Disabled because they can cause floating point exceptions
    //VISKORES_MATH_ASSERT(!(nan >= zero), "Nan not greater or less.");
    //VISKORES_MATH_ASSERT(!(nan <= zero), "Nan not greater or less.");
    //VISKORES_MATH_ASSERT(!(nan >= finite), "Nan not greater or less.");
    //VISKORES_MATH_ASSERT(!(nan <= finite), "Nan not greater or less.");

    VISKORES_MATH_ASSERT(neginf < inf, "Infinity big");
    VISKORES_MATH_ASSERT(zero < inf, "Infinity big");
    VISKORES_MATH_ASSERT(finite < inf, "Infinity big");
    VISKORES_MATH_ASSERT(zero > -inf, "-Infinity small");
    VISKORES_MATH_ASSERT(finite > -inf, "-Infinity small");
    VISKORES_MATH_ASSERT(zero > neginf, "-Infinity small");
    VISKORES_MATH_ASSERT(finite > neginf, "-Infinity small");

    VISKORES_MATH_ASSERT(zero < epsilon, "Negative epsilon");
    VISKORES_MATH_ASSERT(finite > epsilon, "Large epsilon");

    // Math check functions.
    VISKORES_MATH_ASSERT(!viskores::IsNan(zero), "Bad IsNan check.");
    VISKORES_MATH_ASSERT(!viskores::IsNan(finite), "Bad IsNan check.");
    VISKORES_MATH_ASSERT(viskores::IsNan(nan), "Bad IsNan check.");
    VISKORES_MATH_ASSERT(!viskores::IsNan(inf), "Bad IsNan check.");
    VISKORES_MATH_ASSERT(!viskores::IsNan(neginf), "Bad IsNan check.");
    VISKORES_MATH_ASSERT(!viskores::IsNan(epsilon), "Bad IsNan check.");

    VISKORES_MATH_ASSERT(!viskores::IsInf(zero), "Bad infinity check.");
    VISKORES_MATH_ASSERT(!viskores::IsInf(finite), "Bad infinity check.");
    VISKORES_MATH_ASSERT(!viskores::IsInf(nan), "Bad infinity check.");
    VISKORES_MATH_ASSERT(viskores::IsInf(inf), "Bad infinity check.");
    VISKORES_MATH_ASSERT(viskores::IsInf(neginf), "Bad infinity check.");
    VISKORES_MATH_ASSERT(!viskores::IsInf(epsilon), "Bad infinity check.");

    VISKORES_MATH_ASSERT(viskores::IsFinite(zero), "Bad finite check.");
    VISKORES_MATH_ASSERT(viskores::IsFinite(finite), "Bad finite check.");
    VISKORES_MATH_ASSERT(!viskores::IsFinite(nan), "Bad finite check.");
    VISKORES_MATH_ASSERT(!viskores::IsFinite(inf), "Bad finite check.");
    VISKORES_MATH_ASSERT(!viskores::IsFinite(neginf), "Bad finite check.");
    VISKORES_MATH_ASSERT(viskores::IsFinite(epsilon), "Bad finite check.");
  }

  VISKORES_EXEC
  void TestRemainders() const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T numerator = static_cast<T>(table.NumeratorList(index));
      T denominator = static_cast<T>(table.DenominatorList(index));
      T fmodremainder = static_cast<T>(table.FModRemainderList(index));
      T remainder = static_cast<T>(table.RemainderList(index));
      viskores::Int64 quotient = table.QuotientList(index);

      VISKORES_MATH_ASSERT(test_equal(viskores::FMod(numerator, denominator), fmodremainder),
                           "Bad FMod remainder.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Remainder(numerator, denominator), remainder),
                           "Bad remainder.");
      viskores::Int64 q;
      VISKORES_MATH_ASSERT(
        test_equal(viskores::RemainderQuotient(numerator, denominator, q), remainder),
        "Bad remainder-quotient remainder.");
      VISKORES_MATH_ASSERT(test_equal(q, quotient), "Bad reminder-quotient quotient.");
    }
  }

  VISKORES_EXEC
  void TestRound() const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(table.XList(index));
      T fractional = static_cast<T>(table.FractionalList(index));
      T floor = static_cast<T>(table.FloorList(index));
      T ceil = static_cast<T>(table.CeilList(index));
      T round = static_cast<T>(table.RoundList(index));

      T intPart;
      VISKORES_MATH_ASSERT(test_equal(viskores::ModF(x, intPart), fractional),
                           "ModF returned wrong fractional part.");
      VISKORES_MATH_ASSERT(test_equal(intPart, floor), "ModF returned wrong integral part.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Floor(x), floor), "Bad floor.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Ceil(x), ceil), "Bad ceil.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Round(x), round), "Bad round.");
    }
  }

  VISKORES_EXEC
  void TestIsNegative() const
  {
    T x = 0;
    VISKORES_MATH_ASSERT(viskores::SignBit(x) == 0, "SignBit wrong for 0.");
    VISKORES_MATH_ASSERT(!viskores::IsNegative(x), "IsNegative wrong for 0.");

    x = 20;
    VISKORES_MATH_ASSERT(viskores::SignBit(x) == 0, "SignBit wrong for 20.");
    VISKORES_MATH_ASSERT(!viskores::IsNegative(x), "IsNegative wrong for 20.");

    x = -20;
    VISKORES_MATH_ASSERT(viskores::SignBit(x) != 0, "SignBit wrong for -20.");
    VISKORES_MATH_ASSERT(viskores::IsNegative(x), "IsNegative wrong for -20.");

    x = 0.02f;
    VISKORES_MATH_ASSERT(viskores::SignBit(x) == 0, "SignBit wrong for 0.02.");
    VISKORES_MATH_ASSERT(!viskores::IsNegative(x), "IsNegative wrong for 0.02.");

    x = -0.02f;
    VISKORES_MATH_ASSERT(viskores::SignBit(x) != 0, "SignBit wrong for -0.02.");
    VISKORES_MATH_ASSERT(viskores::IsNegative(x), "IsNegative wrong for -0.02.");
  }

  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    this->TestPi();
    this->TestArcTan2();
    this->TestPow();
    this->TestLog2();
    this->TestNonFinites();
    this->TestRemainders();
    this->TestRound();
    this->TestIsNegative();
  }
};

struct TryScalarFieldTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(ScalarFieldTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename VectorType>
struct ScalarVectorFieldTests : public viskores::exec::FunctorBase
{
  using Traits = viskores::VecTraits<VectorType>;
  using ComponentType = typename Traits::ComponentType;
  enum
  {
    NUM_COMPONENTS = Traits::NUM_COMPONENTS
  };

  VISKORES_EXEC
  void TestTriangleTrig() const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType angle;
      VectorType opposite;
      VectorType adjacent;
      VectorType hypotenuse;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        Traits::SetComponent(angle,
                             componentIndex,
                             static_cast<ComponentType>(table.AngleList(componentIndex + index)));
        Traits::SetComponent(
          opposite,
          componentIndex,
          static_cast<ComponentType>(table.OppositeList(componentIndex + index)));
        Traits::SetComponent(
          adjacent,
          componentIndex,
          static_cast<ComponentType>(table.AdjacentList(componentIndex + index)));
        Traits::SetComponent(
          hypotenuse,
          componentIndex,
          static_cast<ComponentType>(table.HypotenuseList(componentIndex + index)));
      }

      VISKORES_MATH_ASSERT(test_equal(viskores::Sin(angle), opposite / hypotenuse),
                           "Sin failed test.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Cos(angle), adjacent / hypotenuse),
                           "Cos failed test.");
      VISKORES_MATH_ASSERT(test_equal(viskores::Tan(angle), opposite / adjacent),
                           "Tan failed test.");

      VISKORES_MATH_ASSERT(test_equal(viskores::ASin(opposite / hypotenuse), angle),
                           "Arc Sin failed test.");

#if defined(VISKORES_ICC)
      // When the intel compiler has vectorization enabled ( -O2/-O3 ) it converts the
      // `adjacent/hypotenuse` divide operation into reciprocal (rcpps) and
      // multiply (mulps) operations. This causes a change in the expected result that
      // is larger than the default tolerance of test_equal.
      //
      VISKORES_MATH_ASSERT(test_equal(viskores::ACos(adjacent / hypotenuse), angle, 0.0004),
                           "Arc Cos failed test.");
#else
      VISKORES_MATH_ASSERT(test_equal(viskores::ACos(adjacent / hypotenuse), angle),
                           "Arc Cos failed test.");
#endif
      VISKORES_MATH_ASSERT(test_equal(viskores::ATan(opposite / adjacent), angle),
                           "Arc Tan failed test.");
    }
  }

  VISKORES_EXEC
  void TestHyperbolicTrig() const
  {
    const VectorType zero(0);
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType x;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        Traits::SetComponent(
          x, componentIndex, static_cast<ComponentType>(table.AngleList(componentIndex + index)));
      }

      const VectorType minusX = zero - x;

      VISKORES_MATH_ASSERT(
        test_equal(viskores::SinH(x), 0.5 * (viskores::Exp(x) - viskores::Exp(minusX))),
        "SinH does not match definition.");
      VISKORES_MATH_ASSERT(
        test_equal(viskores::CosH(x), 0.5 * (viskores::Exp(x) + viskores::Exp(minusX))),
        "SinH does not match definition.");
      VISKORES_MATH_ASSERT(test_equal(viskores::TanH(x), viskores::SinH(x) / viskores::CosH(x)),
                           "TanH does not match definition");

      VISKORES_MATH_ASSERT(test_equal(viskores::ASinH(viskores::SinH(x)), x),
                           "SinH not inverting.");
      VISKORES_MATH_ASSERT(test_equal(viskores::ACosH(viskores::CosH(x)), x),
                           "CosH not inverting.");
      VISKORES_MATH_ASSERT(test_equal(viskores::ATanH(viskores::TanH(x)), x),
                           "TanH not inverting.");
    }
  }

  template <typename FunctionType>
  VISKORES_EXEC void RaiseToTest(FunctionType function, ComponentType exponent) const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(raiseresult, componentIndex, viskores::Pow(x, exponent));
      }

      VectorType mathresult = function(original);

      VISKORES_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct SqrtFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Sqrt(x); }
  };
  VISKORES_EXEC
  void TestSqrt() const { RaiseToTest(SqrtFunctor(), 0.5); }

  struct RSqrtFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::RSqrt(x); }
  };
  VISKORES_EXEC
  void TestRSqrt() const { RaiseToTest(RSqrtFunctor(), -0.5); }

  struct CbrtFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Cbrt(x); }
  };
  VISKORES_EXEC
  void TestCbrt() const { RaiseToTest(CbrtFunctor(), viskores::Float32(1.0 / 3.0)); }

  struct RCbrtFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::RCbrt(x); }
  };
  VISKORES_EXEC
  void TestRCbrt() const { RaiseToTest(RCbrtFunctor(), viskores::Float32(-1.0 / 3.0)); }

  template <typename FunctionType>
  VISKORES_EXEC void RaiseByTest(FunctionType function,
                                 ComponentType base,
                                 ComponentType exponentbias = 0.0,
                                 ComponentType resultbias = 0.0) const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(
          raiseresult, componentIndex, viskores::Pow(base, x + exponentbias) + resultbias);
      }

      VectorType mathresult = function(original);

      VISKORES_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct ExpFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Exp(x); }
  };
  VISKORES_EXEC
  void TestExp() const { RaiseByTest(ExpFunctor(), viskores::Float32(2.71828183)); }

  struct Exp2Functor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Exp2(x); }
  };
  VISKORES_EXEC
  void TestExp2() const { RaiseByTest(Exp2Functor(), 2.0); }

  struct ExpM1Functor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::ExpM1(x); }
  };
  VISKORES_EXEC
  void TestExpM1() const { RaiseByTest(ExpM1Functor(), ComponentType(2.71828183), 0.0, -1.0); }

  struct Exp10Functor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Exp10(x); }
  };
  VISKORES_EXEC
  void TestExp10() const { RaiseByTest(Exp10Functor(), 10.0); }

  template <typename FunctionType>
  VISKORES_EXEC void LogBaseTest(FunctionType function,
                                 ComponentType base,
                                 ComponentType bias = 0.0) const
  {
    Lists table;
    for (viskores::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType basevector(base);
      VectorType original;
      VectorType biased;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(biased, componentIndex, x + bias);
      }

      VectorType logresult = viskores::Log2(biased) / viskores::Log2(basevector);

      VectorType mathresult = function(original);

      VISKORES_MATH_ASSERT(test_equal(mathresult, logresult), "Exponent functions do not agree.");
    }
  }

  struct LogFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Log(x); }
  };
  VISKORES_EXEC
  void TestLog() const { LogBaseTest(LogFunctor(), viskores::Float32(2.71828183)); }

  struct Log10Functor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Log10(x); }
  };
  VISKORES_EXEC
  void TestLog10() const { LogBaseTest(Log10Functor(), 10.0); }

  struct Log1PFunctor
  {
    VISKORES_EXEC
    VectorType operator()(VectorType x) const { return viskores::Log1P(x); }
  };
  VISKORES_EXEC
  void TestLog1P() const { LogBaseTest(Log1PFunctor(), ComponentType(2.71828183), 1.0); }

  VISKORES_EXEC
  void TestCopySign() const
  {
    // Assuming all TestValues positive.
    VectorType positive1 = TestValue(1, VectorType());
    VectorType positive2 = TestValue(2, VectorType());
    VectorType negative1 = -positive1;
    VectorType negative2 = -positive2;

    VISKORES_MATH_ASSERT(test_equal(viskores::CopySign(positive1, positive2), positive1),
                         "CopySign failed.");
    VISKORES_MATH_ASSERT(test_equal(viskores::CopySign(negative1, positive2), positive1),
                         "CopySign failed.");
    VISKORES_MATH_ASSERT(test_equal(viskores::CopySign(positive1, negative2), negative1),
                         "CopySign failed.");
    VISKORES_MATH_ASSERT(test_equal(viskores::CopySign(negative1, negative2), negative1),
                         "CopySign failed.");
  }

  VISKORES_EXEC
  void TestFloatDistance() const
  {
    {
      viskores::UInt64 dist = viskores::FloatDistance(1.0, 1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 1.0 to 1.0 is not zero.");

      dist = viskores::FloatDistance(-1.0, -1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -1.0 to -1.0 is not zero.");

      dist = viskores::FloatDistance(0.0, 0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 0.0 to 0.0 is not zero.");

      // Check nan:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::quiet_NaN(), 1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to a Nan is not the documented value.");

      dist = viskores::FloatDistance(1.0, std::numeric_limits<viskores::Float64>::quiet_NaN());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to a Nan is not the documented value.");

      // Check infinity:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::infinity(), 1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to infinity is not the documented value.");

      dist = viskores::FloatDistance(1.0, std::numeric_limits<viskores::Float64>::infinity());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to infinity is not the documented value.");

      // Check saturation:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::lowest(),
                                     std::numeric_limits<viskores::Float64>::max());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(18437736874454810622uL), dist),
                           "Float distance from lowest to max is incorrect.");

      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::max(),
                                     std::numeric_limits<viskores::Float64>::lowest());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(18437736874454810622uL), dist),
                           "Float distance from max to lowest is incorrect.");

      // Check symmetry:
      dist = viskores::FloatDistance(-2.0, -1.0);
      viskores::UInt64 dist2 = viskores::FloatDistance(-1.0, -2.0);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist), "Symmetry of negative numbers does not hold.");

      dist = viskores::FloatDistance(1.0, 2.0);
      dist2 = viskores::FloatDistance(2.0, 1.0);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist), "Float distance 1->2 != float distance 2->1.");

      // Check symmetry of bound which includes zero:
      dist = viskores::FloatDistance(-0.25, 0.25);
      dist2 = viskores::FloatDistance(0.25, -0.25);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist),
                           "Symmetry is violated over a bound which contains zero.");

      // Check correctness:
      dist = viskores::FloatDistance(1.0, 1.0 + std::numeric_limits<viskores::Float64>::epsilon());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Float distance from 1 to 1 + eps is not = 1.");
      dist = viskores::FloatDistance(1.0 + std::numeric_limits<viskores::Float64>::epsilon(), 1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated");

      dist =
        viskores::FloatDistance(1.0, 1.0 + 2 * std::numeric_limits<viskores::Float64>::epsilon());
      VISKORES_MATH_ASSERT(test_equal(viskores::Int64(2), dist),
                           "Float distance from 1 to 1 + 2eps is not 2.");
      dist =
        viskores::FloatDistance(1.0 + 2 * std::numeric_limits<viskores::Float64>::epsilon(), 1.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::Int64(2), dist), "Symmetry is violated.");

      // Now test x = y:
      viskores::Float64 x = -1;
      for (int i = 0; i < 50; ++i)
      {
        dist = viskores::FloatDistance(x, x);
        VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                             "Float distance from x to x is not zero.");
        x += 0.01;
      }
      // Test zero:
      dist = viskores::FloatDistance(0.0, 0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from zero to zero is not zero.");
      // Test signed zero:
      dist = viskores::FloatDistance(0.0, -0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 0.0 to -0.0 is not zero.");

      dist = viskores::FloatDistance(-0.0, 0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -0.0 to 0.0 is not zero.");

      dist = viskores::FloatDistance(-0.0, -0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -0.0 to 0.0 is not zero.");

      // Negative to negative zero:
      dist = viskores::FloatDistance(-std::numeric_limits<viskores::Float64>::denorm_min(), -0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Negative to zero incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(-0.0, -std::numeric_limits<viskores::Float64>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");

      // Negative to positive zero:
      dist = viskores::FloatDistance(-std::numeric_limits<viskores::Float64>::denorm_min(), 0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Negative to positive zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(0.0, -std::numeric_limits<viskores::Float64>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");

      // Positive to zero:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::denorm_min(), 0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Positive to zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(0.0, std::numeric_limits<viskores::Float64>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated");

      // Positive to negative zero:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float64>::denorm_min(), -0.0);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Positive to negative zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(-0.0, std::numeric_limits<viskores::Float64>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");
    }

    // I would try to just template these, but in fact the double precision version has to saturate,
    // whereas the float version has sufficient range.
    {
      viskores::UInt64 dist = viskores::FloatDistance(1.0f, 1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 1.0 to 1.0 is not zero.");

      dist = viskores::FloatDistance(-1.0f, -1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -1.0 to -1.0 is not zero.");

      dist = viskores::FloatDistance(0.0f, 0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 0.0 to 0.0 is not zero.");

      // Check nan:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::quiet_NaN(), 1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to a Nan is not the documented value.");

      dist = viskores::FloatDistance(1.0f, std::numeric_limits<viskores::Float32>::quiet_NaN());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to a Nan is not the documented value.");

      // Check infinity:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::infinity(), 1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to infinity is not the documented value.");

      dist = viskores::FloatDistance(1.0f, std::numeric_limits<viskores::Float32>::infinity());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0xFFFFFFFFFFFFFFFFL), dist),
                           "Float distance to infinity is not the documented value.");

      // Check saturation:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::lowest(),
                                     std::numeric_limits<viskores::Float32>::max());
      VISKORES_MATH_ASSERT(dist > 0, "Float distance is negative.");

      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::max(),
                                     std::numeric_limits<viskores::Float32>::lowest());
      VISKORES_MATH_ASSERT(dist > 0, "Float distance is negative.");

      // Check symmetry:
      dist = viskores::FloatDistance(-2.0f, -1.0f);
      viskores::UInt64 dist2 = viskores::FloatDistance(-1.0f, -2.0f);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist), "Symmetry of negative numbers does not hold.");

      dist = viskores::FloatDistance(1.0f, 2.0f);
      dist2 = viskores::FloatDistance(2.0f, 1.0f);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist), "Float distance 1->2 != float distance 2->1.");

      // Check symmetry of bound which includes zero:
      dist = viskores::FloatDistance(-0.25f, 0.25f);
      dist2 = viskores::FloatDistance(0.25f, -0.25f);
      VISKORES_MATH_ASSERT(test_equal(dist2, dist),
                           "Symmetry is violated over a bound which contains zero.");

      // Check correctness:
      dist =
        viskores::FloatDistance(1.0f, 1.0f + std::numeric_limits<viskores::Float32>::epsilon());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Float distance from 1 to 1 + eps is not = 1.");
      dist =
        viskores::FloatDistance(1.0f + std::numeric_limits<viskores::Float32>::epsilon(), 1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated");

      dist =
        viskores::FloatDistance(1.0f, 1.0f + 2 * std::numeric_limits<viskores::Float32>::epsilon());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(2), dist),
                           "Float distance from 1 to 1 + 2eps is not 2.");
      dist =
        viskores::FloatDistance(1.0f + 2 * std::numeric_limits<viskores::Float32>::epsilon(), 1.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(2), dist), "Symmetry is violated.");

      // Now test x = y:
      viskores::Float32 x = -1;
      for (int i = 0; i < 50; ++i)
      {
        dist = viskores::FloatDistance(x, x);
        VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                             "Float distance from x to x is not zero.");
        x += 0.01f;
      }
      // Test zero:
      dist = viskores::FloatDistance(0.0f, 0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from zero to zero is not zero.");
      // Test signed zero:
      dist = viskores::FloatDistance(0.0f, -0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from 0.0 to -0.0 is not zero.");

      dist = viskores::FloatDistance(-0.0f, 0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -0.0 to 0.0 is not zero.");

      dist = viskores::FloatDistance(-0.0f, -0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(0), dist),
                           "Float distance from -0.0 to 0.0 is not zero.");

      // Negative to negative zero:
      dist = viskores::FloatDistance(-std::numeric_limits<viskores::Float32>::denorm_min(), -0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Negative to zero incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(-0.0f, -std::numeric_limits<viskores::Float32>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");

      // Negative to positive zero:
      dist = viskores::FloatDistance(-std::numeric_limits<viskores::Float32>::denorm_min(), 0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Negative to positive zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(0.0f, -std::numeric_limits<viskores::Float32>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");

      // Positive to zero:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::denorm_min(), 0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Positive to zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(0.0f, std::numeric_limits<viskores::Float32>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated");

      // Positive to negative zero:
      dist = viskores::FloatDistance(std::numeric_limits<viskores::Float32>::denorm_min(), -0.0f);
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist),
                           "Positive to negative zero is incorrect.");
      // And symmetry:
      dist = viskores::FloatDistance(-0.0f, std::numeric_limits<viskores::Float32>::denorm_min());
      VISKORES_MATH_ASSERT(test_equal(viskores::UInt64(1), dist), "Symmetry is violated.");
    }
  }

  VISKORES_EXEC
  void TestDifferenceOfProducts() const
  {
#if defined FP_FAST_FMA && !defined __HIP__ && !defined _ARCH_PPC && !defined _ARCH_PPC64
    // Example taken from:
    // https://pharr.org/matt/blog/2019/11/03/difference-of-floats.html
    viskores::Float32 a = 33962.035f;
    viskores::Float32 b = -30438.8f;
    viskores::Float32 c = 41563.4f;
    viskores::Float32 d = -24871.969f;
    viskores::Float32 computed = viskores::DifferenceOfProducts(a, b, c, d);
    // Expected result, computed in double precision and cast back to float:
    viskores::Float32 expected = 5.376600027084351f;

    viskores::UInt64 dist = viskores::FloatDistance(expected, computed);
    VISKORES_MATH_ASSERT(
      dist < 2,
      "Float distance for difference of products exceeds 1.5; this is in violation of a theorem "
      "proved by Jeannerod in doi.org/10.1090/S0025-5718-2013-02679-8. Is your build compiled "
      "with FMAs enabled?");
#endif
  }

  VISKORES_EXEC
  void TestQuadraticRoots() const
  {
    // (x-1)(x+1) = x² - 1:
    auto roots = viskores::QuadraticRoots(1.0f, 0.0f, -1.0f);

    viskores::UInt64 dist = viskores::FloatDistance(-1.0f, roots[0]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");

    dist = viskores::FloatDistance(1.0f, roots[1]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");

    // No real roots:
    roots = viskores::QuadraticRoots(1.0f, 0.0f, 1.0f);
    VISKORES_MATH_ASSERT(viskores::IsNan(roots[0]),
                         "Roots should be Nan for a quadratic with complex roots.");
    VISKORES_MATH_ASSERT(viskores::IsNan(roots[1]),
                         "Roots should be Nan for a quadratic with complex roots.");

#if defined FP_FAST_FMA && !defined __HIP__ && !defined _ARCH_PPC && !defined _ARCH_PPC64
    // Wikipedia example:
    // x² + 200x - 0.000015 = 0 has roots
    // -200.000000075, 7.5e-8
    roots = viskores::QuadraticRoots(1.0f, 200.0f, -0.000015f);
    dist = viskores::FloatDistance(-200.000000075f, roots[0]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");

    dist = viskores::FloatDistance(7.5e-8f, roots[1]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");

    // Kahan's example:
    auto roots64 = viskores::QuadraticRoots(94906265.625, 94906267.000, 94906268.375);
    dist = viskores::FloatDistance(1.0, roots64[0]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");

    dist = viskores::FloatDistance(1.000000028975958, roots64[1]);
    VISKORES_MATH_ASSERT(dist < 3, "Float distance for quadratic roots exceeds 3 ulps.");
#endif
  }

  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    this->TestTriangleTrig();
    this->TestHyperbolicTrig();
    this->TestSqrt();
    this->TestRSqrt();
    this->TestCbrt();
    this->TestRCbrt();
    this->TestExp();
    this->TestExp2();
    this->TestExpM1();
    this->TestExp10();
    this->TestLog();
    this->TestLog10();
    this->TestLog1P();
    this->TestCopySign();
    this->TestFloatDistance();
    this->TestDifferenceOfProducts();
    this->TestQuadraticRoots();
  }
};

struct TryScalarVectorFieldTests
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    viskores::cont::Algorithm::Schedule(ScalarVectorFieldTests<VectorType>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AllTypesTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void TestMinMax() const
  {
    T low = TestValue(2, T());
    T high = TestValue(10, T());
    VISKORES_MATH_ASSERT(test_equal(viskores::Min(low, high), low), "Wrong min.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Min(high, low), low), "Wrong min.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Max(low, high), high), "Wrong max.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Max(high, low), high), "Wrong max.");

    using Traits = viskores::VecTraits<T>;
    T mixed1 = low;
    T mixed2 = high;
    Traits::SetComponent(mixed1, 0, Traits::GetComponent(high, 0));
    Traits::SetComponent(mixed2, 0, Traits::GetComponent(low, 0));
    VISKORES_MATH_ASSERT(test_equal(viskores::Min(mixed1, mixed2), low), "Wrong min.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Min(mixed2, mixed1), low), "Wrong min.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Max(mixed1, mixed2), high), "Wrong max.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Max(mixed2, mixed1), high), "Wrong max.");
  }

  VISKORES_EXEC
  void operator()(viskores::Id) const { this->TestMinMax(); }
};

struct TryAllTypesTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(AllTypesTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AbsTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    T positive = TestValue(index, T()); // Assuming all TestValues positive.
    T negative = -positive;

    VISKORES_MATH_ASSERT(test_equal(viskores::Abs(positive), positive),
                         "Abs returned wrong value.");
    VISKORES_MATH_ASSERT(test_equal(viskores::Abs(negative), positive),
                         "Abs returned wrong value.");
  }
};

struct TryAbsTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(AbsTests<T>(), 10);
  }
};

using TypeListAbs = viskores::ListAppend<viskores::List<viskores::Int32, viskores::Int64>,
                                         viskores::TypeListIndex,
                                         viskores::TypeListField>;

//-----------------------------------------------------------------------------
static constexpr viskores::Id BitOpSamples = 128 * 128;

template <typename T>
struct BitOpTests : public viskores::exec::FunctorBase
{
  static constexpr T MaxT = std::numeric_limits<T>::max();
  static constexpr T Offset = MaxT / BitOpSamples;

  VISKORES_EXEC void operator()(viskores::Id i) const
  {
    const T idx = static_cast<T>(i);
    const T word = idx * this->Offset;

    TestWord(word - idx);
    TestWord(word);
    TestWord(word + idx);
  }

  VISKORES_EXEC void TestWord(T word) const
  {
    VISKORES_MATH_ASSERT(test_equal(viskores::CountSetBits(word), this->DumbCountBits(word)),
                         "CountBits returned wrong value.");
    VISKORES_MATH_ASSERT(
      test_equal(viskores::FindFirstSetBit(word), this->DumbFindFirstSetBit(word)),
      "FindFirstSetBit returned wrong value.");
  }

  VISKORES_EXEC viskores::Int32 DumbCountBits(T word) const
  {
    viskores::Int32 bits = 0;
    while (word)
    {
      if (word & 0x1)
      {
        ++bits;
      }
      word >>= 1;
    }
    return bits;
  }

  VISKORES_EXEC viskores::Int32 DumbFindFirstSetBit(T word) const
  {
    if (word == 0)
    {
      return 0;
    }

    viskores::Int32 bit = 1;
    while ((word & 0x1) == 0)
    {
      word >>= 1;
      ++bit;
    }
    return bit;
  }
};

struct TryBitOpTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(BitOpTests<T>(), BitOpSamples);
  }
};

using TypeListBitOp = viskores::List<viskores::UInt32, viskores::UInt64>;

//-----------------------------------------------------------------------------
void RunMathTests()
{
  viskores::testing::Testing::TryTypes(TryScalarFieldTests(), viskores::TypeListFieldScalar());
  viskores::testing::Testing::TryTypes(TryScalarVectorFieldTests(), viskores::TypeListField());
  viskores::testing::Testing::TryTypes(TryAllTypesTests());
  viskores::testing::Testing::TryTypes(TryAbsTests(), TypeListAbs());
  viskores::testing::Testing::TryTypes(TryBitOpTests(), TypeListBitOp());
}

} // namespace UnitTestMathNamespace

int UnitTestMath(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(UnitTestMathNamespace::RunMathTests, argc, argv);
}
