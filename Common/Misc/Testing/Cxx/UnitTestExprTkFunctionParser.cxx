// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSmartPointer.h"

#include "vtkExprTkFunctionParser.h"

#include "vtkMathUtilities.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkTestErrorObserver.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

constexpr bool STATUS_SUCCESS = true;
constexpr bool STATUS_FAILURE = false;

#define SCALAR_FUNC(proc, function, math)                                                          \
  static bool proc(double low, double hi)                                                          \
  {                                                                                                \
    std::cout << "Testing " << #function << "...";                                                 \
    auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();                                 \
    std::string _fun(#function);                                                                   \
    _fun += "(x)";                                                                                 \
    parser->SetFunction(_fun.c_str());                                                             \
                                                                                                   \
    auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();                          \
    for (unsigned int i = 0; i < 1000; ++i)                                                        \
    {                                                                                              \
      double value = rand->GetNextRangeValue(low, hi);                                             \
      parser->SetScalarVariableValue("x", value);                                                  \
      double result = parser->GetScalarResult();                                                   \
      double expected = math(value);                                                               \
      if (!vtkMathUtilities::FuzzyCompare(                                                         \
            result, expected, std::numeric_limits<double>::epsilon() * 1.0))                       \
      {                                                                                            \
        std::cout << "\n" #function " Expected " << expected << " but got " << result              \
                  << " difference is " << result - expected << " eps ratio is: "                   \
                  << (result - expected) / std::numeric_limits<double>::epsilon() << std::endl;    \
        return STATUS_FAILURE;                                                                     \
      }                                                                                            \
    }                                                                                              \
                                                                                                   \
    std::cout << "PASSED\n";                                                                       \
    return STATUS_SUCCESS;                                                                         \
  }

SCALAR_FUNC(TestAbs, abs, std::abs);
SCALAR_FUNC(TestAcos, acos, std::acos);
SCALAR_FUNC(TestAsin, asin, std::asin);
SCALAR_FUNC(TestAtan, atan, std::atan);
SCALAR_FUNC(TestCeil, ceil, std::ceil);
SCALAR_FUNC(TestCos, cos, std::cos);
SCALAR_FUNC(TestCosh, cosh, std::cosh);
SCALAR_FUNC(TestExp, exp, std::exp);
SCALAR_FUNC(TestFloor, floor, std::floor);
SCALAR_FUNC(TestLn, ln, std::log);
SCALAR_FUNC(TestLog10, log10, std::log10);
SCALAR_FUNC(TestSin, sin, std::sin);
SCALAR_FUNC(TestSinh, sinh, std::sinh);
SCALAR_FUNC(TestSqrt, sqrt, std::sqrt);
SCALAR_FUNC(TestTan, tan, std::tan);
SCALAR_FUNC(TestTanh, tanh, std::tanh);
static bool TestScalars();
static bool TestVariableNames();
static bool TestSpacing();
static bool TestUnaryOperations();
static bool TestScientificNotation();
static bool TestVectors();
static bool TestMinMax();
static bool TestScalarLogic();
static bool TestVectorLogic();
static bool TestMiscFunctions();
static bool TestErrors();

int UnitTestExprTkFunctionParser(int, char*[])
{
  bool status = STATUS_SUCCESS;

  status &= TestAbs(-1000.0, 1000);
  status &= TestAcos(-1.0, 1.0);
  status &= TestAsin(-1.0, 1.0);
  status &= TestAtan(-1.0, 1.0);
  status &= TestCeil(-1000.0, 1000.0);
  status &= TestCos(-1000.0, 1000.0);
  status &= TestCosh(-1.0, 1.0);
  status &= TestExp(0, 2.0);
  status &= TestFloor(-1000.0, 1000.0);
  status &= TestLn(0.0, 1000.0);
  status &= TestLog10(0.0, 1000.0);
  status &= TestSin(-1000.0, 1000.0);
  status &= TestSinh(-1.0, 1.0);
  status &= TestSqrt(.1, 1000.0);
  status &= TestTan(-1000.0, 1000.0);
  status &= TestTanh(-1.0, 1.0);

  status &= TestScalars();
  status &= TestVariableNames();
  status &= TestSpacing();
  status &= TestUnaryOperations();
  status &= TestScientificNotation();
  status &= TestVectors();
  status &= TestMinMax();
  status &= TestScalarLogic();
  status &= TestVectorLogic();

  status &= TestMiscFunctions();
  status &= TestErrors();
  if (status == STATUS_FAILURE)
  {
    return EXIT_FAILURE;
  }

  // Test printing of an uninitialized parser
  std::ostringstream functionPrint;
  auto functionParser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  functionParser->Print(functionPrint);

  return EXIT_SUCCESS;
}

bool TestUnaryOperations()
{
  std::cout << "Testing Scalar Unary"
            << "...";
  std::string formula[4] = { "-x * +y", "+x + +y", "+x - -y", "-x - +y" };
  double expected[4] = { -2., 3., 3., -3. };

  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetScalarVariableValue("x", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  for (unsigned i = 0; i < 4; i++)
  {
    parser->SetFunction(formula[i].data());
    double result = parser->GetScalarResult();
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected[i], std::numeric_limits<double>::epsilon() * 1.0))
    {
      std::cout << "FAILED\n";
      return STATUS_FAILURE;
    }
  }

  parser->SetScalarVariableValue("x", 3);
  parser->SetScalarVariableValue("y", 2);
  parser->SetFunction("(-x) ^ +y");
  int result = parser->GetScalarResult();
  if (result != 9)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }

  parser->SetFunction("(-x)");
  result = parser->GetScalarResult();
  if (result != -3)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }

  std::cout << "PASSED\n";
  return STATUS_SUCCESS;
}

bool TestScalars()
{
  std::cout << "Testing Scalar Add / Subtract / Multiply / Divide"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetScalarVariableValue("x", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  parser->SetFunction("+(x-y)/(x-y) * -(x-y)/(x-y) + (x - x)");
  double result = parser->GetScalarResult();
  if (result != -1.0)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }
  else
  {
    std::cout << "PASSED\n";
    return STATUS_SUCCESS;
  }
}

bool TestVariableNames()
{
  std::cout << "Testing variable names similar to math ops with parentheses "
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetScalarVariableValue("absolutex", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  parser->SetFunction("absolutex - (y)");
  double result = parser->GetScalarResult();
  if (result != -1.0)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }
  else
  {
    std::cout << "PASSED\n";
    return STATUS_SUCCESS;
  }
}

bool TestSpacing()
{
  std::cout << "Testing spacing with math ops "
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetScalarVariableValue("x", -1.0);
  parser->SetFunction("abs(x)");
  double result = parser->GetScalarResult();
  if (result != 1.0)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }
  parser->SetFunction("abs  (x)");
  result = parser->GetScalarResult();
  if (result != 1.0)
  {
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }
  else
  {
    std::cout << "PASSED\n";
    return STATUS_SUCCESS;
  }
}

bool TestScientificNotation()
{
  std::cout << "Testing Scientific notation"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetFunction("3.0e+01");
  double expected = 3.0e+01;
  double result = parser->GetScalarResult();
  if (!vtkMathUtilities::FuzzyCompare(
        result, expected, std::numeric_limits<double>::epsilon() * 1.0))
  {
    std::cout << " Scientific notation expected " << expected << " but got " << result;
    std::cout << "eps ratio is: " << (result - expected) / std::numeric_limits<double>::epsilon()
              << std::endl;
    std::cout << "FAILED\n";
    return STATUS_FAILURE;
  }
  else
  {
    std::cout << "PASSED\n";
    return STATUS_SUCCESS;
  }
}

bool TestVectors()
{
  std::cout << "Testing Cross"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();

  bool status1 = STATUS_SUCCESS;
  bool status2 = STATUS_SUCCESS;
  bool status3 = STATUS_SUCCESS;
  bool status4 = STATUS_SUCCESS;
  bool status5 = STATUS_SUCCESS;

  auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  // Cross
  for (unsigned int i = 0; i < 10; ++i)
  {
    double x0 = rand->GetNextRangeValue(-1.0, 1.0);
    double x1 = rand->GetNextRangeValue(-1.0, 1.0);
    double x2 = rand->GetNextRangeValue(-1.0, 1.0);
    parser->SetVectorVariableValue("x", x0, x1, x2);

    double y0 = rand->GetNextRangeValue(-1.0, 1.0);
    double y1 = rand->GetNextRangeValue(-1.0, 1.0);
    double y2 = rand->GetNextRangeValue(-1.0, 1.0);
    parser->SetVectorVariableValue("y", y0, y1, y2);

    parser->SetFunction("cross(x,y)");
    double* result = parser->GetVectorResult();
    double axb[3];
    axb[0] = result[0];
    axb[1] = result[1];
    axb[2] = result[2];
    // repeat to cover a 0 return from Evaluate()
    parser->IsVectorResult();
    parser->IsVectorResult();

    parser->SetFunction("cross(-y,x)");
    result = parser->GetVectorResult();
    double minusBxa[3];
    minusBxa[0] = result[0];
    minusBxa[1] = result[1];
    minusBxa[2] = result[2];

    // a x b = -b x a
    for (int j = 0; j < 3; ++j)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            axb[j], minusBxa[j], std::numeric_limits<double>::epsilon() * 1.0))
      {
        std::cout << " Cross expected " << minusBxa[j] << " but got " << axb[j];
        std::cout << "eps ratio is: "
                  << (axb[j] - minusBxa[j]) / std::numeric_limits<double>::epsilon() << std::endl;
        status1 = STATUS_FAILURE;
      }
    }
  }
  if (status1 == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    std::cout << "FAILED\n";
  }

  parser->RemoveAllVariables();
  // Add / Subtract / Multiply / Unary / Dot / Mag / Norm
  std::cout << "Testing Add / Subtract / Multiply / Unary / Dot"
            << "...";
  for (unsigned int i = 0; i < 10; ++i)
  {
    double x0 = rand->GetNextRangeValue(-1.0, 1.0);
    double x1 = rand->GetNextRangeValue(-1.0, 1.0);
    double x2 = rand->GetNextRangeValue(-1.0, 1.0);
    parser->SetVectorVariableValue("x", x0, x1, x2);

    double y0 = rand->GetNextRangeValue(-1.0, 1.0);
    double y1 = rand->GetNextRangeValue(-1.0, 1.0);
    double y2 = rand->GetNextRangeValue(-1.0, 1.0);
    parser->SetVectorVariableValue("y", y0, y1, y2);

    parser->SetScalarVariableValue("t", 2.0);
    parser->SetFunction("t*(x + y - (x + y))/t");
    double* result = parser->GetVectorResult();
    double a[3];
    a[0] = result[0];
    a[1] = result[1];
    a[2] = result[2];

    parser->SetScalarVariableValue("s", 0.0);
    parser->SetFunction("x * s");
    result = parser->GetVectorResult();
    double b[3];
    b[0] = result[0];
    b[1] = result[1];
    b[2] = result[2];

    // 2.0 * ((x + y - (x + y)) / 2.0 = x * 0.0
    for (int j = 0; j < 3; ++j)
    {
      if (!vtkMathUtilities::FuzzyCompare(a[j], b[j], std::numeric_limits<double>::epsilon() * 1.0))
      {
        std::cout << " Cross expected " << a[j] << " but got " << b[j];
        std::cout << "eps ratio is: " << (a[j] - b[j]) / std::numeric_limits<double>::epsilon()
                  << std::endl;
        status2 = STATUS_FAILURE;
      }
    }
    // Test Dot / Mag / Norm
    // a x b dot a = 0
    parser->SetFunction("dot(cross(x, y),x)");
    double dot = parser->GetScalarResult();
    if (!vtkMathUtilities::FuzzyCompare(dot, 0.0, std::numeric_limits<double>::epsilon() * 1.0))
    {
      std::cout << " Dot " << 0.0 << " but got " << dot;
      std::cout << "eps ratio is: " << (dot - 0.0) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status3 = STATUS_FAILURE;
    }

    // Test Mag and Norm
    // mag(norm(x) == 1
    parser->SetFunction("mag(norm(x))");
    double mag = parser->GetScalarResult();
    if (!vtkMathUtilities::FuzzyCompare(mag, 1.0, std::numeric_limits<double>::epsilon() * 2.0))
    {
      std::cout << " Mag expected" << 1.0 << " but got " << mag;
      std::cout << " eps ratio is: " << (mag - 1.0) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status4 = STATUS_FAILURE;
    }
  }

  parser->RemoveAllVariables();
  // x *iHat + y * jHat + z * zHat
  parser->SetScalarVariableValue("x", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  parser->SetScalarVariableValue("z", 3.0);
  parser->SetFunction("x*iHat + y*jHat + z*kHat");
  double* xyz = parser->GetVectorResult();
  if (xyz[0] != 1.0 || xyz[1] != 2.0 || xyz[2] != 3.0)
  {
    std::cout << "x*iHat + y*jHat + z*kHat expected "
              << "(" << 1.0 << "," << 2.0 << "," << 3.0 << ") but got "
              << "(" << xyz[0] << "," << xyz[1] << "," << xyz[2] << ")" << std::endl;
    status5 = STATUS_FAILURE;
  }

  // Test printing of an initialized parser
  std::ostringstream parserPrint;
  parser->Print(parserPrint);

  // Now clear the variables
  parser->RemoveAllVariables();
  if (parser->GetNumberOfScalarVariables() != 0 || parser->GetNumberOfVectorVariables() != 0)
  {
    std::cout << "RemoveAllVariables failed" << std::endl;
    status1 = STATUS_FAILURE;
  }

  // Invalidate function should change the function's mtime
  vtkMTimeType before = parser->GetMTime();
  parser->InvalidateFunction();
  vtkMTimeType after = parser->GetMTime();

  if (before >= after)
  {
    std::cout << "InvalidateFunction() failed. MTime should have been modified" << std::endl;
    status5 = STATUS_FAILURE;
  }

  bool statusAll = status1 && status2 && status3 & status4 && status5;
  if (statusAll == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  return statusAll;
}

bool TestMinMax()
{
  std::cout << "Testing Min/Max"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();

  parser->SetFunction("min(x,y)");

  auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  bool status = STATUS_SUCCESS;
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double value = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", value);
    parser->SetScalarVariableValue("y", -value);

    double result = parser->GetScalarResult();
    double expected = std::min(value, -value);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected, std::numeric_limits<double>::epsilon() * 1.0))
    {
      std::cout << "\n";
      std::cout << "Min Expected " << expected << " but got " << result << " difference is "
                << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("max(x,y)");

  for (unsigned int i = 0; i < 1000; ++i)
  {
    double value = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", value);
    parser->SetScalarVariableValue("y", -value);

    double result = parser->GetScalarResult();
    double expected = std::max(value, -value);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected, std::numeric_limits<double>::epsilon() * 1.0))
    {
      std::cout << "\n";
      std::cout << "Max Expected " << expected << " but got " << result << " difference is "
                << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  return status;
}

bool TestScalarLogic()
{
  bool status = STATUS_SUCCESS;
  auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();

  std::cout << "Testing Scalar Logic"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();

  parser->SetFunction("if(x < y, x, y)");
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x < y ? x : y;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " < " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("if(x > y, x, y)");
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x > y ? x : y;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " > " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("if(x = y, x, 0.0)");
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = x;
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x == y ? x : 0.0;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " == " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  double ii[] = { 0.0, 0.0, 1.0, 1.0 };
  double jj[] = { 0.0, 1.0, 0.0, 1.0 };
  double expectedOr[] = { 0.0, 1.0, 1.0, 1.0 };
  double expectedAnd[] = { .0, 0.0, 0.0, 1.0 };

  parser->SetFunction("i | j");
  for (int i = 0; i < 3; ++i)
  {
    parser->SetScalarVariableValue("i", ii[i]);
    parser->SetScalarVariableValue("j", jj[i]);
    double result = parser->GetScalarResult();
    if (result != expectedOr[i])
    {
      std::cout << "i | j expected " << expectedOr[i] << " but got " << result << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("i & j");
  for (int i = 0; i < 3; ++i)
  {
    parser->SetScalarVariableValue("i", ii[i]);
    parser->SetScalarVariableValue("j", jj[i]);
    double result = parser->GetScalarResult();
    if (result != expectedAnd[i])
    {
      std::cout << "i | j expected " << expectedAnd[i] << " but got " << result << std::endl;
      status = STATUS_FAILURE;
    }
  }

  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    std::cout << "FAILED\n";
  }
  return status;
}

bool TestVectorLogic()
{
  bool status = STATUS_SUCCESS;
  auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();

  std::cout << "Testing Vector Logic"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();

  parser->SetFunction("if(x < y, v, w)");
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x < y ? v1 : w1;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " < " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("if(x > y, v, w)");
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x > y ? v1 : w1;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " > " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  parser->SetFunction("if(x = y, w, v * 0.0)");
  for (unsigned int i = 0; i < 1000; ++i)
  {

    double x = rand->GetNextRangeValue(-1000.0, 1000.0);
    double y = x;
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double v3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w1 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w2 = rand->GetNextRangeValue(-1000.0, 1000.0);
    double w3 = rand->GetNextRangeValue(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x > y ? v1 : w1;
    if (result != expected)
    {
      std::cout << "\n";
      std::cout << x << " == " << y << " Expected " << expected << " but got " << result
                << std::endl;
      status = STATUS_FAILURE;
    }
  }

  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    std::cout << "FAILED\n";
  }

  return status;
}

bool TestMiscFunctions()
{
  bool statusAll = STATUS_SUCCESS;
  auto rand = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();

  std::cout << "Testing Sign"
            << "...";
  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();
  parser->SetFunction("sign(x)");
  double values[3] = { -100.0, 0.0, 100.0 };
  double expecteds[3] = { -1.0, 0.0, 1.0 };

  bool status = STATUS_SUCCESS;
  for (unsigned int i = 0; i < 3; ++i)
  {
    parser->SetScalarVariableValue("x", values[i]);
    double result = parser->GetScalarResult();
    if (result != expecteds[i])
    {
      std::cout << "Sign expected " << expecteds[i] << " but got " << result << ". ";
      status = STATUS_FAILURE;
    }
  }

  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    statusAll = STATUS_FAILURE;
    std::cout << "FAILED\n";
  }

  std::cout << "Testing Pow"
            << "...";
  status = STATUS_SUCCESS;
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(0.0, 10.0);
    double y = rand->GetNextRangeValue(0.0, 2.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);
    parser->SetFunction("x ^ y");
    double result = parser->GetScalarResult();
    double expected = std::pow(x, y);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected, std::numeric_limits<double>::epsilon() * 128.0))
    {
      std::cout << "\n";
      std::cout << " pow Expected " << expected << " but got " << result << " difference is "
                << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status = STATUS_FAILURE;
    }
  }
  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    statusAll = STATUS_FAILURE;
    std::cout << "FAILED\n";
  }

  std::cout << "Testing Scalar divide"
            << "...";
  status = STATUS_SUCCESS;
  for (unsigned int i = 0; i < 1000; ++i)
  {
    double x = rand->GetNextRangeValue(-10.0, 10.0);
    double y = rand->GetNextRangeValue(-10.0, 10.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);
    parser->SetFunction("x / y");
    double result = parser->GetScalarResult();
    double expected = x / y;
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected, std::numeric_limits<double>::epsilon() * 256.0))
    {
      std::cout << "\n";
      std::cout << " x / y Expected " << expected << " but got " << result << " difference is "
                << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected) / std::numeric_limits<double>::epsilon()
                << std::endl;
      status = STATUS_FAILURE;
    }
  }
  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    statusAll = STATUS_FAILURE;
    std::cout << "FAILED\n";
  }

  // SetScalarVariableValue
  std::cout << "Testing SetScalarVariableValue...";
  parser->SetScalarVariableValue(parser->GetScalarVariableName(0), 123.456);
  if (parser->GetScalarVariableValue(parser->GetScalarVariableName(0)) != 123.456)
  {
    statusAll = STATUS_FAILURE;
    std::cout << "FAILED\n";
  }
  else
  {
    std::cout << "PASSED\n";
  }
  parser->SetScalarVariableValue(0, 123.45);
  parser->GetScalarVariableValue("x");

  parser->SetVectorVariableValue("v1", 1.0, 2.0, 3.0);
  parser->SetVectorVariableValue("v1", 1.0, 1.0, 3.0);
  parser->SetVectorVariableValue("v1", 1.0, 1.0, 1.0);
  parser->SetVectorVariableValue(0, 1.0, 2.0, 3.0);
  parser->SetVectorVariableValue(0, 1.0, 1.0, 3.0);
  parser->SetVectorVariableValue(0, 1.0, 1.0, 1.0);
  parser->GetVectorVariableValue(parser->GetVectorVariableName(0));
  parser->GetVectorVariableName(1000);

  // test functions that can use ReplaceInvalidValue
  std::vector<std::string> testFuncs;
  testFuncs.emplace_back("sqrt(s)");
  testFuncs.emplace_back("ln(s)");
  testFuncs.emplace_back("log10(s)");
  testFuncs.emplace_back("asin(s)");
  testFuncs.emplace_back("acos(s)");
  testFuncs.emplace_back("s/zero");

  parser->ReplaceInvalidValuesOn();
  parser->SetReplacementValue(1234.5);
  parser->SetScalarVariableValue("s", -1000.0);
  parser->SetScalarVariableValue("zero", 0.0);

  for (size_t f = 0; f < testFuncs.size(); ++f)
  {
    parser->SetFunction(testFuncs[f].c_str());
    if (parser->GetScalarResult() != 1234.5)
    {
      std::cout << testFuncs[f]
                << " failed to return a replacement value when ReplaceInvaliValues was On"
                << std::endl;
      statusAll = STATUS_FAILURE;
    }
  }
  parser->GetScalarResult();
  return statusAll;
}

bool TestErrors()
{
  bool status = STATUS_SUCCESS;
  std::cout << "Testing Errors"
            << "...";

  auto parser = vtkSmartPointer<vtkExprTkFunctionParser>::New();

  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  parser->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  // Parse: no function has been set
  parser->SetFunction("cos(a)");
  parser->SetFunction(nullptr);
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Parse: no function has been set");

  double s = -2.0;
  double v[3] = { 1.0, 2.0, 3.0 };
  double w[3] = { 2.0, 1.0, 4.0 };
  parser->SetScalarVariableValue("s", s);
  parser->SetScalarVariableValue("zero", 0.0);
  parser->SetVectorVariableValue("v", v[0], v[1], v[2]);
  parser->SetVectorVariableValue("w", w[0], w[1], w[2]);

  // can't apply cross to scalars
  parser->SetFunction("cross(s,w)");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage(
    "Invalid input parameter sequence for call to generic function: cross");

  // dot product does not operate on scalars
  parser->SetFunction("dot(s, v)");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Failed parameter type check for function 'dot'");

  // magnitude expects a vector, but got a scalar
  parser->SetFunction("mag(s)");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Failed parameter type check for function 'mag'");

  // normalize expects a vector, but got a scalar
  parser->SetFunction("norm(s)");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Failed parameter type check for function 'mag'");

  // Trying to take a natural logarithm of a non-positive value
  parser->SetFunction("ln(s)");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // Trying to take a log10 of a non-positive value
  parser->SetFunction("log10(s)");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // Trying to take a square root of a negative value
  parser->SetFunction("sqrt(s)");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // Trying to take asin of a value < -1 or > 1
  parser->SetFunction("asin(s)");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // Trying to take acos of a value < -1 or > 1
  parser->SetFunction("acos(s)");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // Trying to divide by zero<
  parser->SetFunction("s/zero");
  parser->IsScalarResult();
  status &=
    !errorObserver->CheckErrorMessage("Invalid result because of mathematically wrong input.");

  // GetScalarResult: no valid scalar result
  parser->SetFunction("cross(v,w)");
  parser->GetScalarResult();
  status &= !errorObserver->CheckErrorMessage("GetScalarResult: no valid scalar result");

  // GetVectorResult: no valid vector result
  parser->SetFunction("dot(v, w)");
  parser->GetVectorResult();
  status &= !errorObserver->CheckErrorMessage("GetVectorResult: no valid vector result");

  // GetScalarVariableValue: scalar variable name ... does not exist
  parser->GetScalarVariableValue("xyz");
  status &= !errorObserver->CheckErrorMessage("GetScalarVariableValue: scalar variable name");

  // GetScalarVariableValue: scalar variable number ... does not exist
  parser->GetScalarVariableValue(128);
  status &= !errorObserver->CheckErrorMessage("GetScalarVariableValue: scalar variable number");

  // GetVectorVariableValue: vector variable name ... does not exist
  parser->GetVectorVariableValue("xyz");
  status &= !errorObserver->CheckErrorMessage("GetVectorVariableValue: vector variable name");

  // GetVectorVariableValue: vector variable number ... does not exist
  parser->GetVectorVariableValue(128);
  status &= !errorObserver->CheckErrorMessage("GetVectorVariableValue: vector variable number");

  // Syntax error: expecting a variable name
  parser->SetFunction("acos()");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage(
    "Expected at least one input parameter for function call 'acos'");

  // Parse errors
  parser->SetFunction("-");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Invalid token sequence: '-'");

  parser->SetFunction("s *");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Invalid token sequence: '*'");

  parser->SetFunction("if(v,s)");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage(
    "Expected ',' between if-statement consequent and alternative");

  parser->SetFunction("s * (v + w");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Mismatched brackets: ')'");

  parser->SetFunction("v + w)*s");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Mismatched brackets: ']'");

  parser->SetFunction("s*()");
  parser->IsScalarResult();
  status &= !errorObserver->CheckErrorMessage("Premature end of expression");

  if (status == STATUS_SUCCESS)
  {
    std::cout << "PASSED\n";
  }
  else
  {
    std::cout << "FAILED\n";
  }
  return status;
}
