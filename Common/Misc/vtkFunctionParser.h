// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFunctionParser
 * @brief   Parse and evaluate a mathematical expression
 *
 * vtkFunctionParser is a class that takes in a mathematical expression as
 * a char string, parses it, and evaluates it at the specified values of
 * the variables in the input string.
 *
 * You can use the "if" operator to create conditional expressions
 * such as if ( test, trueresult, falseresult). These evaluate the boolean
 * valued test expression and then evaluate either the trueresult or the
 * falseresult expression to produce a final (scalar or vector valued) value.
 * "test" may contain <,>,=,|,&, and () and all three subexpressions can
 * evaluate arbitrary function operators (ln, cos, +, if, etc)
 *
 * @par Thanks:
 * Juha Nieminen (juha.nieminen@gmail.com) for relicensing this branch of the
 * function parser code that this class is based upon under the new BSD license
 * so that it could be used in VTK. Note, the BSD license applies to this
 * version of the function parser only (by permission of the author), and not
 * the original library.
 *
 * @par Thanks:
 * Thomas Dunne (thomas.dunne@iwr.uni-heidelberg.de) for adding code for
 * two-parameter-parsing and a few functions (sign, min, max).
 *
 * @par Thanks:
 * Sid Sydoriak (sxs@lanl.gov) for adding boolean operations and
 * conditional expressions and for fixing a variety of bugs.
 */

#ifndef vtkFunctionParser_h
#define vtkFunctionParser_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkObject.h"
#include "vtkTuple.h" // needed for vtkTuple
#include <string>     // needed for string.
#include <vector>     // needed for vector

#define VTK_PARSER_IMMEDIATE 1
#define VTK_PARSER_UNARY_MINUS 2
#define VTK_PARSER_UNARY_PLUS 3

// supported math functions
#define VTK_PARSER_ADD 4
#define VTK_PARSER_SUBTRACT 5
#define VTK_PARSER_MULTIPLY 6
#define VTK_PARSER_DIVIDE 7
#define VTK_PARSER_POWER 8
#define VTK_PARSER_ABSOLUTE_VALUE 9
#define VTK_PARSER_EXPONENT 10
#define VTK_PARSER_CEILING 11
#define VTK_PARSER_FLOOR 12
#define VTK_PARSER_LOGARITHM 13
#define VTK_PARSER_LOGARITHME 14
#define VTK_PARSER_LOGARITHM10 15
#define VTK_PARSER_SQUARE_ROOT 16
#define VTK_PARSER_SINE 17
#define VTK_PARSER_COSINE 18
#define VTK_PARSER_TANGENT 19
#define VTK_PARSER_ARCSINE 20
#define VTK_PARSER_ARCCOSINE 21
#define VTK_PARSER_ARCTANGENT 22
#define VTK_PARSER_HYPERBOLIC_SINE 23
#define VTK_PARSER_HYPERBOLIC_COSINE 24
#define VTK_PARSER_HYPERBOLIC_TANGENT 25
#define VTK_PARSER_MIN 26
#define VTK_PARSER_MAX 27
#define VTK_PARSER_SIGN 29

// functions involving vectors
#define VTK_PARSER_CROSS 28
#define VTK_PARSER_VECTOR_UNARY_MINUS 30
#define VTK_PARSER_VECTOR_UNARY_PLUS 31
#define VTK_PARSER_DOT_PRODUCT 32
#define VTK_PARSER_VECTOR_ADD 33
#define VTK_PARSER_VECTOR_SUBTRACT 34
#define VTK_PARSER_SCALAR_TIMES_VECTOR 35
#define VTK_PARSER_VECTOR_TIMES_SCALAR 36
#define VTK_PARSER_VECTOR_OVER_SCALAR 37
#define VTK_PARSER_MAGNITUDE 38
#define VTK_PARSER_NORMALIZE 39

// constants involving vectors
#define VTK_PARSER_IHAT 40
#define VTK_PARSER_JHAT 41
#define VTK_PARSER_KHAT 42

// code for if(bool, trueval, falseval) resulting in a scalar
#define VTK_PARSER_IF 43

// code for if(bool, truevec, falsevec) resulting in a vector
#define VTK_PARSER_VECTOR_IF 44

// codes for boolean expressions
#define VTK_PARSER_LESS_THAN 45

// codes for boolean expressions
#define VTK_PARSER_GREATER_THAN 46

// codes for boolean expressions
#define VTK_PARSER_EQUAL_TO 47

// codes for boolean expressions
#define VTK_PARSER_AND 48

// codes for boolean expressions
#define VTK_PARSER_OR 49

// codes for scalar variables come before those for vectors. Do not define
// values for VTK_PARSER_BEGIN_VARIABLES+1, VTK_PARSER_BEGIN_VARIABLES+2, ...,
// because they are used to look up variables numbered 1, 2, ...
#define VTK_PARSER_BEGIN_VARIABLES 50

// the value that is returned as a result if there is an error
#define VTK_PARSER_ERROR_RESULT VTK_FLOAT_MAX

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMISC_EXPORT vtkFunctionParser : public vtkObject
{
public:
  static vtkFunctionParser* New();
  vtkTypeMacro(vtkFunctionParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return parser's MTime
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get input string to evaluate.
   */
  void SetFunction(const char* function);
  vtkGetStringMacro(Function);
  ///@}

  /**
   * Check whether the result is a scalar result.  If it isn't, then
   * either the result is a vector or an error has occurred.
   */
  int IsScalarResult();

  /**
   * Check whether the result is a vector result.  If it isn't, then
   * either the result is scalar or an error has occurred.
   */
  int IsVectorResult();

  /**
   * Get a scalar result from evaluating the input function.
   */
  double GetScalarResult();

  ///@{
  /**
   * Get a vector result from evaluating the input function.
   */
  double* GetVectorResult() VTK_SIZEHINT(3);
  void GetVectorResult(double result[3])
  {
    double* r = this->GetVectorResult();
    result[0] = r[0];
    result[1] = r[1];
    result[2] = r[2];
  }
  ///@}

  ///@{
  /**
   * Set the value of a scalar variable.  If a variable with this name
   * exists, then its value will be set to the new value.  If there is not
   * already a variable with this name, variableName will be added to the
   * list of variables, and its value will be set to the new value.
   */
  void SetScalarVariableValue(const char* variableName, double value);
  void SetScalarVariableValue(const std::string& variableName, double value)
  {
    this->SetScalarVariableValue(variableName.c_str(), value);
  }
  void SetScalarVariableValue(int i, double value);
  ///@}

  ///@{
  /**
   * Get the value of a scalar variable.
   */
  double GetScalarVariableValue(const char* variableName);
  double GetScalarVariableValue(const std::string& variableName)
  {
    return this->GetScalarVariableValue(variableName.c_str());
  }
  double GetScalarVariableValue(int i);
  ///@}

  ///@{
  /**
   * Set the value of a vector variable.  If a variable with this name
   * exists, then its value will be set to the new value.  If there is not
   * already a variable with this name, variableName will be added to the
   * list of variables, and its value will be set to the new value.
   */
  void SetVectorVariableValue(
    const char* variableName, double xValue, double yValue, double zValue);
  void SetVectorVariableValue(
    const std::string& variableName, double xValue, double yValue, double zValue)
  {
    this->SetVectorVariableValue(variableName.c_str(), xValue, yValue, zValue);
  }
  void SetVectorVariableValue(const char* variableName, const double values[3])
  {
    this->SetVectorVariableValue(variableName, values[0], values[1], values[2]);
  }
  void SetVectorVariableValue(const std::string& variableName, const double values[3])
  {
    this->SetVectorVariableValue(variableName.c_str(), values[0], values[1], values[2]);
  }
  void SetVectorVariableValue(int i, double xValue, double yValue, double zValue);
  void SetVectorVariableValue(int i, const double values[3])
  {
    this->SetVectorVariableValue(i, values[0], values[1], values[2]);
  }
  ///@}

  ///@{
  /**
   * Get the value of a vector variable.
   */
  double* GetVectorVariableValue(const char* variableName) VTK_SIZEHINT(3);
  double* GetVectorVariableValue(const std::string& variableName) VTK_SIZEHINT(3)
  {
    return this->GetVectorVariableValue(variableName.c_str());
  }
  void GetVectorVariableValue(const char* variableName, double value[3])
  {
    double* r = this->GetVectorVariableValue(variableName);
    value[0] = r[0];
    value[1] = r[1];
    value[2] = r[2];
  }
  void GetVectorVariableValue(const std::string& variableName, double value[3])
  {
    this->GetVectorVariableValue(variableName.c_str(), value);
  }
  double* GetVectorVariableValue(int i) VTK_SIZEHINT(3);
  void GetVectorVariableValue(int i, double value[3])
  {
    double* r = this->GetVectorVariableValue(i);
    value[0] = r[0];
    value[1] = r[1];
    value[2] = r[2];
  }
  ///@}

  /**
   * Get the number of scalar variables.
   */
  int GetNumberOfScalarVariables() { return static_cast<int>(this->ScalarVariableNames.size()); }

  /**
   * Get scalar variable index or -1 if not found
   */
  int GetScalarVariableIndex(const char* name);
  int GetScalarVariableIndex(const std::string& name)
  {
    return this->GetScalarVariableIndex(name.c_str());
  }

  /**
   * Get the number of vector variables.
   */
  int GetNumberOfVectorVariables() { return static_cast<int>(this->VectorVariableNames.size()); }

  /**
   * Get scalar variable index or -1 if not found
   */
  int GetVectorVariableIndex(const char* name);
  int GetVectorVariableIndex(const std::string& name)
  {
    return this->GetVectorVariableIndex(name.c_str());
  }

  /**
   * Get the ith scalar variable name.
   */
  const char* GetScalarVariableName(int i);

  /**
   * Get the ith vector variable name.
   */
  const char* GetVectorVariableName(int i);

  ///@{
  /**
   * Returns whether a scalar variable is needed for the function evaluation.
   * This is only valid after a successful Parse(). Thus, call GetScalarResult()
   * or IsScalarResult() or similar method before calling this.
   */
  bool GetScalarVariableNeeded(int i);
  bool GetScalarVariableNeeded(const char* variableName);
  bool GetScalarVariableNeeded(const std::string& variableName)
  {
    return GetScalarVariableNeeded(variableName.c_str());
  }
  ///@}

  ///@{
  /**
   * Returns whether a vector variable is needed for the function evaluation.
   * This is only valid after a successful Parse(). Thus, call GetVectorResult()
   * or IsVectorResult() or similar method before calling this.
   */
  bool GetVectorVariableNeeded(int i);
  bool GetVectorVariableNeeded(const char* variableName);
  bool GetVectorVariableNeeded(const std::string& variableName)
  {
    return this->GetVectorVariableNeeded(variableName.c_str());
  }
  ///@}

  /**
   * Remove all the current variables.
   */
  void RemoveAllVariables();

  /**
   * Remove all the scalar variables.
   */
  void RemoveScalarVariables();

  /**
   * Remove all the vector variables.
   */
  void RemoveVectorVariables();

  ///@{
  /**
   * When ReplaceInvalidValues is on, all invalid values (such as
   * sqrt(-2), note that function parser does not handle complex
   * numbers) will be replaced by ReplacementValue. Otherwise an
   * error will be reported
   */
  vtkSetMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkGetMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkBooleanMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkSetMacro(ReplacementValue, double);
  vtkGetMacro(ReplacementValue, double);
  ///@}

  /**
   * Check the validity of the function expression.
   */
  void CheckExpression(int& pos, char** error);

  /**
   * Allow the user to force the function to be re-parsed
   */
  void InvalidateFunction();

protected:
  vtkFunctionParser();
  ~vtkFunctionParser() override;

  int Parse();

  /**
   * Evaluate the function, returning true on success, false on failure.
   */
  bool Evaluate();

  int CheckSyntax();

  void CopyParseError(int& position, char** error);

  void RemoveSpaces();
  char* RemoveSpacesFrom(const char* variableName);
  int OperatorWithinVariable(int idx);

  int BuildInternalFunctionStructure();
  void BuildInternalSubstringStructure(int beginIndex, int endIndex);
  void AddInternalByte(unsigned int newByte);

  int IsSubstringCompletelyEnclosed(int beginIndex, int endIndex);
  int FindEndOfMathFunction(int beginIndex);
  int FindEndOfMathConstant(int beginIndex);

  int IsVariableName(int currentIndex);
  int IsElementaryOperator(int op);

  int GetMathFunctionNumber(int currentIndex);
  int GetMathFunctionNumberByCheckingParenthesis(int currentIndex);
  int GetMathFunctionStringLength(int mathFunctionNumber);
  int GetMathConstantNumber(int currentIndex);
  int GetMathConstantStringLength(int mathConstantNumber);
  unsigned char GetElementaryOperatorNumber(char op);
  unsigned int GetOperandNumber(int currentIndex);
  int GetVariableNameLength(int variableNumber);

  int DisambiguateOperators();

  /**
   * Collects meta-data about which variables are needed by the current
   * function. This is called only after a successful call to this->Parse().
   */
  void UpdateNeededVariables();

  vtkSetStringMacro(ParseError);

  int FindPositionInOriginalFunction(const int& pos);

  char* Function;
  char* FunctionWithSpaces;

  int FunctionLength;
  std::vector<std::string> ScalarVariableNames;
  std::vector<std::string> VectorVariableNames;
  std::vector<double> ScalarVariableValues;
  std::vector<vtkTuple<double, 3>> VectorVariableValues;
  std::vector<bool> ScalarVariableNeeded;
  std::vector<bool> VectorVariableNeeded;

  std::vector<unsigned int> ByteCode;
  int ByteCodeSize;
  double* Immediates;
  int ImmediatesSize;
  double* Stack;
  int StackSize;
  int StackPointer;

  vtkTimeStamp FunctionMTime;
  vtkTimeStamp ParseMTime;
  vtkTimeStamp CheckMTime;

  vtkTypeBool ReplaceInvalidValues;
  double ReplacementValue;

  int ParseErrorPositon;
  char* ParseError;

private:
  vtkFunctionParser(const vtkFunctionParser&) = delete;
  void operator=(const vtkFunctionParser&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
