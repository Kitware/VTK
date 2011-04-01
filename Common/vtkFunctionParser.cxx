/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFunctionParser.h"
#include "vtkObjectFactory.h"

#include <ctype.h>

vtkStandardNewMacro(vtkFunctionParser);

static double vtkParserVectorErrorResult[3] = { VTK_PARSER_ERROR_RESULT,
                                                VTK_PARSER_ERROR_RESULT,
                                                VTK_PARSER_ERROR_RESULT };
//-----------------------------------------------------------------------------
vtkFunctionParser::vtkFunctionParser()
{
  this->NumberOfScalarVariables = 0;
  this->NumberOfVectorVariables = 0;
  this->ScalarVariableNames = NULL;
  this->VectorVariableNames = NULL;
  this->ScalarVariableValues = NULL;
  this->VectorVariableValues = NULL;
  this->Function = NULL;
  this->FunctionWithSpaces = NULL;
  this->ByteCode = NULL;
  this->ByteCodeSize = 0;
  this->Immediates = NULL;
  this->ImmediatesSize = 0;
  this->Stack = NULL;
  this->StackSize = 0;
  this->StackPointer = 0;

  this->EvaluateMTime.Modified();
  this->VariableMTime.Modified();
  this->ParseMTime.Modified();
  this->FunctionMTime.Modified();
  this->CheckMTime.Modified();

  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;

  this->ParseErrorPositon = -1;
  this->ParseError        = NULL;
}

//-----------------------------------------------------------------------------
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

  if (this->FunctionWithSpaces)
    {
    delete [] this->FunctionWithSpaces;
    this->FunctionWithSpaces = NULL;
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

  if(this->ParseError)
    {
    this->SetParseError(0);
    }
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::SetFunction(const char *function)
{
  if (this->Function && function && strcmp(this->Function,function) == 0)
    {
    return;
    }

  if (this->Function)
    {
    delete [] this->Function;
    delete [] this->FunctionWithSpaces;
    }

  if (function)
    {
    this->Function = new char[strlen(function)+1];
    this->FunctionWithSpaces = new char[strlen(function) + 1];

    strcpy(this->Function,function);
    strcpy(this->FunctionWithSpaces, function);
    }
  else
    {
    this->Function = NULL;
    this->FunctionWithSpaces = NULL;
    }

  this->FunctionMTime.Modified();
  this->Modified();
}

int vtkFunctionParser::Parse()
{
  int result;
  int i;

  if (this->Function == NULL)
    {
    vtkErrorMacro("Parse: no function has been set");
    return 0;
    }

  result = this->CheckSyntax();
  if (!result)
    {
    return 0;
    }

  result = this->BuildInternalFunctionStructure();
  if (!result)
    {
    vtkErrorMacro("Parse: Error creating internal structure for parse string");
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
    vtkErrorMacro("Parse: Error deciding between ambiguous operators");
    return 0;
    }

  // need to recalculate stack size based on number of vector variables
  // in byte code
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    if ((this->ByteCode[i] >= VTK_PARSER_BEGIN_VARIABLES +
         this->NumberOfScalarVariables) ||
        (this->ByteCode[i] == VTK_PARSER_IHAT) ||
        (this->ByteCode[i] == VTK_PARSER_JHAT) ||
        (this->ByteCode[i] == VTK_PARSER_KHAT))
      {
      this->StackSize += 2;
      }
    }

  if (this->StackSize)
    {
    this->Stack = new double[this->StackSize];
    if (!this->Stack)
      {
      vtkErrorMacro("Parse: Out of memory");
      return 0;
      }
    }

  this->ParseMTime.Modified();
  return 1;
}

//-----------------------------------------------------------------------------
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
          this->ByteCode[i] = VTK_PARSER_SCALAR_TIMES_VECTOR;
          tempStack[tempStackPtr-1] = 1;
          }
        else if (tempStack[tempStackPtr-1] == 1 &&
                 tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_TIMES_SCALAR;
          tempStack[tempStackPtr-1] = 1;
          }
        else if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting either 2 scalars or a scalar and"
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
      case VTK_PARSER_LESS_THAN:
      case VTK_PARSER_GREATER_THAN:
      case VTK_PARSER_EQUAL_TO:
      case VTK_PARSER_AND:
      case VTK_PARSER_OR:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("Vectors cannot be used in boolean expressions.");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_ABSOLUTE_VALUE:
      case VTK_PARSER_EXPONENT:
      case VTK_PARSER_CEILING:
      case VTK_PARSER_FLOOR:
      case VTK_PARSER_LOGARITHM:
      case VTK_PARSER_LOGARITHME:
      case VTK_PARSER_LOGARITHM10:
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
      case VTK_PARSER_SIGN:
        if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting a scalar, but got a vector");
          return 0;
          }
        break;
      case VTK_PARSER_MIN:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't apply min to vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MAX:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't apply max to vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_CROSS:
        if (tempStack[tempStackPtr] == 0 || tempStack[tempStackPtr-1] == 0)
          {
          vtkErrorMacro("can't apply cross to scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_UNARY_MINUS:
        if (tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_UNARY_MINUS;
          }
        break;
      case VTK_PARSER_DOT_PRODUCT:
        if (tempStack[tempStackPtr] == 0 || tempStack[tempStackPtr-1] == 0)
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
      case VTK_PARSER_SCALAR_TIMES_VECTOR:
        if (tempStack[tempStackPtr] == 0 && tempStack[tempStackPtr-1] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_MULTIPLY;
          }
        else if (tempStack[tempStackPtr-1] == 1 &&
                 tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_TIMES_SCALAR;
          }
        else
          {
          vtkErrorMacro("expecting a scalar followed by a vector");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_TIMES_SCALAR:
        if (tempStack[tempStackPtr] == 0 && tempStack[tempStackPtr-1] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_MULTIPLY;
          }
        else if (tempStack[tempStackPtr-1] == 0 &&
                 tempStack[tempStackPtr] == 1)
          {
          this->ByteCode[i] = VTK_PARSER_SCALAR_TIMES_VECTOR;
          }
        else
          {
          vtkErrorMacro("expecting a vector followed by a scalar");
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
      case VTK_PARSER_IHAT:
      case VTK_PARSER_JHAT:
      case VTK_PARSER_KHAT:
        tempStackPtr++;
        tempStack[tempStackPtr] = 1;
        break;
      case VTK_PARSER_IF:
        // tempStack[tempStackPtr] = tempStack[2] refers to the bool argument
        // of if(bool,valtrue,valfalse). tempStack[1] is valtrue, and
        // tempStack[0] is valfalse.
        if (tempStack[tempStackPtr] != 0)
          {
          vtkErrorMacro("first argument of if(bool,valtrue,valfalse) cannot be a vector");
          return 0;
          }
        else if (tempStack[tempStackPtr-1] != 0 &&
                 tempStack[tempStackPtr-2] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_IF;
          }
        else if ((tempStack[tempStackPtr-1] == 0 &&
                  tempStack[tempStackPtr-2] != 0) ||
                 (tempStack[tempStackPtr-1] != 0 &&
                  tempStack[tempStackPtr-2] == 0))
          {
          vtkErrorMacro("the if function expects the second and third arguments to be either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_IF:
        // tempStack[tempStackPtr] = tempStack[2] refers to the bool argument
        // of if(bool,valtrue,valfalse). tempStack[1] is valtrue, and
        // tempStack[0] is valfalse.
        if (tempStack[tempStackPtr] != 0)
          {
          vtkErrorMacro("first argument of if(bool,valtrue,valfalse) cannot be a vector");
          return 0;
          }
        else if (tempStack[tempStackPtr-1] != 1 &&
                 tempStack[tempStackPtr-2] != 1)
          {
          this->ByteCode[i] = VTK_PARSER_IF;
          }
        else if ((tempStack[tempStackPtr-1] == 0 &&
                  tempStack[tempStackPtr-2] != 0) ||
                 (tempStack[tempStackPtr-1] != 0 &&
                  tempStack[tempStackPtr-2] == 0))
          {
          vtkErrorMacro("the if function expects the second and third arguments to be either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        tempStackPtr--;
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

//-----------------------------------------------------------------------------
bool vtkFunctionParser::Evaluate()
{
  int numBytesProcessed;
  int numImmediatesProcessed = 0;
  int stackPosition = -1;
  double magnitude;
  double temp[3];

  this->StackPointer = -1;

  if (this->FunctionMTime.GetMTime() > this->ParseMTime.GetMTime())
    {
    if (this->Parse() == 0)
      {
      return false;
      }
    }

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
        this->Stack[stackPosition] = -(this->Stack[stackPosition]);
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
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition-1] = this->ReplacementValue;
            stackPosition--;
            }
          else
            {
            vtkErrorMacro("Trying to divide by zero");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition-1] /= this->Stack[stackPosition];
          stackPosition--;
          }
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
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] = log(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_LOGARITHME:
        if (this->Stack[stackPosition]<=0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] = log(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_LOGARITHM10:
        if (this->Stack[stackPosition]<=0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] =
            log(this->Stack[stackPosition])/log(static_cast<double>(10));
          }
        break;
      case VTK_PARSER_SQUARE_ROOT:
        if (this->Stack[stackPosition] < 0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a square root of a negative value");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] = sqrt(this->Stack[stackPosition]);
          }
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
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take asin of a value < -1 or > 1");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] = asin(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_ARCCOSINE:
        if(this->Stack[stackPosition]<-1 || this->Stack[stackPosition]>1)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take acos of a value < -1 or > 1");
            return false;
            }
          }
        else
          {
          this->Stack[stackPosition] = acos(this->Stack[stackPosition]);
          }
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
      case VTK_PARSER_MIN:
        if (this->Stack[stackPosition] < this->Stack[stackPosition-1])
          {
          this->Stack[stackPosition-1] = this->Stack[stackPosition];
          }
        stackPosition--;
        break;
      case VTK_PARSER_MAX:
        if (this->Stack[stackPosition] > this->Stack[stackPosition-1])
          {
          this->Stack[stackPosition-1] = this->Stack[stackPosition];
          }
        stackPosition--;
        break;
      case VTK_PARSER_CROSS:
        // Cross Product
        #define Ux stackPosition-5
        #define Uy stackPosition-4
        #define Uz stackPosition-3
        #define Vx stackPosition-2
        #define Vy stackPosition-1
        #define Vz stackPosition
        temp[0] = this->Stack[Uy]*this->Stack[Vz] -
                  this->Stack[Uz]*this->Stack[Vy];
        temp[1] = this->Stack[Uz]*this->Stack[Vx] -
                  this->Stack[Ux]*this->Stack[Vz];
        temp[2] = this->Stack[Ux]*this->Stack[Vy] -
                  this->Stack[Uy]*this->Stack[Vx];
        this->Stack[Ux] = temp[0];
        this->Stack[Uy] = temp[1];
        this->Stack[Uz] = temp[2];
        #undef Ux
        #undef Uy
        #undef Uz
        #undef Vx
        #undef Vy
        #undef Vz
        stackPosition-=3;
        break;
      case VTK_PARSER_SIGN:
        if (this->Stack[stackPosition] < 0)
          {
          this->Stack[stackPosition] = -1;
          }
        else if (this->Stack[stackPosition] ==  0)
          {
          this->Stack[stackPosition] = 0;
          }
        else
          {
          this->Stack[stackPosition] = 1;
          }
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
      case VTK_PARSER_SCALAR_TIMES_VECTOR:
        this->Stack[stackPosition] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-1] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-2] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-3] = this->Stack[stackPosition-2];
        this->Stack[stackPosition-2] = this->Stack[stackPosition-1];
        this->Stack[stackPosition-1] = this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_VECTOR_TIMES_SCALAR:
        this->Stack[stackPosition-3] *= this->Stack[stackPosition];
        this->Stack[stackPosition-2] *= this->Stack[stackPosition];
        this->Stack[stackPosition-1] *= this->Stack[stackPosition];
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
        if (magnitude != 0)
          {
          this->Stack[stackPosition] /= magnitude;
          this->Stack[stackPosition-1] /= magnitude;
          this->Stack[stackPosition-2] /= magnitude;
          }
        break;
      case VTK_PARSER_IHAT:
        this->Stack[++stackPosition] = 1;
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 0;
        break;
      case VTK_PARSER_JHAT:
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 1;
        this->Stack[++stackPosition] = 0;
        break;
      case VTK_PARSER_KHAT:
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 1;
        break;
      case VTK_PARSER_LESS_THAN:
        this->Stack[stackPosition-1] = (this->Stack[stackPosition-1] < this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_GREATER_THAN:
        this->Stack[stackPosition-1] = (this->Stack[stackPosition-1] > this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_EQUAL_TO:
        this->Stack[stackPosition-1] = (this->Stack[stackPosition-1] == this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_AND:
        this->Stack[stackPosition-1] = (this->Stack[stackPosition-1] && this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_OR:
        this->Stack[stackPosition-1] = (this->Stack[stackPosition-1] || this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_IF:
        {
        // Stack[stackPosition]=Stack[2] refers to the bool
        // argument of if(bool,valtrue,valfalse). Stack[1] is valtrue, and
        // Stack[0] is valfalse.
        int result = stackPosition - 2;
        int valFalse = stackPosition - 2;
        int valTrue = stackPosition - 1;
        int boolArg = stackPosition;

        if (this->Stack[boolArg])
          this->Stack[result] = this->Stack[valTrue];
        else
          this->Stack[result] = this->Stack[valFalse];
        stackPosition -= 2;
        break;
        }
      case VTK_PARSER_VECTOR_IF:
        {
        int xResult = stackPosition - 6;
        int yResult = stackPosition - 5;
        int zResult = stackPosition - 4;
        int xValFalse = stackPosition - 6;
        int yValFalse = stackPosition - 5;
        int zValFalse = stackPosition - 4;
        int xValTrue = stackPosition - 3;
        int yValTrue = stackPosition - 2;
        int zValTrue = stackPosition - 1;
        int boolArg = stackPosition;

        if (this->Stack[boolArg])
          {
          this->Stack[xResult] = this->Stack[xValTrue];
          this->Stack[yResult] = this->Stack[yValTrue];
          this->Stack[zResult] = this->Stack[zValTrue];
          }
        else
          {
          this->Stack[xResult] = this->Stack[xValFalse];
          this->Stack[yResult] = this->Stack[yValFalse];
          this->Stack[zResult] = this->Stack[zValFalse];
          }
        stackPosition -= 4;
        break;
        }
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
  this->StackPointer = stackPosition;

  this->EvaluateMTime.Modified();

  return true;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::IsScalarResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() ||
      this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
    {
      if (this->Evaluate() == false)
        return 0;
    }
  return (this->StackPointer == 0);
}

//-----------------------------------------------------------------------------
double vtkFunctionParser::GetScalarResult()
{
  if (!(this->IsScalarResult()))
    {
    vtkErrorMacro("GetScalarResult: no valid scalar result");
    return VTK_PARSER_ERROR_RESULT;
    }
  return this->Stack[0];
}

int vtkFunctionParser::IsVectorResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() ||
      this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
    {
      if (this->Evaluate() == false)
        return 0;
    }
  return (this->StackPointer == 2);
}

//-----------------------------------------------------------------------------
double *vtkFunctionParser::GetVectorResult()
{
  if (!(this->IsVectorResult()))
    {
    vtkErrorMacro("GetVectorResult: no valid vector result");
    return vtkParserVectorErrorResult;
    }
  return this->Stack;
}

//-----------------------------------------------------------------------------
char* vtkFunctionParser::GetScalarVariableName(int i)
{
  if (i >= 0 && i < this->NumberOfScalarVariables)
    {
    return this->ScalarVariableNames[i];
    }
  return NULL;
}

//-----------------------------------------------------------------------------
char* vtkFunctionParser::GetVectorVariableName(int i)
{
  if (i >= 0 && i < this->NumberOfVectorVariables)
    {
    return this->VectorVariableNames[i];
    }
  return NULL;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
int vtkFunctionParser::IsElementaryOperator(int op)
{
  return strchr("+-.*/^", op) != NULL;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::SetScalarVariableValue(const char* inVariableName,
                                               double value)
{
  int i;
  double *tempValues;
  char** tempNames;
  char* variableName = this->RemoveSpacesFrom(inVariableName);

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strcmp(variableName, this->ScalarVariableNames[i]) == 0)
      {
      if (this->ScalarVariableValues[i] != value)
        {
        this->ScalarVariableValues[i] = value;
        this->VariableMTime.Modified();
        this->Modified();
        }
      delete [] variableName;
      return;
      }
    }

  tempValues = new double [this->NumberOfScalarVariables];
  tempNames = new char *[this->NumberOfScalarVariables];
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    tempValues[i] = this->ScalarVariableValues[i];
    tempNames[i] = new char [strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(tempNames[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    }

  if (this->ScalarVariableValues)
    {
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }

  this->ScalarVariableValues = new double [this->NumberOfScalarVariables + 1];
  this->ScalarVariableNames = new char *[this->NumberOfScalarVariables + 1];
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    this->ScalarVariableValues[i] = tempValues[i];
    this->ScalarVariableNames[i] = new char [strlen(tempNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], tempNames[i]);
    delete [] tempNames[i];
    tempNames[i] = NULL;
    }
  delete [] tempValues;
  delete [] tempNames;

  this->ScalarVariableValues[i] = value;
  this->ScalarVariableNames[i] = new char [strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[i], variableName);
  this->NumberOfScalarVariables++;

  this->VariableMTime.Modified();
  this->Modified();
  delete [] variableName;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::SetScalarVariableValue(int i, double value)
{
  if (i < 0 || i >= this->NumberOfScalarVariables)
    {
    return;
    }

  if (this->ScalarVariableValues[i] != value)
    {
    this->ScalarVariableValues[i] = value;
    this->VariableMTime.Modified();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
double vtkFunctionParser::GetScalarVariableValue(const char* inVariableName)
{
  int i;
  char* variableName = this->RemoveSpacesFrom(inVariableName);

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strcmp(variableName, this->ScalarVariableNames[i]) == 0)
      {
      delete [] variableName;
      return this->ScalarVariableValues[i];
      }
    }
  vtkErrorMacro("GetScalarVariableValue: scalar variable " << variableName
                << " does not exist");
  delete [] variableName;
  return VTK_PARSER_ERROR_RESULT;
}

//-----------------------------------------------------------------------------
double vtkFunctionParser::GetScalarVariableValue(int i)
{
  if (i < 0 || i >= this->NumberOfScalarVariables)
    {
    vtkErrorMacro("GetScalarVariableValue: scalar variable " << i
                  << " does not exist");
    return VTK_PARSER_ERROR_RESULT;
    }

  return this->ScalarVariableValues[i];
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::SetVectorVariableValue(const char* inVariableName,
                                               double xValue, double yValue,
                                               double zValue)
{
  int i;
  double **tempValues;
  char** tempNames;
  char* variableName = this->RemoveSpacesFrom(inVariableName);

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strcmp(variableName, this->VectorVariableNames[i]) == 0)
      {
      if (this->VectorVariableValues[i][0] != xValue ||
          this->VectorVariableValues[i][1] != yValue ||
          this->VectorVariableValues[i][2] != zValue)
        {
        this->VectorVariableValues[i][0] = xValue;
        this->VectorVariableValues[i][1] = yValue;
        this->VectorVariableValues[i][2] = zValue;
        this->VariableMTime.Modified();
        this->Modified();
        }
      delete [] variableName;
      return;
      }
    }

  tempValues = new double *[this->NumberOfVectorVariables];
  tempNames = new char *[this->NumberOfVectorVariables];
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    tempValues[i] = new double[3];
    tempValues[i][0] = this->VectorVariableValues[i][0];
    tempValues[i][1] = this->VectorVariableValues[i][1];
    tempValues[i][2] = this->VectorVariableValues[i][2];
    tempNames[i] = new char [strlen(this->VectorVariableNames[i]) + 1];
    strcpy(tempNames[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    delete [] this->VectorVariableValues[i];
    this->VectorVariableValues[i] = NULL;
    }

  if (this->VectorVariableValues)
    {
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }

  this->VectorVariableValues = new double *[this->NumberOfVectorVariables + 1];
  this->VectorVariableNames = new char *[this->NumberOfVectorVariables + 1];
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    this->VectorVariableValues[i] = new double[3];
    this->VectorVariableValues[i][0] = tempValues[i][0];
    this->VectorVariableValues[i][1] = tempValues[i][1];
    this->VectorVariableValues[i][2] = tempValues[i][2];
    this->VectorVariableNames[i] = new char [strlen(tempNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], tempNames[i]);
    delete [] tempNames[i];
    tempNames[i] = NULL;
    delete [] tempValues[i];
    tempValues[i] = NULL;
    }
  delete [] tempValues;
  delete [] tempNames;

  this->VectorVariableValues[i] = new double[3];
  this->VectorVariableValues[i][0] = xValue;
  this->VectorVariableValues[i][1] = yValue;
  this->VectorVariableValues[i][2] = zValue;
  this->VectorVariableNames[i] = new char [strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[i], variableName);
  this->NumberOfVectorVariables++;

  this->VariableMTime.Modified();
  this->Modified();
  delete [] variableName;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::SetVectorVariableValue(int i, double xValue,
                                               double yValue, double zValue)
{
  if (i < 0 || i >= this->NumberOfVectorVariables)
    {
    return;
    }
  if (this->VectorVariableValues[i][0] != xValue ||
      this->VectorVariableValues[i][1] != yValue ||
      this->VectorVariableValues[i][2] != zValue)
    {
    this->VectorVariableValues[i][0] = xValue;
    this->VectorVariableValues[i][1] = yValue;
    this->VectorVariableValues[i][2] = zValue;
    this->VariableMTime.Modified();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
double* vtkFunctionParser::GetVectorVariableValue(const char* inVariableName)
{
  int i;
  char* variableName = this->RemoveSpacesFrom(inVariableName);

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strcmp(variableName, this->VectorVariableNames[i]) == 0)
      {
      delete [] variableName;
      return this->VectorVariableValues[i];
      }
    }
  vtkErrorMacro("GetVectorVariableValue: vector variable " << variableName
                << " does not exist");
  delete [] variableName;
  return vtkParserVectorErrorResult;
}

//-----------------------------------------------------------------------------
double* vtkFunctionParser::GetVectorVariableValue(int i)
{
  if (i < 0 || i >= this->NumberOfVectorVariables)
    {
    vtkErrorMacro("GetVectorVariableValue: vector variable " << i
                  << " does not exist");
    return vtkParserVectorErrorResult;
    }
  return this->VectorVariableValues[i];
}

//-----------------------------------------------------------------------------
char* vtkFunctionParser::RemoveSpacesFrom(const char* variableName)
{
  int len = static_cast<int>(strlen(variableName));
  int i;
  char* resultString = new char[len+1];
  char* out = resultString;
  for(i=0; i < len; ++i)
    {
    if(variableName[i] != ' ')
      {
      *out++ = variableName[i];
      }
    }
  *out = '\0';
  return resultString;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::RemoveSpaces()
{
  char *tempString;
  int i, length;

  this->FunctionLength = 0;
  length = static_cast<int>(strlen(this->Function));

  tempString = new char[length+1];
  for (i = 0; i < length; i++)
    {
    if (!isspace(this->Function[i]))
      {
      tempString[this->FunctionLength] = this->Function[i];
      this->FunctionLength++;
      }
    }

  delete [] this->Function;
  this->Function = new char[this->FunctionLength+1];
  strncpy(this->Function, tempString,
          static_cast<size_t>(this->FunctionLength));
  this->Function[this->FunctionLength] = '\0';
  delete [] tempString;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::OperatorWithinVariable(int idx)
{
  int i;
  char *tmpString = NULL;
  int start, end;

  for ( i = 0;  i < this->NumberOfScalarVariables;  i ++ )
    {
    end = 0;

    if (  strchr( this->ScalarVariableNames[i], this->Function[idx] ) != 0  )
      {
      if (    (  tmpString = strstr( this->Function, this->ScalarVariableNames[i] )  )    )
        {
        do
          {
          if (tmpString)
            {
            start = static_cast<int>(tmpString - this->Function);
            end   = start + static_cast<int>( strlen(this->ScalarVariableNames[i]) );

            // the variable being investigated does contain an operator (at idx)
            if ( start <= idx && idx <= end )  return  1;

            // just in case of one or even more occurrences of the
            // variable name (being investigated) preceding "idx" in this->Function[]
            // As suggested by 7islands, a greedy search is used here
            if ( end <= idx )  // to save strstr()  whenever possible
            tmpString = strstr( this->Function + end, this->ScalarVariableNames[i] );
            }
          else  break;
          } while ( end <= idx );
        }
      }
    }

  for ( i = 0;  i < this->NumberOfVectorVariables;  i ++ )
    {
    end = 0;

    if (  strchr( this->VectorVariableNames[i], this->Function[idx] ) != 0  )
      {
      if (    (  tmpString = strstr( this->Function, this->VectorVariableNames[i] )  )    )
        {
        do
          {
          if (tmpString)
            {
            start = static_cast<int>(tmpString - this->Function);
            end   = start + static_cast<int>( strlen(this->VectorVariableNames[i]) );

            // the variable being investigated does contain an operator (at idx)
            if ( start <= idx && idx <= end )  return  1;

            // just in case of one or even more occurrences of the
            // variable name (being investigated) preceding "idx" in this->Function[]
            // As suggested by 7islands, a greedy search is used here
            if ( end <= idx )  // to save strstr()  whenever possible
            tmpString = strstr( this->Function + end, this->VectorVariableNames[i] );
            }
          else  break;
          } while ( end <= idx );
        }
      }
    }

  tmpString = NULL;
  return  0;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::CheckSyntax()
{
  int     pos = -1;
  char*   error = NULL;

  this->CheckExpression(pos, &error);

  if(pos != -1 || error)
    {
    vtkErrorMacro(<< error << "; " << " see position " << pos);
    return 0;
    }
  else
    {
    return 1;
    }
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::CopyParseError(int &position, char **error)
{
  if(!error)
    {
    return;
    }

  position = this->ParseErrorPositon;
  *error   = this->ParseError;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkFunctionParser::BuildInternalSubstringStructure(int beginIndex,
                                                        int endIndex)
{
  int mathFunctionNum, beginIndex2;
  int opNum, parenthesisCount, i;
  // in order of reverse precedence
  static const char* const elementaryMathOps = "|&=<>+-.*/^";

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
    if (this->GetMathConstantNumber(beginIndex+1) > 0 &&
        this->FindEndOfMathConstant(beginIndex+1) == endIndex)
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
      while (this->Function[beginIndex2] != '(' && beginIndex2 <= endIndex)
        {
        beginIndex2++;
        }
      if (this->IsSubstringCompletelyEnclosed(beginIndex2, endIndex))
        {
        if ((mathFunctionNum == VTK_PARSER_MIN) ||
            (mathFunctionNum == VTK_PARSER_MAX) ||
            (mathFunctionNum == VTK_PARSER_CROSS))
          {
          parenthesisCount = 0;
          for (i = endIndex-1; i > beginIndex2; i--)
            {
            if (this->Function[i] == ')')
              {
              parenthesisCount++;
              }
            else if (this->Function[i] == '(')
              {
              parenthesisCount--;
              }
            if (parenthesisCount == 0 && this->Function[i] == ',')
              {
              this->BuildInternalSubstringStructure(beginIndex2+1, i-1);
              this->BuildInternalSubstringStructure(i+1, endIndex-1);
              this->AddInternalByte(
                static_cast<unsigned char>(mathFunctionNum));
              this->StackPointer--;
              return;
              }
            } // for (i = endIndex-1; i > beginIndex2; i--)
          } // VTK_PARSER_MIN, ...

        if (mathFunctionNum == VTK_PARSER_IF)
          {
          // if(bool, valtrue, valfalse)
          int numCommas = 0;
          int secondCommaIndex = endIndex;
          parenthesisCount = 0;
          for (i = endIndex-1; i > beginIndex2; i--)
            {
            if (this->Function[i] == ')')
              {
              parenthesisCount++;
              }
            else if (this->Function[i] == '(')
              {
              parenthesisCount--;
              }
            if (parenthesisCount == 0 && this->Function[i] == ',')
              {
              numCommas++;
              if (numCommas == 1)
                {
                // third arg
                secondCommaIndex = i;
                this->BuildInternalSubstringStructure(i+1, endIndex-1);
                }
              else // numCommas == 2
                {
                // second arg
                this->BuildInternalSubstringStructure(i+1, secondCommaIndex-1);
                // first arg
                this->BuildInternalSubstringStructure(beginIndex2+1, i-1);
                this->AddInternalByte(
                  static_cast<unsigned char>(mathFunctionNum));
                this->StackPointer--;
                return;
                }
              } // if (parenthesisCount == 0 ... )
            } // for (i = endIndex-1; i > beginIndex2; i--)
          } // VTK_PARSER_IF, ...

        this->BuildInternalSubstringStructure(beginIndex2+1, endIndex-1);
        this->AddInternalByte(static_cast<unsigned char>(mathFunctionNum));
        return;
        } // if (this->IsSubstringCompletelyEnclosed ... )
      } // if (mathFunctionNum > 0)
    } // if (isalpha(this->Function[beginIndex]))

  int numMathOps = static_cast<int>(strlen(elementaryMathOps));
  for (opNum = 0; opNum < numMathOps; opNum++)
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
          // arithmetic or boolean
          this->Function[i] == elementaryMathOps[opNum] &&
          !(this->Function[i] == '-' &&
            (this->IsElementaryOperator(this->Function[i-1]) ||
             this->Function[i-1] == '(' ||
             (this->Function[i-1] == 'e' && i > 1 &&
              isdigit(this->Function[i-2])))) &&
          !(this->Function[i] == '.' &&
            (i+1 < this->FunctionLength) &&
             (isdigit(this->Function[i+1]))) &&
          !this->OperatorWithinVariable(i))
        {
        this->BuildInternalSubstringStructure(beginIndex, i-1);
        this->BuildInternalSubstringStructure(i+1, endIndex);
        this->AddInternalByte(
          this->GetElementaryOperatorNumber(elementaryMathOps[opNum]));
        this->StackPointer--;
        return;
        }
      } // end of   for (i = endIndex; i > beginIndex; i--)
    } // end of   for (opNum = 0; opNum < numMathOps; opNum++)

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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetMathFunctionNumber(int currentIndex)
{
  // For addition of any new math function, please update
  // function GetMathFunctionNumberByCheckingParenthesis()

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
  if (strncmp(&this->Function[currentIndex], "ln", 2) == 0)
    {
    return VTK_PARSER_LOGARITHME;
    }
  if (strncmp(&this->Function[currentIndex], "log10", 5) == 0)
    {
    return VTK_PARSER_LOGARITHM10;
    }
  if (strncmp(&this->Function[currentIndex], "log", 3) == 0)
    {
    vtkErrorMacro("The use of log function is being deprecated. "
                  "Please use log10 or ln instead");
    return VTK_PARSER_LOGARITHM;
    }
  if (strncmp(&this->Function[currentIndex], "sqrt", 4) == 0)
    {
    return VTK_PARSER_SQUARE_ROOT;
    }
  if (strncmp(&this->Function[currentIndex], "sin", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "sinh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_SINE;
      }
    return VTK_PARSER_SINE;
    }
  if (strncmp(&this->Function[currentIndex], "cos", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "cosh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_COSINE;
      }
    return VTK_PARSER_COSINE;
    }
  if (strncmp(&this->Function[currentIndex], "tan", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "tanh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_TANGENT;
      }
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
  if (strncmp(&this->Function[currentIndex], "min", 3) == 0)
    {
    return VTK_PARSER_MIN;
    }
  if (strncmp(&this->Function[currentIndex], "max", 3) == 0)
    {
    return VTK_PARSER_MAX;
    }
  if (strncmp(&this->Function[currentIndex], "cross", 5) == 0)
    {
    return VTK_PARSER_CROSS;
    }
  if (strncmp(&this->Function[currentIndex], "sign", 4) == 0)
    {
    return VTK_PARSER_SIGN;
    }
  if (strncmp(&this->Function[currentIndex], "mag", 3) == 0)
    {
    return VTK_PARSER_MAGNITUDE;
    }
  if (strncmp(&this->Function[currentIndex], "norm", 4) == 0)
    {
    return VTK_PARSER_NORMALIZE;
    }
  if (strncmp(&this->Function[currentIndex], "if", 2) == 0)
    {
    return VTK_PARSER_IF;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetMathFunctionNumberByCheckingParenthesis
  ( int currentIndex )
{
  // This function assumes that RemoveSpaces() has been called and
  // hence involves the check on the '(' that immediately follows a
  // valid function. Addressing '(' here instead of in CheckSyntax()
  // allows for early detection of grammar errors, i.e., lack of '(',
  // and hence simplifies the parsing process.

  // For addition of any new math function, please update NUMBFUNCS
  // and add an entry to each of the three arrays below.

  const  int  NUMBFUNCS                =  24;

  static int  charsLens[NUMBFUNCS]     = { 4,       4,        5,       6,
                                           3,       6,        4,       5,
                                           4,       5,        4,       5,
                                           4,       5,        5,       5,
                                           5,       4,        4,       6,
                                           5,       4,        5,       3
                                         };

  static int  funcNumbs[NUMBFUNCS]     = { VTK_PARSER_ABSOLUTE_VALUE,
                                           VTK_PARSER_EXPONENT,
                                           VTK_PARSER_CEILING,
                                           VTK_PARSER_FLOOR,

                                           VTK_PARSER_LOGARITHME,
                                           VTK_PARSER_LOGARITHM10,
                                           VTK_PARSER_LOGARITHM,
                                           VTK_PARSER_SQUARE_ROOT,

                                           VTK_PARSER_SINE,
                                           VTK_PARSER_HYPERBOLIC_SINE,
                                           VTK_PARSER_COSINE,
                                           VTK_PARSER_HYPERBOLIC_COSINE,

                                           VTK_PARSER_TANGENT,
                                           VTK_PARSER_HYPERBOLIC_TANGENT,
                                           VTK_PARSER_ARCSINE,
                                           VTK_PARSER_ARCCOSINE,

                                           VTK_PARSER_ARCTANGENT,
                                           VTK_PARSER_MIN,
                                           VTK_PARSER_MAX,
                                           VTK_PARSER_CROSS,

                                           VTK_PARSER_SIGN,
                                           VTK_PARSER_MAGNITUDE,
                                           VTK_PARSER_NORMALIZE,
                                           VTK_PARSER_IF
                                         };

  static char funcNames[NUMBFUNCS][10] = { "abs(",  "exp(",   "ceil(", "floor(",
                                           "ln(",   "log10(", "log(",  "sqrt(",
                                           "sin(",  "sinh(",  "cos(",  "cosh(",
                                           "tan(",  "tanh(",  "asin(", "acos(",
                                           "atan(", "min(",   "max(",  "cross(",
                                           "sign(", "mag(",   "norm(", "if("
                                         };

  int   isMatched = 0;
  int   retNumber = 0;
  for ( int i = 0; i < NUMBFUNCS && isMatched == 0; i ++ )
    {
    isMatched = (  strncmp( this->Function + currentIndex,
                            funcNames[i], charsLens[i]
                          ) == 0
                )  ?  1  :  0;
    retNumber = isMatched * funcNumbs[i];
    }

  return retNumber;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetMathFunctionStringLength(int mathFunctionNumber)
{
  switch (mathFunctionNumber)
    {
    case VTK_PARSER_LOGARITHME:
    case VTK_PARSER_IF:
      return 2;
    case VTK_PARSER_ABSOLUTE_VALUE:
    case VTK_PARSER_EXPONENT:
    case VTK_PARSER_LOGARITHM:
    case VTK_PARSER_SINE:
    case VTK_PARSER_COSINE:
    case VTK_PARSER_TANGENT:
    case VTK_PARSER_MAGNITUDE:
    case VTK_PARSER_MIN:
    case VTK_PARSER_MAX:
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
    case VTK_PARSER_SIGN:
      return 4;
    case VTK_PARSER_FLOOR:
    case VTK_PARSER_LOGARITHM10:
    case VTK_PARSER_CROSS:
      return 5;
    default:
      vtkWarningMacro("Unknown math function");
      return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetMathConstantNumber(int currentIndex)
{
  if (strncmp(&this->Function[currentIndex], "iHat", 4) == 0)
    {
    return VTK_PARSER_IHAT;
    }
  if (strncmp(&this->Function[currentIndex], "jHat", 4) == 0)
    {
    return VTK_PARSER_JHAT;
    }
  if (strncmp(&this->Function[currentIndex], "kHat", 4) == 0)
    {
    return VTK_PARSER_KHAT;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetMathConstantStringLength(int mathConstantNumber)
{
  switch (mathConstantNumber)
    {
    case VTK_PARSER_IHAT:
    case VTK_PARSER_JHAT:
    case VTK_PARSER_KHAT:
      return 4;
    default:
      vtkWarningMacro("Unknown math constant");
      return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::GetVariableNameLength(int variableNumber)
{
  if (variableNumber < this->NumberOfScalarVariables)
    {
    return static_cast<int>(strlen(this->ScalarVariableNames[variableNumber]));
    }
  else
    {
    return static_cast<int>(strlen(
      this->VectorVariableNames[variableNumber -
                               this->NumberOfScalarVariables]));
    }
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::FindEndOfMathFunction(int beginIndex)
{
  int i = beginIndex, parenthesisCount;

  while (this->Function[i] != '(' )
    {
    i++;
    }
  i++;
  for (parenthesisCount = 1; parenthesisCount > 0; ++i)
    {
    parenthesisCount += (this->Function[i] == '(' ? 1 :
                         (this->Function[i] == ')' ? -1 : 0));
    }
  return i - 1;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::FindEndOfMathConstant(int beginIndex)
{
  if(int constantNumber = this->GetMathConstantNumber(beginIndex))
    {
    return beginIndex+this->GetMathConstantStringLength(constantNumber);
    }
  return beginIndex;
}

//-----------------------------------------------------------------------------
unsigned char vtkFunctionParser::GetElementaryOperatorNumber(char op)
{
  static const char* const operators = "+-*/^";
  int i;

  if (op == '<')
    {
    return VTK_PARSER_LESS_THAN;
    }
  if (op == '>')
    {
    return VTK_PARSER_GREATER_THAN;
    }
  if (op == '=')
    {
    return VTK_PARSER_EQUAL_TO;
    }
  if (op == '&')
    {
    return VTK_PARSER_AND;
    }
  if (op == '|')
    {
    return VTK_PARSER_OR;
    }

  for(i = 0; i < 5; i++)
    {
    if (operators[i] == op)
      {
      return static_cast<unsigned char>(VTK_PARSER_ADD + i);
      }
    }
  if (op == '.')
    {
    return VTK_PARSER_DOT_PRODUCT;
    }

  return 0;
}

//-----------------------------------------------------------------------------
unsigned char vtkFunctionParser::GetOperandNumber(int currentIndex)
{
  int i, variableIndex = -1;

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

  if (!strncmp(&this->Function[currentIndex], "iHat", 4))
    {
    return VTK_PARSER_IHAT;
    }
  if (!strncmp(&this->Function[currentIndex], "jHat", 4))
    {
    return VTK_PARSER_JHAT;
    }
  if (!strncmp(&this->Function[currentIndex], "kHat", 4))
    {
    return VTK_PARSER_KHAT;
    }


  bool scalarVar = false;
  size_t currentLen = 0;
  //Bug 7396. If a scalar variable name is a subset of a vector var name it will
  //casue the scripting to crash. So instead of ending once we find a var name that matches in scalars
  //we will also check vectors
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->ScalarVariableNames[i],
                strlen(this->ScalarVariableNames[i])) == 0)
      {
      if (variableIndex == -1 ||
          strlen(this->ScalarVariableNames[i]) > currentLen )
        {
        currentLen = strlen(this->ScalarVariableNames[i]);
        variableIndex = i;
        }
      }
    }
  if (variableIndex >= 0)
    {
    scalarVar = true;
    }

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->VectorVariableNames[i],
                strlen(this->VectorVariableNames[i])) == 0)
      {
      if (variableIndex == -1
        || strlen(this->VectorVariableNames[i]) > currentLen )
        {
        scalarVar = false;
        currentLen = strlen(this->VectorVariableNames[i]);
        variableIndex = i;
        }
      }
    }
  if (variableIndex >= 0)
    {
    //add the offset if vector
    variableIndex = (scalarVar)?
      variableIndex:this->NumberOfScalarVariables + variableIndex;
    return static_cast<unsigned char>(
      VTK_PARSER_BEGIN_VARIABLES + variableIndex);
    }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::RemoveScalarVariables()
{
  int i;

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    }
  if (this->NumberOfScalarVariables > 0)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }
  this->NumberOfScalarVariables = 0;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::RemoveVectorVariables()
{
  int i;

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    }
  if (this->NumberOfVectorVariables > 0)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }
  this->NumberOfVectorVariables = 0;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::CheckExpression(int &pos, char **error)
{
  if(this->FunctionMTime.GetMTime() > this->CheckMTime.GetMTime())
    {
    // Need to parse again.

    // Reset previous error cache.
    this->ParseErrorPositon = -1;
    this->ParseError        = NULL;

    this->CopyParseError(pos, error);
    }
  else
    {
    this->CopyParseError(pos, error);
    return;
    }

  this->CheckMTime.Modified();

  this->RemoveSpaces();

  int index = 0, parenthesisCount = 0, currentChar;
  char* ptr;
  int functionNumber, constantNumber;
  int *expectCommaOnParenthesisCount = new int[this->FunctionLength];
  int *expectTwoCommasOnParenthesisCount = new int[this->FunctionLength];
  int i;

  for (i = 0; i < this->FunctionLength; i++)
    {
    expectCommaOnParenthesisCount[i] = 0;
    expectTwoCommasOnParenthesisCount[i] = 0;
    }

  while (1)
    {
    currentChar = this->Function[index];
    bool breakToOuterLoop = false;

    // Check for valid operand (must appear)

    // Check for leading -
    if (currentChar == '-')
      {
      currentChar = this->Function[++index];
      if(index == this->FunctionLength)
        {
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax error: unary minus with no operand");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }
      }

    // Check for math function
    if ((functionNumber = this->GetMathFunctionNumberByCheckingParenthesis(index)))
      {
      if ((functionNumber == VTK_PARSER_MIN) ||
          (functionNumber == VTK_PARSER_MAX) ||
          (functionNumber == VTK_PARSER_CROSS))
        {
        expectCommaOnParenthesisCount[parenthesisCount+1] = 1;
        }
      if (functionNumber == VTK_PARSER_IF)
        {
        expectTwoCommasOnParenthesisCount[parenthesisCount+1] = 1;
        }
      index += this->GetMathFunctionStringLength(functionNumber);
      currentChar = this->Function[index];

      // == currentChar should always be '(' here == a fix to Bug #9208
      // since GetMathFunctionNumberByCheckingParenthesis() is employed above

      //if ( currentChar != '(' )
      //  {
      //  vtkErrorMacro("Syntax error: input to math function not in "
      //                << "parentheses; see position " << index);
      //  delete [] expectCommaOnParenthesisCount;
      //  delete [] expectTwoCommasOnParenthesisCount;
      //  return 0;
      //  }
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
      double value=strtod(&this->Function[index], &ptr);
      // ignore the return value, we just try to figure out
      // the position of the pointer after the double value.
      static_cast<void>(value);
      index += int(ptr-&this->Function[index]);
      currentChar = this->Function[index];
      }
    // Check for named constant
    else if ((constantNumber = this->GetMathConstantNumber(index)))
      {
      index += this->GetMathConstantStringLength(constantNumber);
      currentChar = this->Function[index];
      }
    // End paraenthesis should indicate that the next character might be a
    // comma. This is a hack because the while (currentChar == ') below checks
    // for an incorrect number of commas.
    else if (currentChar == ')')
      {
      ++index;
      currentChar = this->Function[index];
      }
    else
      { // Check for variable
      if (!this->IsVariableName(index))
        {
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax error: expecting a variable name");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }
      index += this->GetVariableNameLength(this->GetOperandNumber(index) -
                                           VTK_PARSER_BEGIN_VARIABLES);
      currentChar = this->Function[index];
      }

    // Check for possible second number from min or max function
    if (expectCommaOnParenthesisCount[parenthesisCount] > 0)
      {
      // Check for comma
      if (currentChar == ',')
        {
        expectCommaOnParenthesisCount[parenthesisCount] += 1;
        index++;
        continue;
        }
      }

    // Check for possible second or third number from if function
    if (expectTwoCommasOnParenthesisCount[parenthesisCount] > 0)
      {
      // Check for comma
      if (currentChar == ',')
        {
        expectTwoCommasOnParenthesisCount[parenthesisCount] += 1;
        index++;
        continue;
        }
      }

    // Check for closing parenthesis
    while ( currentChar == ')' )
      {
      if (expectCommaOnParenthesisCount[parenthesisCount] != 0 &&
            expectCommaOnParenthesisCount[parenthesisCount] != 2)
        {
        // We can't be closing this function if
        // expectCommaOnParenthesisCount[..] is not 2; either it was always
        // 0 or it should have been incremented to 2.
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax Error: two parameters separated by commas expected");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }
      if (expectTwoCommasOnParenthesisCount[parenthesisCount] != 0 &&
          expectTwoCommasOnParenthesisCount[parenthesisCount] != 3)
        {
        // We can't be closing this function if
        // expectCommaOnParenthesisCount[..] is not 3; either it was always
        // 0 or it should have been incremented to 3.
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax Error: three parameters separated by commas expected");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }
      parenthesisCount--;
      if(parenthesisCount < 0)
        {
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax Error: mismatched parenthesis");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }
      if( this->Function[index - 1] == '(' )
        {
        this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
        this->SetParseError("Syntax Error: empty parentheses");
        this->CopyParseError(pos, error);
        delete [] expectCommaOnParenthesisCount;
        delete [] expectTwoCommasOnParenthesisCount;
        return;
        }

      // Check for possible argument in a multiple argument function. In this
      // case the next character might be a comman, so break out to the outer
      // loop before incrementing index.
      if ((expectCommaOnParenthesisCount[parenthesisCount] > 0 &&
           expectCommaOnParenthesisCount[parenthesisCount] < 2) ||
          (expectTwoCommasOnParenthesisCount[parenthesisCount] > 0 &&
           expectTwoCommasOnParenthesisCount[parenthesisCount] < 3))
        {
        breakToOuterLoop = true;
        break;
        }

      currentChar = this->Function[++index];
      } // while ( currentChar == ')' )

    // If necessary, break out to the outer loop.
    if (breakToOuterLoop == true)
      {
      continue;
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
    if(!this->IsElementaryOperator(currentChar) &&
       currentChar != '<' &&
       currentChar != '>' &&
       currentChar != '=' &&
       currentChar != '&' &&
       currentChar != '|' &&
       currentChar != ',')
      {
      this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
      this->SetParseError("Syntax error: operator expected");
      this->CopyParseError(pos, error);
      delete [] expectCommaOnParenthesisCount;
      delete [] expectTwoCommasOnParenthesisCount;
      return;
      }

    if (currentChar != ',')
      {
      // If we get here, we have an operand and an operator; the next loop will
      // check for another operand (must appear)
      index++;
      }
    } // while(1)

  // Check that all opened parentheses are also closed
  if(parenthesisCount > 0)
    {
    this->ParseErrorPositon = this->FindPositionInOriginalFunction(index);
    this->SetParseError("Syntax Error: missing closing parenthesis");
    this->CopyParseError(pos, error);
    delete [] expectCommaOnParenthesisCount;
    delete [] expectTwoCommasOnParenthesisCount;
    return;
    }


  // The string is ok
  delete [] expectCommaOnParenthesisCount;
  delete [] expectTwoCommasOnParenthesisCount;

  return;
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
}

//-----------------------------------------------------------------------------
void vtkFunctionParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int i;

  os << indent << "Function: "
     << (this->Function ? this->Function : "(none)") << endl;

  os << indent << "FunctionWithSpaces: "
     << (this->FunctionWithSpaces ? this->FunctionWithSpaces : "(none)") << endl;

  os << indent << "NumberOfScalarVariables: "
     << this->NumberOfScalarVariables << endl;

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    os << indent << "  " << this->GetScalarVariableName(i) << ": "
       << this->GetScalarVariableValue(i) << endl;
    }

  os << indent << "NumberOfVectorVariables: "
     << this->NumberOfVectorVariables << endl;

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    os << indent << "  " << this->GetVectorVariableName(i) << ": ("
       << this->GetVectorVariableValue(i)[0] << ", "
       << this->GetVectorVariableValue(i)[1] << ", "
       << this->GetVectorVariableValue(i)[2] << ")" << endl;
    }

  if (this->EvaluateMTime.GetMTime() > this->FunctionMTime.GetMTime() &&
      this->EvaluateMTime.GetMTime() > this->VariableMTime.GetMTime() &&
      (this->StackPointer == 0 || this->StackPointer == 2))
    {
    if (this->StackPointer == 0)
      {
      os << indent << "ScalarResult: " << this->GetScalarResult() << endl;
      os << indent << "VectorResult: " << "(none)" << endl;
      }
    else if (this->StackPointer == 2)
      {
      os << indent << "ScalarResult: " << "(none)" << endl;
      os << indent << "VectorResult: " << "("
       << this->GetVectorResult()[0] << ", "
       << this->GetVectorResult()[1] << ", "
       << this->GetVectorResult()[2] << ")" << endl;
      }
    }
  else
    {
    os << indent << "ScalarResult: " << "(none)" << endl;
    os << indent << "VectorResult: " << "(none)" << endl;
    }

  os << indent << "Replace Invalid Values: "
     << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;

  os << indent << "Parse Error Position: " << this->ParseErrorPositon << endl;

  os << indent << "Parse Error: " << (this->ParseError ? this->ParseError : "NULL")
    << endl;
}

//-----------------------------------------------------------------------------
int vtkFunctionParser::FindPositionInOriginalFunction(const int &pos)
{
  // Copy the value.
  int origPos = pos;

  if(this->Function && this->FunctionWithSpaces)
    {
    size_t withSpacesLen    = strlen(this->FunctionWithSpaces);
    size_t withoutSpacesLen = strlen(this->Function);

    int counter = 0;
    for(size_t i=0; i < withSpacesLen; ++i)
      {
      // If we have covered all the characters excluding the spaces.
      if(counter == static_cast<int>(withoutSpacesLen) || counter == pos)
        {
        return origPos;
        }

      char currentChar = this->FunctionWithSpaces[i];
      if(currentChar == ' ')
        {
        // Every time we hit a whitespace increment the origPos
        // as the pos is counted without spaces.
        ++origPos;
        continue;
        }

      // This needs to be incremented for all the characters except
      // spaces.
      ++counter;
      }
    }

  return origPos;
}
