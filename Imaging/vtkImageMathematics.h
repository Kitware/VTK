/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
  vtkTypeRevisionMacro(vtkImageMathematics,vtkImageTwoInputFilter);
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

  // How to handle divide by zero
  vtkSetMacro(DivideByZeroToC,int);
  vtkGetMacro(DivideByZeroToC,int);
  vtkBooleanMacro(DivideByZeroToC,int);

protected:
  vtkImageMathematics();
  ~vtkImageMathematics() {};

  int Operation;
  double ConstantK;
  double ConstantC;
  int DivideByZeroToC;
  
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageMathematics(const vtkImageMathematics&);  // Not implemented.
  void operator=(const vtkImageMathematics&);  // Not implemented.
};

#endif













