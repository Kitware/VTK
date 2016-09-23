/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkType.h"

#include <vector>

static int TestPi();
#if 0
static int TestDoublePi();
static int TestDoubleTwoPi();
#endif
static int TestDegreesFromRadians();
static int TestRound();
static int TestFloor();
static int TestCeil();
static int TestCeilLog2();
static int TestIsPowerOfTwo();
static int TestNearestPowerOfTwo();
static int TestFactorial();
static int TestBinomial();
static int TestRandom();
static int TestAddSubtract();
static int TestMultiplyScalar();
static int TestMultiplyScalar2D();
static int TestDot();
static int TestOuter();
static int TestCross();
static int TestNorm();
static int TestNormalize();
static int TestPerpendiculars();
static int TestProjectVector();
static int TestProjectVector2D();
static int TestDistance2BetweenPoints();
static int TestAngleBetweenVectors();
static int TestGaussianAmplitude();
static int TestGaussianWeight();
static int TestDot2D();
static int TestNorm2D();
static int TestNormalize2D();
static int TestDeterminant2x2();
static int TestDeterminant3x3();
static int TestLUFactor3x3();
static int TestLUSolve3x3();
static int TestLinearSolve3x3();
static int TestMultiply3x3();
static int TestMultiplyMatrix();
static int TestTranspose3x3();
static int TestInvert3x3();
static int TestInvertMatrix();
static int TestIdentity3x3();
static int TestQuaternionToMatrix3x3();
static int TestMatrix3x3ToQuaternion();
static int TestMultiplyQuaternion();
static int TestOrthogonalize3x3();
static int TestDiagonalize3x3();
static int TestSingularValueDecomposition3x3();
static int TestSolveLinearSystem();
static int TestSolveLeastSquares();
static int TestSolveHomogeneousLeastSquares();
static int TestLUSolveLinearSystemEstimateMatrixCondition();
static int TestJacobiN();
static int TestClampValue();
static int TestClampValues();
static int TestClampAndNormalizeValue();
static int TestGetScalarTypeFittingRange();
static int TestGetAdjustedScalarRange();
static int TestExtentIsWithinOtherExtent();
static int TestBoundsIsWithinOtherBounds();
static int TestPointIsWithinBounds();
static int TestSolve3PointCircle();
static int TestRGBToHSV();
static int TestInf();
static int TestNegInf();
static int TestNan();

int UnitTestMath(int,char *[])
{
  int status = 0;

  status += TestPi();

#if 0
  status += TestDoublePi(); // legacy
  status += TestDoubleTwoPi(); // legacy
#endif

  status += TestDegreesFromRadians();
  status += TestRound();
  status += TestFloor();
  status += TestCeil();
  status += TestCeilLog2();
  status += TestIsPowerOfTwo();
  status += TestNearestPowerOfTwo();
  status += TestFactorial();
  status += TestBinomial();
  status += TestRandom();
  status += TestAddSubtract();
  status += TestMultiplyScalar();
  status += TestMultiplyScalar2D();
  status += TestDot();
  status += TestOuter();
  status += TestCross();
  status += TestNorm();
  status += TestNormalize();
  status += TestPerpendiculars();
  status += TestProjectVector();
  status += TestProjectVector2D();
  status += TestDistance2BetweenPoints();
  status += TestAngleBetweenVectors();
  status += TestGaussianAmplitude();
  status += TestGaussianWeight();
  status += TestDot2D();
  status += TestNorm2D();
  status += TestNormalize2D();
  status += TestDeterminant2x2();
  status += TestDeterminant3x3();
  status += TestLUFactor3x3();
  status += TestLUSolve3x3();
  status += TestLinearSolve3x3();
  status += TestMultiply3x3();
  status += TestMultiplyMatrix();
  status += TestTranspose3x3();
  status += TestInvert3x3();
  status += TestInvertMatrix();
  status += TestIdentity3x3();
  status += TestQuaternionToMatrix3x3();
  status += TestMatrix3x3ToQuaternion();
  status += TestMultiplyQuaternion();
  status += TestOrthogonalize3x3();
  status += TestDiagonalize3x3();
  status += TestSingularValueDecomposition3x3();
  status += TestSolveLinearSystem();
  status += TestSolveLeastSquares();
  status += TestSolveHomogeneousLeastSquares();
  status += TestLUSolveLinearSystemEstimateMatrixCondition();
  status += TestJacobiN();
  status += TestClampValue();
  status += TestClampValues();
  status += TestClampAndNormalizeValue();
  status += TestGetScalarTypeFittingRange();
  status += TestGetAdjustedScalarRange();
  status += TestExtentIsWithinOtherExtent();
  status += TestBoundsIsWithinOtherBounds();
  status += TestPointIsWithinBounds();
  status += TestSolve3PointCircle();
  status += TestRGBToHSV();
  status += TestInf();
  status += TestNegInf();
  status += TestNan();
  if (status != 0)
  {
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkMath> math =
    vtkSmartPointer<vtkMath>::New();
  math->Print(std::cout);

  return EXIT_SUCCESS;
}

// Validate by comparing to atan/4
int TestPi()
{
  int status = 0;
  std::cout << "Pi..";
  if (vtkMath::Pi() != std::atan(1.0) * 4.0)
  {
    std::cout << "Expected " << vtkMath::Pi()
              << " but got " << std::atan(1.0) * 4.0;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

#if 0
// Validate by comparing to atan/4
int TestDoublePi()
{
  int status = 0;
  std::cout << "DoublePi..";

  if (vtkMath::DoublePi() != std::atan(1.0) * 4.0)
  {
    std::cout << "Expected " << vtkMath::Pi()
              << " but got " << std::atan(1.0) * 4.0;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by comparing to atan/4 * 2
int TestDoubleTwoPi()
{
  int status = 0;
  std::cout << "DoubleTwoPi..";

  if (vtkMath::DoubleTwoPi() != std::atan(1.0) * 4.0 * 2.0)
  {
    std::cout << "Expected " << vtkMath::Pi() * 2.0
              << " but got " << std::atan(1.0) * 4.0 * 2.0;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}
#endif

// Validate against RadiansFromDegress
int TestDegreesFromRadians()
{
  int status = 0;
  std::cout << "DegreesFromRadians..";

  unsigned int numSamples = 1000;
  for (unsigned int i = 0; i < numSamples; ++i)
  {
    float floatDegrees = vtkMath::Random(-180.0, 180.0);
    float floatRadians = vtkMath::RadiansFromDegrees(floatDegrees);
    float result = vtkMath::DegreesFromRadians(floatRadians);
    if (!vtkMathUtilities::FuzzyCompare(
          result, floatDegrees,
          std::numeric_limits<float>::epsilon()*128.0f))
    {
      std::cout << "Float  Expected " << floatDegrees
                << " but got " << result
                << " difference is " << result - floatDegrees << " ";
      std::cout << "eps ratio is: " << (result - floatDegrees)
        / std::numeric_limits<float>::epsilon() << std::endl;
      ++status;
    }
  }
  for (unsigned int i = 0; i < numSamples; ++i)
  {
    double doubleDegrees = vtkMath::Random(-180.0, 180.0);
    double doubleRadians = vtkMath::RadiansFromDegrees(doubleDegrees);
    double result = vtkMath::DegreesFromRadians(doubleRadians);
    if (!vtkMathUtilities::FuzzyCompare(
          result, doubleDegrees,
          std::numeric_limits<double>::epsilon()*256.0))
    {
      std::cout << " Double Expected " << doubleDegrees
                << " but got " << result
                << " difference is " << result - doubleDegrees;
      std::cout << " eps ratio is: " << (result - doubleDegrees)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status;
    }
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with http://en.wikipedia.org/wiki/Rounding#Rounding_to_integer
int TestRound()
{
  int status = 0;
  std::cout << "Round..";
  int result;
  {
  std::vector<float> values;
  std::vector<int> expecteds;

  values.push_back(23.67f); expecteds.push_back(24);
  values.push_back(23.50f); expecteds.push_back(24);
  values.push_back(23.35f); expecteds.push_back(23);
  values.push_back(23.00f); expecteds.push_back(23);
  values.push_back(0.00f); expecteds.push_back(  0);
  values.push_back(-23.00f); expecteds.push_back(-23);
  values.push_back(-23.35f); expecteds.push_back(-23);
  values.push_back(-23.50f); expecteds.push_back(-24);
  values.push_back(-23.67f); expecteds.push_back(-24);
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::Round(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " Float Round(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }
  {
  std::vector<double> values;
  std::vector<int> expecteds;

  values.push_back(23.67); expecteds.push_back(24);
  values.push_back(23.50); expecteds.push_back(24);
  values.push_back(23.35); expecteds.push_back(23);
  values.push_back(23.00); expecteds.push_back(23);
  values.push_back(0.00); expecteds.push_back(  0);
  values.push_back(-23.00); expecteds.push_back(-23);
  values.push_back(-23.35); expecteds.push_back(-23);
  values.push_back(-23.50); expecteds.push_back(-24);
  values.push_back(-23.67); expecteds.push_back(-24);
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::Round(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " Double Round(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with http://en.wikipedia.org/wiki/Floor_and_ceiling_functions
int TestFloor()
{
  int status = 0;
  std::cout << "Floor..";

  int result;
  std::vector<double> values;
  std::vector<int> expecteds;

  values.push_back(2.4); expecteds.push_back(2);
  values.push_back(2.7); expecteds.push_back(2);
  values.push_back(-2.7); expecteds.push_back(-3);
  values.push_back(-2.0); expecteds.push_back(-2);
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::Floor(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " Floor(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with http://en.wikipedia.org/wiki/Floor_and_ceiling_functions
int TestCeil()
{
  int status = 0;
  std::cout << "Ceil..";

  int result;
  std::vector<double> values;
  std::vector<int> expecteds;

  values.push_back(2.4); expecteds.push_back(3);
  values.push_back(2.7); expecteds.push_back(3);
  values.push_back(-2.7); expecteds.push_back(-2);
  values.push_back(-2.0); expecteds.push_back(-2);
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::Ceil(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " Ceil(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by powers of 2 perturbations
int TestCeilLog2()
{
  int status = 0;
  std::cout << "CeilLog2..";

  int result;
  std::vector<vtkTypeUInt64> values;
  std::vector<int> expecteds;

  for ( unsigned int p = 0; p < 30; ++p)
  {
    vtkTypeUInt64 shifted = (2 << p) + 1;
    values.push_back(shifted); expecteds.push_back(p + 2);
    shifted = (2 << p);
    values.push_back(shifted); expecteds.push_back(p + 1);
  }
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::CeilLog2(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " CeilLog2(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by powers of 2 perturbations
int TestIsPowerOfTwo()
{
  int status = 0;
  std::cout << "IsPowerOfTwo..";
  bool result;

  std::vector<vtkTypeUInt64> values;
  std::vector<bool> expecteds;
  int largestPower = std::numeric_limits<vtkTypeUInt64>::digits;
  vtkTypeUInt64 shifted = 1;
  for ( int p = 1; p < largestPower - 1; ++p)
  {
    shifted *= 2;
    values.push_back(shifted); expecteds.push_back(true);
    if (shifted != 2)
    {
      values.push_back(shifted - 1); expecteds.push_back(false);
    }
    if (shifted < std::numeric_limits<vtkTypeUInt64>::max() - 1)
    {
      values.push_back(shifted + 1); expecteds.push_back(false);
    }
  }
  for ( size_t i = 0; i < values.size(); ++i)
  {
    result = vtkMath::IsPowerOfTwo(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " IsPowerOfTwo(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by powers of 2 perturbations
int TestNearestPowerOfTwo()
{
  int status = 0;
  std::cout << "NearestPowerOfTwo..";

  std::vector<vtkTypeUInt64> values;
  std::vector<int> expecteds;
  int largestPower = std::numeric_limits<int>::digits;
  vtkTypeUInt64 shifted = 1;
  for ( int p = 1; p < largestPower; ++p)
  {
    shifted *= 2;
    values.push_back(shifted); expecteds.push_back(shifted);
    values.push_back(shifted + 1); expecteds.push_back(shifted * 2);
    if (shifted !=2 )
    {
      values.push_back(shifted - 1); expecteds.push_back(shifted);
    }
  }
  for ( size_t i = 0; i < values.size(); ++i)
  {
    int result = vtkMath::NearestPowerOfTwo(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " NearestPowerOfTwo(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by alternate computation
int TestFactorial()
{
  int status = 0;
  std::cout << "Factorial..";

  std::vector<int> values;
  std::vector<vtkTypeInt64> expecteds;
  vtkTypeInt64 expected = 1;
  for ( int f = 2; f < 10; ++f)
  {
    expected *= f;
    values.push_back(f); expecteds.push_back(expected);
  }
  for ( size_t i = 0; i < values.size(); ++i)
  {
    int result = vtkMath::Factorial(values[i]);
    if (result != expecteds[i])
    {
      std::cout << " Factorial(" << values[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by alternative computations
int TestBinomial()
{
  int status = 0;
  int m;
  int n;
  std::cout << "Binomial..";

  std::vector<int> mvalues;
  std::vector<int> nvalues;

  std::vector<vtkTypeInt64> expecteds;
  double expected;
  for (m = 1; m < 31; ++m)
  {
    for (n = 1; n <= m; ++n)
    {
      mvalues.push_back(m);
      nvalues.push_back(n);
      expected = 1;
      for (int i = 1; i <= n; ++i)
      {
        expected *= static_cast<double>(m - i + 1) / i;
      }
      expecteds.push_back(static_cast<vtkTypeInt64>(expected));
    }
  }

  for ( size_t i = 0; i < mvalues.size(); ++i)
  {
    int result = vtkMath::Binomial(mvalues[i], nvalues[i]);
    if (result != expecteds[i])
    {
      std::cout << " Binomial("
                << mvalues[i] << ", " << nvalues[i] << ") got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  // Now test the combination iterator
  m = 6;
  n = 3;
  int more = 1;
  int count = 0;
  int *comb;
  // First, m < n should produce 0
  comb = vtkMath::BeginCombination(n, m);
  if (comb != NULL)
  {
    ++status;
    std::cout << " Combinations("
              << n << ", " << m << ") should return 0 "
                << " but got " << comb;
  }
  comb =  vtkMath::BeginCombination(m, n);
  while (more)
  {
    ++count;
    more = vtkMath::NextCombination (m, n, comb);
  }
  vtkMath::FreeCombination(comb);
  if (count != vtkMath::Binomial(m, n))
  {
    ++status;
    std::cout << " Combinations("
              << m << ", " << n << ") got " << count
                << " but expected " << vtkMath::Binomial(m, n);
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// No validation
int TestRandom()
{
  int status = 0;
  std::cout << "Random..";
  // Not really a test of randomness, just covering code
  int n = 1000;
  vtkMath::RandomSeed(8775070);
  vtkMath::GetSeed(); // just for coverage
  double accum = 0.0;
  for (int i = 0; i < n; ++i)
  {
    float random = vtkMath::Random();
    accum += random;
    if ( random < 0.0 || random > 1.0)
    {
      std::cout << "Random(): " << random <<  " out of range" << std::endl;
      ++status;
    }
    random = vtkMath::Gaussian ();
    accum += random;

    random = vtkMath::Gaussian (0.0, 1.0);
    accum += random;

    random = vtkMath::Random (-1000.0, 1000.0);
    accum += random;
    if (random < -1000.0 || random > 1000.0)
    {
      std::cout << "Random (-1000.0, 1000.0): " << random <<  " out of range" << std::endl;
      ++status;
    }
  }
  if (accum == 0.0)
  {
    ++status;
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int AddSubtract()
{
  int status = 0;
  T da[3], db[3], dc[3], dd[3];
  for (int n = 0; n < 100000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      da[i] = vtkMath::Random(-10.0, 10.0);
      db[i] = vtkMath::Random(-10.0, 10.0);
    }
    vtkMath::Add(da, db, dc);
    vtkMath::Subtract(dc, db, dd);
    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            da[i], dd[i],
            std::numeric_limits<T>::epsilon() * (T) 256.0))
      {
        std::cout << " Add/Subtract got " << dd[i]
                  << " but expected " << da[i];
      }
    }
  }
  return status;
}

// Validate by a + b - b = a
int TestAddSubtract()
{
  int status = 0;
  std::cout << "AddSubtract..";

  status += AddSubtract<double>();
  status += AddSubtract<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int MultiplyScalar()
{
  int status = 0;
  // first T
  T da[3], db[3];
  for (int n = 0; n < 100000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      da[i] = vtkMath::Random(-10.0, 10.0);
      db[i] = da[i];
    }
    T scale = vtkMath::Random();
    vtkMath::MultiplyScalar(da, scale);

    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            da[i], db[i] * scale,
            std::numeric_limits<T>::epsilon() * (T) 256.0))
      {
        std::cout << " MultiplyScalar got " << da[i]
                  << " but expected " << db[i] * scale;
      }
    }
  }

  return status;
}

int TestMultiplyScalar()
{
  int status = 0;
  std::cout << "MultiplyScalar..";

  status += MultiplyScalar<double>();
  status += MultiplyScalar<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestMultiplyScalar2D()
{
  int status = 0;
  std::cout << "MultiplyScalar2D..";

  // now 2D
  // first double
  double da[2], db[2];
  for (int n = 0; n < 100000; ++n)
  {
    for (int i = 0; i < 2; ++i)
    {
      da[i] = vtkMath::Random(-10.0, 10.0);
      db[i] = da[i];
    }
    double scale = vtkMath::Random();
    vtkMath::MultiplyScalar2D(da, scale);

    for (int i = 0; i < 2; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            da[i], db[i] * scale,
            std::numeric_limits<double>::epsilon() * 256.0))
      {
        std::cout << " MultiplyScalar2D got " << da[i]
                  << " but expected " << db[i] * scale;
      }
    }
  }

  // then float
  float fa[2], fb[2];
  for (int n = 0; n < 100000; ++n)
  {
    for (int i = 0; i < 2; ++i)
    {
      fa[i] = vtkMath::Random(-10.0, 10.0);
      fb[i] = fa[i];
    }
    float scale = vtkMath::Random();
    vtkMath::MultiplyScalar2D(fa, scale);

    for (int i = 0; i < 2; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            fa[i], fb[i] * scale,
            std::numeric_limits<float>::epsilon() * 256.0f))
      {
        std::cout << " MultiplyScalar2D got " << fa[i]
                  << " but expected " << fb[i] * scale;
      }
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

class valueDouble3D
{
public:
  valueDouble3D() {}
  valueDouble3D(double aa[3], double bb[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      a[i] = aa[i];
      b[i] = bb[i];
    }
  }
  double a[3];
  double b[3];
};

class valueFloat3D
{
public:
  valueFloat3D() {}
  valueFloat3D(float aa[3], float bb[3])
  {
    for (int i = 0; i < 3; ++i)
    {
      a[i] = aa[i];
      b[i] = bb[i];
    }
  }
  float a[3];
  float b[3];
};
int TestDot()
{
  int status = 0;
  std::cout << "Dot..";

  {
  std::vector<valueDouble3D> values;
  std::vector<double> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    valueDouble3D v;
    double dot = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      v.a[i] = vtkMath::Random();
      v.b[i] = vtkMath::Random();
      dot += (v.a[i] * v.b[i]);
    }
    values.push_back(v);
    expecteds.push_back(dot);
  }
  valueDouble3D test;
  test.a[0] = 0.0; test.a[1] = 0.0;   test.a[2] = 1.0;
  test.b[0] = 1.0; test.b[1] = 0.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);
  test.a[0] = 0.0; test.a[1] = 0.0;   test.a[2] = 1.0;
  test.b[0] = 0.0; test.b[1] = 1.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);
  test.a[0] = 1.0; test.a[1] = 0.0;   test.a[2] = 0.0;
  test.b[0] = 0.0; test.b[1] = 1.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);

  for ( size_t i = 0; i < values.size(); ++i)
  {
    double result = vtkMath::Dot(values[i].a, values[i].b);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expecteds[i],
          std::numeric_limits<double>::epsilon()*128.0))
    {
      std::cout << " Dot got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }

  // now float
  {
  std::vector<valueFloat3D> values;
  std::vector<float> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    valueFloat3D v;
    float dot = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      v.a[i] = vtkMath::Random();
      v.b[i] = vtkMath::Random();
      dot += (v.a[i] * v.b[i]);
    }
    values.push_back(v);
    expecteds.push_back(dot);
  }
  valueFloat3D test;
  test.a[0] = 0.0; test.a[1] = 0.0;   test.a[2] = 1.0;
  test.b[0] = 1.0; test.b[1] = 0.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);
  test.a[0] = 0.0; test.a[1] = 0.0;   test.a[2] = 1.0;
  test.b[0] = 0.0; test.b[1] = 1.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);
  test.a[0] = 1.0; test.a[1] = 0.0;   test.a[2] = 0.0;
  test.b[0] = 0.0; test.b[1] = 1.0;   test.b[2] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);

  for ( size_t i = 0; i < values.size(); ++i)
  {
    float result = vtkMath::Dot(values[i].a, values[i].b);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expecteds[i],
          std::numeric_limits<float>::epsilon()*128.0f))
    {
      std::cout << " Dot got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestOuter()
{
  int status = 0;
  std::cout << "Outer..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Verify by anticommutative property
template<typename T>
int Cross()
{
  int status = 0;
  T a[3];
  T b[3];
  T c[3];
  T d[3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      a[i] = vtkMath::Random(-1.0, 1.0);
      b[i] = vtkMath::Random(-1.0, 1.0);
    }
    vtkMath::Cross(a, b, c);
    vtkMath::MultiplyScalar(b, (T) -1.0);
    vtkMath::Cross(b, a, d);
    // a x b = -b x a
    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            c[i], d[i],
            std::numeric_limits<T>::epsilon()*(T)128.0))
      {
        std::cout << " Cross expected " << c[i]
                  << " but got " << d[i];
        std::cout << "eps ratio is: " << (c[i] - d[i])
          / std::numeric_limits<T>::epsilon() << std::endl;
        ++status;
      }
    }
  }
  return status;
}

int TestCross()
{
  int status = 0;
  std::cout << "Cross..";

  status += Cross<double>();
  status += Cross<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T, int NDimension>
int Norm()
{
  int status = 0;
  T x[NDimension];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < NDimension; ++i)
    {
      x[i] = (T) vtkMath::Random(-10.0, 10.0);
    }

    T norm = vtkMath::Norm(x, NDimension);

    for (int i = 0; i < NDimension; ++i)
    {
      x[i] /= norm;
    }

    T unitNorm = vtkMath::Norm(x, NDimension);
    if (!vtkMathUtilities::FuzzyCompare(
          unitNorm, (T) 1.0,
          std::numeric_limits<T>::epsilon()*(T)128.0))
    {
      std::cout << "Norm  Expected " << 1.0
                << " but got " << unitNorm;
      std::cout << " eps ratio is: " <<  ((T) 1.0 - unitNorm)
        / std::numeric_limits<T>::epsilon() << std::endl;
      ++status;
    }
  }

  return status;
}

int TestNorm()
{
  int status = 0;
  std::cout << "Norm..";

  status += Norm<double,1>();
  status += Norm<double,3>();
  status += Norm<double,1000>();
  status += Norm<float,1>();
  status += Norm<float,3>();
  status += Norm<float,1000>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate compute Norm, should be 1.0

template<typename T>
int Normalize()
{
  int status = 0;
  for (int n = 0; n < 1000; ++n)
  {
    T a[3];
    for (int i = 0; i < 3; ++i)
    {
      a[i] = vtkMath::Random(-10000.0,
                             10000.0);
    }
    vtkMath::Normalize(a);
    T value= vtkMath::Norm(a);
    T expected = 1.0;
    if (!vtkMathUtilities::FuzzyCompare(
          value, expected,
          std::numeric_limits<T>::epsilon() * (T) 128.0))
    {
      std::cout << " Normalize expected " << expected
                << " but got " << value;
      std::cout << "eps ratio is: " << value - expected
        / std::numeric_limits<T>::epsilon() << std::endl;
      ++status;
    }
  }
  return status;
}

int TestNormalize()
{
  int status = 0;
  std::cout << "Normalize..";

  status += Normalize<double>();
  status += Normalize<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestPerpendiculars()
{
  int status = 0;
  std::cout << "Perpendiculars..";
  {
  // first double
  double x[3], y[3], z[3];
  std::vector<valueDouble3D> values;
  std::vector<double> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      x[i] = vtkMath::Random(-10.0, 10.0);
    }
    vtkMath::Perpendiculars(x, y, z,
                            vtkMath::Random(-vtkMath::Pi(), vtkMath::Pi()));
    {
    valueDouble3D value(x, y);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    {
    valueDouble3D value(x, z);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    {
    valueDouble3D value(y, z);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    vtkMath::Perpendiculars(x, y, z,
                            0.0);
    {
    valueDouble3D value(x, y);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
  }
    for (size_t i = 0; i < values.size(); ++i)
    {
      double test = vtkMath::Dot(values[i].a, values[i].b);
      if (!vtkMathUtilities::FuzzyCompare(
            expecteds[i], test,
            std::numeric_limits<double>::epsilon() * 256.0))
      {
        std::cout << " Perpendiculars got " << test
                  << " but expected " << expecteds[i];
      }
    }
  }
  {
  // then floats
  float x[3], y[3], z[3];
  std::vector<valueFloat3D> values;
  std::vector<float> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      x[i] = vtkMath::Random(-10.0, 10.0);
    }
    vtkMath::Perpendiculars(x, y, z,
                            vtkMath::Random(-vtkMath::Pi(), vtkMath::Pi()));
    {
    valueFloat3D value(x, y);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    {
    valueFloat3D value(x, z);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    {
    valueFloat3D value(y, z);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
    vtkMath::Perpendiculars(x, y, z,
                            0.0);
    {
    valueFloat3D value(x, y);
    values.push_back(value);
    expecteds.push_back(0.0);
    }
  }
    for (size_t i = 0; i < values.size(); ++i)
    {
      float test = vtkMath::Dot(values[i].a, values[i].b);
      if (!vtkMathUtilities::FuzzyCompare(
            expecteds[i], test,
            std::numeric_limits<float>::epsilon() * 256.0f))
      {
        std::cout << " Perpendiculars got " << test
                  << " but expected " << expecteds[i];
      }
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int ProjectVector()
{
  int status = 0;
  T a[3], b[3], c[3];
  for (int i = 0; i < 3; ++i)
  {
    a[i] = 0.0;
    b[i] = 0.0;
  }
  if (vtkMath::ProjectVector(a, b, c))
  {
    std::cout << "ProjectVector of a 0 vector should return false ";
    ++status;
  }
  return status;
}

// Validate 0 vector case. TestMath does the rest
int TestProjectVector()
{
  int status = 0;
  std::cout << "ProjectVector..";

  status += ProjectVector<double>();
  status += ProjectVector<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int ProjectVector2D()
{
  int status = 0;
  T a[2], b[2], c[2];
  for (int i = 0; i < 2; ++i)
  {
    a[i] = 0.0;
    b[i] = 0.0;
  }
  if (vtkMath::ProjectVector2D(a, b, c))
  {
    std::cout << "ProjectVector2D of a 0 vector should return false ";
    ++status;
  }
  return status;
}

// Validate 0 vector case. TestMath does the rest
int TestProjectVector2D()
{
  int status = 0;
  std::cout << "ProjectVector2D..";

  status += ProjectVector2D<double>();
  status += ProjectVector2D<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestDistance2BetweenPoints()
{
  int status = 0;
  std::cout << "Distance2BetweenPoints..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestAngleBetweenVectors()
{
  int status = 0;
  std::cout << "AngleBetweenVectors..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestGaussianAmplitude()
{
  int status = 0;
  std::cout << "GaussianAmplitude..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestGaussianWeight()
{
  int status = 0;
  std::cout << "GaussianWeight..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

class valueDouble2D
{
public:
  double a[2];
  double b[2];
};
class valueFloat2D
{
public:
  float a[2];
  float b[2];
};
int TestDot2D()
{
  int status = 0;
  std::cout << "Dot2D..";

  {
  std::vector<valueDouble2D> values;
  std::vector<double> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    valueDouble2D v;
    double dot = 0.0;
    for (int i = 0; i < 2; ++i)
    {
      v.a[i] = vtkMath::Random();
      v.b[i] = vtkMath::Random();
      dot += (v.a[i] * v.b[i]);
    }
    values.push_back(v);
    expecteds.push_back(dot);
  }
  valueDouble2D test;
  test.a[0] = 1.0; test.a[1] = 0.0;
  test.b[0] = 0.0; test.b[1] = 1.0;
  values.push_back(test);
  expecteds.push_back(0.0);

  for ( size_t i = 0; i < values.size(); ++i)
  {
    double result = vtkMath::Dot2D(values[i].a, values[i].b);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expecteds[i],
          std::numeric_limits<double>::epsilon()*128.0))
    {
      std::cout << " Dot got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }

  // now float
  {
  std::vector<valueFloat2D> values;
  std::vector<float> expecteds;
  for (int n = 0; n < 100; ++n)
  {
    valueFloat2D v;
    float dot = 0.0;
    for (int i = 0; i < 2; ++i)
    {
      v.a[i] = vtkMath::Random();
      v.b[i] = vtkMath::Random();
      dot += (v.a[i] * v.b[i]);
    }
    values.push_back(v);
    expecteds.push_back(dot);
  }
  valueFloat2D test;
  test.a[0] = 0.0; test.a[1] = 1.0;
  test.b[0] = 1.0; test.b[1] = 0.0;
  values.push_back(test);
  expecteds.push_back(0.0);

  for ( size_t i = 0; i < values.size(); ++i)
  {
    float result = vtkMath::Dot2D(values[i].a, values[i].b);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expecteds[i],
          std::numeric_limits<float>::epsilon()*128.0f))
    {
      std::cout << " Dot got " << result
                << " but expected " << expecteds[i];
      ++status;
    }
  }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestNorm2D()
{
  int status = 0;
  std::cout << "Norm2D..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestNormalize2D()
{
  int status = 0;
  std::cout << "Normalize2D..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestDeterminant2x2()
{
  int status = 0;
  std::cout << "Determinant2x2..";
  // Frank Matrix
  {
  double a[2][2];
  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      if (j < i - 2)
      {
        a[i-1][j-1] = 0.0;
      }
      else if (j == (i - 1))
      {
        a[i-1][j-1] = 2 + 1 - i;
      }
      else
      {
        a[i-1][j-1] = 2 + 1 - j;
      }
    }
  }
  if (vtkMath::Determinant2x2(a[0], a[1]) != 1.0)
  {
    std::cout << "Determinant2x2 expected " << 1.0
              << " but got " << vtkMath::Determinant2x2(a[0], a[1])
              << std::endl;
    ++status;
  };
  }
  {
  float a[2][2];
  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      if (j < i - 2)
      {
        a[i-1][j-1] = 0.0;
      }
      else if (j == (i - 1))
      {
        a[i-1][j-1] = 2 + 1 - i;
      }
      else
      {
        a[i-1][j-1] = 2 + 1 - j;
      }
    }
  }
  if (vtkMath::Determinant2x2(a[0], a[1]) != 1.0)
  {
    std::cout << "Determinant2x2 expected " << 1.0
              << " but got " << vtkMath::Determinant2x2(a[0], a[1])
              << std::endl;
    ++status;
  };
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestDeterminant3x3()
{
  int status = 0;
  std::cout << "Determinant3x3..";

  // Frank Matrix
  {
  double a[3][3];
  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      if (j < i - 3)
      {
        a[i-1][j-1] = 0.0;
      }
      else if (j == (i - 1))
      {
        a[i-1][j-1] = 3 + 1 - i;
      }
      else
      {
        a[i-1][j-1] = 3 + 1 - j;
      }
    }
  }
  if (vtkMath::Determinant3x3(a[0], a[1], a[2]) != 1.0)
  {
    std::cout << "Determinant3x3 expected " << 1.0
              << " but got " << vtkMath::Determinant3x3(a[0], a[1], a[2])
              << std::endl;
    ++status;
  };
  }
  {
  float a[3][3];
  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      if (j < i - 3)
      {
        a[i-1][j-1] = 0.0;
      }
      else if (j == (i - 1))
      {
        a[i-1][j-1] = 3 + 1 - i;
      }
      else
      {
        a[i-1][j-1] = 3 + 1 - j;
      }
    }
  }
  if (vtkMath::Determinant3x3(a[0], a[1], a[2]) != 1.0)
  {
    std::cout << "Determinant3x3 expected " << 1.0
              << " but got " << vtkMath::Determinant3x3(a[0], a[1], a[2])
              << std::endl;
    ++status;
  };
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int LUFactor3x3()
{
  int status = 0;

  T A[3][3];
  int index[3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i =0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        A[i][j] = vtkMath::Random(-10.0, 10.0);
      }
    }
    vtkMath::LUFactor3x3(A, index);
  }
  return status;
}

// Just for coverage, validated as part of TestLUSolve3x3
int TestLUFactor3x3()
{
  int status = 0;
  std::cout << "LUFactor3x3..";

  status += LUFactor3x3<double>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}
template<typename T>
int LUSolve3x3()
{
  int status = 0;

  // Generate a Hilbert Matrix
  T mat[3][3];
  int index[3];
  T lhs[3];
  T rhs[3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      lhs[i] = vtkMath::Random(-1.0, 1.0);
    }

    for (int i = 1; i <= 3; ++i)
    {
      rhs[i-1] = 0.0;
      for (int j = 1; j <= 3; ++j)
      {
        mat[i-1][j-1] = 1.0 / (i + j - 1);
        rhs[i-1] += (mat[i-1][j-1] * lhs[j-1]);
      }
    }
    vtkMath::LUFactor3x3(mat, index);
    vtkMath::LUSolve3x3(mat, index, rhs);
    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            lhs[i], rhs[i],
            std::numeric_limits<T>::epsilon() * (T) 256.0))
      {
        std::cout << " LUSolve3x3(T) expected " << lhs[i]
                  << " but got " << rhs[i];
        ++status;
      }
    }
  }
  return status;
}

int TestLUSolve3x3()
{
  int status = 0;
  std::cout << "LUSolve3x3..";

  status += LUSolve3x3<double>();
  status += LUSolve3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int LinearSolve3x3()
{
  int status = 0;

  // Generate a Hilbert Matrix
  T mat[3][3];
  T lhs[3];
  T rhs[3];
  T solution[3];

  for (int n = 0; n < 2; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      lhs[i] = vtkMath::Random(-1.0, 1.0);
    }

    for (int i = 1; i <= 3; ++i)
    {
      rhs[i-1] = 0.0;
      for (int j = 1; j <= 3; ++j)
      {
        mat[i-1][j-1] = 1.0 / (i + j - 1);
        rhs[i-1] += (mat[i-1][j-1] * lhs[j-1]);
      }
    }
    vtkMath::LinearSolve3x3(mat, rhs, solution);

    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            lhs[i], solution[i],
            std::numeric_limits<T>::epsilon() * (T) 512.0))
      {
        std::cout << " LinearSolve3x3(T) expected " << lhs[i]
                  << " but got " << solution[i];
        ++status;
      }
    }
  }
  return status;
}

int TestLinearSolve3x3()
{
  int status = 0;
  std::cout << "LinearSolve3x3..";

  status += LinearSolve3x3<double>();
  status += LinearSolve3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int Multiply3x3()
{
  int status = 0;
  T A[3][3];
  T V[3];
  T U[3];

  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      A[i][j] = vtkMath::Random(-10.0, 10.0);
    }
    V[i] = vtkMath::Random(-10.0, 10.0);
  }

  vtkMath::Multiply3x3(A, V, U);

  return status;
}

int TestMultiply3x3()
{
  int status = 0;
  std::cout << "Multiply3x3..";

  status += Multiply3x3<double>();
  status += Multiply3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// For coverage only. Validated as part of TestInvertMatrix
int TestMultiplyMatrix()
{
  int status = 0;
  std::cout << "MultiplyMatrix..";

  double a[3][3] =
    {{1.0, 2.0, 3.0},
     {4.0, 5.0, 6.0},
     {7.0, 8.0, 9.0}};
  double b[3][3] =
    {{1.0, 1.0, 1.0},
     {1.0, 1.0, 1.0},
     {1.0, 1.0, 1.0}};
  double c[3][3];

  double *aa[3];
  double *bb[3];
  double *cc[3];
  for (int i = 0; i < 3; ++i)
  {
    aa[i] = a[i];
    bb[i] = b[i];
    cc[i] = c[i];
  }
  vtkMath::MultiplyMatrix(aa, bb, 3, 3, 3, 3, cc);

  // WARNING: Number of columns of A must match number of rows of B.
  vtkMath::MultiplyMatrix(aa, bb, 3, 2, 3, 3, cc);

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestTranspose3x3()
{
  int status = 0;
  std::cout << "Transpose3x3..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestInvert3x3()
{
  int status = 0;
  std::cout << "Invert3x3..";
  {
  // Generate a Hilbert Matrix
  double mat[3][3];
  double matI[3][3];
  double expected[3][3] =
    {{9.0, -36.0, 30.0},
     {-36.0, 192.0, -180.0},
     {30.0, -180.0, 180.0}};

  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      mat[i-1][j-1] = 1.0 / (i + j - 1);
    }
  }
  vtkMath::Invert3x3(mat, matI);
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            matI[i][j], expected[i][j],
            std::numeric_limits<double>::epsilon()*16384.0))
      {
        std::cout << " Invert3x3(double) expected " << expected[i][j]
                  << " but got " << matI[i][j];
        ++status;
      }
    }
  }
  }
  {
  // Generate a Hilbert Matrix
  float mat[3][3];
  float matI[3][3];
  float expected[3][3] =
    {{9.0f, -36.0f, 30.0f},
     {-36.0f, 192.0f, -180.0f},
     {30.0f, -180.0f, 180.0f}};

  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      mat[i-1][j-1] = 1.0 / (i + j - 1);
    }
  }
  vtkMath::Invert3x3(mat, matI);
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            matI[i][j], expected[i][j],
            std::numeric_limits<float>::epsilon()*8192.0f))
      {
        std::cout << " Invert3x3(single) expected " << expected[i][j]
                  << " but got " << matI[i][j];
        ++status;
      }
    }
  }
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T, int NDimension>
int InvertMatrix()
{
  int status = 0;

  // Generate a Hilbert Matrix
  T **mat = new T *[NDimension];
  T **orig = new T *[NDimension];
  T **matI = new T *[NDimension];
  T **ident = new T *[NDimension];
  int *tmp1 = new int [NDimension];
  T *tmp2 = new T [NDimension];
  for (int i = 1; i <= NDimension; ++i)
  {
    mat[i-1] = new T[NDimension];
    matI[i-1] = new T[NDimension];
    orig[i-1] = new T[NDimension];
    ident[i-1] = new T[NDimension];
    for (int j = 1; j <= NDimension; ++j)
    {
      orig[i-1][j-1] = mat[i-1][j-1] = 1.0 / (i + j - 1);
    }
  }
  if (vtkMath::InvertMatrix(mat, matI, NDimension, tmp1, tmp2) == 0)
  {
    delete [] mat;
    delete [] orig;
    delete [] matI;
    delete [] ident;
    delete [] tmp1;
    delete [] tmp2;
    return status;
  }
  vtkMath::MultiplyMatrix(orig,
                          matI,
                          NDimension, NDimension, NDimension, NDimension,
                          ident);

  T expected;
  for (int i = 0; i < NDimension; ++i)
  {
    for (int j = 0; j < NDimension; ++j)
    {
      if (i == j)
      {
        expected = 1.0;
      }
      else
      {
        expected = 0.0;
      }
      if (!vtkMathUtilities::FuzzyCompare(
            *(ident[i] + j), expected,
            std::numeric_limits<T>::epsilon() * (T) 100000.0))
      {
        std::cout << " InvertMatrix(T) expected " << expected
                  << " but got " << *(ident[i] + j);
        std::cout << "eps ratio is: " << (*(ident[i] + j) - expected)
          / std::numeric_limits<T>::epsilon() << std::endl;
        ++status;
      }
    }
  }
  for (int i = 0; i < NDimension; i++)
  {
    delete [] mat[i];
    delete [] orig[i];
    delete [] matI[i];
    delete [] ident[i];
  }
  delete [] mat;
  delete [] orig;
  delete [] matI;
  delete [] ident;
  delete [] tmp1;
  delete [] tmp2;
  return status;
}
int TestInvertMatrix()
{
  int status = 0;
  status += InvertMatrix<double,3> ();
  status += InvertMatrix<double,4> ();
  status += InvertMatrix<double,5> ();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestIdentity3x3()
{
  int status = 0;
  std::cout << "Identity3x3..";

  double m[3][3];
  vtkMath::Identity3x3(m);

  double expected;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (i == j)
      {
        expected = 1.0;
      }
      else
      {
        expected = 0.0;
      }
      if (expected != m[i][j])
      {
        std::cout << " Identity expected " << expected
                  << " but got " << m[i][j] << std::endl;
        ++status;
      }
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int Matrix3x3ToQuaternion()
{
  int status = 0;

  T A[3][3];
  T quat[4];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        A[i][j] = vtkMath::Random(-1.0, 1.0);
      }
    }
    vtkMath::Matrix3x3ToQuaternion(A, quat);
  }
  return status;

}
int TestMatrix3x3ToQuaternion()
{
  int status = 0;
  std::cout << "Matrix3x3ToQuaternion..";

  status += Matrix3x3ToQuaternion<double>();
  status += Matrix3x3ToQuaternion<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int QuaternionToMatrix3x3()
{
  int status = 0;

  T A[3][3];
  T quat[4];

  for (int n = 0; n < 1000; ++n)
  {
    quat[0] = vtkMath::Random(-vtkMath::Pi(), vtkMath::Pi());
    for (int i = 1; i < 2; ++i)
    {
      quat[i] = vtkMath::Random(-10.0,10.0);

    }
    vtkMath::QuaternionToMatrix3x3(quat, A);
  }
  return status;

}

int TestQuaternionToMatrix3x3()
{
  int status = 0;
  std::cout << "QuaternionToMatrix3x3..";

  status += QuaternionToMatrix3x3<double>();
  status += QuaternionToMatrix3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int MultiplyQuaternion()
{
  int status = 0;

  T q1[4];
  T q2[4];
  T q3[4];
  for (int n = 0; n < 1000; ++n)
  {
    q1[0] = vtkMath::Random(-vtkMath::Pi(), vtkMath::Pi());
    q2[0] = vtkMath::Random(-vtkMath::Pi(), vtkMath::Pi());
    vtkMath::MultiplyQuaternion(q1, q2, q3);
  }

  return status;
}
int TestMultiplyQuaternion()
{
  int status = 0;
  std::cout << "MultiplyQuaternion..";

  status += MultiplyQuaternion<double>();
  status += MultiplyQuaternion<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestOrthogonalize3x3()
{
  int status = 0;
  std::cout << "Orthogonalize3x3..";

  {
  // Generate a random matrix
  double mat[3][3];
  double matO[3][3];
  double matI[3][3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        mat[i][j] = vtkMath::Random();
      }
    }
    vtkMath::Orthogonalize3x3(mat, matO);
    vtkMath::Transpose3x3(matO, mat);
    vtkMath::Multiply3x3(mat, matO, matI);

    double identity[3][3];
    vtkMath::Identity3x3(identity);
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              matI[i][j], identity[i][j],
              std::numeric_limits<double>::epsilon()*128.0))
        {
          std::cout << " Orthogonalize3x3 expected " << identity[i][j]
                    << " but got " << matI[i][j];
          ++status;
        }
      }
    }
  }
  }

  {
  // Generate a random matrix
  float mat[3][3];
  float matO[3][3];
  float matI[3][3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        mat[i][j] = vtkMath::Random();
      }
    }
    vtkMath::Orthogonalize3x3(mat, matO);
    vtkMath::Transpose3x3(matO, mat);
    vtkMath::Multiply3x3(mat, matO, matI);

    float identity[3][3];
    vtkMath::Identity3x3(identity);
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              matI[i][j], identity[i][j],
              std::numeric_limits<float>::epsilon()*128.0f))
        {
          std::cout << " Orthogonalize3x3 expected " << identity[i][j]
                    << " but got " << matI[i][j];
          ++status;
        }
      }
    }
  }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int Diagonalize3x3()
{
  int status = 0;

  T mat[3][3];
  T eigenVector[3][3], eigenVectorT[3][3];
  T temp[3][3];
  T result[3][3];
  T eigen[3];

  for (int n = 0; n < 0; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = i; j < 3; ++j)
      {
        mat[i][j] = mat[j][i] = vtkMath::Random(-1.0, 1.0);
      }
    }

    vtkMath::Diagonalize3x3(mat, eigen, eigenVector);

    // Pt * A * P = diagonal matrix with eigenvalues on diagonal
    vtkMath::Multiply3x3(mat, eigenVector, temp);
    vtkMath::Invert3x3(eigenVector, eigenVectorT);
    vtkMath::Multiply3x3(eigenVectorT, temp, result);
    T expected;
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        if (i == j)
        {
          expected = eigen[i];
        }
        else
        {
          expected = 0.0;
        }
        if (!vtkMathUtilities::FuzzyCompare(
              result[i][j], expected,
              std::numeric_limits<T>::epsilon()*(T)128.0))
        {
          std::cout << " Diagonalize3x3 expected " << expected
                    << " but got " << result[i][j];
          ++status;
        }
      }
    }
  }

  // Now test for 2 and 3 equal eigenvalues
  vtkMath::Identity3x3(mat);
  mat[0][0] = 5.0;
  mat[1][1] = 5.0;
  mat[1][1] = 1.0;

  vtkMath::Diagonalize3x3(mat, eigen, eigenVector);
  std::cout << "eigen: "
            << eigen[0] << ","
            << eigen[1] << ","
            << eigen[2] << std::endl;

  vtkMath::Identity3x3(mat);
  mat[0][0] = 2.0;
  mat[1][1] = 2.0;
  mat[1][1] = 2.0;

  vtkMath::Diagonalize3x3(mat, eigen, eigenVector);
  std::cout << "eigen: "
            << eigen[0] << ","
            << eigen[1] << ","
            << eigen[2] << std::endl;
  return status;
}

// Validate Pt * A * P = diagonal matrix with eigenvalues on diagonal
int TestDiagonalize3x3()
{
  int status = 0;
  std::cout << "Diagonalize3x3..";

  status += Diagonalize3x3<double>();
  status += Diagonalize3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int SingularValueDecomposition3x3()
{
  int status = 0;

  T a[3][3];
  T orig[3][3];
  T u[3][3];
  T w[3];
  T vt[3][3];

  for (int n = 0; n < 1000; ++n)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        orig[i][j] = a[i][j] = vtkMath::Random(-10.0, 10.0);
      }
    }
    vtkMath::SingularValueDecomposition3x3(a, u, w, vt);

    T m[3][3];
    T W[3][3];
    vtkMath::Identity3x3(W);
    W[0][0] = w[0]; W[1][1] = w[1]; W[2][2] = w[2];
    vtkMath::Multiply3x3(u, W, m);
    vtkMath::Multiply3x3(m, vt, m);

    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              m[i][j], orig[i][j],
              std::numeric_limits<T>::epsilon() * (T) 128.0))
        {
          std::cout << " SingularValueDecomposition3x3 expected " << orig[i][j]
                    << " but got " << m[i][j];
          std::cout << " eps ratio is: " << (m[i][j] - orig[i][j])
            / std::numeric_limits<T>::epsilon() << std::endl;
          ++status;
        }
      }
    }
  }
  return status;
}
// Validate u * w * vt = m
int TestSingularValueDecomposition3x3()
{
  int status = 0;
  std::cout << "SingularValueDecomposition3x3..";

  status += SingularValueDecomposition3x3<double>();
  status += SingularValueDecomposition3x3<float>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}


template<typename T, int NDimension>
int SolveLinearSystem()
{
  int status = 0;

  for (int n = 0; n < 100; ++n)
  {
    // Generate a Random Matrix
    T **mat = new T *[NDimension];
    T *lhs = new T [NDimension];
    T *rhs = new T [NDimension];

    for (int i = 0; i < NDimension; ++i)
    {
      mat[i] = new T[NDimension];
      lhs[i] =vtkMath::Random(-1.0, 1.0);
      for (int j = 0; j < NDimension; ++j)
      {
        *(mat[i] + j) = vtkMath::Random(-1.0, 1.0);
      }
    }

    for (int i = 0; i < NDimension; ++i)
    {
      rhs[i] = 0.0;
      for (int j = 0; j < NDimension; ++j)
      {
        rhs[i] += (*(mat[i] + j) * lhs[j]);
      }
    }
    vtkMath::SolveLinearSystem(mat, rhs, NDimension);

    for (int i = 0; i < NDimension; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            lhs[i], rhs[i],
            std::numeric_limits<double>::epsilon()*32768.0))
      {
        std::cout << " SolveLinearSystem(double) expected " << lhs[i]
                  << " but got " << rhs[i];
        std::cout << " eps ratio is: " << (lhs[i] - rhs[i])
            / std::numeric_limits<T>::epsilon() << std::endl;
        ++status;
      }
    }

    if (NDimension == 1 || NDimension == 2)
    {
      for (int i = 0; i < NDimension; ++i)
      {
        for (int j = 0; j < NDimension; ++j)
        {
          *(mat[i] + j) = 0.0;
        }
      }
      if (vtkMath::SolveLinearSystem(mat, rhs, NDimension) != 0.0)
      {
        std::cout << " SolveLinearSystem for a zero matrix expected " << 0
                  << " but got 1" ;
      }
    }
    for (int i = 0; i < NDimension; i++)
    {
      delete [] mat[i];
    }
    delete [] mat;
    delete [] rhs;
    delete [] lhs;
  }
  return status;
}

// Validate with a known left hand side
int TestSolveLinearSystem()
{
  int status = 0;
  std::cout << "SolveLinearSystem..";

  status += SolveLinearSystem<double,1> ();
  status += SolveLinearSystem<double,2> ();
  status += SolveLinearSystem<double,3> ();
  status += SolveLinearSystem<double,50> ();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with a known solution
int TestSolveLeastSquares()
{
  int status = 0;
  std::cout << "SolveLeastSquares..";

  double **m = new double *[2];
  double **x = new double *[3];
  double **y = new double *[3];

  for (int i = 0; i < 2; ++i)
  {
    m[i] = new double[1];
  }
  for (int i = 0; i < 3; ++i)
  {
    x[i] = new double[2];
  }
  x[0][0] = 1; x[0][1] = 4;
  x[1][0] = 1; x[1][1] = 2;
  x[2][0] = 2; x[2][1] = 3;

  for (int i = 0; i < 3; ++i)
  {
    y[i] = new double[1];
  }
  y[0][0] = -2;
  y[1][0] = 6;
  y[2][0] = 1;

  vtkMath::SolveLeastSquares(3, x, 2, y, 1, m);

  std::vector<double> results;
  std::vector<double> expecteds;
  expecteds.push_back(3.0);
  results.push_back(m[0][0]);
  expecteds.push_back(-1.0);
  results.push_back(m[1][0]);

  for ( size_t i = 0; i < results.size(); ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(
          results[i], expecteds[i],
          std::numeric_limits<double>::epsilon()*128.0))
    {
      std::cout << " Solve Least Squares got " << results[i]
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  // Now make one solution homogeous
  y[0][0] = 0.0;
  vtkMath::SolveLeastSquares(3, x, 2, y, 1, m);

  // Now make all homogeous
  y[0][0] = 0.0;
  y[1][0] = 0.0;
  y[2][0] = 0.0;
  vtkMath::SolveLeastSquares(3, x, 2, y, 1, m);

  // Insufficient number of samples. Underdetermined.
  if (vtkMath::SolveLeastSquares(1, x, 2, y, 1, m) != 0)
  {
    std::cout << " Solve Least Squares got " << 1
              << " but expected " << 0;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }

  for (int i = 0; i < 3; i++)
  {
    delete [] x[i];
    delete [] y[i];
  }
  for (int i = 0; i < 2; i++)
  {
    delete [] m[i];
  }
  delete [] x;
  delete [] y;
  delete [] m;

  return status;
}

// Only warning cases validate
// No validation, just coverage
int TestSolveHomogeneousLeastSquares()
{
  int status = 0;
  std::cout << "SolveHomogenousLeastSquares..";

  double **m = new double *[2];
  double **x = new double *[3];
  double **y = new double *[3];

  for (int i = 0; i < 2; ++i)
  {
    m[i] = new double[1];
  }
  for (int i = 0; i < 3; ++i)
  {
    x[i] = new double[2];
    y[i] = new double[1];
  }
  x[0][0] = 1; x[0][1] = 2;
  x[1][0] = 2; x[1][1] = 4;
  x[2][0] = 3; x[2][1] = 6;

  vtkMath::SolveHomogeneousLeastSquares(3, x, 1, m);

  vtkMath::MultiplyMatrix(x, m, 3, 2, 2, 1, y);

  std::vector<double> results;
  std::vector<double> expecteds;

  for ( size_t i = 0; i < results.size(); ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(
          results[i], expecteds[i],
          std::numeric_limits<double>::epsilon()*128.0))
    {
      std::cout << " SolveHomogeneousLeastSquares got " << results[i]
                << " but expected " << expecteds[i];
      ++status;
    }
  }

  // Insufficient number of samples. Underdetermined.
  if (vtkMath::SolveHomogeneousLeastSquares(3, x, 5, m) != 0)
  {
    std::cout << " SolveHomogeneousLeastSquares got " << 1
              << " but expected " << 0;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }

  for (int i = 0; i < 3; i++)
  {
    delete [] x[i];
    delete [] y[i];
  }
  for (int i = 0; i < 2; i++)
  {
    delete [] m[i];
  }
  delete [] x;
  delete [] y;
  delete [] m;

  return status;
}

template<typename T, int NDimension>
int LUSolveLinearSystemEstimateMatrixCondition()
{
  int status = 0;

  // Generate a Hilbert Matrix
  T **mat = new T *[NDimension];
  int index[NDimension];

  for (int i = 1; i <= NDimension; ++i)
  {
    mat[i-1] = new T[NDimension];
    for (int j = 1; j <= NDimension; ++j)
    {
      mat[i-1][j-1] = 1.0 / (i + j - 1);
    }
  }
  vtkMath::LUFactorLinearSystem(mat, index, NDimension);
  T condition = vtkMath::EstimateMatrixCondition(mat, NDimension);
  std::cout << "Condition is: " << condition << std::endl;

  T expected = condition;
  if (!vtkMathUtilities::FuzzyCompare(
        condition, expected,
        std::numeric_limits<T>::epsilon() * (T) 128.0))
  {
    std::cout << " EstimateMatrixCondition(T) expected " << expected
              << " but got " << condition;
    std::cout << "eps ratio is: " << condition - expected
      / std::numeric_limits<T>::epsilon() << std::endl;
    ++status;
  }
  for (int i = 0; i < NDimension; i++)
  {
    delete [] mat[i];
  }
  delete [] mat;
  return status;
}

// Validate by obervation that the condition of a hilbert matrix
// increases with dimension
int TestLUSolveLinearSystemEstimateMatrixCondition()
{
  int status = 0;
  std::cout << "LUSolveLinearSystemEstimateMatrixCondition..";
  status += LUSolveLinearSystemEstimateMatrixCondition<double,10>();
  status += LUSolveLinearSystemEstimateMatrixCondition<double,8>();
  status += LUSolveLinearSystemEstimateMatrixCondition<double,6>();
  status += LUSolveLinearSystemEstimateMatrixCondition<double,4>();
  status += LUSolveLinearSystemEstimateMatrixCondition<double,3>();
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T, int NDimension>
int JacobiN()
{
  int status = 0;

  T mat[NDimension][NDimension];
  T orig[NDimension][NDimension];
  T eigenVector[NDimension][NDimension], eigenVectorT[NDimension][NDimension];
  T temp[NDimension][NDimension];
  T result[NDimension][NDimension];
  T eigen[NDimension];

  for (int n = 0; n < 10; ++n)
  {
    for (int i = 0; i < NDimension; ++i)
    {
      for (int j = i; j < NDimension; ++j)
      {
        mat[i][j] = mat[j][i] = vtkMath::Random(0.0, 1.0);
        orig[i][j] = orig[j][i] = mat[i][j];
      }
    }

    // convert to jacobiN format
    T *origJ[NDimension];
    T *matJ[NDimension];
    T *eigenVectorJ[NDimension];
    T *eigenVectorTJ[NDimension];
    T *resultJ[NDimension];
    T *tempJ[NDimension];
    for (int i = 0; i < NDimension; ++i)
    {
      matJ[i] = mat[i];
      origJ[i] = orig[i];
      eigenVectorJ[i] = eigenVector[i];
      eigenVectorTJ[i] = eigenVectorT[i];
      tempJ[i] = temp[i];
      resultJ[i] = result[i];
    }

    if (NDimension == 3)
    {
      vtkMath::Jacobi(matJ,
                       eigen,
                       eigenVectorJ);
    }
    else
    {
      vtkMath::JacobiN(matJ,
                       NDimension,
                       eigen,
                       eigenVectorJ);
    }

    // P^-1 * A * P = diagonal matrix with eigenvalues on diagonal
    vtkMath::MultiplyMatrix(origJ,
                            eigenVectorJ,
                            NDimension, NDimension, NDimension, NDimension,
                            tempJ);
    vtkMath::InvertMatrix(eigenVectorJ, eigenVectorTJ, NDimension);
    vtkMath::MultiplyMatrix(eigenVectorTJ,
                            tempJ,
                            NDimension, NDimension, NDimension, NDimension,
                            resultJ);
    T expected;
    for (int i = 0; i < NDimension; ++i)
    {
      for (int j = 0; j < NDimension; ++j)
      {
        if (i == j)
        {
          expected = eigen[i];
        }
        else
        {
          expected = 0.0;
        }
        if (!vtkMathUtilities::FuzzyCompare(
              result[i][j], expected,
              std::numeric_limits<T>::epsilon() * (T) 256.0))
        {
          std::cout << " JacobiN expected " << expected
                    << " but got " << result[i][j];
          std::cout << "eps ratio is: " << (result[i][j] - expected)
            / std::numeric_limits<T>::epsilon() << std::endl;
          ++status;
        }
      }
    }
  }
  return status;
}

// Validate P^-1 * A * P = diagonal matrix with eigenvalues on diagonal
int TestJacobiN()
{
  int status = 0;
  std::cout << "JacobiN..";
  status += JacobiN<double,3>();
  status += JacobiN<double,10>();
  status += JacobiN<double,50>();

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

template<typename T>
int RGBToHSV()
{
  int status = 0;
  T R, G, B;
  T H, S, V;
  T CR, CG, CB;
  for (int n = 0; n < 1000; ++n)
  {
    std::vector<T> values;
    std::vector<T> expecteds;
    R = (T) vtkMath::Random(0.0, 1.0);
    G = (T) vtkMath::Random(0.0, 1.0);
    B = (T) vtkMath::Random(0.0, 1.0);

    vtkMath::RGBToHSV(R, G, B, &H, &S, &V);
    vtkMath::HSVToRGB(H, S, V, &CR, &CG, &CB);
    values.push_back(CR); values.push_back(CG); values.push_back(CB);
    expecteds.push_back(R); expecteds.push_back(G); expecteds.push_back(B);

    for ( size_t i = 0; i < values.size(); ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            values[i], expecteds[i],
            std::numeric_limits<T>::epsilon() * (T) 128.0))
      {
        std::cout << " RGBToHSV got " << values[i]
                  << " but expected " << expecteds[i];
        std::cout << " eps ratio is: " << (values[i] - expecteds[i])
          / std::numeric_limits<T>::epsilon() << std::endl;
        ++status;
      }
    }
  }
  return status;
}

// Validate by rgb->hsv->rgb
int TestRGBToHSV()
{
  int status = 0;
  std::cout << "RGBToHSV..";

  status += RGBToHSV<double>();
  status += RGBToHSV<float>();
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestClampValue()
{
  int status = 0;
  std::cout << "ClampValue..";

  double value;
  double clampedValue;
  double range[2] = {-1.0, 1.0};

  value = -800.0;
  clampedValue = vtkMath::ClampValue(value, range[0], range[1]);
  if (clampedValue != range[0])
  {
    std::cout << " ClampValue expected " << range[0]
              << " but got " << value;
    ++status;
  }

  value = 900.0;
  clampedValue = vtkMath::ClampValue(value, range[0], range[1]);
  if (clampedValue != range[1])
  {
    std::cout << " ClampValue expected " << range[1]
              << " but got " << value;
    ++status;
  }

  value = 0.0;
  clampedValue = vtkMath::ClampValue(value, range[0], range[1]);
  if (clampedValue != 0.0)
  {
    std::cout << " ClampValue expected " << 0.0
              << " but got " << value;
    ++status;
  }


  value = -100.0;
  vtkMath::ClampValue(&value, range);
  if (value != range[0])
  {
    std::cout << " ClampValue expected " << range[0]
              << " but got " << value;
    ++status;
  }
  value = 100.0;
  vtkMath::ClampValue(&value, range);
  if (value != range[1])
  {
    std::cout << " ClampValue expected " << range[1]
              << " but got " << value;
    ++status;
  }
  value = -100.0;
  vtkMath::ClampValue(value, range, &clampedValue);
  if (clampedValue != range[0])
  {
    std::cout << " ClampValue expected " << range[0]
              << " but got " << clampedValue;
    ++status;
  }

  value = 100.0;
  vtkMath::ClampValue(value, range, &clampedValue);
  if (clampedValue != range[1])
  {
    std::cout << " ClampValue expected " << range[1]
              << " but got " << clampedValue;
    ++status;
  }

  value = 0.0;
  vtkMath::ClampValue(value, range, &clampedValue);
  if (clampedValue != value)
  {
    std::cout << " ClampValue expected " << value
              << " but got " << clampedValue;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestClampValues()
{
  int status = 0;
  std::cout << "ClampValues..";

  double values[1000];
  double clampedValues[1000];
  for (int n = 0; n < 1000; ++n)
  {
    values[n] = vtkMath::Random(-2.0, 2.0);
  }
  double range[2] = {-1.0, 1.0};
  vtkMath::ClampValues( values, 1000, range, clampedValues);
  vtkMath::ClampValues( values, 1000, range);

  for (int n = 0; n < 1000; ++n)
  {
    if (values[n] != clampedValues[n])
    {
      ++status;
    }
  }

  vtkMath::ClampValues( NULL, 1000, NULL);
  vtkMath::ClampValues( NULL, 1000, NULL, NULL);;

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestClampAndNormalizeValue()
{
  int status = 0;
  std::cout << "ClampAndNormalizeValue..";

  double value;
  double result;
  double range[3] = {-1.0, 1.0};

  value = -100.0;
  result = vtkMath::ClampAndNormalizeValue(value, range);
  if (result != 0.0)
  {
    std::cout << " ClampAndNormalizeValue expected " << 0.0
              << " but got " << result;
    ++status;
  }
  value = 100.0;
  result = vtkMath::ClampAndNormalizeValue(value, range);
  if (result != 1.0)
  {
    std::cout << " ClampAndNormalizeValue expected " << 1.0
              << " but got " << result;
    ++status;
  }

  range[0] = 0.0; range[1] = 1.0;
  value = 0.5;
  result = vtkMath::ClampAndNormalizeValue(value, range);
  if (result != 0.5)
  {
    std::cout << " ClampValue expected " << 0.5
              << " but got " << result;
    ++status;
  }

  range[0] = 1.0; range[1] = 1.0;
  value = 1.0;
  result = vtkMath::ClampAndNormalizeValue(value, range);
  if (result != 0.0)
  {
    std::cout << " ClampValue expected " << 0.0
              << " but got " << result;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate by checking ranges with numeric_limits
int TestGetScalarTypeFittingRange()
{
  int status = 0;
  std::cout << "GetScalarTypeFittingRange..";

  double rangeMin;
  double rangeMax;

  rangeMin = (double) std::numeric_limits<char>::min();
  rangeMax = (double) std::numeric_limits<char>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_CHAR)
  {
    std::cout << " Bad fitting range for VTK_CHAR" << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<unsigned char>::min();
  rangeMax = (double) std::numeric_limits<unsigned char>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_UNSIGNED_CHAR)
  {
    std::cout << " Bad fitting range for VTK_UNSIGNED_CHAR " << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<short>::min();
  rangeMax = (double) std::numeric_limits<short>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_SHORT)
  {
    std::cout << " Bad fitting range for VTK_SHORT" << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<unsigned short>::min();
  rangeMax = (double) std::numeric_limits<unsigned short>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_UNSIGNED_SHORT)
  {
    std::cout << " Bad fitting range for VTK_UNSIGNED_SHORT" << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<int>::min();
  rangeMax = (double) std::numeric_limits<int>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_INT)
  {
    std::cout << " Bad fitting range for VTK_INT" << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<unsigned int>::min();
  rangeMax = (double) std::numeric_limits<unsigned int>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.0, 0.0) != VTK_UNSIGNED_INT)
  {
    std::cout << " Bad fitting range for VTK_UNSIGNED_INT" << std::endl;
    ++status;
  }

  rangeMin = (double) std::numeric_limits<long>::min();
  rangeMax = (double) std::numeric_limits<long>::max();
  int scalarType = vtkMath::GetScalarTypeFittingRange(
    rangeMin, rangeMax, 1.0, 0.0);
  if (sizeof(long) == sizeof(int))
  {
    if (scalarType != VTK_INT)
    {
      std::cout << " Bad fitting range for VTK_LONG" << std::endl;
      std::cout << " Expected " << VTK_INT << " but got " << scalarType;
      ++status;
    }
  }
  else
  {
    if (scalarType != VTK_LONG)
    {
      std::cout << " Bad fitting range for VTK_LONG" << std::endl;
      std::cout << " Expected " << VTK_LONG << " but got " << scalarType;
      ++status;
    }
  }

  rangeMin = (double) std::numeric_limits<unsigned long>::min();
  rangeMax = (double) std::numeric_limits<unsigned long>::max();
  scalarType = vtkMath::GetScalarTypeFittingRange(
    rangeMin, rangeMax, 1.0, 0.0);
  if (sizeof(unsigned long) == sizeof(unsigned int))
  {
    if (scalarType != VTK_UNSIGNED_INT)
    {
      std::cout << " Bad fitting range for VTK_UNSIGNED_LONG" << std::endl;
      std::cout << " Expected " << VTK_UNSIGNED_INT << " but got " << scalarType;
      ++status;
    }
  }
  else
  {
    if (scalarType != VTK_UNSIGNED_LONG)
    {
      std::cout << " Bad fitting range for VTK_UNSIGNED_LONG" << std::endl;
      std::cout << " Expected " << VTK_UNSIGNED_LONG << " but got " << scalarType;
      ++status;
    }
  }

  rangeMin = (double) std::numeric_limits<short>::min();
  rangeMax = (double) std::numeric_limits<short>::max();
  if (vtkMath::GetScalarTypeFittingRange(
        rangeMin, rangeMax, 1.1, 0.0) != VTK_FLOAT)
  {
    std::cout << " Bad fitting range for VTK_FLOAT" << std::endl;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestGetAdjustedScalarRange()
{
  int status = 0;
  std::cout << "GetAdjustedScalarRange..";

  vtkSmartPointer<vtkUnsignedCharArray> uc =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  uc->SetNumberOfComponents(3);
  uc->SetNumberOfTuples(100);
  for (int i = 0; i < 100; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      uc->SetComponent(i, j, vtkMath::Random(
                         std::numeric_limits<unsigned char>::min(),
                         std::numeric_limits<unsigned char>::max()));
    }
  }
  double range[2];
  vtkMath::GetAdjustedScalarRange(uc, 1, range);
  if (range[0] != uc->GetDataTypeMin() ||
      range[1] != uc->GetDataTypeMax())
  {
    std::cout << " GetAjustedScalarRange(unsigned char) expected "
              << uc->GetDataTypeMin() << ", " << uc->GetDataTypeMax()
              << " but got " << range[0] << ", " << range[1]
              << std::endl;
    ++status;
  }

  vtkSmartPointer<vtkUnsignedShortArray> us =
    vtkSmartPointer<vtkUnsignedShortArray>::New();
  us->SetNumberOfComponents(3);
  us->SetNumberOfTuples(10000);
  for (int i = 0; i < 10000; ++i)
  {
    us->SetComponent(i, 0, vtkMath::Random(
                       std::numeric_limits<unsigned short>::min(),
                       std::numeric_limits<unsigned short>::max()));
    us->SetComponent(i, 1, vtkMath::Random(
                       std::numeric_limits<unsigned short>::min(),
                       std::numeric_limits<unsigned char>::max() + 100));
    us->SetComponent(i, 2, vtkMath::Random(
                       std::numeric_limits<unsigned short>::min(),
                       std::numeric_limits<unsigned char>::max()));
  }
  vtkMath::GetAdjustedScalarRange(us, 0, range);
  if (range[0] != us->GetDataTypeMin() ||
      range[1] != us->GetDataTypeMax())
  {
    std::cout << " GetAjustedScalarRange(unsigned short) expected "
              << us->GetDataTypeMin() << ", " << us->GetDataTypeMax()
              << " but got " << range[0] << ", " << range[1]
              << std::endl;
    ++status;
  }

  vtkMath::GetAdjustedScalarRange(us, 1, range);
  if (range[0] != us->GetDataTypeMin() ||
      range[1] != 4095.0)
  {
    std::cout << " GetAjustedScalarRange(unsigned short) expected "
              << us->GetDataTypeMin() << ", " << 4095.0
              << " but got " << range[0] << ", " << range[1]
              << std::endl;
    ++status;
  }

  vtkMath::GetAdjustedScalarRange(us, 2, range);
  if (range[0] != us->GetDataTypeMin() ||
      range[1] >= uc->GetDataTypeMax())
  {
    std::cout << " GetAjustedScalarRange(unsigned short) expected "
              << us->GetDataTypeMin() << ", " << ">= " << uc->GetDataTypeMax()
              << " but got " << range[0] << ", " << range[1]
              << std::endl;
    ++status;
  }

  // Test NULL array
  if (vtkMath::GetAdjustedScalarRange(NULL, 1000, NULL))
  {
    std::cout << " GetAjustedScalarRange with a NULL array expected "
              << 0
              << " but got " << 1
              << std::endl;
    ++status;
  }
  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestExtentIsWithinOtherExtent()
{
  int status = 0;
  std::cout << "ExtentIsWithinOtherExtent..";

  if (vtkMath::ExtentIsWithinOtherExtent(NULL, NULL))
  {
    std::cout << " ExtentIsWithinOtherExtent expected 0 but got 1"
              << std::endl;
    ++status;
  }

  int extent1[6];
  int extent2[6];
  extent1[0] = 100; extent1[1] = 101;
  extent1[2] = 100; extent1[3] = 101;
  extent1[4] = 100; extent1[5] = 101;

  extent2[0] = 100; extent2[1] = 101;
  extent2[2] = 100; extent2[3] = 101;
  extent2[4] = 100; extent2[5] = 101;

  if (!vtkMath::ExtentIsWithinOtherExtent(extent1, extent2))
  {
    std::cout << " ExtentIsWithinOtherExtent expected 1 but got 0"
              << std::endl;
    ++status;
  }

  extent1[0] = 99; extent1[1] = 101;
  if (vtkMath::ExtentIsWithinOtherExtent(extent1, extent2))
  {
    std::cout << " ExtentIsWithinOtherExtent expected 0 but got 1"
              << std::endl;
    ++status;
  }

  extent1[0] = 98; extent1[1] = 99;
  if (vtkMath::ExtentIsWithinOtherExtent(extent1, extent2))
  {
    std::cout << " ExtentIsWithinOtherExtent expected 0 but got 1"
              << std::endl;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestBoundsIsWithinOtherBounds()
{
  int status = 0;
  std::cout << "BoundsIsWithinOtherBounds..";

  if (vtkMath::BoundsIsWithinOtherBounds(NULL, NULL, NULL))
  {
    std::cout << " BoundsIsWithinOtherBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  double delta[3];
  delta[0] = delta[1] = delta[2] = std::numeric_limits<double>::epsilon();

  double bounds1[6];
  double bounds2[6];
  bounds1[0] = 1 - delta[0]; bounds1[1] = 2 + delta[0];
  bounds1[2] = 1; bounds1[3] = 2;
  bounds1[4] = 1; bounds1[5] = 2;

  bounds2[0] = 1; bounds2[1] = 2;
  bounds2[2] = 1; bounds2[3] = 2;
  bounds2[4] = 1; bounds2[5] = 2;

  if (!vtkMath::BoundsIsWithinOtherBounds(bounds1, bounds2, delta))
  {
    std::cout << " BoundsIsWithinOtherBounds expected 1 but got 0"
              << std::endl;
    ++status;
  }

  bounds1[0] = 1 - 2.0 * delta[0]; bounds1[1] = 2 + 2.0 * delta[0];
  if (vtkMath::BoundsIsWithinOtherBounds(bounds1, bounds2, delta))
  {
    std::cout << " BoundsIsWithinOtherBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  bounds1[0] = 1 - 4.0 * delta[0]; bounds1[1] = 1 - 2.0 * delta[0];
  if (vtkMath::BoundsIsWithinOtherBounds(bounds1, bounds2, delta))
  {
    std::cout << " BoundsIsWithinOtherBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with known solutions
int TestPointIsWithinBounds()
{
  int status = 0;
  std::cout << "PointIsWithinBounds..";

  if (vtkMath::PointIsWithinBounds(NULL, NULL, NULL))
  {
    std::cout << " PointIsWithinBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  double delta[3];
  delta[0] = std::numeric_limits<double>::epsilon();
  delta[1] = std::numeric_limits<double>::epsilon() * 2.0;
  delta[2] = std::numeric_limits<double>::epsilon() * 256.0;

  double bounds1[6];
  bounds1[0] = 1.0; bounds1[1] = 2.0;
  bounds1[2] = 1.0; bounds1[3] = 2.0;
  bounds1[4] = 1.0; bounds1[5] = 2.0;

  double point[3];
  point[0] = bounds1[0] - delta[0];
  point[1] = bounds1[2] - delta[1];
  point[2] = bounds1[4];

  if (!vtkMath::PointIsWithinBounds(point, bounds1, delta))
  {
    std::cout << " PointIsWithinBounds expected 1 but got 0"
              << std::endl;
    ++status;
  }

  point[0] = bounds1[0] - delta[0];
  point[1] = bounds1[2] - delta[1];
  point[2] = bounds1[4] - 2.0 * delta[2];

  if (vtkMath::PointIsWithinBounds(point, bounds1, delta))
  {
    std::cout << " PointIsWithinBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  point[0] = bounds1[1] + delta[0];
  point[1] = bounds1[3] + delta[1];
  point[2] = bounds1[5] + 2.0 * delta[2];

  if (vtkMath::PointIsWithinBounds(point, bounds1, delta))
  {
    std::cout << " PointIsOtherBounds expected 0 but got 1"
              << std::endl;
    ++status;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Validate with with alternative solution
int TestSolve3PointCircle()
{
  int status = 0;
  std::cout << "Solve3PointCircle..";

  for (int n = 0; n < 1000; ++n)
  {
    double A[3], B[3], C[3];
    double center[3];
    double a[3], b[3], aMinusb[3], aCrossb[3];

    for (int i = 0; i < 3; ++i)
    {
      A[i] = vtkMath::Random(-1.0, 1.0);
      B[i] = vtkMath::Random(-1.0, 1.0);
      C[i] = vtkMath::Random(-1.0, 1.0);
    }

    vtkMath::Subtract(A, C, a);
    vtkMath::Subtract(B, C, b);
    vtkMath::Subtract(a, b, aMinusb);
    vtkMath::Cross(a, b, aCrossb);

    double expectedRadius;
    expectedRadius =
      (vtkMath::Norm(a) * vtkMath::Norm(b) * vtkMath::Norm(aMinusb)) /
      (2.0 * vtkMath::Norm(aCrossb));

    double radius;
    radius = vtkMath::Solve3PointCircle(A, B, C, center);
    if (!vtkMathUtilities::FuzzyCompare(
          radius, expectedRadius,
          std::numeric_limits<double>::epsilon()*1024.0))
    {
      std::cout << " Solve3PointCircle radius expected " << expectedRadius
                << " but got " << radius;
      std::cout << "eps ratio is: " << (expectedRadius - radius)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status;
    }

    double ab[3], ba[3];
    double abMinusba[3];
    double abMinusbaCrossaCrossb[3];

    vtkMath::Subtract(B, C, ab);
    vtkMath::Subtract(A, C, ba);
    vtkMath::MultiplyScalar(ab, vtkMath::Norm(a) * vtkMath::Norm(a));
    vtkMath::MultiplyScalar(ba, vtkMath::Norm(b) * vtkMath::Norm(b));
    vtkMath::Subtract(ab, ba, abMinusba);
    vtkMath::Cross(abMinusba, aCrossb, abMinusbaCrossaCrossb);

    double expectedCenter[3];
    vtkMath::MultiplyScalar(
      abMinusbaCrossaCrossb,
      1.0 / (2.0 * vtkMath::Norm(aCrossb) * vtkMath::Norm(aCrossb)));
    vtkMath::Add(abMinusbaCrossaCrossb, C, expectedCenter);

    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            center[i], expectedCenter[i],
          std::numeric_limits<double>::epsilon()*1024.0))
      {
        std::cout << " Solve3PointCircle center expected " << expectedCenter[i]
                  << " but got " << center[i];
        std::cout << "eps ratio is: " << (expectedCenter[i] - center[i])
          / std::numeric_limits<double>::epsilon() << std::endl;
        ++status;
      }
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Tested by TestMath
int TestInf()
{
  int status = 0;
  std::cout << "Inf..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Tested by TestMath
int TestNegInf()
{
  int status = 0;
  std::cout << "NegInf..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

// Tested by TestMath
int TestNan()
{
  int status = 0;
  std::cout << "Nan..";

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}
