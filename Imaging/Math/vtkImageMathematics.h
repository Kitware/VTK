/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMathematics - Add, subtract, multiply, divide, invert, sin,
// cos, exp, log.
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

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageMathematics : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMathematics *New();
  vtkTypeMacro(vtkImageMathematics,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Operation to perform.
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);

  // Description:
  // Set each pixel in the output image to the sum of the corresponding pixels
  // in Input1 and Input2.
  void SetOperationToAdd() {this->SetOperation(VTK_ADD);};

  // Description:
  // Set each pixel in the output image to the difference of the corresponding pixels
  // in Input1 and Input2 (output = Input1 - Input2).
  void SetOperationToSubtract() {this->SetOperation(VTK_SUBTRACT);};

  // Description:
  // Set each pixel in the output image to the product of the corresponding pixels
  // in Input1 and Input2.
  void SetOperationToMultiply() {this->SetOperation(VTK_MULTIPLY);};

  // Description:
  // Set each pixel in the output image to the quotient of the corresponding pixels
  // in Input1 and Input2 (Output = Input1 / Input2).
  void SetOperationToDivide() {this->SetOperation(VTK_DIVIDE);};

  void SetOperationToConjugate() {this->SetOperation(VTK_CONJUGATE);};

  void SetOperationToComplexMultiply()
    {this->SetOperation(VTK_COMPLEX_MULTIPLY);};

  // Description:
  // Set each pixel in the output image to 1 over the corresponding pixel
  // in Input1 and Input2 (output = 1 / Input1). Input2 is not used.
  void SetOperationToInvert() {this->SetOperation(VTK_INVERT);};

  // Description:
  // Set each pixel in the output image to the sine of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToSin() {this->SetOperation(VTK_SIN);};

  // Description:
  // Set each pixel in the output image to the cosine of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToCos() {this->SetOperation(VTK_COS);};

  // Description:
  // Set each pixel in the output image to the exponential of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToExp() {this->SetOperation(VTK_EXP);};

  // Description:
  // Set each pixel in the output image to the log of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToLog() {this->SetOperation(VTK_LOG);};

  // Description:
  // Set each pixel in the output image to the absolute value of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToAbsoluteValue() {this->SetOperation(VTK_ABS);};

  // Description:
  // Set each pixel in the output image to the square of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToSquare() {this->SetOperation(VTK_SQR);};

  // Description:
  // Set each pixel in the output image to the square root of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToSquareRoot() {this->SetOperation(VTK_SQRT);};

  // Description:
  // Set each pixel in the output image to the minimum of the corresponding pixels
  // in Input1 and Input2. (Output = min(Input1, Input2))
  void SetOperationToMin() {this->SetOperation(VTK_MIN);};

  // Description:
  // Set each pixel in the output image to the maximum of the corresponding pixels
  // in Input1 and Input2. (Output = max(Input1, Input2))
  void SetOperationToMax() {this->SetOperation(VTK_MAX);};

  // Description:
  // Set each pixel in the output image to the arctangent of the corresponding pixel
  // in Input1. Input2 is not used.
  void SetOperationToATAN() {this->SetOperation(VTK_ATAN);};

  void SetOperationToATAN2() {this->SetOperation(VTK_ATAN2);};

  // Description:
  // Set each pixel in the output image to the product of ConstantK with the
  // corresponding pixel in Input1. Input2 is not used.
  void SetOperationToMultiplyByK() {this->SetOperation(VTK_MULTIPLYBYK);};

  // Description:
  // Set each pixel in the output image to the product of ConstantC with the
  // corresponding pixel in Input1. Input2 is not used.
  void SetOperationToAddConstant() {this->SetOperation(VTK_ADDC);};

  // Description:
  // Find every pixel in Input1 that equals ConstantC and set the corresponding pixels
  // in the Output to ConstantK. Input2 is not used.
  void SetOperationToReplaceCByK() {this->SetOperation(VTK_REPLACECBYK);};

  // Description:
  // A constant used by some operations (typically multiplicative). Default is 1.
  vtkSetMacro(ConstantK,double);
  vtkGetMacro(ConstantK,double);

  // Description:
  // A constant used by some operations (typically additive). Default is 0.
  vtkSetMacro(ConstantC,double);
  vtkGetMacro(ConstantC,double);

  // Description:
  // How to handle divide by zero. Default is 0.
  vtkSetMacro(DivideByZeroToC,int);
  vtkGetMacro(DivideByZeroToC,int);
  vtkBooleanMacro(DivideByZeroToC,int);

  // Description:
  // Set the two inputs to this filter. For some operations, the second input
  // is not used.
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

protected:
  vtkImageMathematics();
  ~vtkImageMathematics() {}

  int Operation;
  double ConstantK;
  double ConstantC;
  int DivideByZeroToC;

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkImageMathematics(const vtkImageMathematics&);  // Not implemented.
  void operator=(const vtkImageMathematics&);  // Not implemented.
};

#endif

