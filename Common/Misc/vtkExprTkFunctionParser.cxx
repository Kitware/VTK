/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExprTkFunctionParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExprTkFunctionParser.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cctype>
#include <random>
#include <regex>

// exprtk macros
#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io_file
#define exprtk_disable_caseinsensitivity
#include "vtk_exprtk.h"
#include "vtksys/SystemTools.hxx"

using ExprTkResultType = exprtk::results_context<double>::type_store_t::store_type;

/**
 * Implementation of vtkExprTkTools
 */
struct vtkExprTkTools
{
  exprtk::symbol_table<double> SymbolTable;
  exprtk::expression<double> Expression;
  exprtk::parser<double> Parser;
};

/**
 * Implementation of the magnitude function
 */
template <typename T>
class mag : public exprtk::igeneric_function<T>
{
public:
  typedef typename exprtk::igeneric_function<T> igfun_t;
  typedef typename igfun_t::parameter_list_t parameter_list_t;
  typedef typename igfun_t::generic_type generic_type;
  typedef typename generic_type::scalar_view scalar_t;
  typedef typename generic_type::vector_view vector_t;

  using exprtk::igeneric_function<T>::operator();

  mag()
    : exprtk::igeneric_function<T>("V|VTT")
  /*
     Overloads:
     0. V   - x(vector)
     1. VTT - x(vector), r0, r1
  */
  {
  }

  inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
  {
    const vector_t x(parameters[0]);

    std::size_t r0 = 0;
    std::size_t r1 = x.size() - 1;

    if ((1 == ps_index) &&
      !exprtk::rtl::vecops::helper::load_vector_range<T>::process(parameters, r0, r1, 2, 3, 0))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }
    else if (exprtk::rtl::vecops::helper::invalid_range(x, r0, r1))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }

    T result = T(0);

    for (std::size_t i = r0; i <= r1; ++i)
    {
      result += (x[i] * x[i]);
    }
    result = std::sqrt(result);

    return result;
  }
};

/**
 * Implementation of the x element of cross product function
 */
template <typename T>
class crossX : public exprtk::igeneric_function<T>
{
public:
  typedef typename exprtk::igeneric_function<T> igfun_t;
  typedef typename igfun_t::parameter_list_t parameter_list_t;
  typedef typename igfun_t::generic_type generic_type;
  typedef typename generic_type::scalar_view scalar_t;
  typedef typename generic_type::vector_view vector_t;

  using exprtk::igeneric_function<T>::operator();

  crossX()
    : exprtk::igeneric_function<T>("VV|VVTT")
  /*
     Overloads:
     0. VV   - x(vector), y(vector)
     1. VVTT - x(vector), y(vector), r0, r1
  */
  {
  }

  inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
  {
    const vector_t x(parameters[0]);
    const vector_t y(parameters[1]);

    std::size_t r0 = 0;
    std::size_t r1 = std::min(x.size(), y.size()) - 1;

    if ((1 == ps_index) &&
      !exprtk::rtl::vecops::helper::load_vector_range<T>::process(parameters, r0, r1, 2, 3, 0))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }

    T result = x[1] * y[2] - x[2] * y[1];

    return result;
  }
};

/**
 * Implementation of the y element of cross product function
 */
template <typename T>
class crossY : public exprtk::igeneric_function<T>
{
public:
  typedef typename exprtk::igeneric_function<T> igfun_t;
  typedef typename igfun_t::parameter_list_t parameter_list_t;
  typedef typename igfun_t::generic_type generic_type;
  typedef typename generic_type::scalar_view scalar_t;
  typedef typename generic_type::vector_view vector_t;

  using exprtk::igeneric_function<T>::operator();

  crossY()
    : exprtk::igeneric_function<T>("VV|VVTT")
  /*
     Overloads:
     0. VV   - x(vector), y(vector)
     1. VVTT - x(vector), y(vector), r0, r1
  */
  {
  }

  inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
  {
    const vector_t x(parameters[0]);
    const vector_t y(parameters[1]);

    std::size_t r0 = 0;
    std::size_t r1 = std::min(x.size(), y.size()) - 1;

    if ((1 == ps_index) &&
      !exprtk::rtl::vecops::helper::load_vector_range<T>::process(parameters, r0, r1, 2, 3, 0))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }

    T result = x[2] * y[0] - x[0] * y[2];

    return result;
  }
};

/**
 * Implementation of the z element of cross product function
 */
template <typename T>
class crossZ : public exprtk::igeneric_function<T>
{
public:
  typedef typename exprtk::igeneric_function<T> igfun_t;
  typedef typename igfun_t::parameter_list_t parameter_list_t;
  typedef typename igfun_t::generic_type generic_type;
  typedef typename generic_type::scalar_view scalar_t;
  typedef typename generic_type::vector_view vector_t;

  using exprtk::igeneric_function<T>::operator();

  crossZ()
    : exprtk::igeneric_function<T>("VV|VVTT")
  /*
     Overloads:
     0. VV   - x(vector), y(vector)
     1. VVTT - x(vector), y(vector), r0, r1
  */
  {
  }

  inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
  {
    const vector_t x(parameters[0]);
    const vector_t y(parameters[1]);

    std::size_t r0 = 0;
    std::size_t r1 = std::min(x.size(), y.size()) - 1;

    if ((1 == ps_index) &&
      !exprtk::rtl::vecops::helper::load_vector_range<T>::process(parameters, r0, r1, 2, 3, 0))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
    {
      return std::numeric_limits<T>::quiet_NaN();
    }

    T result = x[0] * y[1] - x[1] * y[0];

    return result;
  }
};

namespace
{
/**
 * Implementation of sign function.
 */
inline double sign(double v)
{
  if (v == 0.)
  {
    return 0.;
  }
  else if (std::signbit(v))
  {
    return -1.;
  }
  else
  {
    return 1.;
  }
}

// compile-time declaration of needed function/variables/vectors/packages
// these are useful to minimize the construction cost, especially when
// multiple instances of this class are instantiated
exprtk::rtl::vecops::package<double> vectorOperationsPackage;
std::vector<double> iHat = { 1, 0, 0 };
std::vector<double> jHat = { 0, 1, 0 };
std::vector<double> kHat = { 0, 0, 1 };
mag<double> magnitude;
crossX<double> crossXProduct;
crossY<double> crossYProduct;
crossZ<double> crossZProduct;

// the value that is returned as a result if there is an error
double vtkParserErrorResult = std::numeric_limits<double>::quiet_NaN();
double vtkParserVectorErrorResult[3] = { vtkParserErrorResult, vtkParserErrorResult,
  vtkParserErrorResult };

//------------------------------------------------------------------------------
std::string RemoveSpacesFrom(std::string str)
{
  str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
  return str;
}

//------------------------------------------------------------------------------
bool HasEnding(const std::string& fullString, const std::string& ending)
{
  if (fullString.size() >= ending.size())
  {
    return (fullString.compare(fullString.size() - ending.size(), ending.size(), ending) == 0);
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
std::string GenerateRandomAlphabeticString(unsigned int len)
{
  static constexpr auto chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz";
  thread_local auto rng = std::default_random_engine(std::random_device{}());
  auto dist = std::uniform_int_distribution<int>(0, static_cast<int>(std::strlen(chars) - 1));
  auto result = std::string(len, '\0');
  std::generate_n(begin(result), len, [&]() { return chars[dist(rng)]; });

  return result;
}

//------------------------------------------------------------------------------
std::string GenerateUniqueVariableName(
  const std::vector<std::string>& variableNames, const std::string& variableName)
{
  std::string sanitizedName = vtkExprTkFunctionParser::SanitizeName(variableName.c_str());
  do
  {
    sanitizedName += GenerateRandomAlphabeticString(5);
  } while (
    std::find(variableNames.begin(), variableNames.end(), sanitizedName) != variableNames.end());

  return sanitizedName;
}
}

vtkStandardNewMacro(vtkExprTkFunctionParser);

//------------------------------------------------------------------------------
vtkExprTkFunctionParser::vtkExprTkFunctionParser()
{
  this->EvaluateMTime.Modified();
  this->VariableMTime.Modified();
  this->ParseMTime.Modified();
  this->FunctionMTime.Modified();

  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;

  this->ExprTkTools = new vtkExprTkTools;
  // add vector support
  this->ExprTkTools->SymbolTable.add_package(vectorOperationsPackage);
  // add unit vectors
  this->ExprTkTools->SymbolTable.add_vector("iHat", iHat);
  this->ExprTkTools->SymbolTable.add_vector("jHat", jHat);
  this->ExprTkTools->SymbolTable.add_vector("kHat", kHat);
  // add ln and sign
  this->ExprTkTools->SymbolTable.add_function("ln", std::log);
  this->ExprTkTools->SymbolTable.add_function("sign", sign);
  // add magnitude function
  this->ExprTkTools->SymbolTable.add_function("mag", magnitude);
  // add functions which are used to implement cross product
  this->ExprTkTools->SymbolTable.add_function("crossX", crossXProduct);
  this->ExprTkTools->SymbolTable.add_function("crossY", crossYProduct);
  this->ExprTkTools->SymbolTable.add_function("crossZ", crossZProduct);
  // register symbol table
  this->ExprTkTools->Expression.register_symbol_table(this->ExprTkTools->SymbolTable);
  // enable the collection of variables, which will be used in UpdateNeededVariables
  this->ExprTkTools->Parser.dec().collect_variables() = true;
}

//------------------------------------------------------------------------------
vtkExprTkFunctionParser::~vtkExprTkFunctionParser()
{
  this->RemoveAllVariables();
  delete this->ExprTkTools;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetFunction(const char* function)
{
  // check if we have already set the same function string
  if (!this->Function.empty() && function && this->Function == function)
  {
    return;
  }

  if (function)
  {
    this->Function = function;
    this->FunctionWithUsedVariableNames = this->Function;
  }
  else
  {
    this->Function = std::string();
    this->FunctionWithUsedVariableNames = std::string();
  }

  this->FunctionMTime.Modified();
  this->ScalarVariableNeeded.clear();
  this->VectorVariableNeeded.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::Parse(ParseMode mode)
{
  if (this->Function.empty())
  {
    vtkErrorMacro("Parse: no function has been set");
    return 0;
  }

  // During the parsing of the first mode, perform the necessary changes in the function
  if (mode == ParseMode::DetectReturnType)
  {
    // Before parsing, replace the original variable names in the function
    // with the valid ones if needed.
    for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); ++i)
    {
      if (this->OriginalScalarVariableNames[i] != this->UsedScalarVariableNames[i])
      {
        vtksys::SystemTools::ReplaceString(this->FunctionWithUsedVariableNames,
          this->OriginalScalarVariableNames[i], this->UsedScalarVariableNames[i]);
      }
    }
    for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); ++i)
    {
      if (this->OriginalVectorVariableNames[i] != this->UsedVectorVariableNames[i])
      {
        vtksys::SystemTools::ReplaceString(this->FunctionWithUsedVariableNames,
          this->OriginalVectorVariableNames[i], this->UsedVectorVariableNames[i]);
      }
    }

    // remove spaces to perform replacement for norm and cross
    this->FunctionWithUsedVariableNames = RemoveSpacesFrom(this->FunctionWithUsedVariableNames);

    // check for usage of old dot format product, e.g. (v1.v2) instead of dot(v1,v2)
    if (this->CheckOldFormatOfDotProductUsage())
    {
      std::string oldDotUsageError =
        "Warn: 0000 Type: [Old Usage] Msg: "
        "Possible usage of old format of dot product v1.v2. Please use dot(v1,v2)."
        "\tExpression: " +
        this->Function + '\n';
      vtkWarningMacro(<< oldDotUsageError);
    }

    // fix cross occurrences with something that ExprTk can understand
    this->FunctionWithUsedVariableNames =
      FixVectorReturningFunctionOccurrences(VectorReturningFunction::Cross);

    // fix norm occurrences with something that ExprTk can understand
    this->FunctionWithUsedVariableNames =
      FixVectorReturningFunctionOccurrences(VectorReturningFunction::Norm);
  }

  if (mode == ParseMode::DetectReturnType)
  {
    // ExprTK, in order to extract vector and scalar results, and identify the result type,
    // it requires to "return results" instead of just evaluating an expression
    this->ExpressionString = "return [" + this->FunctionWithUsedVariableNames + "];";
  }
  else
  {
    // Since we know now the return type, we can assign the result to a result scalar/vector
    std::string resultName = GenerateRandomAlphabeticString(10);
    if (this->ResultType == ExprTkResultType::e_scalar)
    {
      this->ExprTkTools->SymbolTable.add_variable(resultName, this->Result[0]);
      this->ExpressionString = resultName + " := " + this->FunctionWithUsedVariableNames + ";";
    }
    else
    {
      this->ExprTkTools->SymbolTable.add_vector(
        resultName, this->Result.GetData(), this->Result.GetSize());
      this->ExpressionString = resultName + " := [" + this->FunctionWithUsedVariableNames + "];";
    }
  }

  bool parsingResult =
    this->ExprTkTools->Parser.compile(this->ExpressionString, this->ExprTkTools->Expression);
  // check parsing result
  if (!parsingResult)
  {
    // print error only once
    if (mode == ParseMode::DetectReturnType)
    {
      std::stringstream parsingErrorStream;
      // save error
      for (std::size_t i = 0; i < this->ExprTkTools->Parser.error_count(); ++i)
      {
        auto error = this->ExprTkTools->Parser.get_error(i);
        parsingErrorStream << "Err: " << i << " Type: [" << exprtk::parser_error::to_str(error.mode)
                           << "] Msg: " << error.diagnostic << "\tExpression: " << this->Function
                           << "\n";
      }
      vtkErrorMacro(<< parsingErrorStream.str());
    }

    return 0;
  }

  if (mode == ParseMode::DetectReturnType)
  {
    // Collect meta-data about variables that are needed for evaluation of the
    // function.
    this->UpdateNeededVariables();
  }
  this->ParseMTime.Modified();
  return 1;
}

std::string vtkExprTkFunctionParser::FixVectorReturningFunctionOccurrences(
  VectorReturningFunction vectorReturningFunction)
{
  std::string desiredFunction;
  std::string functionWithoutParenthesis;
  if (vectorReturningFunction == VectorReturningFunction::Cross)
  {
    desiredFunction = "cross(";
    functionWithoutParenthesis = "cross";
  }
  else
  {
    desiredFunction = "norm(";
    functionWithoutParenthesis = "norm";
  }

  // collect all the variables that end with the desired function, e.g. mycross, m1cross
  std::vector<std::string> variableNamesContainingFunction;
  for (const auto& scalarVariable : this->UsedScalarVariableNames)
  {
    if (HasEnding(scalarVariable, functionWithoutParenthesis))
    {
      variableNamesContainingFunction.push_back(scalarVariable);
    }
  }
  for (const auto& vectorVariable : this->UsedVectorVariableNames)
  {
    if (HasEnding(vectorVariable, functionWithoutParenthesis))
    {
      variableNamesContainingFunction.push_back(vectorVariable);
    }
  }
  // sort vector by size to ensure that the largest variables names will be checked first
  std::sort(variableNamesContainingFunction.begin(), variableNamesContainingFunction.end(),
    [](const std::string& s1, const std::string& s2) -> bool { return s1.size() > s2.size(); });

  static const std::string allowedChars = "01234565789.,()+-*/%^|&=<>!";
  std::string::size_type pos = 0;
  std::string function = this->FunctionWithUsedVariableNames;
  while ((pos = function.find(desiredFunction, pos)) != std::string::npos)
  {
    // if we are not in the beginning
    if (static_cast<int>(pos) - 1 != -1)
    {
      // check the found occurrence if it's part of a variable
      // this check is required because the previous character could be a number
      // and that is part of a variable name which includes cross, such m1cross
      bool foundVariableOccurrence = false;
      for (const auto& variable : variableNamesContainingFunction)
      {
        // check the size of the variable vs the function
        if (variable.size() >= functionWithoutParenthesis.size())
        {
          const int sizeDifference =
            static_cast<int>(variable.size() - functionWithoutParenthesis.size());
          // check pos to not exceed the beginning
          if (static_cast<int>(pos) - sizeDifference >= 0)
          {
            // check if occurrence match variable
            if (function.substr(pos - sizeDifference, variable.size()) == variable)
            {
              foundVariableOccurrence = true;
              break;
            }
          }
        }
      }
      // skip the found occurrence if it's part of a variable
      if (foundVariableOccurrence)
      {
        pos += desiredFunction.size();
        continue;
      }

      // check if a character that is allowed is found
      bool allowedCharFound = false;
      for (char allowedChar : allowedChars)
      {
        if (function[pos - 1] == allowedChar)
        {
          allowedCharFound = true;
          break;
        }
      }

      // skip the found occurrence if no allowed character has been found
      if (!allowedCharFound)
      {
        pos += desiredFunction.size();
        continue;
      }
    }

    pos += desiredFunction.size();

    // match the number of parenthesis
    int leftParenthesis = 1; // 1 because we have already detected one
    int rightParenthesis = 0;
    std::stringstream interior;
    for (size_t i = pos; i < function.size(); ++i)
    {
      if (function[i] == ')')
      {
        ++rightParenthesis;
      }
      if (function[i] == '(')
      {
        ++leftParenthesis;
      }

      if (leftParenthesis == rightParenthesis)
      {
        break;
      }
      else
      {
        interior << function[i];
      }
    }
    // if the number of left and right parenthesis is equal, then replace appropriately
    if (rightParenthesis == leftParenthesis)
    {
      // go back to replace
      pos -= desiredFunction.size();

      std::string replacement;
      if (vectorReturningFunction == VectorReturningFunction::Cross)
      {
        // (iHat*crossX(v1,v2)+jHat*crossY(v1,v2)+kHat*crossZ(v1,v2))
        replacement = "(iHat*crossX(" + interior.str() + ")" + "+jHat*crossY(" + interior.str() +
          ")" + "+kHat*crossZ(" + interior.str() + "))";
      }
      else
      {
        // ((v)/mag(v))
        replacement = "((" + interior.str() + ")/mag(" + interior.str() + "))";
      }

      // perform replacement, +1 is for the right parenthesis
      function.replace(pos, desiredFunction.size() + interior.str().size() + 1, replacement);
    }
    else
    {
      // ExprTk will catch it the parenthesis mismatch
      // ExprTk will also catch all the cases that the interior is not valid
      break;
    }
  }
  return function;
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::CheckOldFormatOfDotProductUsage()
{
  const std::string function = this->FunctionWithUsedVariableNames;
  std::string::size_type pos = 0;
  while ((pos = function.find('.', pos)) != std::string::npos)
  {
    // if we are not in the beginning
    if (static_cast<int>(pos) - 1 != -1)
    {
      // check if left character is digit
      bool leftCharacterIsDigit = false;
      if (std::isdigit(function[pos - 1]))
      {
        leftCharacterIsDigit = true;
      }

      // check if right character is digit
      bool rightCharacterIsDigit = false;
      // before that check, check if you can look at the right character
      if (pos + 1 < function.size())
      {
        if (std::isdigit(function[pos + 1]))
        {
          rightCharacterIsDigit = true;
        }
      }

      // both left character and right character are not digits
      // then this is a possible product usage
      if (!leftCharacterIsDigit && !rightCharacterIsDigit)
      {
        return true;
      }
      else
      {
        ++pos;
      }
    }
    else
    {
      // check if right character is number
      bool rightCharacterIsNumber = false;
      // before that check, check if you can look at the right character
      if (pos + 1 < function.size())
      {
        if (std::isdigit(function[pos + 1]))
        {
          rightCharacterIsNumber = true;
        }
      }
      // right character is not a number
      // then this is a possible product usage
      if (!rightCharacterIsNumber)
      {
        return true;
      }
      else
      {
        ++pos;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::InvalidateFunction()
{
  this->FunctionMTime.Modified();
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::Evaluate()
{
  if (this->FunctionMTime.GetMTime() > this->ParseMTime.GetMTime())
  {
    // compile with mode 0 to identify return type
    if (this->Parse(ParseMode::DetectReturnType) == 0)
    {
      return false;
    }
    // perform evaluation to identify the return type
    this->ExprTkTools->Expression.value();
    this->ResultType = this->ExprTkTools->Expression.results()[0].type;

    // compile with mode 1 to save results in the result array
    if (this->Parse(ParseMode::SaveResultInVariable) == 0)
    {
      return false;
    }
  }
  // perform evaluation
  this->ExprTkTools->Expression.value();

  switch (this->ResultType)
  {
    case ExprTkResultType::e_scalar:
      if (std::isnan(this->Result[0]) || std::isinf(this->Result[0]))
      {
        if (this->ReplaceInvalidValues)
        {
          this->Result[0] = this->ReplacementValue;
        }
        else
        {
          vtkErrorMacro("Invalid result because of mathematically wrong input.");
          return false;
        }
      }
      break;
    case ExprTkResultType::e_vector:
      for (int i = 0; i < 3; i++)
      {
        if (std::isnan(this->Result[i]) || std::isinf(this->Result[i]))
        {
          if (this->ReplaceInvalidValues)
          {
            this->Result[i] = this->ReplacementValue;
          }
          else
          {
            vtkErrorMacro("Invalid vector element result because of mathematically wrong input.");
            return false;
          }
        }
      }
      break;
    default:
      vtkErrorMacro("Not supported result type.");
      return false;
  }

  this->EvaluateMTime.Modified();

  return true;
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::IsScalarResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() ||
    this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
  {
    if (this->Evaluate() == false)
      return 0;
  }
  return (this->ResultType == ExprTkResultType::e_scalar);
}

//------------------------------------------------------------------------------
double vtkExprTkFunctionParser::GetScalarResult()
{
  if (!(this->IsScalarResult()))
  {
    vtkErrorMacro("GetScalarResult: no valid scalar result");
    return vtkParserErrorResult;
  }
  return this->Result[0];
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::IsVectorResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() ||
    this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
  {
    if (this->Evaluate() == false)
      return 0;
  }
  return (this->ResultType == ExprTkResultType::e_vector);
}

//------------------------------------------------------------------------------
double* vtkExprTkFunctionParser::GetVectorResult()
{
  if (!(this->IsVectorResult()))
  {
    vtkErrorMacro("GetVectorResult: no valid vector result");
    return vtkParserVectorErrorResult;
  }
  return this->Result.GetData();
}

//------------------------------------------------------------------------------
std::string vtkExprTkFunctionParser::GetScalarVariableName(int i)
{
  if (i >= 0 && i < this->GetNumberOfScalarVariables())
  {
    return this->OriginalScalarVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
std::string vtkExprTkFunctionParser::GetVectorVariableName(int i)
{
  if (i >= 0 && i < this->GetNumberOfVectorVariables())
  {
    return this->OriginalVectorVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetScalarVariableValue(
  const std::string& inVariableName, double value)
{
  if (inVariableName.empty())
  {
    vtkErrorMacro("Variable name is empty");
    return;
  }
  // check if variable name exists in vectors
  for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); i++)
  {
    if (this->OriginalVectorVariableNames[i] == inVariableName)
    {
      vtkErrorMacro("Scalar variable name is already registered as a vector variable name");
      return;
    }
  }
  // check if variable already exists
  for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); i++)
  {
    if (this->OriginalScalarVariableNames[i] == inVariableName)
    {
      if (*this->ScalarVariableValues[i] != value)
      {
        *this->ScalarVariableValues[i] = value;
        this->VariableMTime.Modified();
        this->Modified();
      }
      return;
    }
  }

  double* scalarValue = new double(value);
  // if variable name is not sanitized, create a unique sanitized string and set it as variable name
  std::string variableName = vtkExprTkFunctionParser::SanitizeName(inVariableName.c_str());
  if (variableName != inVariableName)
  {
    variableName = GenerateUniqueVariableName(this->UsedScalarVariableNames, inVariableName);
  }

  // check if variable is a registered keyword, e.g. sin().
  bool additionResult = this->ExprTkTools->SymbolTable.add_variable(variableName, *scalarValue);
  if (additionResult)
  {
    this->ScalarVariableValues.push_back(scalarValue);
    this->OriginalScalarVariableNames.push_back(inVariableName);
    this->UsedScalarVariableNames.push_back(variableName);

    this->VariableMTime.Modified();
    this->Modified();
  }
  else
  {
    delete scalarValue;
    vtkErrorMacro("Scalar variable `" << inVariableName << "` is a reserved keyword");
  }
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetScalarVariableValue(int i, double value)
{
  if (i < 0 || i >= this->GetNumberOfScalarVariables())
  {
    return;
  }

  if (*this->ScalarVariableValues[i] != value)
  {
    *this->ScalarVariableValues[i] = value;
    this->VariableMTime.Modified();
  }
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkExprTkFunctionParser::GetScalarVariableValue(const std::string& inVariableName)
{
  for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); i++)
  {
    if (this->OriginalScalarVariableNames[i] == inVariableName)
    {
      return *this->ScalarVariableValues[i];
    }
  }
  vtkErrorMacro(
    "GetScalarVariableValue: scalar variable name " << inVariableName << " does not exist");
  return vtkParserErrorResult;
}

//------------------------------------------------------------------------------
double vtkExprTkFunctionParser::GetScalarVariableValue(int i)
{
  if (i < 0 || i >= this->GetNumberOfScalarVariables())
  {
    vtkErrorMacro("GetScalarVariableValue: scalar variable number " << i << " does not exist");
    return vtkParserErrorResult;
  }

  return *this->ScalarVariableValues[i];
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetVectorVariableValue(
  const std::string& inVariableName, double xValue, double yValue, double zValue)
{
  if (inVariableName.empty())
  {
    vtkErrorMacro("Variable name is empty");
    return;
  }
  // check if variable name exists in vectors
  for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); i++)
  {
    if (this->OriginalScalarVariableNames[i] == inVariableName)
    {
      vtkErrorMacro("Vector variable name is already registered as a scalar variable name");
      return;
    }
  }
  // check if variable already exists
  for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); i++)
  {
    if (this->OriginalVectorVariableNames[i] == inVariableName)
    {
      if ((*this->VectorVariableValues[i])[0] != xValue ||
        (*this->VectorVariableValues[i])[1] != yValue ||
        (*this->VectorVariableValues[i])[2] != zValue)
      {
        (*this->VectorVariableValues[i])[0] = xValue;
        (*this->VectorVariableValues[i])[1] = yValue;
        (*this->VectorVariableValues[i])[2] = zValue;
        this->VariableMTime.Modified();
        this->Modified();
      }
      return;
    }
  }

  vtkTuple<double, 3>* vector = new vtkTuple<double, 3>();
  (*vector)[0] = xValue;
  (*vector)[1] = yValue;
  (*vector)[2] = zValue;

  // if variable name is not sanitized, create a unique sanitized string and set it as variable name
  std::string variableName = vtkExprTkFunctionParser::SanitizeName(inVariableName.c_str());
  if (variableName != inVariableName)
  {
    variableName = GenerateUniqueVariableName(this->UsedVectorVariableNames, inVariableName);
  }
  // check if variable is a registered keyword, e.g. sin().
  bool additionResult =
    this->ExprTkTools->SymbolTable.add_vector(variableName, vector->GetData(), vector->GetSize());
  if (additionResult)
  {
    this->VectorVariableValues.push_back(vector);
    this->OriginalVectorVariableNames.push_back(inVariableName);
    this->UsedVectorVariableNames.push_back(variableName);

    this->VariableMTime.Modified();
    this->Modified();
  }
  else
  {
    delete vector;
    vtkErrorMacro("Vector variable `" << inVariableName << "` is a reserved keyword");
  }
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetVectorVariableValue(
  int i, double xValue, double yValue, double zValue)
{
  if (i < 0 || i >= this->GetNumberOfVectorVariables())
  {
    return;
  }
  if ((*this->VectorVariableValues[i])[0] != xValue ||
    (*this->VectorVariableValues[i])[1] != yValue || (*this->VectorVariableValues[i])[2] != zValue)
  {
    (*this->VectorVariableValues[i])[0] = xValue;
    (*this->VectorVariableValues[i])[1] = yValue;
    (*this->VectorVariableValues[i])[2] = zValue;
    this->VariableMTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkExprTkFunctionParser::GetVectorVariableValue(const std::string& inVariableName)
{
  for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); i++)
  {
    if (this->OriginalVectorVariableNames[i] == inVariableName)
    {
      return this->VectorVariableValues[i]->GetData();
    }
  }
  vtkErrorMacro(
    "GetVectorVariableValue: vector variable name " << inVariableName << " does not exist");
  return vtkParserVectorErrorResult;
}

//------------------------------------------------------------------------------
double* vtkExprTkFunctionParser::GetVectorVariableValue(int i)
{
  if (i < 0 || i >= this->GetNumberOfVectorVariables())
  {
    vtkErrorMacro("GetVectorVariableValue: vector variable number " << i << " does not exist");
    return vtkParserVectorErrorResult;
  }
  return this->VectorVariableValues[i]->GetData();
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::RemoveScalarVariables()
{
  this->ExprTkTools->SymbolTable.clear_variables();
  this->OriginalScalarVariableNames.clear();
  this->UsedScalarVariableNames.clear();
  for (size_t i = 0; i < this->ScalarVariableValues.size(); ++i)
  {
    delete this->ScalarVariableValues[i];
  }
  this->ScalarVariableValues.clear();
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::RemoveVectorVariables()
{
  // we clear vector variables to avoid removing iHat,jHat, kHat
  for (size_t i = 0; i < this->UsedVectorVariableNames.size(); ++i)
  {
    this->ExprTkTools->SymbolTable.remove_vector(this->UsedVectorVariableNames[i]);
  }
  this->OriginalVectorVariableNames.clear();
  this->UsedVectorVariableNames.clear();
  for (size_t i = 0; i < this->VectorVariableValues.size(); ++i)
  {
    delete this->VectorVariableValues[i];
  }
  this->VectorVariableValues.clear();
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkExprTkFunctionParser::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->EvaluateMTime > mTime)
  {
    mTime = this->EvaluateMTime;
  }
  if (this->VariableMTime > mTime)
  {
    mTime = this->VariableMTime;
  }
  if (this->ParseMTime > mTime)
  {
    mTime = this->ParseMTime;
  }
  if (this->FunctionMTime > mTime)
  {
    mTime = this->FunctionMTime;
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Function: " << (this->GetFunction() ? this->GetFunction() : "(none)") << endl;

  os << indent << "FunctionWithUsedVariableNames: "
     << (!this->FunctionWithUsedVariableNames.empty() ? this->FunctionWithUsedVariableNames
                                                      : "(none)")
     << endl;

  os << indent << "ExpressionString: "
     << (!this->ExpressionString.empty() ? this->ExpressionString : "(none)") << endl;

  for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); i++)
  {
    os << indent << "  " << this->OriginalScalarVariableNames[i] << " / "
       << this->UsedScalarVariableNames[i] << ": " << (*this->ScalarVariableValues[i]) << endl;
  }

  for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); i++)
  {
    os << indent << "  " << this->OriginalVectorVariableNames[i] << " / "
       << this->UsedVectorVariableNames[i] << ": (" << (*this->VectorVariableValues[i])[0] << ", "
       << (*this->VectorVariableValues[i])[1] << ", " << (*this->VectorVariableValues[i])[2] << ")"
       << endl;
  }

  if (this->EvaluateMTime.GetMTime() > this->FunctionMTime.GetMTime() &&
    this->EvaluateMTime.GetMTime() > this->VariableMTime.GetMTime() &&
    this->ExprTkTools->Expression.results().count() > 0)
  {
    if (this->ResultType == ExprTkResultType::e_scalar)
    {
      os << indent << "ScalarResult: " << this->GetScalarResult() << endl;
      os << indent << "VectorResult: "
         << "(none)" << endl;
    }
    else
    {
      os << indent << "ScalarResult: "
         << "(none)" << endl;
      os << indent << "VectorResult: "
         << "(" << this->GetVectorResult()[0] << ", " << this->GetVectorResult()[1] << ", "
         << this->GetVectorResult()[2] << ")" << endl;
    }
  }
  else
  {
    os << indent << "ScalarResult: "
       << "(none)" << endl;
    os << indent << "VectorResult: "
       << "(none)" << endl;
  }

  os << indent << "Replace Invalid Values: " << (this->GetReplaceInvalidValues() ? "On" : "Off")
     << endl;
  os << indent << "Replacement Value: " << this->GetReplacementValue() << endl;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::UpdateNeededVariables()
{
  this->ScalarVariableNeeded.clear();
  this->ScalarVariableNeeded.resize(this->UsedScalarVariableNames.size(), false);

  this->VectorVariableNeeded.clear();
  this->VectorVariableNeeded.resize(this->UsedVectorVariableNames.size(), false);

  // store variables after parsing
  std::deque<exprtk::parser<double>::dependent_entity_collector::symbol_t> symbolList;
  this->ExprTkTools->Parser.dec().symbols(symbolList);
  // convert them to a set to remove duplicates
  std::set<std::string> variables;
  for (const auto& symbol : symbolList)
  {
    variables.insert(symbol.first);
  }

  for (auto& variable : variables)
  {
    // check if variable exists in scalars
    for (size_t j = 0; j < this->UsedScalarVariableNames.size(); ++j)
    {
      if (variable == this->UsedScalarVariableNames[j])
      {
        this->ScalarVariableNeeded[j] = true;
        break;
      }
    }

    // check if variable exists in vectors
    for (size_t j = 0; j < this->UsedVectorVariableNames.size(); ++j)
    {
      if (variable == this->UsedVectorVariableNames[j])
      {
        this->VectorVariableNeeded[j] = true;
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
std::string vtkExprTkFunctionParser::SanitizeName(const char* name)
{
  if (!name || name[0] == '\0')
  {
    return std::string();
  }

  std::ostringstream cname;
  for (size_t cc = 0; name[cc]; cc++)
  {
    if (isalnum(name[cc]) || name[cc] == '_')
    {
      cname << name[cc];
    }
  }
  // if first character is not an alphabet, add an 'a' to it.
  if (cname.str().empty() || isalpha(cname.str()[0]))
  {
    return cname.str();
  }
  else
  {
    return "a" + cname.str();
  }
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::GetScalarVariableIndex(const std::string& inVariableName)
{
  for (size_t i = 0; i < this->OriginalScalarVariableNames.size(); ++i)
  {
    if (this->OriginalScalarVariableNames[i] == inVariableName)
    {
      return static_cast<int>(i);
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::GetScalarVariableNeeded(int i)
{
  if (i < 0 || i >= static_cast<int>(this->ScalarVariableNeeded.size()))
  {
    return false;
  }
  return this->ScalarVariableNeeded[i];
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::GetScalarVariableNeeded(const std::string& inVariableName)
{
  std::vector<std::string>::const_iterator iter =
    std::find(this->OriginalScalarVariableNames.begin(), this->OriginalScalarVariableNames.end(),
      inVariableName);
  if (iter != this->OriginalScalarVariableNames.end())
  {
    return this->GetScalarVariableNeeded(
      static_cast<int>(iter - this->OriginalScalarVariableNames.begin()));
  }
  else
  {
    vtkErrorMacro(
      "GetScalarVariableNeeded: scalar variable name " << inVariableName << " does not exist");
    return false;
  }
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::GetVectorVariableIndex(const std::string& inVariableName)
{
  for (size_t i = 0; i < this->OriginalVectorVariableNames.size(); i++)
  {
    if (this->OriginalVectorVariableNames[i] == inVariableName)
    {
      return static_cast<int>(i);
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::GetVectorVariableNeeded(int i)
{
  if (i < 0 || i >= static_cast<int>(this->VectorVariableNeeded.size()))
  {
    return false;
  }
  return this->VectorVariableNeeded[i];
}

//------------------------------------------------------------------------------
bool vtkExprTkFunctionParser::GetVectorVariableNeeded(const std::string& inVariableName)
{
  std::vector<std::string>::const_iterator iter =
    std::find(this->OriginalVectorVariableNames.begin(), this->OriginalVectorVariableNames.end(),
      inVariableName);
  if (iter != this->OriginalVectorVariableNames.end())
  {
    return this->GetVectorVariableNeeded(
      static_cast<int>(iter - this->OriginalVectorVariableNames.begin()));
  }
  else
  {
    vtkErrorMacro(
      "GetVectorVariableNeeded: scalar variable name " << inVariableName << " does not exist");
    return false;
  }
}
