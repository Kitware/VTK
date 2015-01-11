/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestFunctionParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkFunctionParser.h"

#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkTestErrorObserver.h"

#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#define SCALAR_FUNC(proc,function,math) \
  static int proc(double low, double hi)          \
{ \
  std::cout << "Testing " << #function << "...";\
  vtkSmartPointer<vtkFunctionParser> parser = \
    vtkSmartPointer<vtkFunctionParser>::New(); \
  std::string _fun(#function); \
  _fun += "(x)"; \
  parser->SetFunction(_fun.c_str());    \
 \
  for (unsigned int i = 0; i < 1000; ++i) \
    { \
    double value = vtkMath::Random(low, hi); \
    parser->SetScalarVariableValue("x", value); \
    double result = parser->GetScalarResult(); \
    double expected = math(value); \
    if (!vtkMathUtilities::FuzzyCompare( \
          result, expected, \
          std::numeric_limits<double>::epsilon() * 1.0)) \
      { \
      std::cout << "\n";                                       \
      std::cout << #function " Expected " << expected \
                << " but got " << result \
                << " difference is " << result - expected << " "; \
      std::cout << "eps ratio is: " << (result - expected) \
        / std::numeric_limits<double>::epsilon() << std::endl; \
      return EXIT_FAILURE; \
      } \
    }  \
 \
  std::cout << "PASSED\n"; \
  return EXIT_SUCCESS; \
}
#define CHECK_ERROR_MSG(msg) \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
    { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
    status++; \
    } \
  else \
    { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
      { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
      } \
    } \
  } \
  errorObserver->Clear()

SCALAR_FUNC(TestAbs,abs,std::abs);
SCALAR_FUNC(TestAcos,acos,std::acos);
SCALAR_FUNC(TestAsin,asin,std::asin);
SCALAR_FUNC(TestAtan,atan,std::atan);
SCALAR_FUNC(TestCeil,ceil,std::ceil);
SCALAR_FUNC(TestCos,cos,std::cos);
SCALAR_FUNC(TestCosh,cosh,std::cosh);
SCALAR_FUNC(TestExp,exp,std::exp);
SCALAR_FUNC(TestFloor,floor,std::floor);
SCALAR_FUNC(TestLn,ln,std::log);
SCALAR_FUNC(TestLog,log,std::log);
SCALAR_FUNC(TestLog10,log10,std::log10);
SCALAR_FUNC(TestSin,sin,std::sin);
SCALAR_FUNC(TestSinh,sinh,std::sinh);
SCALAR_FUNC(TestSqrt,sqrt,std::sqrt);
SCALAR_FUNC(TestTan,tan,std::tan);
SCALAR_FUNC(TestTanh,tanh,std::tanh);
static int TestScalars();
static int TestVectors();
static int TestMinMax();
static int TestScalarLogic();
static int TestVectorLogic();
static int TestMiscFunctions();
static int TestErrors();

int UnitTestFunctionParser(int,char *[])
{
  int status = 0;

  status += TestAbs(-1000.0, 1000);
  status += TestAcos(-1.0, 1.0);
  status += TestAsin(-1.0, 1.0);
  status += TestAtan(-1.0, 1.0);
  status += TestCeil(-1000.0, 1000.0);
  status += TestCos(-1000.0, 1000.0);
  status += TestCosh(-1.0, 1.0);
  status += TestExp(0, 2.0);
  status += TestFloor(-1000.0, 1000.0);
  status += TestLn(0.0, 1000.0);
  status += TestLog(0.0, 1000.0);
  status += TestLog10(0.0, 1000.0);
  status += TestSin(-1000.0, 1000.0);
  status += TestSinh(-1.0, 1.0);
  status += TestSqrt(.1, 1000.0);
  status += TestTan(-1000.0, 1000.0);
  status += TestTanh(-1.0, 1.0);

  status += TestScalars();
  status += TestVectors();
  status += TestMinMax();
  status += TestScalarLogic();
  status += TestVectorLogic();

  status += TestMiscFunctions();
  status += TestErrors();
  if (status != 0)
    {
    return EXIT_FAILURE;
    }

  // Test printing of an uninitialized parser
  std::ostringstream functionPrint;
  vtkSmartPointer<vtkFunctionParser> functionParser =
    vtkSmartPointer<vtkFunctionParser>::New();
  functionParser->Print(functionPrint);

  return EXIT_SUCCESS;
}

int TestScalars()
{
  std::cout << "Testing Scalar Add / Subtract / Multiply / Divide" << "...";
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();
  parser->SetScalarVariableValue("x", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  parser->SetFunction( "(x-y)/(x-y) * -(x-y)/(x-y) + (x - x)");
  double result = parser->GetScalarResult();
  if (result != -1.0)
    {
    std::cout << "FAILED\n";
    return 1;
    }
  else
    {
    std::cout << "PASSED\n";
    return 0;
    }
}

int TestVectors()
{
  std::cout << "Testing Cross" << "...";
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();

  int status1 = 0;
  int status2 = 0;
  int status3 = 0;
  int status4 = 0;
  int status5 = 0;

  // Cross
  for (unsigned int i = 0; i < 10; ++i)
    {
    double x0 = vtkMath::Random(-1.0, 1.0);
    double x1 = vtkMath::Random(-1.0, 1.0);
    double x2 = vtkMath::Random(-1.0, 1.0);
    parser->SetVectorVariableValue("x", x0, x1, x2);

    double y0 = vtkMath::Random(-1.0, 1.0);
    double y1 = vtkMath::Random(-1.0, 1.0);
    double y2 = vtkMath::Random(-1.0, 1.0);
    parser->SetVectorVariableValue("y", y0, y1, y2);

    parser->SetFunction("cross(x,y)");
    double *result = parser->GetVectorResult();
    double axb[3];
    axb[0] = result[0];
    axb[1] = result[1];
    axb[2] = result[2];
    // repeat to cover a 0 return from Evaulate()
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
            axb[j], minusBxa[j],
            std::numeric_limits<double>::epsilon() * 1.0))
        {
        std::cout << " Cross expected " << minusBxa[j]
                  << " but got " << axb[j];
        std::cout << "eps ratio is: " << (axb[j] - minusBxa[j])
          / std::numeric_limits<double>::epsilon() << std::endl;
        ++status1;
        }
      }
    }
  if (status1 == 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    std::cout << "FAILED\n";
    }

  // Add / Subtract / Multiply / Unary / Dot / Mag / Norm
  std::cout << "Testing Add / Subtract / Multiply / Unary / Dot" << "...";
  for (unsigned int i = 0; i < 10; ++i)
    {
    double x0 = vtkMath::Random(-1.0, 1.0);
    double x1 = vtkMath::Random(-1.0, 1.0);
    double x2 = vtkMath::Random(-1.0, 1.0);
    parser->SetVectorVariableValue("x", x0, x1, x2);

    double y0 = vtkMath::Random(-1.0, 1.0);
    double y1 = vtkMath::Random(-1.0, 1.0);
    double y2 = vtkMath::Random(-1.0, 1.0);
    parser->SetVectorVariableValue("y", y0, y1, y2);

    parser->SetScalarVariableValue("t", 2.0);
    parser->SetFunction("t*(x + y - (x + y))/t");
    double *result = parser->GetVectorResult();
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
      if (!vtkMathUtilities::FuzzyCompare(
            a[j], b[j],
            std::numeric_limits<double>::epsilon() * 1.0))
        {
        std::cout << " Cross expected " << a[j]
                  << " but got " << b[j];
        std::cout << "eps ratio is: " << (a[j] - b[j])
          / std::numeric_limits<double>::epsilon() << std::endl;
        ++status2;
        }
      }
    // Test Dot / Mag / Norm
    // a x b dot a = 0
    parser->SetFunction("cross(x, y).x");
    double dot = parser->GetScalarResult();
    if (!vtkMathUtilities::FuzzyCompare(
          dot, 0.0,
          std::numeric_limits<double>::epsilon() * 1.0))
      {
      std::cout << " Dot " << 0.0
                << " but got " << dot;
      std::cout << "eps ratio is: " << (dot - 0.0)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status3;
      }

    // Test Mag and Norm
    // max(norm(x) == 1
    parser->SetFunction("mag(norm(x))");
    double mag = parser->GetScalarResult();
    if (!vtkMathUtilities::FuzzyCompare(
          mag, 1.0,
          std::numeric_limits<double>::epsilon() * 2.0))
      {
      std::cout << " Mag expected" << 1.0
                << " but got " << mag;
      std::cout << " eps ratio is: " << (mag - 1.0)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status4;
      }
    }

  // x *iHat + y * jHat + z * zHat
  parser->SetScalarVariableValue("x", 1.0);
  parser->SetScalarVariableValue("y", 2.0);
  parser->SetScalarVariableValue("z", 3.0);
  parser->SetFunction("x*iHat + y*jHat + z*kHat");
  double *xyz = parser->GetVectorResult();
  if (xyz[0] != 1.0 ||
      xyz[1] != 2.0 ||
      xyz[2] != 3.0)
    {
    std::cout << "x*iHat + y*jHat + z*kHat expected "
              << "(" << 1.0
              << "," << 2.0
              << "," << 3.0 << ") but got "
              << "(" << xyz[0]
              << "," << xyz[1]
              << "," << xyz[2] << ")" << std::endl;
    ++status5;
    }

  // Test printing of an initialized parser
  std::ostringstream parserPrint;
  parser->Print(parserPrint);

  // Now clear the variables
  parser->RemoveAllVariables();
  if (parser->GetNumberOfScalarVariables() != 0 ||
      parser->GetNumberOfVectorVariables() != 0)
    {
    std::cout << "RemoveAllVariables failed" << std::endl;
    ++status1;
    }

  // Invalidate function should change the function's mtime
  unsigned long int before = parser->GetMTime();
  parser->InvalidateFunction();
  unsigned long int after = parser->GetMTime();

  if (before >= after)
    {
    std::cout << "InvalidateFunction() failed. MTime should have been modified" << std::endl;
    ++status5;
    }

  if (status1 + status2 + status3 + status4 + status5 == 0)
    {
    std::cout << "PASSED\n";
    }
  return status1 + status2 + status3 + status4 + status5;
}

int TestMinMax()
{
  std::cout << "Testing Min/Max" << "...";\
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();

  parser->SetFunction("min(x,y)");

  int status = 0;
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double value = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", value);
    parser->SetScalarVariableValue("y", -value);

    double result = parser->GetScalarResult();
    double expected = std::min(value, -value);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected,
          std::numeric_limits<double>::epsilon() * 1.0))
      {
      std::cout << "\n";
      std::cout << "Min Expected " << expected
                << " but got " << result
                << " difference is " << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected)
        / std::numeric_limits<double>::epsilon() << std::endl;
      status++;
      }
    }

  parser->SetFunction("max(x,y)");

  for (unsigned int i = 0; i < 1000; ++i)
    {
    double value = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", value);
    parser->SetScalarVariableValue("y", -value);

    double result = parser->GetScalarResult();
    double expected = std::max(value, -value);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected,
          std::numeric_limits<double>::epsilon() * 1.0))
      {
      std::cout << "\n";
      std::cout << "Max Expected " << expected
                << " but got " << result
                << " difference is " << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected)
        / std::numeric_limits<double>::epsilon() << std::endl;
      status++;
      }
    }

  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  return status;
}

int TestScalarLogic()
{
  int status = 0;

  std::cout << "Testing Scalar Logic" << "...";\
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();

  parser->SetFunction("if(x < y, x, y)");
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x < y ? x : y;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " < " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  parser->SetFunction("if(x > y, x, y)");
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x > y ? x : y;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " > " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  parser->SetFunction("if(x = y, x, 0.0)");
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = x;
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double result = parser->GetScalarResult();
    double expected = x == y ? x : 0.0;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " == " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  double ii[] = {0.0, 0.0, 1.0, 1.0};
  double jj[] = {0.0, 1.0, 0.0, 1.0};
  double expectedOr[]  = {0.0, 1.0, 1.0, 1.0};
  double expectedAnd[] = {.0, 0.0, 0.0, 1.0};

  parser->SetFunction("i | j");
  for (int i = 0; i < 3; ++i)
    {
    parser->SetScalarVariableValue("i", ii[i]);
    parser->SetScalarVariableValue("j", jj[i]);
    double result = parser->GetScalarResult();
    if (result != expectedOr[i])
      {
      std::cout << "i | j expected "
                << expectedOr[i]
                << " but got "
                << result << std::endl;
      ++status;
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
      std::cout << "i | j expected "
                << expectedAnd[i]
                << " but got "
                << result << std::endl;
      ++status;
      }
    }

  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    std::cout << "FAILED\n";
    }
  return status;
}

int TestVectorLogic()
{
  int status = 0;

  std::cout << "Testing Vector Logic" << "...";\
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();

  parser->SetFunction("if(x < y, v, w)");
  for (unsigned int i = 0; i < 1000; ++i)
    {

    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = vtkMath::Random(-1000.0, 1000.0);
    double v2 = vtkMath::Random(-1000.0, 1000.0);
    double v3 = vtkMath::Random(-1000.0, 1000.0);
    double w1 = vtkMath::Random(-1000.0, 1000.0);
    double w2 = vtkMath::Random(-1000.0, 1000.0);
    double w3 = vtkMath::Random(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x < y ? v1 : w1;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " < " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  parser->SetFunction("if(x > y, v, w)");
  for (unsigned int i = 0; i < 1000; ++i)
    {

    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = vtkMath::Random(-1000.0, 1000.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = vtkMath::Random(-1000.0, 1000.0);
    double v2 = vtkMath::Random(-1000.0, 1000.0);
    double v3 = vtkMath::Random(-1000.0, 1000.0);
    double w1 = vtkMath::Random(-1000.0, 1000.0);
    double w2 = vtkMath::Random(-1000.0, 1000.0);
    double w3 = vtkMath::Random(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x > y ? v1 : w1;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " > " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  parser->SetFunction("if(x = y, w, v * 0.0)");
  for (unsigned int i = 0; i < 1000; ++i)
    {

    double x = vtkMath::Random(-1000.0, 1000.0);
    double y = x;
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);

    double v1 = vtkMath::Random(-1000.0, 1000.0);
    double v2 = vtkMath::Random(-1000.0, 1000.0);
    double v3 = vtkMath::Random(-1000.0, 1000.0);
    double w1 = vtkMath::Random(-1000.0, 1000.0);
    double w2 = vtkMath::Random(-1000.0, 1000.0);
    double w3 = vtkMath::Random(-1000.0, 1000.0);
    parser->SetVectorVariableValue("v", v1, v2, v3);
    parser->SetVectorVariableValue("w", w1, w2, w3);

    double result = parser->GetVectorResult()[0];
    double expected = x > y ? v1 : w1;
    if (result != expected)
      {
      std::cout << "\n";
      std::cout << x << " == " << y << " Expected " << expected << " but got " << result << std::endl;
      status++;
      }
    }

  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    std::cout << "FAILED\n";
    }
  return status;
}

int TestMiscFunctions()
{
  int statusAll = 0;

  std::cout << "Testing Sign" << "...";
  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();
  parser->SetFunction("sign(x)");
  double values[3] = {-100.0, 0.0, 100.0};
  double expecteds[3] = {-1.0, 0.0, 1.0};

  int status = 0;
  for (unsigned int i = 0; i < 3; ++i)
    {
    parser->SetScalarVariableValue("x", values[i]);
    double result = parser->GetScalarResult();
    if (result != expecteds[i])
      {
      std::cout << "Sign expected " << expecteds[i]
                << " but got " << result << ". ";
      ++status;
      }
    }

  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    ++statusAll;
    std::cout << "FAILED\n";
    }

  std::cout << "Testing Pow" << "...";
  status = 0;
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double x = vtkMath::Random(0.0, 10.0);
    double y = vtkMath::Random(0.0, 2.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);
    parser->SetFunction("x ^ y");
    double result = parser->GetScalarResult();
    double expected = std::pow(x, y);
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected,
          std::numeric_limits<double>::epsilon() * 1.0))
      {
      std::cout << "\n";
      std::cout <<  " pow Expected " << expected
                << " but got " << result
                << " difference is " << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status;
      }
    }
  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    ++statusAll;
    std::cout << "FAILED\n";
    }

  std::cout << "Testing Scalar divide" << "...";
  status = 0;
  for (unsigned int i = 0; i < 1000; ++i)
    {
    double x = vtkMath::Random(-10.0, 10.0);
    double y = vtkMath::Random(-10.0, 10.0);
    parser->SetScalarVariableValue("x", x);
    parser->SetScalarVariableValue("y", y);
    parser->SetFunction("x / y");
    double result = parser->GetScalarResult();
    double expected = x / y;
    if (!vtkMathUtilities::FuzzyCompare(
          result, expected,
          std::numeric_limits<double>::epsilon() * 1.0))
      {
      std::cout << "\n";
      std::cout <<  " x / y Expected " << expected
                << " but got " << result
                << " difference is " << result - expected << " ";
      std::cout << "eps ratio is: " << (result - expected)
        / std::numeric_limits<double>::epsilon() << std::endl;
      ++status;
      }
    }
  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    ++statusAll;
    std::cout << "FAILED\n";
    }

  // SetScalarVariableValue
  std::cout << "Testing SetScalarVariableValue...";
  parser->SetScalarVariableValue(parser->GetScalarVariableName(0), 123.456);
  if (parser->GetScalarVariableValue(parser->GetScalarVariableName(0)) != 123.456)
    {
    ++statusAll;
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
  testFuncs.push_back("sqrt(s)");
  testFuncs.push_back("log(s)");
  testFuncs.push_back("ln(s)");
  testFuncs.push_back("log10(s)");
  testFuncs.push_back("asin(s)");
  testFuncs.push_back("acos(s)");
  testFuncs.push_back("s/zero");

  parser->ReplaceInvalidValuesOn();
  parser->SetReplacementValue(1234.5);
  parser->SetScalarVariableValue("s", -1000.0);
  parser->SetScalarVariableValue("zero", 0.0);

  for (size_t f = 0; f < testFuncs.size(); ++f)
    {
    parser->SetFunction(testFuncs[f].c_str());
    if (parser->GetScalarResult() != 1234.5)
      {
      std::cout << testFuncs[f] << " failed to return a replacement value when ReplaceInvaliValues was On" << std::endl;
      ++statusAll;
      }
    }
  parser->GetScalarResult();
  return statusAll;
}

int TestErrors()
{
  int status = 0;
  std::cout << "Testing Errors" << "...";

  vtkSmartPointer<vtkFunctionParser> parser =
    vtkSmartPointer<vtkFunctionParser>::New();

  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  parser->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  // Parse: no function has been set
  parser->SetFunction("cos(a)");
  parser->SetFunction(NULL);
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Parse: no function has been set");

  double s = -2.0;
  double v[3] = {1.0, 2.0, 3.0};
  double w[3] = {2.0, 1.0, 0.0};
  parser->SetScalarVariableValue("s", s);
  parser->SetScalarVariableValue("zero", 0.0);
  parser->SetVectorVariableValue("v", v[0], v[1], v[2]);
  parser->SetVectorVariableValue("w", w[0], w[1], w[2]);

  // addition expects either 2 vectors or 2 scalars
  parser->SetFunction("s + v");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("addition expects either 2 vectors or 2 scalars");

  // subtraction expects either 2 vectors or 2 scalars
  parser->SetFunction("s - v");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("subtraction expects either 2 vectors or 2 scalars");

  // multiply expecting either 2 scalars or a scalar and a vector
  parser->SetFunction("v * w");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("multiply expecting either 2 scalars or a scalar and a vector");

  // can't divide vectors
  parser->SetFunction("v / w");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("can't divide vectors");

  // can't raise a vector to a power
  parser->SetFunction("v ^ 2");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("can't raise a vector to a power");

  // Vectors cannot be used in boolean expressions
  parser->SetFunction("v | w");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Vectors cannot be used in boolean expressions");

  // expecting a scalar, but got a vector
  parser->SetFunction("cos(v)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("expecting a scalar, but got a vector");

  // can't apply min to vectors
  parser->SetFunction("min(v,w)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("can't apply min to vectors");

  // can't apply max to vectors
  parser->SetFunction("max(v,w)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("can't apply max to vectors");

  // can't apply cross to scalars
  parser->SetFunction("cross(s,w)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("can't apply cross to scalars");

  // dot product does not operate on scalars
  parser->SetFunction("s . v");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("dot product does not operate on scalars");

  // magnitude expects a vector, but got a scalar
  parser->SetFunction("mag(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("magnitude expects a vector, but got a scalar");

  // normalize expects a vector, but got a scalar
  parser->SetFunction("norm(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("normalize expects a vector, but got a scalar");

  // first argument of if(bool,valtrue,valfalse) cannot be a vector
  parser->SetFunction("if(v,s,s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("first argument of if(bool,valtrue,valfalse) cannot be a vector");

  // first argument of if(bool,valtrue,valfalse) cannot be a vector
  parser->SetFunction("if(v,s,s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("first argument of if(bool,valtrue,valfalse) cannot be a vector");

  // the if function expects the second and third arguments to be either 2 vectors or 2 scalars
  parser->SetFunction("if(s,v,s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("the if function expects the second and third arguments to be either 2 vectors or 2 scalars");

  // Trying to take a natural logarithm of a negative value
  parser->SetFunction("ln(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take a natural logarithm of a negative value");

  // Trying to take a natural logarithm of a negative value
  parser->SetFunction("ln(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take a natural logarithm of a negative value");

  // Trying to take a log10 of a negative value
  parser->SetFunction("log10(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take a log10 of a negative value");

  // Trying to take a log of a negative value
  parser->SetFunction("log(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take a log of a negative value");

  // Trying to take a square root of a negative value
  parser->SetFunction("sqrt(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take a square root of a negative value");

  // Trying to take asin of a value < -1 or > 1
  parser->SetFunction("asin(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take asin of a value < -1 or > 1");

  // Trying to take acos of a value < -1 or > 1
  parser->SetFunction("acos(s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to take acos of a value < -1 or > 1");

  // Trying to divide by zero<
  parser->SetFunction("s/zero");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Trying to divide by zero");

  // GetScalarResult: no valid scalar result
  parser->SetFunction("cross(v,w)");
  parser->GetScalarResult();
  CHECK_ERROR_MSG("GetScalarResult: no valid scalar result");

  // GetVectorResult: no valid vector result
  parser->SetFunction("v . w");
  parser->GetVectorResult();
  CHECK_ERROR_MSG("GetVectorResult: no valid vector result");

  // GetScalarVariableValue: scalar variable name ... does not exist
  parser->GetScalarVariableValue("xyz");
  CHECK_ERROR_MSG("GetScalarVariableValue: scalar variable name");

  // GetScalarVariableValue: scalar variable number ... does not exist
  parser->GetScalarVariableValue(128);
  CHECK_ERROR_MSG("GetScalarVariableValue: scalar variable number");

  // GetVectorVariableValue: vector variable name ... does not exist
  parser->GetVectorVariableValue("xyz");
  CHECK_ERROR_MSG("GetVectorVariableValue: vector variable name");

  // GetVectorVariableValue: vector variable number ... does not exist
  parser->GetVectorVariableValue(128);
  CHECK_ERROR_MSG("GetVectorVariableValue: vector variable number");

  // Syntax error: expecting a variable name
  parser->SetFunction("acos()");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax error: expecting a variable name");

  // The use of log function is being deprecated
  parser->SetFunction("log(1.0)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("The use of log function is being deprecated");

  // Parse errors
  parser->SetFunction("-");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax error: unary minus with no operand");

  parser->SetFunction("s *");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax error: expecting a variable name");

  parser->SetFunction("cross(v)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax Error: two parameters separated by commas expected");

  parser->SetFunction("if(v,s)");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax Error: three parameters separated by commas expected");

  parser->SetFunction("s * (v + w");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax Error: missing closing parenthesis");

  parser->SetFunction("v + w)*s");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax Error: mismatched parenthesis");

  parser->SetFunction("s s");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax error: operator expected");

#if 0
  parser->SetFunction("s*()");
  parser->IsScalarResult();
  CHECK_ERROR_MSG("Syntax Error: empty parentheses");
#endif

  if (status== 0)
    {
    std::cout << "PASSED\n";
    }
  else
    {
    std::cout << "FAILED\n";
    }
  return status;
}
