/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionParser.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkFunctionParser - Parse and evaluate a mathematical expression
// .SECTION Description
// vtkFunctionParser is a class that takes in a mathematical expression as
// a char string, parses it, and evaluates it at the specified values of
// the variables in the input string.

#ifndef __vtkFunctionParser_h
#define __vtkFunctionParser_h

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
#define VTK_PARSER_SQUARE_ROOT 13
#define VTK_PARSER_SINE 14
#define VTK_PARSER_COSINE 15
#define VTK_PARSER_TANGENT 16
#define VTK_PARSER_ARCSINE 17
#define VTK_PARSER_ARCCOSINE 18
#define VTK_PARSER_ARCTANGENT 19
#define VTK_PARSER_HYPERBOLIC_SINE 20
#define VTK_PARSER_HYPERBOLIC_COSINE 21
#define VTK_PARSER_HYPERBOLIC_TANGENT 22

// functions involving vectors
#define VTK_PARSER_VECTOR_UNARY_MINUS 23
#define VTK_PARSER_DOT_PRODUCT 24
#define VTK_PARSER_VECTOR_ADD 25
#define VTK_PARSER_VECTOR_SUBTRACT 26
#define VTK_PARSER_SCALAR_MULTIPLE 27
#define VTK_PARSER_MAGNITUDE 28
#define VTK_PARSER_NORMALIZE 29

// codes for scalar variables come before those for vectors
#define VTK_PARSER_BEGIN_VARIABLES 30

class VTK_EXPORT vtkFunctionParser : public vtkObject
{
public:
  static vtkFunctionParser *New();
  vtkTypeMacro(vtkFunctionParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Convert the input string to an internal format.  (The function string
  // and variable names must be set before Parse() is called.)
  int Parse();

  // Description:
  // Evaluate the input function with the specified values.  (These values
  // must be set before Evaluate() is called.)
  double* Evaluate();

  // Decription:
  // Set/Get input string to parse.
  vtkSetStringMacro(Function);
  vtkGetStringMacro(Function);
  
  // Description:
  // Add a variable name.
  void AddScalarVariableName(char* variableName);
  void AddVectorVariableName(char* variableName);

  // Description:
  // Set the values for the variables in the order that their variable names
  // were added.
  void SetScalarVariableValues(double* values);
  void SetVectorVariableValues(double values[][3]);
  
protected:
  vtkFunctionParser();
  ~vtkFunctionParser();
  vtkFunctionParser(const vtkFunctionParser&) {};
  void operator=(const vtkFunctionParser&) {};
  
  void RemoveSpaces();
  int CheckSyntax();
  
  int BuildInternalFunctionStructure();
  void BuildInternalSubstringStructure(int beginIndex, int endIndex);
  void AddInternalByte(unsigned char newByte);
  
  int IsSubstringCompletelyEnclosed(int beginIndex, int endIndex);
  int FindEndOfMathFunction(int beginIndex);
  
  int IsVariableName(int currentIndex);
  int IsElementaryOperator(int op);
  
  int GetMathFunctionNumber(int currentIndex);
  int GetMathFunctionStringLength(int mathFunctionNumber);
  int GetElementaryOperatorNumber(char op);
  int GetOperandNumber(int currentIndex);
  int GetVariableNameLength(int variableNumber);
  
  int DisambiguateOperators();
  
  char* Function;
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
  unsigned int StackSize;
  unsigned int StackPointer;
};

#endif
