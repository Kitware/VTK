/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionParser.cxx
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

#include "vtkFunctionParser.h"
#include "vtkObjectFactory.h"


vtkFunctionParser* vtkFunctionParser::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFunctionParser");
  if(ret)
    {
    return (vtkFunctionParser*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFunctionParser;
}

vtkFunctionParser::vtkFunctionParser() 
{
  this->NumberOfScalarVariables = 0;
  this->NumberOfVectorVariables = 0;
  this->ScalarVariableNames = NULL;
  this->VectorVariableNames = NULL;
  this->ScalarVariableValues = NULL;
  this->VectorVariableValues = NULL;
  this->Function = NULL;
  this->ByteCode = NULL;
  this->ByteCodeSize = 0;
  this->Immediates = NULL;
  this->ImmediatesSize = 0;
  this->Stack = NULL;
  this->StackSize = 0;
  this->StackPointer = 0;
//  this->Result = NULL;
}

vtkFunctionParser::~vtkFunctionParser() 
{
  int i;
  
  if (this->ScalarVariableNames)
    {
    for (i = 0; i < this->NumberOfScalarVariables; i++)
      {
      delete [] this->ScalarVariableNames[i];
      this->ScalarVariableNames[i] = NULL;
      }
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }

  if (this->VectorVariableNames)
    {
    for (i = 0; i < this->NumberOfVectorVariables; i++)
      {
      delete [] this->VectorVariableNames[i];
      this->VectorVariableNames[i] = NULL;
      }
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }

  if (this->ScalarVariableValues)
    {
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }

  if (this->VectorVariableValues)
    {
    for (i = 0; i < this->NumberOfVectorVariables; i++)
      {
      delete [] this->VectorVariableValues[i];
      this->VectorVariableValues[i] = NULL;
      }
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }

  if (this->Function)
    {
    delete [] this->Function;
    this->Function = NULL;
    }
  
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    this->ByteCode = NULL;
    }
  
  if (this->Immediates)
    {
    delete [] this->Immediates;
    this->Immediates = NULL;
    }
  
  if (this->Stack)
    {
    delete [] this->Stack;
    this->Stack = NULL;
    }
}

int vtkFunctionParser::Parse()
{
  int result;
  int i;
  
  this->RemoveSpaces();
  
  result = this->CheckSyntax();
  if (!result)
    {
    return 0;
    }
  
  result = this->BuildInternalFunctionStructure();
  if (!result)
    {
    vtkErrorMacro("Error creating internal structure for parse string");
    return 0;
    }
  
  // need to make sure that the ambiguous operators are correct
  // - scalar/vector +
  // - scalar/vector -
  // - scalar/vector unary minus
  // - * (2 scalars) or scalar multiple (scalar, vector)
  result = this->DisambiguateOperators();
  if (!result)
    {
    vtkErrorMacro("Error deciding between ambiguous operators");
    return 0;
    }
  
  // need to recalculate stack size based on number of vector variables
  // in byte code
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    if (this->ByteCode[i] >= VTK_PARSER_BEGIN_VARIABLES +
        this->NumberOfScalarVariables)
      {
      this->StackSize += 2;
      }
    }
  
  if (this->StackSize)
    {
    this->Stack = new double[this->StackSize];
    if (!this->Stack)
      {
      vtkErrorMacro("Out of memory");
      return 0;
      }
    }  

  return 1;
}

int vtkFunctionParser::DisambiguateOperators()
{
  unsigned char* tempStack = new unsigned char[this->ByteCodeSize];
  int i;
  int tempStackPtr = -1;
  
  // using 0 for scalars and 1 for vectors
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    switch (this->ByteCode[i])
      {
      case VTK_PARSER_IMMEDIATE:
        tempStackPtr++;
        tempStack[tempStackPtr] = 0;
        break;
      case VTK_PARSER_UNARY_MINUS:
        if (tempStack[tempStackPtr] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_UNARY_MINUS;
          }
        break;
      case VTK_PARSER_ADD:
        if (tempStack[tempStackPtr] != 0 && tempStack[tempStackPtr-1] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_ADD;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_SUBTRACT:
        if (tempStack[tempStackPtr] != 0 && tempStack[tempStackPtr-1] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_SUBTRACT;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MULTIPLY:
        if (tempStack[tempStackPtr-1] == 0 && tempStack[tempStackPtr] == 1)
          {
          this->ByteCode[i] = VTK_PARSER_SCALAR_MULTIPLE;
          tempStack[tempStackPtr-1] = 1;
          }
        else if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting either 2 scalars or a scalar followed by"
                        << " a vector");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_DIVIDE:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't divide vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_POWER:
        if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("can't raise a vector to a power");
          return 0;
          }
      case VTK_PARSER_ABSOLUTE_VALUE:
      case VTK_PARSER_EXPONENT:
      case VTK_PARSER_CEILING:
      case VTK_PARSER_FLOOR:
      case VTK_PARSER_LOGARITHM:
      case VTK_PARSER_SQUARE_ROOT:
      case VTK_PARSER_SINE:
      case VTK_PARSER_COSINE:
      case VTK_PARSER_TANGENT:
      case VTK_PARSER_ARCSINE:
      case VTK_PARSER_ARCCOSINE:
      case VTK_PARSER_ARCTANGENT:
      case VTK_PARSER_HYPERBOLIC_SINE:
      case VTK_PARSER_HYPERBOLIC_COSINE:
      case VTK_PARSER_HYPERBOLIC_TANGENT:
        if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting a scalar, but got a vector");
          return 0;
          }
        break;
      case VTK_PARSER_VECTOR_UNARY_MINUS:
        if (tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_UNARY_MINUS;
          }
        break;
      case VTK_PARSER_DOT_PRODUCT:
        if (tempStack[tempStackPtr] == 0 || tempStack[tempStackPtr] == 0)
          {
          vtkErrorMacro("dot product does not operate on scalars");
          return 0;
          }
        tempStack[tempStackPtr-1] = 0;
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_ADD:
        if (tempStack[tempStackPtr] != 1 && tempStack[tempStackPtr-1] != 1)
          {
          this->ByteCode[i] = VTK_PARSER_ADD;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_SUBTRACT:
        if (tempStack[tempStackPtr] != 1 && tempStack[tempStackPtr-1] != 1)
          {
          this->ByteCode[i] = VTK_PARSER_SUBTRACT;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("subtraction expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_SCALAR_MULTIPLE:
        if (tempStack[tempStackPtr] == 0 && tempStack[tempStackPtr-1] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_MULTIPLY;
          }
        else if (tempStack[tempStackPtr-1] != 0 ||
                 tempStack[tempStackPtr] != 1)
          {
          vtkErrorMacro("scalar multiple expects a scalar followed "
                        << "by a vector");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MAGNITUDE:
        if (tempStack[tempStackPtr] == 0)
          {
          vtkErrorMacro("magnitude expects a vector, but got a scalar");
          return 0;
          }
        tempStack[tempStackPtr] = 0;
        break;
      case VTK_PARSER_NORMALIZE:
        if (tempStack[tempStackPtr] == 0)
          {
          vtkErrorMacro("normalize expects a vector, but got a scalar");
          return 0;
          }
        break;
      default:
        if ((this->ByteCode[i] - VTK_PARSER_BEGIN_VARIABLES) <
            this->NumberOfScalarVariables)
          {
          tempStackPtr++;
          tempStack[tempStackPtr] = 0;
          }
        else
          {
          tempStackPtr++;
          tempStack[tempStackPtr] = 1;
          }
      }
    }
  
  delete [] tempStack;
  return 1;
}

double* vtkFunctionParser::Evaluate()
{
  int numBytesProcessed;
  unsigned int numImmediatesProcessed = 0;
  int stackPosition = -1;
  double magnitude;
  
  for (numBytesProcessed = 0; numBytesProcessed < this->ByteCodeSize;
       numBytesProcessed++)
    {
    switch (this->ByteCode[numBytesProcessed])
      {
      case VTK_PARSER_IMMEDIATE:
        this->Stack[++stackPosition] =
          this->Immediates[numImmediatesProcessed++];
        break;
      case VTK_PARSER_UNARY_MINUS:
        this->Stack[stackPosition] =- this->Stack[stackPosition];
        break;
      case VTK_PARSER_ADD:
        this->Stack[stackPosition-1] += this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_SUBTRACT:
        this->Stack[stackPosition-1] -= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_MULTIPLY:
        this->Stack[stackPosition-1] *= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_DIVIDE:
        if (this->Stack[stackPosition]==0)
          {
          vtkErrorMacro("Trying to divide by zero");
          return NULL;
          }
        this->Stack[stackPosition-1] /= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_POWER:
        this->Stack[stackPosition-1] = pow(this->Stack[stackPosition-1],
                                           this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_ABSOLUTE_VALUE:
        this->Stack[stackPosition] = fabs(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_EXPONENT:
        this->Stack[stackPosition] = exp(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_CEILING:
        this->Stack[stackPosition] = ceil(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_FLOOR:
        this->Stack[stackPosition] = floor(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_LOGARITHM:
        if (this->Stack[stackPosition]<=0)
          {
          vtkErrorMacro("Trying to take a logarithm of a negative value");
          return NULL;
          }
        this->Stack[stackPosition] = log(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_SQUARE_ROOT:
        if (this->Stack[stackPosition] < 0)
          {
          vtkErrorMacro("Trying to take a square root of a negative value");
          return NULL;
          }
        this->Stack[stackPosition] = sqrt(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_SINE:
        this->Stack[stackPosition] = sin(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_COSINE:
        this->Stack[stackPosition] = cos(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_TANGENT:
        this->Stack[stackPosition] = tan(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_ARCSINE:
        if (this->Stack[stackPosition] < -1 || this->Stack[stackPosition] > 1)
          {
          vtkErrorMacro("Trying to take asin of a value < -1 or > 1");
          return NULL;
          }
        this->Stack[stackPosition] = asin(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_ARCCOSINE:
        if(this->Stack[stackPosition]<-1 || this->Stack[stackPosition]>1)
          {
          vtkErrorMacro("Trying to take acos of a value < -1 or > 1");
          return NULL;
          }
        this->Stack[stackPosition] = acos(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_ARCTANGENT:
        this->Stack[stackPosition] = atan(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_SINE:
        this->Stack[stackPosition] = sinh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_COSINE:
        this->Stack[stackPosition] = cosh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_TANGENT:
        this->Stack[stackPosition] = tanh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_VECTOR_UNARY_MINUS:
        this->Stack[stackPosition] = -this->Stack[stackPosition];
        this->Stack[stackPosition-1] = -this->Stack[stackPosition-1];
        this->Stack[stackPosition-2] = -this->Stack[stackPosition-2];
        break;
      case VTK_PARSER_DOT_PRODUCT:
        this->Stack[stackPosition-3] *= this->Stack[stackPosition];
        this->Stack[stackPosition-4] *= this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] *= this->Stack[stackPosition-2];
        this->Stack[stackPosition-5] = this->Stack[stackPosition-5] +
          this->Stack[stackPosition-4] + this->Stack[stackPosition-3];
        stackPosition -= 5;
        break;
      case VTK_PARSER_VECTOR_ADD:
        this->Stack[stackPosition-3] += this->Stack[stackPosition];
        this->Stack[stackPosition-4] += this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] += this->Stack[stackPosition-2];
        stackPosition -= 3;
        break;
      case VTK_PARSER_VECTOR_SUBTRACT:
        this->Stack[stackPosition-3] -= this->Stack[stackPosition];
        this->Stack[stackPosition-4] -= this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] -= this->Stack[stackPosition-2];
        stackPosition -= 3;
        break;
      case VTK_PARSER_SCALAR_MULTIPLE:
        this->Stack[stackPosition] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-1] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-2] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-3] = this->Stack[stackPosition-2];
        this->Stack[stackPosition-2] = this->Stack[stackPosition-1];
        this->Stack[stackPosition-1] = this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_MAGNITUDE:
        this->Stack[stackPosition-2] =
          sqrt(pow(this->Stack[stackPosition], 2) +
               pow(this->Stack[stackPosition-1], 2) +
               pow(this->Stack[stackPosition-2], 2));
        stackPosition -= 2;
        break;
      case VTK_PARSER_NORMALIZE:
        magnitude = sqrt(pow(this->Stack[stackPosition], 2) +
                         pow(this->Stack[stackPosition-1], 2) +
                         pow(this->Stack[stackPosition-2], 2));
        if (magnitude == 0)
          {
          vtkErrorMacro("The magnitude of this vector is zero; can't"
                        << " normalize");
          return NULL;
          }
        this->Stack[stackPosition] /= magnitude;
        this->Stack[stackPosition-1] /= magnitude;
        this->Stack[stackPosition-2] /= magnitude;
        break;
      default:
        if ((this->ByteCode[numBytesProcessed] -
             VTK_PARSER_BEGIN_VARIABLES) < this->NumberOfScalarVariables)
          {
          this->Stack[++stackPosition] = 
            this->ScalarVariableValues[this->ByteCode[numBytesProcessed] -
                                      VTK_PARSER_BEGIN_VARIABLES];
          }
        else
          {
          int vectorNum = this->ByteCode[numBytesProcessed] -
            VTK_PARSER_BEGIN_VARIABLES - this->NumberOfScalarVariables;
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][0];
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][1];
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][2];
          }
      }
    }

  return this->Stack;
}

void vtkFunctionParser::AddScalarVariableName(char* variableName)
{
  int size;
  int i;
  
  size = this->NumberOfScalarVariables;

  char ** newVariableNameList = new char *[size]; // temporary array    
  
  // copy descriptions to temporary array
  for (i = 0; i < size; i++)
    {
    newVariableNameList[i] =
      new char[strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(newVariableNameList[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    }
  delete [] this->ScalarVariableNames;
  
  // make room for new description
  this->ScalarVariableNames = new char *[size+1];
  
  // copy existing descriptions back to first array
  for (i = 0; i < size; i++)
    {
    this->ScalarVariableNames[i] =
      new char[strlen(newVariableNameList[i]) + 1];
    strcpy(this->ScalarVariableNames[i], newVariableNameList[i]);
    delete [] newVariableNameList[i];
    }
  delete [] newVariableNameList;
  
  // add new description at end of first array
  this->ScalarVariableNames[size] = new char[strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[size], variableName);
  this->NumberOfScalarVariables++;
}

void vtkFunctionParser::AddVectorVariableName(char* variableName)
{
  int size;
  int i;
  
  size = this->NumberOfVectorVariables;

  char ** newVariableNameList = new char *[size]; // temporary array    
  
  // copy descriptions to temporary array
  for (i = 0; i < size; i++)
    {
    newVariableNameList[i] =
      new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(newVariableNameList[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    }
  delete [] this->VectorVariableNames;
  
  // make room for new description
  this->VectorVariableNames = new char *[size+1];
  
  // copy existing descriptions back to first array
  for (i = 0; i < size; i++)
    {
    this->VectorVariableNames[i] =
      new char[strlen(newVariableNameList[i]) + 1];
    strcpy(this->VectorVariableNames[i], newVariableNameList[i]);
    delete [] newVariableNameList[i];
    }
  delete [] newVariableNameList;
  
  // add new description at end of first array
  this->VectorVariableNames[size] = new char[strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[size], variableName);
  this->NumberOfVectorVariables++;
}

int vtkFunctionParser::IsVariableName(int currentIndex)
{
  int i;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strncmp(this->ScalarVariableNames[i], &this->Function[currentIndex],
                strlen(this->ScalarVariableNames[i])) == 0)
      {
      return 1;
      }
    }
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strncmp(this->VectorVariableNames[i], &this->Function[currentIndex],
                strlen(this->VectorVariableNames[i])) == 0)
      {
      return 1;
      }
    }

  return 0;
}

int vtkFunctionParser::IsElementaryOperator(int op)
{
  return strchr("+-.*/^", op) != NULL;
}

void vtkFunctionParser::SetScalarVariableValues(double* values)
{
  int i;
  
  if (this->ScalarVariableValues)
    {
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }
  
  this->ScalarVariableValues = new double[this->NumberOfScalarVariables];
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    this->ScalarVariableValues[i] = values[i];
    }
}

void vtkFunctionParser::SetVectorVariableValues(double values[][3])
{
  int i;
  
  if (this->VectorVariableValues)
    {
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }
  
  this->VectorVariableValues = new double *[this->NumberOfVectorVariables];
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    this->VectorVariableValues[i] = new double[3];
    this->VectorVariableValues[i][0] = values[i][0];
    this->VectorVariableValues[i][1] = values[i][1];
    this->VectorVariableValues[i][2] = values[i][2];
    }
}

void vtkFunctionParser::RemoveSpaces()
{
  char *tempString;
  int i, length;
  
  this->FunctionLength = 0;
  length = strlen(this->Function);
  
  tempString = new char[length];
  for (i = 0; i < length; i++)
    {
    if (!isspace(this->Function[i]))
      {
      tempString[this->FunctionLength] = this->Function[i];
      this->FunctionLength++;
      }
    }
  
  delete [] this->Function;
  this->Function = new char[this->FunctionLength];
  strncpy(this->Function, tempString, this->FunctionLength);
  delete [] tempString;
}

int vtkFunctionParser::CheckSyntax()
{
  int index = 0, parenthesisCount = 0, currentChar;
  char* ptr;
  int functionNumber;
  
  while (true)
    {
    currentChar = this->Function[index];
    
    // Check for valid operand (must appear)
    
    // Check for leading -
    if (currentChar == '-')
      {
      currentChar = this->Function[++index];  
      if(index == this->FunctionLength)
        {
        vtkErrorMacro("Syntax error: unary minus with no operand;"
                      << " see position " << index);
        return 0;
        }
      }

    // Check for math function
    if (functionNumber = this->GetMathFunctionNumber(index))
      {
      index += this->GetMathFunctionStringLength(functionNumber);
      currentChar = this->Function[index];
      if ( currentChar != '(' )
        {
        vtkErrorMacro("Syntax error: input to math function not in "
                      << "parentheses; see position " << index);
        return 0;
        }
      }
    
    // Check for opening parenthesis
    if( currentChar == '(' )
      {
      parenthesisCount++;
      index++;
      continue;
      }
    
    // Check for number
    if(isdigit(currentChar) ||
       (currentChar == '.' && isdigit(this->Function[index+1])))
      {
      strtod(&this->Function[index], &ptr);
      index += int(ptr-&this->Function[index]);
      currentChar = this->Function[index];
      }
    else
      { // Check for variable
      if (!this->IsVariableName(index))
        {
        vtkErrorMacro("Syntax error: expecting a variable name; "
                      << "see position " << index);
        return 0;
        }
      index += this->GetVariableNameLength(this->GetOperandNumber(index) -
                                           VTK_PARSER_BEGIN_VARIABLES);
      currentChar = this->Function[index];
      }
    
    // Check for closing parenthesis
    while ( currentChar == ')' )
      {
      parenthesisCount--;
      if(parenthesisCount < 0)
        {
        vtkErrorMacro("Mismatched parenthesis; see position " << index);
        return 0;
        }
      if( this->Function[index - 1] == '(' )
        {
        vtkErrorMacro("Empty parentheses; see position " << index);
        return 0;
        }
      currentChar = this->Function[++index];
      }
    
    // If we get here, we have a legal operand and now a legal operator or
    // end of string must follow.
    
    // Check for EOS
    // The only way to end the checking loop without error.
    if (index == this->FunctionLength)
      {
      break;
      }
    // Check for operator
    if(!this->IsElementaryOperator(currentChar))
      {
      vtkErrorMacro("Syntax error: operator expected; see position "
                    << index);
      return 0;
      }
    
    // If we get here, we have an operand and an operator; the next loop will
    // check for another operand (must appear)
    index++;
    } // while
  
  // Check that all opened parentheses are also closed
  if(parenthesisCount > 0)
    {
    vtkErrorMacro("Missing closing parenthesis; see position " << index);
    return 0;
    }
  
  // The string is ok
  return 1;
}

int vtkFunctionParser::BuildInternalFunctionStructure()
{
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    this->ByteCode = NULL;
    }
  if (this->Immediates)
    {
    delete [] this->Immediates;
    this->Immediates = NULL;
    }
  if (this->Stack)
    {
    delete [] this->Stack;
    this->Stack = NULL;
    }
  
  this->ByteCodeSize = this->ImmediatesSize = this->StackSize = 0;
  this->StackPointer = 0;
  this->BuildInternalSubstringStructure(0, this->FunctionLength - 1);  

  return 1;
}

void vtkFunctionParser::BuildInternalSubstringStructure(int beginIndex,
                                                        int endIndex)
{
  int mathFunctionNum, beginIndex2;
  int opNum, parenthesisCount, i;
  static const char* const elementaryMathOps = "+-.*/^";
  
  if (this->IsSubstringCompletelyEnclosed(beginIndex, endIndex))
    {
    this->BuildInternalSubstringStructure(beginIndex+1, endIndex-1);
    return;
    }

  if (this->Function[beginIndex] == '-')
    {
    if (this->IsSubstringCompletelyEnclosed(beginIndex+1, endIndex))
      {
      this->BuildInternalSubstringStructure(beginIndex+2, endIndex-1);
      this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
      return;
      }
    if (this->GetMathFunctionNumber(beginIndex+1) > 0 &&
        this->FindEndOfMathFunction(beginIndex+1) == endIndex)
      {
      this->BuildInternalSubstringStructure(beginIndex+1, endIndex);
      this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
      return;
      }
    }

  if (isalpha(this->Function[beginIndex]))
    {
    mathFunctionNum = this->GetMathFunctionNumber(beginIndex);
    if (mathFunctionNum > 0)
      {
      beginIndex2 = beginIndex;
      while (this->Function[beginIndex2] != '(')
        {
        beginIndex2++;
        }
      if (this->IsSubstringCompletelyEnclosed(beginIndex2, endIndex))
        {
        this->BuildInternalSubstringStructure(beginIndex2+1, endIndex-1);
        this->AddInternalByte(mathFunctionNum);
        return;
        }
      }
    }

  for (opNum = 0; opNum < 6; opNum++)
    {
    parenthesisCount = 0;
    for (i = endIndex; i > beginIndex; i--)
      {
      if (this->Function[i] == ')')
        {
        parenthesisCount++;
        }
      else if (this->Function[i] == '(')
        {
        parenthesisCount--;
        }
      if (parenthesisCount == 0 &&
          this->Function[i] == elementaryMathOps[opNum] &&
          !(this->Function[i] == '-' &&
            (this->IsElementaryOperator(i-1) || this->Function[i-1] == '(' ||
             (this->Function[i-1] == 'e' && i > 1 &&
              isdigit(this->Function[i-2])))))
        {
        this->BuildInternalSubstringStructure(beginIndex, i-1);
        this->BuildInternalSubstringStructure(i+1, endIndex);
        this->AddInternalByte(
          this->GetElementaryOperatorNumber(elementaryMathOps[opNum]));
        this->StackPointer--;
        return;
        }
      }
    }

  beginIndex2 = beginIndex;
  if (this->Function[beginIndex] == '-')
    {
    beginIndex2++;
    }

  this->AddInternalByte(this->GetOperandNumber(beginIndex2));
  this->StackPointer++;
    
  if (this->StackPointer > this->StackSize)
    {
    this->StackSize++;
    }
  if (beginIndex2 > beginIndex)
    {
    this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
    }
}

void vtkFunctionParser::AddInternalByte(unsigned char newByte)
{
  int i;
  unsigned char *tempByteCode = new unsigned char[this->ByteCodeSize];

  for (i = 0; i < this->ByteCodeSize; i++)
    { // Copy current byte code to a temporary array
    tempByteCode[i] = this->ByteCode[i];
    }
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    }
  
  // Allocate space for new byte.
  this->ByteCode = new unsigned char[this->ByteCodeSize + 1];
  
  // Copy contents of temporary array back to ByteCode.
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    this->ByteCode[i] = tempByteCode[i];
    }
  
  this->ByteCode[this->ByteCodeSize] = newByte;
  this->ByteCodeSize++;
  delete [] tempByteCode;
}

int vtkFunctionParser::IsSubstringCompletelyEnclosed(int beginIndex,
                                                     int endIndex)
{
  int i, parenthesisCount;
  
  if ( this->Function[beginIndex] == '(' && this->Function[endIndex]== ')' )
    {
    parenthesisCount = 1;
    for (i = beginIndex + 1; i < endIndex; i++)
      {
      if (this->Function[i] == '(' )
        {
        parenthesisCount++;
        }
      else if(this->Function[i] == ')' )
        {
        parenthesisCount--;
        }
      if (parenthesisCount == 0)
        {
        break;
        }
      }
    if (i == endIndex)
      {
      return 1;
      }
    }
  return 0;
}

int vtkFunctionParser::GetMathFunctionNumber(int currentIndex)
{
  if (strncmp(&this->Function[currentIndex], "abs", 3) == 0)
    {
    return VTK_PARSER_ABSOLUTE_VALUE;
    }
  if (strncmp(&this->Function[currentIndex], "exp", 3) == 0)
    {
    return VTK_PARSER_EXPONENT;
    }
  if (strncmp(&this->Function[currentIndex], "ceil", 4) == 0)
    {
    return VTK_PARSER_CEILING;
    }
  if (strncmp(&this->Function[currentIndex], "floor", 5) == 0)
    {
    return VTK_PARSER_FLOOR;
    }
  if (strncmp(&this->Function[currentIndex], "log", 3) == 0)
    {
    return VTK_PARSER_LOGARITHM;
    }
  if (strncmp(&this->Function[currentIndex], "sqrt", 4) == 0)
    {
    return VTK_PARSER_SQUARE_ROOT;
    }
  if (strncmp(&this->Function[currentIndex], "sin", 3) == 0)
    {
    return VTK_PARSER_SINE;
    }
  if (strncmp(&this->Function[currentIndex], "cos", 3) == 0)
    {
    return VTK_PARSER_COSINE;
    }
  if (strncmp(&this->Function[currentIndex], "tan", 3) == 0)
    {
    return VTK_PARSER_TANGENT;
    }
  if (strncmp(&this->Function[currentIndex], "asin", 4) == 0)
    {
    return VTK_PARSER_ARCSINE;
    }
  if (strncmp(&this->Function[currentIndex], "acos", 4) == 0)
    {
    return VTK_PARSER_ARCCOSINE;
    }
  if (strncmp(&this->Function[currentIndex], "atan", 4) == 0)
    {
    return VTK_PARSER_ARCTANGENT;
    }
  if (strncmp(&this->Function[currentIndex], "sinh", 4) == 0)
    {
    return VTK_PARSER_HYPERBOLIC_SINE;
    }
  if (strncmp(&this->Function[currentIndex], "cosh", 4) == 0)
    {
    return VTK_PARSER_HYPERBOLIC_COSINE;
    }
  if (strncmp(&this->Function[currentIndex], "tanh", 4) == 0)
    {
    return VTK_PARSER_HYPERBOLIC_TANGENT;
    }
  if (strncmp(&this->Function[currentIndex], "mag", 3) == 0)
    {
    return VTK_PARSER_MAGNITUDE;
    }
  if (strncmp(&this->Function[currentIndex], "norm", 4) == 0)
    {
    return VTK_PARSER_NORMALIZE;
    }
  
  return 0;
}

int vtkFunctionParser::GetMathFunctionStringLength(int mathFunctionNumber)
{
  switch (mathFunctionNumber)
    {
    case VTK_PARSER_ABSOLUTE_VALUE:
    case VTK_PARSER_EXPONENT:
    case VTK_PARSER_LOGARITHM:
    case VTK_PARSER_SINE:
    case VTK_PARSER_COSINE:
    case VTK_PARSER_TANGENT:
    case VTK_PARSER_MAGNITUDE:
      return 3;
    case VTK_PARSER_CEILING:
    case VTK_PARSER_SQUARE_ROOT:
    case VTK_PARSER_ARCSINE:
    case VTK_PARSER_ARCCOSINE:
    case VTK_PARSER_ARCTANGENT:
    case VTK_PARSER_HYPERBOLIC_SINE:
    case VTK_PARSER_HYPERBOLIC_COSINE:
    case VTK_PARSER_HYPERBOLIC_TANGENT:
    case VTK_PARSER_NORMALIZE:
      return 4;
    case VTK_PARSER_FLOOR:
      return 5;
    default:
      vtkWarningMacro("Unknown math function");
      return 0;
    }
}

int vtkFunctionParser::GetVariableNameLength(int variableNumber)
{
  if (variableNumber < this->NumberOfScalarVariables)
    {
    return strlen(this->ScalarVariableNames[variableNumber]);
    }
  else
    {
    return strlen(this->VectorVariableNames[variableNumber -
                                           this->NumberOfScalarVariables]);
    }
}

int vtkFunctionParser::FindEndOfMathFunction(int beginIndex)
{
  int i = beginIndex, parenthesisCount;
  
  while (this->Function[i] != '(' )
    {
    i++;
    }
  
  for (parenthesisCount = 1; parenthesisCount > 0; ++i)
    {
    parenthesisCount += (this->Function[i] == '(' ? 1 :
                         (this->Function[i] == ')' ? -1 : 0));
    }
  return i - 1;
}

int vtkFunctionParser::GetElementaryOperatorNumber(char op)
{
  static const char* const operators = "+-*/^";
  int i;
  
  for(i = 0; i < 5; i++)
    {
    if (operators[i] == op)
      {
      return VTK_PARSER_ADD + i;
      }
    }
  if (op == '.')
    {
    return VTK_PARSER_DOT_PRODUCT;
    }
  
  return 0;
}

int vtkFunctionParser::GetOperandNumber(int currentIndex)
{
  int i;

  if (isdigit(this->Function[currentIndex]) ||
     this->Function[currentIndex] == '.') // Number
    {
    double *tempImmediates = new double[this->ImmediatesSize];
    for (i = 0; i < this->ImmediatesSize; i++)
      { // Copy current immediates to a temporary array
      tempImmediates[i] = this->Immediates[i];
      }
    if (this->Immediates)
      {
      delete [] this->Immediates;
      }
    
    // Allocate space for new immediate value.
    this->Immediates = new double[this->ImmediatesSize + 1];
    
    // Copy contents of temporary array back to Immediates.
    for (i = 0; i < this->ImmediatesSize; i++)
      {
      this->Immediates[i] = tempImmediates[i];
      }
  
    this->Immediates[this->ImmediatesSize] =
      atof(&this->Function[currentIndex]);
    this->ImmediatesSize++;
    delete [] tempImmediates;
    return VTK_PARSER_IMMEDIATE;
    }
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->ScalarVariableNames[i],
                strlen(this->ScalarVariableNames[i])) == 0)
      {
      return VTK_PARSER_BEGIN_VARIABLES + i;
      }
    }
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->VectorVariableNames[i],
                strlen(this->VectorVariableNames[i])) == 0)
      {
      return VTK_PARSER_BEGIN_VARIABLES + i + this->NumberOfScalarVariables;
      }
    }
  
  return 0;
}

void vtkFunctionParser::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Function: "
     << (this->Function ? this->Function : "(none)") << endl;
}
