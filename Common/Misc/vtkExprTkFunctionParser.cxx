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

using ResultType = exprtk::results_context<double>::type_store_t::store_type;

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
      return std::numeric_limits<T>::quiet_NaN();
    else if (exprtk::rtl::vecops::helper::invalid_range(x, r0, r1))
      return std::numeric_limits<T>::quiet_NaN();

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
      return std::numeric_limits<T>::quiet_NaN();
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
      return std::numeric_limits<T>::quiet_NaN();

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
      return std::numeric_limits<T>::quiet_NaN();
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
      return std::numeric_limits<T>::quiet_NaN();

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
      return std::numeric_limits<T>::quiet_NaN();
    else if (exprtk::rtl::vecops::helper::invalid_range(y, r0, r1))
      return std::numeric_limits<T>::quiet_NaN();

    T result = x[0] * y[1] - x[1] * y[0];

    return result;
  }
};

namespace
{
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

//------------------------------------------------------------------------------
std::string RemoveSpacesFrom(const char* string)
{
  std::string str = string;
  str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
  return str;
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
  const std::vector<std::string>& variableNames, const char* variableName)
{
  std::string sanitizedName = vtkExprTkFunctionParser::SanitizeName(variableName);
  while (
    std::find(variableNames.begin(), variableNames.end(), sanitizedName) != variableNames.end())
  {
    sanitizedName += "1";
  }
  return sanitizedName;
}
}

static double vtkParserVectorErrorResult[3] = { VTK_PARSER_ERROR_RESULT, VTK_PARSER_ERROR_RESULT,
  VTK_PARSER_ERROR_RESULT };

vtkStandardNewMacro(vtkExprTkFunctionParser);

//------------------------------------------------------------------------------
vtkExprTkFunctionParser::vtkExprTkFunctionParser()
{
  this->Function = nullptr;

  this->EvaluateMTime.Modified();
  this->VariableMTime.Modified();
  this->ParseMTime.Modified();
  this->FunctionMTime.Modified();

  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;

  this->ParseError = nullptr;

  this->ExprTkTools = new vtkExprTkTools;
  // add vector support
  this->ExprTkTools->SymbolTable.add_package(vectorOperationsPackage);
  // add basic constants (e, pi, infinity)
  this->ExprTkTools->SymbolTable.add_constants();
  // add unit vectors
  this->ExprTkTools->SymbolTable.add_vector("iHat", iHat);
  this->ExprTkTools->SymbolTable.add_vector("jHat", jHat);
  this->ExprTkTools->SymbolTable.add_vector("kHat", kHat);
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
  if (this->ParseError)
  {
    this->SetParseError(nullptr);
  }

  this->RemoveAllVariables();
  delete this->ExprTkTools;
  delete this->ParseError;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetFunction(const char* function)
{
  // check if we have already set the same function string
  if (this->Function && function && strcmp(this->Function, function) == 0)
  {
    return;
  }

  if (this->Function)
  {
    delete[] this->Function;
  }

  if (function)
  {
    this->Function = new char[strlen(function) + 1];

    strcpy(this->Function, function);
    this->FunctionWithUsedVariableNames = this->Function;
  }
  else
  {
    this->Function = nullptr;
    this->FunctionWithUsedVariableNames = std::string();
  }

  this->FunctionMTime.Modified();
  this->ScalarVariableNeeded.clear();
  this->VectorVariableNeeded.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkExprTkFunctionParser::Parse(int mode)
{
  if (this->Function == nullptr)
  {
    vtkErrorMacro("Parse: no function has been set");
    return 0;
  }

  // During the parsing of the first mode, perform the necessary changes in the function
  if (mode == 0)
  {
    // Before parsing, replace the original variable names in the function
    // with the valid ones if needed.
    for (int i = 0; i < this->GetNumberOfScalarVariables(); ++i)
    {
      if (this->OriginalScalarVariableNames[i] != this->UsedScalarVariableNames[i])
      {
        vtksys::SystemTools::ReplaceString(this->FunctionWithUsedVariableNames,
          this->OriginalScalarVariableNames[i], this->UsedScalarVariableNames[i]);
      }
    }
    for (int i = 0; i < this->GetNumberOfVectorVariables(); ++i)
    {
      if (this->OriginalVectorVariableNames[i] != this->UsedVectorVariableNames[i])
      {
        vtksys::SystemTools::ReplaceString(this->FunctionWithUsedVariableNames,
          this->OriginalVectorVariableNames[i], this->UsedVectorVariableNames[i]);
      }
    }

    // remove spaces to perform replacement for norm and cross
    this->FunctionWithUsedVariableNames =
      RemoveSpacesFrom(this->FunctionWithUsedVariableNames.c_str());

    // check if cross(v1,v2) operation exist in the function,
    // and replace with (iHat*crossX(v1, v2)+jHat*crossY(v1, v2)+kHat*crossZ(v1, v2))
    // @note this regex works ONLY if v1, and v2 are just variables,
    // not sub-expressions which include parenthesis and commas
    std::string temp = this->FunctionWithUsedVariableNames;
    std::regex crossProductRegex = std::regex("cross\\(([^)]+),([^)]+)\\)");
    std::smatch sm;
    while (std::regex_search(temp, sm, crossProductRegex))
    {
      // if cross product has been extracted and the 2 vectors variables were identified
      if (sm.size() == 3)
      {
        std::string substring = "cross(" + sm[1].str() + "," + sm[2].str() + ")";
        std::string replacement = "(iHat*crossX(" + sm[1].str() + "," + sm[2].str() + ")" +
          "+jHat*crossY(" + sm[1].str() + "," + sm[2].str() + ")" + "+kHat*crossZ(" + sm[1].str() +
          "," + sm[2].str() + "))";
        vtksys::SystemTools::ReplaceString(
          this->FunctionWithUsedVariableNames, substring, replacement);
      }
      temp = sm.suffix().str();
    }

    // check if norm(v) operation exist in the function,
    // and replace with (v/mag(v))
    // @note this regex works ONLY if v is just a variable,
    // not a sub-expression which includes parenthesis and commas
    temp = this->FunctionWithUsedVariableNames;
    std::regex normRegex = std::regex("norm\\(([^)]+)\\)");
    while (std::regex_search(temp, sm, normRegex))
    {
      // if norm has been extracted and the vector variable was identified
      if (sm.size() == 2)
      {
        std::string substring = "norm(" + sm[1].str() + ")";
        std::string replacement = "(" + sm[1].str() + "/mag(" + sm[1].str() + "))";
        vtksys::SystemTools::ReplaceString(
          this->FunctionWithUsedVariableNames, substring, replacement);
      }
      temp = sm.suffix().str();
    }
  }

  // check if "if-statement exists in the function and replace it with branches to support vectors
  // ExprTk only supports scalars in the if statement format: if(statement, true-val, false-val)
  // @note this regex works ONLY if statement, true-val, false-val are just variables,
  // not a sub-expression which includes parenthesis,and commas
  std::string temp = this->FunctionWithUsedVariableNames;
  std::regex ifStatementRegex = std::regex("if\\(([^)]+),([^)]+),([^)]+)\\)");
  std::smatch sm;
  bool ifStatementFound = std::regex_search(temp, sm, ifStatementRegex);
  // if the if-statement has been extracted and the statement, true-val, false-val have been
  // identified
  if (ifStatementFound && sm.size() == 4)
  {
    std::string substring = "if(" + sm[1].str() + "," + sm[2].str() + "," + sm[3].str() + ")";
    std::string replacement;
    if (mode == 0)
    {
      // ExprTK, in order to extract vector and scalar results, and identify the result type,
      // it requires to "return results" instead of just evaluating an expression
      replacement =
        "if(" + sm[1].str() + ") return [" + sm[2].str() + "]; else return [" + sm[3].str() + "];";
    }
    else
    {
      // Since we know now the return type, we can assign the result to a result vector
      std::string resultArray = GenerateRandomAlphabeticString(10);
      this->ExprTkTools->SymbolTable.add_vector(
        resultArray, this->Result.GetData(), this->Result.GetSize());

      replacement = "if(" + sm[1].str() + ") " + resultArray + " := [" + sm[2].str() + "]; else " +
        resultArray + " := [" + sm[3].str() + "];";
    }
    this->ExpressionString = this->FunctionWithUsedVariableNames;
    vtksys::SystemTools::ReplaceString(this->ExpressionString, substring, replacement);
  }
  else
  {
    if (mode == 0)
    {
      // ExprTK, in order to extract vector and scalar results, and identify the result type,
      // it requires to "return results" instead of just evaluating an expression
      this->ExpressionString = "return [" + this->FunctionWithUsedVariableNames + "];";
    }
    else
    {
      // Since we know now the return type, we can assign the result to a result vector
      std::string resultArray = GenerateRandomAlphabeticString(10);
      this->ExprTkTools->SymbolTable.add_vector(
        resultArray, this->Result.GetData(), this->Result.GetSize());
      // Since we know now the return type, we can assign the result to a result vector
      this->ExpressionString = resultArray + " := [" + this->FunctionWithUsedVariableNames + "];";
    }
  }

  bool parsingResult =
    this->ExprTkTools->Parser.compile(this->ExpressionString, this->ExprTkTools->Expression);
  // check parsing result
  if (!parsingResult)
  {
    // print error only once
    if (mode == 0)
    {
      std::stringstream parsingErrorStream;
      // save error
      for (std::size_t i = 0; i < this->ExprTkTools->Parser.error_count(); ++i)
      {
        auto error = this->ExprTkTools->Parser.get_error(i);
        parsingErrorStream << "Err: " << i << " Pos: " << error.token.position << " Type: ["
                           << exprtk::parser_error::to_str(error.mode)
                           << "] Msg: " << error.diagnostic << "\tExpression: " << this->Function
                           << "\n";
      }
      vtkErrorMacro(<< parsingErrorStream.str());
      this->SetParseError(parsingErrorStream.str().c_str());
    }

    return 0;
  }

  if (mode == 0)
  {
    // Collect meta-data about variables that are needed for evaluation of the
    // function.
    this->UpdateNeededVariables();
  }
  this->ParseMTime.Modified();
  return 1;
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
    if (this->Parse(0) == 0)
    {
      return false;
    }
    // perform evaluation to identify the return type
    this->ExprTkTools->Expression.value();
    this->ResultType = this->ExprTkTools->Expression.results()[0].type;

    // compile with mode 1 to save results in the result array
    if (this->Parse(1) == 0)
    {
      return false;
    }
  }
  // perform evaluation
  this->ExprTkTools->Expression.value();

  switch (this->ResultType)
  {
    case ResultType::e_scalar:
      if (std::isnan(this->Result[0]) ||
        std::abs(this->Result[0]) == std::numeric_limits<double>::infinity())
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
    case ResultType::e_vector:
      for (int i = 0; i < 3; i++)
      {
        if (std::isnan(this->Result[i]) ||
          std::abs(this->Result[i]) == std::numeric_limits<double>::infinity())
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
  return (this->ResultType == ResultType::e_scalar);
}

//------------------------------------------------------------------------------
double vtkExprTkFunctionParser::GetScalarResult()
{
  if (!(this->IsScalarResult()))
  {
    vtkErrorMacro("GetScalarResult: no valid scalar result");
    return VTK_PARSER_ERROR_RESULT;
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
  return (this->ResultType == ResultType::e_vector);
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
const char* vtkExprTkFunctionParser::GetScalarVariableName(int i)
{
  if (i >= 0 && i < this->GetNumberOfScalarVariables())
  {
    return this->OriginalScalarVariableNames[i].c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
const char* vtkExprTkFunctionParser::GetVectorVariableName(int i)
{
  if (i >= 0 && i < this->GetNumberOfVectorVariables())
  {
    return this->OriginalVectorVariableNames[i].c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetScalarVariableValue(const char* inVariableName, double value)
{
  if (!inVariableName || inVariableName[0] == '\0')
  {
    vtkErrorMacro("Variable name is empty");
    return;
  }
  for (int i = 0; i < this->GetNumberOfScalarVariables(); i++)
  {
    if (strcmp(inVariableName, this->OriginalScalarVariableNames[i].c_str()) == 0)
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
  this->ScalarVariableValues.push_back(scalarValue);
  this->OriginalScalarVariableNames.emplace_back(inVariableName);

  // if variable name is not sanitized, create a random sanitized string and set it as variable name
  std::string variableName = vtkExprTkFunctionParser::SanitizeName(inVariableName);
  if (variableName != inVariableName)
  {
    variableName = GenerateUniqueVariableName(this->UsedScalarVariableNames, inVariableName);
  }
  this->ExprTkTools->SymbolTable.add_variable(
    variableName, *this->ScalarVariableValues[this->ScalarVariableValues.size() - 1]);
  this->UsedScalarVariableNames.push_back(variableName);

  this->VariableMTime.Modified();
  this->Modified();
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
double vtkExprTkFunctionParser::GetScalarVariableValue(const char* inVariableName)
{
  for (int i = 0; i < this->GetNumberOfScalarVariables(); i++)
  {
    if (strcmp(inVariableName, this->OriginalScalarVariableNames[i].c_str()) == 0)
    {
      return *this->ScalarVariableValues[i];
    }
  }
  vtkErrorMacro(
    "GetScalarVariableValue: scalar variable name " << inVariableName << " does not exist");
  return VTK_PARSER_ERROR_RESULT;
}

//------------------------------------------------------------------------------
double vtkExprTkFunctionParser::GetScalarVariableValue(int i)
{
  if (i < 0 || i >= this->GetNumberOfScalarVariables())
  {
    vtkErrorMacro("GetScalarVariableValue: scalar variable number " << i << " does not exist");
    return VTK_PARSER_ERROR_RESULT;
  }

  return *this->ScalarVariableValues[i];
}

//------------------------------------------------------------------------------
void vtkExprTkFunctionParser::SetVectorVariableValue(
  const char* inVariableName, double xValue, double yValue, double zValue)
{
  if (!inVariableName || inVariableName[0] == '\0')
  {
    vtkErrorMacro("Variable name is empty");
    return;
  }
  for (int i = 0; i < this->GetNumberOfVectorVariables(); i++)
  {
    if (strcmp(inVariableName, this->OriginalVectorVariableNames[i].c_str()) == 0)
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
  this->VectorVariableValues.push_back(vector);
  this->OriginalVectorVariableNames.emplace_back(inVariableName);

  // if variable name is not sanitized, create a random sanitized string and set it as variable name
  std::string variableName = vtkExprTkFunctionParser::SanitizeName(inVariableName);
  if (variableName != inVariableName)
  {
    variableName = GenerateUniqueVariableName(this->UsedVectorVariableNames, inVariableName);
  }
  this->ExprTkTools->SymbolTable.add_vector(variableName,
    this->VectorVariableValues[this->VectorVariableValues.size() - 1]->GetData(),
    vector->GetSize());
  this->UsedVectorVariableNames.push_back(variableName);

  this->VariableMTime.Modified();
  this->Modified();
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
double* vtkExprTkFunctionParser::GetVectorVariableValue(const char* inVariableName)
{
  for (int i = 0; i < this->GetNumberOfVectorVariables(); i++)
  {
    if (strcmp(inVariableName, this->OriginalVectorVariableNames[i].c_str()) == 0)
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

  for (int i = 0; i < this->GetNumberOfScalarVariables(); i++)
  {
    os << indent << "  " << this->OriginalScalarVariableNames[i] << " / "
       << this->GetScalarVariableName(i) << ": " << this->GetScalarVariableValue(i) << endl;
  }

  for (int i = 0; i < this->GetNumberOfVectorVariables(); i++)
  {
    os << indent << "  " << this->OriginalVectorVariableNames[i] << " / "
       << this->GetVectorVariableName(i) << ": (" << this->GetVectorVariableValue(i)[0] << ", "
       << this->GetVectorVariableValue(i)[1] << ", " << this->GetVectorVariableValue(i)[2] << ")"
       << endl;
  }

  if (this->EvaluateMTime.GetMTime() > this->FunctionMTime.GetMTime() &&
    this->EvaluateMTime.GetMTime() > this->VariableMTime.GetMTime() &&
    this->ExprTkTools->Expression.results().count() > 0)
  {
    auto result = this->ExprTkTools->Expression.results()[0];
    if (result.type == ResultType::e_scalar)
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
  os << indent << "Parse Error: " << (this->ParseError ? this->ParseError : "nullptr") << endl;
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
int vtkExprTkFunctionParser::GetScalarVariableIndex(const char* inVariableName)
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
bool vtkExprTkFunctionParser::GetScalarVariableNeeded(const char* inVariableName)
{
  std::vector<std::string>::const_iterator iter =
    std::find(this->OriginalScalarVariableNames.begin(), this->OriginalScalarVariableNames.end(),
      std::string(inVariableName));
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
int vtkExprTkFunctionParser::GetVectorVariableIndex(const char* inVariableName)
{
  for (int i = 0; i < this->GetNumberOfVectorVariables(); ++i)
  {
    if (this->OriginalVectorVariableNames[i] == inVariableName)
    {
      return i;
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
bool vtkExprTkFunctionParser::GetVectorVariableNeeded(const char* inVariableName)
{
  std::vector<std::string>::const_iterator iter =
    std::find(this->OriginalVectorVariableNames.begin(), this->OriginalVectorVariableNames.end(),
      std::string(inVariableName));
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
