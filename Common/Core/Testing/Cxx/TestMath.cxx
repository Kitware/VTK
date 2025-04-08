// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2005 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkMath.h"
#include "vtkMathConfigure.h"

#include <array>
#include <cmath>
#include <limits>

template <class A>
bool fuzzyCompare1DWeak(A a, A b)
{
  return std::abs(a - b) < .0001;
}

template <class A>
bool fuzzyCompare1D(A a, A b)
{
  return std::abs(a - b) < std::numeric_limits<A>::epsilon();
}

template <class A>
bool fuzzyCompare2D(A a[2], A b[2])
{
  return fuzzyCompare1D(a[0], b[0]) && fuzzyCompare1D(a[1], b[1]);
}

template <class A>
bool fuzzyCompare3D(A a[3], A b[3])
{
  return fuzzyCompare1D(a[0], b[0]) && fuzzyCompare1D(a[1], b[1]) && fuzzyCompare1D(a[2], b[2]);
}

template <class A>
bool fuzzyCompareNDWeak(const A& a, const A& b, int size)
{
  for (int i = 0; i < size; i++)
  {
    if (!fuzzyCompare1DWeak(a[i], b[i]))
    {
      return false;
    }
  }
  return true;
}

//=============================================================================
// Helpful class for storing and using color triples.
class Triple
{
public:
  Triple() = default;
  Triple(double a, double b, double c)
  {
    data[0] = a;
    data[1] = b;
    data[2] = c;
  }
  const double* operator()() const { return data; }
  double* operator()() { return data; }
  const double& operator[](int i) const { return data[i]; }
  double& operator[](int i) { return data[i]; }
  bool operator==(const Triple& triple) const { return *this == triple.data; }
  bool operator==(const double* triple) const
  {
    return ((this->data[0] - triple[0] <= 0.01 * std::abs(data[0]) + 0.02) &&
      (this->data[0] - triple[0] >= -0.01 * std::abs(data[0]) - 0.02) &&
      (this->data[1] - triple[1] <= 0.01 * std::abs(data[1]) + 0.02) &&
      (this->data[1] - triple[1] >= -0.01 * std::abs(data[1]) - 0.02) &&
      (this->data[2] - triple[2] <= 0.01 * std::abs(data[2]) + 0.02) &&
      (this->data[2] - triple[2] >= -0.01 * std::abs(data[2]) - 0.02));
  }
  bool operator!=(const Triple& triple) const { return *this != triple.data; }
  bool operator!=(const double* triple) const { return !(*this == triple); }

private:
  double data[3];
};

static ostream& operator<<(ostream& os, const Triple& t)
{
  os << t[0] << ", " << t[1] << ", " << t[2];
  return os;
}

//=============================================================================
// Function for comparing colors.  Each value should be equivalent in the
// respective color space.
static int TestColorConvert(
  const Triple& rgb, const Triple& hsv, const Triple& xyz, const Triple& lab, const Triple& prolab);

// Function for comparing special doubles like Inf and NaN.
#define TestSpecialDoubles(value, inftest, nantest)                                                \
  TestSpecialDoublesReal(value, #value, inftest, nantest)
static int TestSpecialDoublesReal(double value, const char* name, bool inftest, bool nantest);

int TestMath(int, char*[])
{
  // Test ProjectVector float
  {
    std::cout << "Testing ProjectVector float" << std::endl;
    float a[3] = { 2, -5, 0 };
    float b[3] = { 5, 1, 0 };
    float projection[3];
    float correct[3] = { 25.f / 26.f, 5.f / 26.f, 0.f };
    vtkMath::ProjectVector(a, b, projection);
    if (!fuzzyCompare3D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (25./26., 5./26., 0) but it is ("
                << projection[0] << " " << projection[1] << " " << projection[2] << ")"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test ProjectVector2D float
  {
    std::cout << "Testing ProjectVector2D float" << std::endl;
    float a[2] = { 2, -5 };
    float b[2] = { 5, 1 };
    float projection[2];
    float correct[3] = { 25.f / 26.f, 5.f / 26.f };
    vtkMath::ProjectVector2D(a, b, projection);
    if (!fuzzyCompare2D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (25./26., 5./26.) but it is (" << projection[0]
                << " " << projection[1] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test ProjectVector double
  {
    std::cout << "Testing ProjectVector double" << std::endl;
    double a[3] = { 2, -5, 0 };
    double b[3] = { 5, 1, 0 };
    double projection[3];
    double correct[3] = { 25. / 26., 5. / 26., 0 };
    vtkMath::ProjectVector(a, b, projection);
    if (!fuzzyCompare3D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (25./26., 5./26., 0) but it is ("
                << projection[0] << " " << projection[1] << " " << projection[2] << ")"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test ProjectVector2D double
  {
    std::cout << "Testing ProjectVector2D double" << std::endl;
    double a[2] = { 2, -5 };
    double b[2] = { 5, 1 };
    double projection[2];
    double correct[3] = { 25. / 26., 5. / 26. };
    vtkMath::ProjectVector2D(a, b, projection);
    if (!fuzzyCompare2D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (25./26., 5./26.) but it is (" << projection[0]
                << " " << projection[1] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Tests for AngleBetweenVectors()
  {
    std::cout << "Testing AngleBetweenVectors" << std::endl;
    std::cout << "  * vector along x-axis, vector along y-axis" << std::endl;
    double v1[3] = { 2.0, 0.0, 0.0 };
    double v2[3] = { 0.0, 5.0, 0.0 };
    double expected = vtkMath::RadiansFromDegrees(90.0);
    double angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(2,0,0 , 0,5,0) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * 0,0,0 vector, vector along y-axis" << std::endl;
    v1[0] = 0.0;
    expected = 0.0;
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(0,0,0 , 0,5,0) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * 0,0,0 vector, 0,0,0 vector" << std::endl;
    v2[1] = 0.0;
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(0,0,0 , 0,0,0) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * z unit vector, negative z vector" << std::endl;
    v1[2] = 1.0;
    v2[2] = -3.0;
    expected = vtkMath::RadiansFromDegrees(180.0);
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(z unit , neg z) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }
    angle = vtkMath::AngleBetweenVectors(v2, v1);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(neg z , z unit) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * z unit vector, 4,4,4" << std::endl;
    v2[0] = 4.0;
    v2[1] = 4.0;
    v2[2] = 4.0;
    expected = 0.9553166181245093; // vtkMath::RadiansFromDegrees(54.735610317245346);
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(z unit , 4,4,4) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * y unit vector, 4,4,4" << std::endl;
    v1[0] = 0.0;
    v1[1] = 1.0;
    v1[2] = 0.0;
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(y unit , 4,4,4) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }

    std::cout << "  * x unit vector, 4,4,4" << std::endl;
    v1[0] = 1.0;
    v1[1] = 0.0;
    v1[2] = 0.0;
    angle = vtkMath::AngleBetweenVectors(v1, v2);
    if (!fuzzyCompare1D(angle, expected))
    {
      vtkGenericWarningMacro(
        "AngleBetweenVectors(x unit , 4,4,4) = " << expected << "  != " << angle);
      return EXIT_FAILURE;
    }
  }

  // Tests for GaussianAmplitude(double,double)
  {
    double gaussianAmplitude = vtkMath::GaussianAmplitude(1.0, 0);
    if (!fuzzyCompare1DWeak(gaussianAmplitude, 0.39894))
    {
      vtkGenericWarningMacro("GaussianAmplitude(1,0) = 0.39894 "
        << " != " << gaussianAmplitude);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianAmplitude = vtkMath::GaussianAmplitude(2.0, 0);
    if (!fuzzyCompare1DWeak(gaussianAmplitude, 0.28209))
    {
      vtkGenericWarningMacro("GaussianAmplitude(2,0) = 0.28209 "
        << " != " << gaussianAmplitude);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianAmplitude = vtkMath::GaussianAmplitude(1.0, 1.0);
    if (!fuzzyCompare1DWeak(gaussianAmplitude, 0.24197))
    {
      vtkGenericWarningMacro("GaussianAmplitude(1,2) = 0.24197 "
        << " != " << gaussianAmplitude);
      return EXIT_FAILURE;
    }
  }

  // Tests for GaussianAmplitude(double,double,double)

  {
    double gaussianAmplitude = vtkMath::GaussianAmplitude(0, 1.0, 1.0);
    if (!fuzzyCompare1DWeak(gaussianAmplitude, 0.24197))
    {
      vtkGenericWarningMacro("GaussianAmplitude(0,1,1) = 0.24197 "
        << " != " << gaussianAmplitude);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianAmplitude = vtkMath::GaussianAmplitude(1.0, 1.0, 2.0);
    if (!fuzzyCompare1DWeak(gaussianAmplitude, 0.24197))
    {
      vtkGenericWarningMacro("GaussianAmplitude(1,1,2) = 0.24197 "
        << " != " << gaussianAmplitude);
      return EXIT_FAILURE;
    }
  }

  // Tests for GaussianWeight(double,double)
  {
    double gaussianWeight = vtkMath::GaussianWeight(1.0, 0);
    if (!fuzzyCompare1DWeak(gaussianWeight, 1.0))
    {
      vtkGenericWarningMacro("GaussianWeight(1,0) = 1.0 "
        << " != " << gaussianWeight);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianWeight = vtkMath::GaussianWeight(2.0, 0);
    if (!fuzzyCompare1DWeak(gaussianWeight, 1.0))
    {
      vtkGenericWarningMacro("GaussianWeight(2,0) = 1.0 "
        << " != " << gaussianWeight);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianWeight = vtkMath::GaussianWeight(1.0, 1.0);
    if (!fuzzyCompare1DWeak(gaussianWeight, 0.60653))
    {
      vtkGenericWarningMacro("GaussianWeight(1,1) = 0.60653 "
        << " != " << gaussianWeight);
      return EXIT_FAILURE;
    }
  }

  // Tests for GaussianWeight(double,double,double)

  {
    double gaussianWeight = vtkMath::GaussianWeight(0, 1.0, 1.0);
    if (!fuzzyCompare1DWeak(gaussianWeight, 0.60653))
    {
      vtkGenericWarningMacro("GaussianWeight(0,1,1) = 0.60653 "
        << " != " << gaussianWeight);
      return EXIT_FAILURE;
    }
  }

  {
    double gaussianWeight = vtkMath::GaussianWeight(1.0, 1.0, 2.0);
    if (!fuzzyCompare1DWeak(gaussianWeight, 0.60653))
    {
      vtkGenericWarningMacro("GaussianWeight(1,1,2) = 0.60653 "
        << " != " << gaussianWeight);
      return EXIT_FAILURE;
    }
  }

  int testIntValue;

  testIntValue = vtkMath::Factorial(5);
  if (testIntValue != 120)
  {
    vtkGenericWarningMacro("Factorial(5) = " << testIntValue << " != 120");
    return 1;
  }

  testIntValue = vtkMath::Binomial(8, 3);
  if (testIntValue != 56)
  {
    vtkGenericWarningMacro("Binomial(8,3) = " << testIntValue << " != 56");
    return 1;
  }

  testIntValue = vtkMath::Binomial(5, 3);
  if (testIntValue != 10)
  {
    vtkGenericWarningMacro("Binomial(5,3) = " << testIntValue << " != 10");
    return 1;
  }

  // test CeilLog2
  const static vtkTypeUInt64 testCeilLog2Inputs[7] = { 0ull, 1ull, 31ull, 32ull, 33ull,
    9223372036854775808ull /* 2^63 */, 18446744073709551615ull /* 2^64-1 */ };
  const static int testCeilLog2Outputs[7] = { 0, 0, 5, 5, 6, 63, 64 };
  for (int cl2 = 0; cl2 < 7; cl2++)
  {
    int po2v = vtkMath::CeilLog2(testCeilLog2Inputs[cl2]);
    if (po2v != testCeilLog2Outputs[cl2])
    {
      vtkGenericWarningMacro("CeilLog2(" << testCeilLog2Inputs[cl2] << ") = " << po2v
                                         << " != " << testCeilLog2Outputs[cl2]);
      return 1;
    }
  }

  // test Min
  int iMin = 0;
  int iMax = 1;
  if (iMin != vtkMath::Min(iMin, iMax))
  {
    vtkGenericWarningMacro("Min(" << iMin << ", " << iMax << " != " << iMin);
    return 1;
  }

  double dMin = 3.0;
  double dMax = 4.1;
  if (dMin != vtkMath::Min(dMin, dMax))
  {
    vtkGenericWarningMacro("Min(" << dMin << ", " << dMax << " != " << dMin);
    return 1;
  }

  // If a NaN is passed, the first argument must be returned:
  if (!std::isnan(vtkMath::Min(std::nan(""), 0.)) || std::isnan(vtkMath::Min(0., std::nan(""))))
  {
    vtkGenericWarningMacro("Min() does not properly handle NaN inputs.");
    return 1;
  }

  // test Max
  if (iMax != vtkMath::Max(iMin, iMax))
  {
    vtkGenericWarningMacro("Max(" << iMin << ", " << iMax << " != " << iMax);
    return 1;
  }

  if (dMax != vtkMath::Max(dMin, dMax))
  {
    vtkGenericWarningMacro("Max(" << dMin << ", " << dMax << " != " << dMax);
    return 1;
  }

  // If a NaN is passed, the first argument must be returned:
  if (!std::isnan(vtkMath::Max(std::nan(""), 0.)) || std::isnan(vtkMath::Max(0., std::nan(""))))
  {
    vtkGenericWarningMacro("Max() does not properly handle NaN inputs.");
    return 1;
  }

  // test is-power-of-two
  const static vtkTypeUInt64 isPowerOfTwoInputs[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 255, 256,
    257, 9223372036854775808ull /* 2^63 */, 18446744073709551615ull /* 2^64-1 */ };
  const static int isPowerOfTwoOutputs[16] = { 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
  for (int ip2 = 0; ip2 < 10; ip2++)
  {
    int ip2v = vtkMath::IsPowerOfTwo(isPowerOfTwoInputs[ip2]);
    if (ip2v ^ isPowerOfTwoOutputs[ip2])
    {
      vtkGenericWarningMacro("IsPowerOfTwo(" << isPowerOfTwoInputs[ip2] << ") = " << ip2v
                                             << " != " << isPowerOfTwoOutputs[ip2]);
      return 1;
    }
  }

  // test nearest-power-of-two
  const static int testPowerOfTwoInputs[10] = { 0, 1, 31, 32, 33, -1, -8, VTK_INT_MAX, 1073741824,
    1073741825 };
  const static int testPowerOfTwoOutputs[10] = { 1, 1, 32, 32, 64, 1, 1, VTK_INT_MIN, 1073741824,
    VTK_INT_MIN };
  for (int po2 = 0; po2 < 10; po2++)
  {
    int po2v = vtkMath::NearestPowerOfTwo(testPowerOfTwoInputs[po2]);
    if (po2v != testPowerOfTwoOutputs[po2])
    {
      vtkGenericWarningMacro("NearestPowerOfTwo(" << testPowerOfTwoInputs[po2] << ") = " << po2v
                                                  << " != " << testPowerOfTwoOutputs[po2]);
      return 1;
    }
  }

  // test Floor and Ceil
  const static double fcInputs[19] = { 0.0, -VTK_DBL_EPSILON, VTK_DBL_EPSILON, 1.0,
    1 - VTK_DBL_EPSILON, 1 + VTK_DBL_EPSILON, 2.0, 2 - 2 * VTK_DBL_EPSILON, 2 + 2 * VTK_DBL_EPSILON,
    -1.0, -1 - VTK_DBL_EPSILON, -1 + VTK_DBL_EPSILON, -2.0, -2 - 2 * VTK_DBL_EPSILON,
    -2 + 2 * VTK_DBL_EPSILON, 2147483647.0, 2147483647.0 - 2147483648.0 * VTK_DBL_EPSILON,
    -2147483648.0, -2147483648.0 + 2147483648.0 * VTK_DBL_EPSILON };
  const static int floorOutputs[19] = { 0, -1, 0, 1, 0, 1, 2, 1, 2, -1, -2, -1, -2, -3, -2,
    VTK_INT_MAX, VTK_INT_MAX - 1, VTK_INT_MIN, VTK_INT_MIN };
  const static int ceilOutputs[19] = { 0, 0, 1, 1, 1, 2, 2, 2, 3, -1, -1, 0, -2, -2, -1,
    VTK_INT_MAX, VTK_INT_MAX, VTK_INT_MIN, VTK_INT_MIN + 1 };
  for (int fcc = 0; fcc < 19; fcc++)
  {
    int floorOut = vtkMath::Floor(fcInputs[fcc]);
    int ceilOut = vtkMath::Ceil(fcInputs[fcc]);
    if (floorOut != floorOutputs[fcc])
    {
      vtkGenericWarningMacro(
        "Floor(" << fcInputs[fcc] << ") = " << floorOut << " != " << floorOutputs[fcc]);
      return 1;
    }
    if (ceilOut != ceilOutputs[fcc])
    {
      vtkGenericWarningMacro(
        "Ceil(" << fcInputs[fcc] << ") = " << ceilOut << " != " << ceilOutputs[fcc]);
      return 1;
    }
  }

  // Test add, subtract, scalar multiplication.
  double a[3] = { 1.0, 2.0, 3.0 };
  double b[3] = { 0.0, 1.0, 2.0 };
  double c[3];
  double ans1[3] = { 1.0, 3.0, 5.0 };
  double ans2[3] = { 1.0, 1.0, 1.0 };
  double ans3[3] = { 3.0, 6.0, 9.0 };
  float af[3] = { 1.0f, 2.0f, 3.0f };
  float bf[3] = { 0.0f, 1.0f, 2.0f };
  float cf[3];
  float ans1f[3] = { 1.0, 3.0, 5.0 };
  float ans2f[3] = { 1.0, 1.0, 1.0 };
  float ans3f[3] = { 3.0, 6.0, 9.0 };

  vtkMath::Add(a, b, c);
  if (!fuzzyCompare3D(c, ans1))
  {
    vtkGenericWarningMacro("Double addition failed.");
    return 1;
  }
  vtkMath::Subtract(a, b, c);
  if (!fuzzyCompare3D(c, ans2))
  {
    vtkGenericWarningMacro("Double subtraction failed.");
    return 1;
  }
  vtkMath::MultiplyScalar(a, 3.0);
  if (!fuzzyCompare3D(a, ans3))
  {
    vtkGenericWarningMacro("Double scalar multiplication failed.");
    return 1;
  }
  vtkMath::Add(af, bf, cf);
  if (!fuzzyCompare3D(cf, ans1f))
  {
    vtkGenericWarningMacro("Float addition failed.");
    cout << "Result: { " << cf[0] << ", " << cf[1] << ", " << cf[2] << " }" << endl;
    return 1;
  }
  vtkMath::Subtract(af, bf, cf);
  if (!fuzzyCompare3D(cf, ans2f))
  {
    vtkGenericWarningMacro("Float subtraction failed.");
    return 1;
  }
  vtkMath::MultiplyScalar(af, 3.0f);
  if (!fuzzyCompare3D(af, ans3f))
  {
    vtkGenericWarningMacro("Float scalar multiplication failed.");
    return 1;
  }

  // Test QuadraticRoot
  double root[2];
  int numRoot;

  // x2 = 0
  numRoot = vtkMath::QuadraticRoot(1, 0, 0, -10, 10, root);
  if (numRoot != 1 || root[0] != 0)
  {
    vtkGenericWarningMacro("QuadraticRoot failed.");
    std::cout << "numRoot should be 1, it is " << numRoot << " ." << std::endl;
    std::cout << "root should be 0, it is " << root[0] << " ." << std::endl;
    return 1;
  }
  // x2 - x - 2 = 0
  numRoot = vtkMath::QuadraticRoot(1, -1, -2, -10, 10, root);
  if (numRoot != 2 || root[0] != -1 || root[1] != 2)
  {
    vtkGenericWarningMacro("QuadraticRoot failed.");
    std::cout << "numRoot should be 2, it is " << numRoot << " ." << std::endl;
    std::cout << "root should be -1, it is " << root[0] << " ." << std::endl;
    std::cout << "root should be 2, it is " << root[1] << " ." << std::endl;
    return 1;
  }

  // Test color conversion.
  int colorsPassed = 1;

  colorsPassed &= TestColorConvert(Triple(1.0, 1.0, 1.0), // RGB
    Triple(0.0, 0.0, 1.0),                                // HSV (H ambiguous)
    Triple(0.9505, 1.000, 1.089),                         // XYZ
    Triple(100.0, 0.0, 0.0),                              // CIELAB
    Triple(100.0, 0.0, 0.0));                             // PROLAB

  colorsPassed &= TestColorConvert(Triple(0.5, 0.5, 0.0), // RGB
    Triple(1.0 / 6.0, 1.0, 0.5),                          // HSV
    Triple(0.165, 0.199, 0.030),                          // XYZ
    Triple(51.7, -12.90, 56.54),                          // CIELAB
    Triple(58.784, -6.041, 20.815));                      // PROLAB

  colorsPassed &= TestColorConvert(Triple(0.25, 0.25, 0.5), // RGB
    Triple(2.0 / 3.0, 0.5, 0.5),                            // HSV
    Triple(0.078, 0.063, 0.211),                            // XYZ
    Triple(30.11, 18.49, -36.18),                           // CIELAB
    Triple(42.4114, 5.50106, -18.9549));                    // PROLAB

  colorsPassed &= TestColorConvert(Triple(0.0, 0.0, 0.0), // RGB
    Triple(0.0, 0.0, 0.0),                                // HSV (H&S ambiguous)
    Triple(0.0, 0.0, 0.0),                                // XYZ
    Triple(0.0, 0.0, 0.0),                                // CIELAB
    Triple(0.0, 0.0, 0.0));                               // PROLAB

  if (!colorsPassed)
  {
    return 1;
  }

  if (!TestSpecialDoubles(0, false, false))
    return 1;
  if (!TestSpecialDoubles(5, false, false))
    return 1;
  if (!TestSpecialDoubles(vtkMath::Inf(), true, false))
    return 1;
  if (!TestSpecialDoubles(vtkMath::NegInf(), true, false))
    return 1;
  if (!TestSpecialDoubles(vtkMath::Nan(), false, true))
    return 1;

  if (!(0 < vtkMath::Inf()))
  {
    vtkGenericWarningMacro(<< "Odd comparison for infinity.");
    return 1;
  }
  if (!(0 > vtkMath::NegInf()))
  {
    vtkGenericWarningMacro(<< "Odd comparison for negative infinity.");
    return 1;
  }

  // Test 1D convolution
  constexpr std::array<double, 4> sample1{ 1, 4.5, 2, 6.2 };
  constexpr std::array<double, 3> kernel1{ 4, 0, 3.5 };
  constexpr std::array<double, 7> sample2{ 1, 3, 4.3, 8.7, 6.5, 4.8, 0 };
  constexpr std::array<double, 7> kernel2{ 2, 0, 1.5, 8.4, 6.2, 2.8, 6.9 };

  constexpr std::array<double, 13> expectedFull1{ 4.0, 18.0, 11.5, 40.55, 7.0, 21.7, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0 };
  constexpr std::array<double, 13> expectedSame1{ 18.0, 11.5, 40.55, 7.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0 };
  constexpr std::array<double, 13> expectedValid1{ 11.5, 40.55, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0 };
  constexpr std::array<double, 13> expectedFull2{ 2.0, 6.0, 10.1, 30.3, 50.85, 80.17, 124.79,
    148.48, 134.65, 107.99, 58.29, 33.12, 0.0 };
  constexpr std::array<double, 13> expectedSame2{ 30.3, 50.85, 80.17, 124.79, 148.48, 134.65,
    107.99, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  constexpr std::array<double, 13> expectedValid2{ 124.79, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0 };

  std::array<double, 13> output;
  output.fill(0.0);
  vtkMath::Convolve1D(sample1.begin(), sample1.end(), kernel1.begin(), kernel1.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::FULL);
  if (!fuzzyCompareNDWeak(output, expectedFull1, 13))
  {
    vtkGenericWarningMacro(<< "First \"full\" 1D convolution test failed.");
    return 1;
  }

  output.fill(0.0);
  vtkMath::Convolve1D(sample1.begin(), sample1.end(), kernel1.begin(), kernel1.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::SAME);
  if (!fuzzyCompareNDWeak(output, expectedSame1, 13))
  {
    vtkGenericWarningMacro(<< "First \"same\" 1D convolution test failed.");
    return 1;
  }

  output.fill(0.0);
  vtkMath::Convolve1D(sample1.begin(), sample1.end(), kernel1.begin(), kernel1.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::VALID);
  if (!fuzzyCompareNDWeak(output, expectedValid1, 13))
  {
    vtkGenericWarningMacro(<< "First \"valid\" 1D convolution test failed.");
    return 1;
  }

  output.fill(0.0);
  vtkMath::Convolve1D(sample2.begin(), sample2.end(), kernel2.begin(), kernel2.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::FULL);
  if (!fuzzyCompareNDWeak(output, expectedFull2, 13))
  {
    vtkGenericWarningMacro(<< "Second \"full\" 1D convolution test failed.");
    return 1;
  }

  output.fill(0.0);
  vtkMath::Convolve1D(sample2.begin(), sample2.end(), kernel2.begin(), kernel2.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::SAME);
  if (!fuzzyCompareNDWeak(output, expectedSame2, 13))
  {
    vtkGenericWarningMacro(<< "Second \"same\" 1D convolution test failed.");
    return 1;
  }

  output.fill(0.0);
  vtkMath::Convolve1D(sample2.begin(), sample2.end(), kernel2.begin(), kernel2.end(),
    output.begin(), output.end(), vtkMath::ConvolutionMode::VALID);
  if (!fuzzyCompareNDWeak(output, expectedValid2, 13))
  {
    vtkGenericWarningMacro(<< "Second \"valid\" 1D convolution test failed.");
    return 1;
  }

  // Test GetPointAlongLine
  double p1[3] = { 1.0, 1.0, 1.0 };
  double p2[3] = { 2.0, 2.0, 2.0 };
  double result[3] = { 0.0 };

  auto roundTo3 = [](double value) { return (double)(round(value * 1000)) / 1000; };

  double expectedForward[3] = { 3.0, 3.0, 3.0 };
  vtkMath::GetPointAlongLine(result, p1, p2, sqrt(3.0));
  if (roundTo3(result[0]) != expectedForward[0] && roundTo3(result[1]) != expectedForward[1] &&
    roundTo3(result[2]) != expectedForward[2])
  {
    vtkGenericWarningMacro(<< "GetPointAlongLine test failed in forward direction.");
    return 1;
  }

  // Without roundTo3, result is -1.0000000000000004 in each dimension.
  double expectedBackward[3] = { -1.0, -1.0, -1.0 };
  vtkMath::GetPointAlongLine(result, p1, p2, -3 * sqrt(3.0));
  if (roundTo3(result[0]) != expectedBackward[0] && roundTo3(result[1]) != expectedBackward[1] &&
    roundTo3(result[2]) != expectedBackward[2])
  {
    vtkGenericWarningMacro(<< "GetPointAlongLine test failed in backward direction.");
    return 1;
  }

  return 0;
}

static int TestColorConvert(
  const Triple& rgb, const Triple& hsv, const Triple& xyz, const Triple& lab, const Triple& prolab)
{
  cout << "Ensuring the following colors are consistent: " << endl;
  cout << "   RGB:      " << rgb << endl;
  cout << "   HSV:      " << hsv << endl;
  cout << "   CIE XYZ:  " << xyz << endl;
  cout << "   CIE-L*ab: " << lab << endl;
  cout << "   ProLAB:   " << prolab << endl;

  Triple result1;

#define COMPARE(testname, target, dest)                                                            \
  do                                                                                               \
  {                                                                                                \
    if ((target) != (dest))                                                                        \
    {                                                                                              \
      vtkGenericWarningMacro(<< "Incorrect " #testname " conversion.  Got " << (dest)              \
                             << " expected " << (target));                                         \
      return 0;                                                                                    \
    }                                                                                              \
  } while (false)

  // Test conversion between RGB and HSV.
  vtkMath::RGBToHSV(rgb(), result1());
  COMPARE(RGBToHSV, hsv, result1);
  vtkMath::HSVToRGB(hsv(), result1());
  COMPARE(HSVToRGB, rgb, result1);

  vtkMath::RGBToHSV(rgb[0], rgb[1], rgb[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToHSV, hsv, result1);
  vtkMath::HSVToRGB(hsv[0], hsv[1], hsv[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(HSVToRGB, rgb, result1);

  // Test conversion between RGB and XYZ.
  vtkMath::RGBToXYZ(rgb(), result1());
  COMPARE(RGBToXYZ, xyz, result1);
  vtkMath::XYZToRGB(xyz(), result1());
  COMPARE(XYZToRGB, rgb, result1);

  vtkMath::RGBToXYZ(rgb[0], rgb[1], rgb[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToXYZ, xyz, result1);
  vtkMath::XYZToRGB(xyz[0], xyz[1], xyz[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(XYZToRGB, rgb, result1);

  // Test conversion between Lab and XYZ.
  vtkMath::LabToXYZ(lab(), result1());
  COMPARE(LabToXYZ, xyz, result1);
  vtkMath::XYZToLab(xyz(), result1());
  COMPARE(XYZToLab, lab, result1);

  vtkMath::LabToXYZ(lab[0], lab[1], lab[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(LabToXYZ, xyz, result1);
  vtkMath::XYZToLab(xyz[0], xyz[1], xyz[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(XYZToLab, lab, result1);

  // Test conversion between Lab and RGB.
  vtkMath::LabToRGB(lab(), result1());
  COMPARE(LabToRGB, rgb, result1);
  vtkMath::RGBToLab(rgb(), result1());
  COMPARE(RGBToLab, lab, result1);

  vtkMath::LabToRGB(lab[0], lab[1], lab[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(LabToRGB, rgb, result1);
  vtkMath::RGBToLab(rgb[0], rgb[1], rgb[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToLab, lab, result1);

  // Test conversion between ProLAB and XYZ
  vtkMath::ProLabToXYZ(prolab(), result1());
  COMPARE(ProLabToXYZ, xyz, result1);
  vtkMath::XYZToProLab(xyz(), result1());
  COMPARE(XYZToProLab, prolab, result1);

  vtkMath::ProLabToXYZ(prolab[0], prolab[1], prolab[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(ProLabToXYZ, xyz, result1);
  vtkMath::XYZToProLab(xyz[0], xyz[1], xyz[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(XYZToProLab, prolab, result1);

  // Test conversion between ProLAB and RGB
  vtkMath::ProLabToRGB(prolab(), result1());
  COMPARE(ProLabToRGB, rgb, result1);
  vtkMath::RGBToProLab(rgb(), result1());
  COMPARE(RGBToProLab, prolab, result1);

  vtkMath::ProLabToRGB(prolab[0], prolab[1], prolab[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(ProLabToRGB, rgb, result1);
  vtkMath::RGBToProLab(rgb[0], rgb[1], rgb[2], &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToProLab, prolab, result1);

  return 1;
}

static int TestSpecialDoublesReal(double value, const char* name, bool inftest, bool nantest)
{
  cout << "Testing comparison of " << name << " to non-finite values." << endl;
  cout << "  * IsNan test." << endl;
  if (vtkMath::IsNan(value) != static_cast<int>(nantest))
  {
    cout << value << " failed the IsNan test." << endl;
    return 0;
  }
  cout << "  * IsInf test." << endl;
  if (vtkMath::IsInf(value) != static_cast<int>(inftest))
  {
    cout << value << " failed the IsInf test." << endl;
    return 0;
  }
  cout << "  * Tests passed." << endl;

  return 1;
}
