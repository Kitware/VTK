// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExprTkFunctionParser
 * @brief   Parse and evaluate a mathematical expression
 *
 * vtkExprTkFunctionParser is a wrapper class of the ExprTK library that takes
 * in a mathematical expression as a char string, parses it, and evaluates it
 * at the specified values of the variables in the input string.
 *
 * The detailed documentation of the supported functionality is described in
 * https://github.com/ArashPartow/exprtk. In addition to the documented
 * functionality, the following vector operations have been implemented:
 * 1) cross(v1, v2), cross product of two vectors,
 * 2) mag(v), magnitude of a vector,
 * 3) norm(v), the normalized version of a vector.
 *
 * @par Thanks:
 * Arash Partow for implementing the ExprTk library.
 */

#ifndef vtkExprTkFunctionParser_h
#define vtkExprTkFunctionParser_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkObject.h"
#include "vtkTuple.h" // needed for vtkTuple
#include <string>     // needed for string.
#include <vector>     // needed for vector

// forward declarations for ExprTk tools
VTK_ABI_NAMESPACE_BEGIN
struct vtkExprTkTools;

class VTKCOMMONMISC_EXPORT vtkExprTkFunctionParser : public vtkObject
{
public:
  static vtkExprTkFunctionParser* New();
  vtkTypeMacro(vtkExprTkFunctionParser, vtkObject);
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
  const char* GetFunction() { return this->Function.c_str(); }
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
   *
   * If ReplaceInvalidValues is not set, then the error value
   * that will be return is NaN.
   */
  double GetScalarResult();

  ///@{
  /**
   * Get a vector result from evaluating the input function.
   *
   * If ReplaceInvalidValues is not set, then the error value
   * that will be return is [NaN, NaN, NaN].
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
   * list of variables, and its value will be set to the new value. If the
   * variable name is not sanitized, it should be provided in quotes, and
   * a valid unique string will be used as a replacement by the parser.
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  void SetScalarVariableValue(const std::string& variableName, double value);
  void SetScalarVariableValue(int i, double value);
  ///@}

  ///@{
  /**
   * Get the value of a scalar variable.
   */
  double GetScalarVariableValue(const std::string& variableName);
  double GetScalarVariableValue(int i);
  ///@}

  ///@{
  /**
   * Set the value of a vector variable.  If a variable with this name
   * exists, then its value will be set to the new value.  If there is not
   * already a variable with this name, variableName will be added to the
   * list of variables, and its value will be set to the new value. If the
   * variable name is not sanitized, it should be provided in quotes, and
   * a valid unique string will be used as a replacement by the parser.
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  void SetVectorVariableValue(
    const std::string& variableName, double xValue, double yValue, double zValue);
  void SetVectorVariableValue(const std::string& variableName, double values[3])
  {
    this->SetVectorVariableValue(variableName, values[0], values[1], values[2]);
  }
  void SetVectorVariableValue(int i, double xValue, double yValue, double zValue);
  void SetVectorVariableValue(int i, double values[3])
  {
    this->SetVectorVariableValue(i, values[0], values[1], values[2]);
  }
  ///@}

  ///@{
  /**
   * Get the value of a vector variable.
   */
  double* GetVectorVariableValue(const std::string& variableName) VTK_SIZEHINT(3);
  void GetVectorVariableValue(const std::string& variableName, double value[3])
  {
    double* r = this->GetVectorVariableValue(variableName);
    value[0] = r[0];
    value[1] = r[1];
    value[2] = r[2];
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
  int GetNumberOfScalarVariables()
  {
    return static_cast<int>(this->UsedScalarVariableNames.size());
  }

  /**
   * Get scalar variable index or -1 if not found
   */
  int GetScalarVariableIndex(const std::string& name);

  /**
   * Get the number of vector variables.
   */
  int GetNumberOfVectorVariables()
  {
    return static_cast<int>(this->UsedVectorVariableNames.size());
  }

  /**
   * Get scalar variable index or -1 if not found
   */
  int GetVectorVariableIndex(const std::string& name);

  /**
   * Get the ith scalar variable name.
   */
  std::string GetScalarVariableName(int i);

  /**
   * Get the ith vector variable name.
   */
  std::string GetVectorVariableName(int i);

  ///@{
  /**
   * Returns whether a scalar variable is needed for the function evaluation.
   * This is only valid after a successful Parse(). Thus, call GetScalarResult()
   * or IsScalarResult() or similar method before calling this.
   */
  bool GetScalarVariableNeeded(int i);
  bool GetScalarVariableNeeded(const std::string& variableName);
  ///@}

  ///@{
  /**
   * Returns whether a vector variable is needed for the function evaluation.
   * This is only valid after a successful Parse(). Thus, call GetVectorResult()
   * or IsVectorResult() or similar method before calling this.
   */
  bool GetVectorVariableNeeded(int i);
  bool GetVectorVariableNeeded(const std::string& variableName);
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
   * Allow the user to force the function to be re-parsed
   */
  void InvalidateFunction();

  /**
   * Sanitize a label/name to remove spaces, delimiters etc.
   * Once the label/name is sanitized is can be accepted by the
   * following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   *
   * @note taken from vtkSMCoreUtilities
   */
  static std::string SanitizeName(const char* name);

protected:
  vtkExprTkFunctionParser();
  ~vtkExprTkFunctionParser() override;

  /**
   * The first mode parses the function and uses a return statement
   * to identify the ReturnType. The second mode parses the function and uses a result
   * vector to store the results of the function knowing its return type. The second mode
   * is used because it's a lot faster than the first.
   */
  enum ParseMode
  {
    DetectReturnType,
    SaveResultInVariable
  };

  /**
   * Parses the given function, returning true on success, false on failure.
   * It has 2 modes (see ParseMode). Both modes need to be utilized to extract
   * results of a function.
   */
  int Parse(ParseMode mode);

  /**
   * Enum that defines the vector returning functions that are not supported by ExprTk.
   */
  enum VectorReturningFunction
  {
    Cross,
    Norm
  };

  /**
   * ExprTk does not support functions which return a vector.
   *
   * All the cross(v1,v2) occurrences will be replaced with
   * (iHat*crossX(v1,v2)+jHat*crossY(v1,v2)+kHat*crossZ(v1,v2)).
   *
   * All the norm(v) occurrences will be replaced with ((v)/mag(v)).
   */
  std::string FixVectorReturningFunctionOccurrences(
    VectorReturningFunction vectorReturningFunction);

  /**
   *  Check possible usage of old format of dot product, e.g. v1.v2
   */
  bool CheckOldFormatOfDotProductUsage();

  /**
   * Evaluate the function, returning true on success, false on failure.
   */
  bool Evaluate();

  /**
   * Collects meta-data about which variables are needed by the current
   * function. This is called only after a successful call to this->Parse().
   */
  void UpdateNeededVariables();

  std::string Function;
  std::string FunctionWithUsedVariableNames;
  std::string ExpressionString;

  // original and used variables names are the same, except if the original
  // ones are not valid.
  std::vector<std::string> OriginalScalarVariableNames;
  std::vector<std::string> UsedScalarVariableNames;
  std::vector<std::string> OriginalVectorVariableNames;
  std::vector<std::string> UsedVectorVariableNames;
  // pointers for scalar and vector variables are used to enforce
  // that their memory address will not change due to a possible
  // resizing of their container (std::vector), ExprTk requires the
  // memory address of the given variable to remain the same.
  std::vector<double*> ScalarVariableValues;
  std::vector<vtkTuple<double, 3>*> VectorVariableValues;
  std::vector<bool> ScalarVariableNeeded;
  std::vector<bool> VectorVariableNeeded;

  vtkTimeStamp FunctionMTime;
  vtkTimeStamp ParseMTime;

  vtkTypeBool ReplaceInvalidValues;
  double ReplacementValue;

  vtkExprTkTools* ExprTkTools;

  int ResultType;
  vtkTuple<double, 3> Result;

private:
  vtkExprTkFunctionParser(const vtkExprTkFunctionParser&) = delete;
  void operator=(const vtkExprTkFunctionParser&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
