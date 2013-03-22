/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFunctionParser - Parse and evaluate a mathematical expression
// .SECTION Description
// vtkFunctionParser is a class that takes in a mathematical expression as
// a char string, parses it, and evaluates it at the specified values of
// the variables in the input string.
//
// You can use the "if" operator to create conditional expressions
// such as if ( test, trueresult, falseresult). These evaluate the boolean
// valued test expression and then evaluate either the trueresult or the
// falseresult expression to produce a final (scalar or vector valued) value.
// "test" may contain <,>,=,|,&, and () and all three subexpressions can
// evaluate arbitrary function operators (ln, cos, +, if, etc)
//
// .SECTION Thanks
// Juha Nieminen (juha.nieminen@gmail.com) for relicensing this branch of the
// function parser code that this class is based upon under the new BSD license
// so that it could be used in VTK. Note, the BSD license applies to this
// version of the function parser only (by permission of the author), and not
// the original library.
//
// Thomas Dunne (thomas.dunne@iwr.uni-heidelberg.de) for adding code for
// two-parameter-parsing and a few functions (sign, min, max).
//
// Sid Sydoriak (sxs@lanl.gov) for adding boolean operations and
// conditional expressions and for fixing a variety of bugs.

#ifndef __vtkFunctionParser_h
#define __vtkFunctionParser_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkObject.h"

#define VTK_PARSER_IMMEDIATE 1
#define VTK_PARSER_UNARY_MINUS 2

// supported math functions
#define VTK_PARSER_ADD 3
#define VTK_PARSER_SUBTRACT 4
#define VTK_PARSER_MULTIPLY 5
#define VTK_PARSER_DIVIDE 6
#define VTK_PARSER_POWER 7
#define VTK_PARSER_ABSOLUTE_VALUE 8
#define VTK_PARSER_EXPONENT 9
#define VTK_PARSER_CEILING 10
#define VTK_PARSER_FLOOR 11
#define VTK_PARSER_LOGARITHM 12
#define VTK_PARSER_LOGARITHME 13
#define VTK_PARSER_LOGARITHM10 14
#define VTK_PARSER_SQUARE_ROOT 15
#define VTK_PARSER_SINE 16
#define VTK_PARSER_COSINE 17
#define VTK_PARSER_TANGENT 18
#define VTK_PARSER_ARCSINE 19
#define VTK_PARSER_ARCCOSINE 20
#define VTK_PARSER_ARCTANGENT 21
#define VTK_PARSER_HYPERBOLIC_SINE 22
#define VTK_PARSER_HYPERBOLIC_COSINE 23
#define VTK_PARSER_HYPERBOLIC_TANGENT 24
#define VTK_PARSER_MIN 25
#define VTK_PARSER_MAX 26
#define VTK_PARSER_CROSS 27
#define VTK_PARSER_SIGN 28

// functions involving vectors
#define VTK_PARSER_VECTOR_UNARY_MINUS 29
#define VTK_PARSER_DOT_PRODUCT 30
#define VTK_PARSER_VECTOR_ADD 31
#define VTK_PARSER_VECTOR_SUBTRACT 32
#define VTK_PARSER_SCALAR_TIMES_VECTOR 33
#define VTK_PARSER_VECTOR_TIMES_SCALAR 34
#define VTK_PARSER_VECTOR_OVER_SCALAR 35
#define VTK_PARSER_MAGNITUDE 36
#define VTK_PARSER_NORMALIZE 37

// constants involving vectors
#define VTK_PARSER_IHAT 38
#define VTK_PARSER_JHAT 39
#define VTK_PARSER_KHAT 40

// code for if(bool, trueval, falseval) resulting in a scalar
#define VTK_PARSER_IF 41

// code for if(bool, truevec, falsevec) resulting in a vector
#define VTK_PARSER_VECTOR_IF 42

// codes for boolean expressions
#define VTK_PARSER_LESS_THAN 43

// codes for boolean expressions
#define VTK_PARSER_GREATER_THAN 44

// codes for boolean expressions
#define VTK_PARSER_EQUAL_TO 45

// codes for boolean expressions
#define VTK_PARSER_AND 46

// codes for boolean expressions
#define VTK_PARSER_OR 47

// codes for scalar variables come before those for vectors. Do not define
// values for VTK_PARSER_BEGIN_VARIABLES+1, VTK_PARSER_BEGIN_VARIABLES+2, ...,
// because they are used to look up variables numbered 1, 2, ...
#define VTK_PARSER_BEGIN_VARIABLES 48

// the value that is retuned as a result if there is an error
#define VTK_PARSER_ERROR_RESULT VTK_FLOAT_MAX

class VTKCOMMONMISC_EXPORT vtkFunctionParser : public vtkObject
{
public:
  static vtkFunctionParser *New();
  vtkTypeMacro(vtkFunctionParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get input string to evaluate.
  void SetFunction(const char *function);
  vtkGetStringMacro(Function);

  // Description:
  // Check whether the result is a scalar result.  If it isn't, then
  // either the result is a vector or an error has occurred.
  int IsScalarResult();

  // Description:
  // Check whether the result is a vector result.  If it isn't, then
  // either the result is scalar or an error has occurred.
  int IsVectorResult();

  // Description:
  // Get a scalar result from evaluating the input function.
  double GetScalarResult();

  // Description:
  // Get a vector result from evaluating the input function.
  double* GetVectorResult();
  void GetVectorResult(double result[3]) {
    double *r = this->GetVectorResult();
    result[0] = r[0]; result[1] = r[1]; result[2] = r[2]; };

  // Description:
  // Set the value of a scalar variable.  If a variable with this name
  // exists, then its value will be set to the new value.  If there is not
  // already a variable with this name, variableName will be added to the
  // list of variables, and its value will be set to the new value.
  void SetScalarVariableValue(const char* variableName, double value);
  void SetScalarVariableValue(int i, double value);

  // Description:
  // Get the value of a scalar variable.
  double GetScalarVariableValue(const char* variableName);
  double GetScalarVariableValue(int i);

  // Description:
  // Set the value of a vector variable.  If a variable with this name
  // exists, then its value will be set to the new value.  If there is not
  // already a variable with this name, variableName will be added to the
  // list of variables, and its value will be set to the new value.
  void SetVectorVariableValue(const char* variableName, double xValue,
                              double yValue, double zValue);
  void SetVectorVariableValue(const char* variableName,
                              const double values[3]) {
    this->SetVectorVariableValue(variableName,values[0],values[1],values[2]);};
  void SetVectorVariableValue(int i, double xValue, double yValue,
                              double zValue);
  void SetVectorVariableValue(int i, const double values[3]) {
    this->SetVectorVariableValue(i,values[0],values[1],values[2]);};

  // Description:
  // Get the value of a vector variable.
  double* GetVectorVariableValue(const char* variableName);
  void GetVectorVariableValue(const char* variableName, double value[3]) {
    double *r = this->GetVectorVariableValue(variableName);
    value[0] = r[0]; value[1] = r[1]; value[2] = r[2]; };
  double* GetVectorVariableValue(int i);
  void GetVectorVariableValue(int i, double value[3]) {
    double *r = this->GetVectorVariableValue(i);
    value[0] = r[0]; value[1] = r[1]; value[2] = r[2]; };

  // Description:
  // Get the number of scalar variables.
  vtkGetMacro(NumberOfScalarVariables,int);

  // Description:
  // Get the number of vector variables.
  vtkGetMacro(NumberOfVectorVariables,int);

  // Description:
  // Get the ith scalar variable name.
  char* GetScalarVariableName(int i);

  // Description:
  // Get the ith vector variable name.
  char* GetVectorVariableName(int i);

  // Description:
  // Remove all the current variables.
  void RemoveAllVariables();

  // Description:
  // Remove all the scalar variables.
  void RemoveScalarVariables();

  // Description:
  // Remove all the vector variables.
  void RemoveVectorVariables();

  // Description:
  // When ReplaceInvalidValues is on, all invalid values (such as
  // sqrt(-2), note that function parser does not handle complex
  // numbers) will be replaced by ReplacementValue. Otherwise an
  // error will be reported
  vtkSetMacro(ReplaceInvalidValues,int);
  vtkGetMacro(ReplaceInvalidValues,int);
  vtkBooleanMacro(ReplaceInvalidValues,int);
  vtkSetMacro(ReplacementValue,double);
  vtkGetMacro(ReplacementValue,double);

  // Description:
  // Check the validity of the function expression.
  void CheckExpression(int &pos, char **error);

  // Description:
  // Allow the user to force the function to be re-parsed
  void InvalidateFunction();

protected:
  vtkFunctionParser();
  ~vtkFunctionParser();

  int Parse();

  // Description:
  // Evaluate the function, returning true on success, false on failure.
  bool Evaluate();

  int CheckSyntax();

  void CopyParseError(int &position, char **error);

  void RemoveSpaces();
  char* RemoveSpacesFrom(const char* variableName);
  int OperatorWithinVariable(int idx);

  int BuildInternalFunctionStructure();
  void BuildInternalSubstringStructure(int beginIndex, int endIndex);
  void AddInternalByte(unsigned char newByte);

  int IsSubstringCompletelyEnclosed(int beginIndex, int endIndex);
  int FindEndOfMathFunction(int beginIndex);
  int FindEndOfMathConstant(int beginIndex);

  int IsVariableName(int currentIndex);
  int IsElementaryOperator(int op);

  int GetMathFunctionNumber(int currentIndex);
  int GetMathFunctionNumberByCheckingParenthesis( int currentIndex );
  int GetMathFunctionStringLength(int mathFunctionNumber);
  int GetMathConstantNumber(int currentIndex);
  int GetMathConstantStringLength(int mathConstantNumber);
  unsigned char GetElementaryOperatorNumber(char op);
  unsigned char GetOperandNumber(int currentIndex);
  int GetVariableNameLength(int variableNumber);

  int DisambiguateOperators();

  vtkSetStringMacro(ParseError);

  int FindPositionInOriginalFunction(const int& pos);

  char* Function;
  char* FunctionWithSpaces;

  int FunctionLength;
  int NumberOfScalarVariables;
  int NumberOfVectorVariables;
  char** ScalarVariableNames;
  char** VectorVariableNames;
  double* ScalarVariableValues;
  double** VectorVariableValues;
  unsigned char *ByteCode;
  int ByteCodeSize;
  double *Immediates;
  int ImmediatesSize;
  double *Stack;
  int StackSize;
  int StackPointer;

  vtkTimeStamp FunctionMTime;
  vtkTimeStamp ParseMTime;
  vtkTimeStamp VariableMTime;
  vtkTimeStamp EvaluateMTime;
  vtkTimeStamp CheckMTime;

  int ReplaceInvalidValues;
  double ReplacementValue;

  int   ParseErrorPositon;
  char* ParseError;

private:
  vtkFunctionParser(const vtkFunctionParser&);  // Not implemented.
  void operator=(const vtkFunctionParser&);  // Not implemented.
};

#endif
