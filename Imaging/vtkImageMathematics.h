/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageMathematics - Add, subtract, multiply, divide, invert, sin, cos, exp, log.
// .SECTION Description
// vtkImageMathematics implements basic mathematic operations SetOperation is
// used to select the filters behavior.  The filter can take two or one
// input.


#ifndef __vtkImageMathematics_h
#define __vtkImageMathematics_h


// Operation options.
#define VTK_ADD                0
#define VTK_SUBTRACT           1
#define VTK_MULTIPLY           2
#define VTK_DIVIDE             3
#define VTK_INVERT             4  
#define VTK_SIN                5
#define VTK_COS                6
#define VTK_EXP                7
#define VTK_LOG                8
#define VTK_ABS                9
#define VTK_SQR               10
#define VTK_SQRT              11
#define VTK_MIN               12
#define VTK_MAX               13
#define VTK_ATAN              14
#define VTK_ATAN2             15
#define VTK_MULTIPLYBYK       16
#define VTK_ADDC              17
#define VTK_CONJUGATE         18
#define VTK_COMPLEX_MULTIPLY  19
#define VTK_REPLACECBYK       20

#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageMathematics : public vtkImageTwoInputFilter
{
public:
  static vtkImageMathematics *New();
  vtkTypeMacro(vtkImageMathematics,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Operation to perform.
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);
  void SetOperationToAdd() {this->SetOperation(VTK_ADD);};
  void SetOperationToSubtract() {this->SetOperation(VTK_SUBTRACT);};
  void SetOperationToMultiply() {this->SetOperation(VTK_MULTIPLY);};
  void SetOperationToDivide() {this->SetOperation(VTK_DIVIDE);};
  void SetOperationToConjugate() {this->SetOperation(VTK_CONJUGATE);};
  void SetOperationToComplexMultiply() 
    {this->SetOperation(VTK_COMPLEX_MULTIPLY);};

  void SetOperationToInvert() {this->SetOperation(VTK_INVERT);};
  void SetOperationToSin() {this->SetOperation(VTK_SIN);};
  void SetOperationToCos() {this->SetOperation(VTK_COS);};
  void SetOperationToExp() {this->SetOperation(VTK_EXP);};
  void SetOperationToLog() {this->SetOperation(VTK_LOG);};
  void SetOperationToAbsoluteValue() {this->SetOperation(VTK_ABS);};
  void SetOperationToSquare() {this->SetOperation(VTK_SQR);};
  void SetOperationToSquareRoot() {this->SetOperation(VTK_SQRT);};
  void SetOperationToMin() {this->SetOperation(VTK_MIN);};
  void SetOperationToMax() {this->SetOperation(VTK_MAX);};

  void SetOperationToATAN() {this->SetOperation(VTK_ATAN);};
  void SetOperationToATAN2() {this->SetOperation(VTK_ATAN2);};
  void SetOperationToMultiplyByK() {this->SetOperation(VTK_MULTIPLYBYK);};
  void SetOperationToAddConstant() {this->SetOperation(VTK_ADDC);};
  void SetOperationToReplaceCByK() {this->SetOperation(VTK_REPLACECBYK);};
  vtkSetMacro(ConstantK,double);
  vtkGetMacro(ConstantK,double);
  vtkSetMacro(ConstantC,double);
  vtkGetMacro(ConstantC,double);

protected:
  vtkImageMathematics();
  ~vtkImageMathematics() {};
  vtkImageMathematics(const vtkImageMathematics&);
  void operator=(const vtkImageMathematics&);

  int Operation;
  double ConstantK;
  double ConstantC;
  
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
};

#endif













