// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMathematics
 * @brief   Add, subtract, multiply, divide, invert, sin,
 * cos, exp, log.
 *
 * vtkImageMathematics implements basic mathematic operations SetOperation is
 * used to select the filters behavior.  The filter can take two or one
 * input.
 */

#ifndef vtkImageMathematics_h
#define vtkImageMathematics_h

// Operation options.
#define VTK_ADD 0
#define VTK_SUBTRACT 1
#define VTK_MULTIPLY 2
#define VTK_DIVIDE 3
#define VTK_INVERT 4
#define VTK_SIN 5
#define VTK_COS 6
#define VTK_EXP 7
#define VTK_LOG 8
#define VTK_ABS 9
#define VTK_SQR 10
#define VTK_SQRT 11
#define VTK_MIN 12
#define VTK_MAX 13
#define VTK_ATAN 14
#define VTK_ATAN2 15
#define VTK_MULTIPLYBYK 16
#define VTK_ADDC 17
#define VTK_CONJUGATE 18
#define VTK_COMPLEX_MULTIPLY 19
#define VTK_REPLACECBYK 20

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGMATH_EXPORT vtkImageMathematics : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMathematics* New();
  vtkTypeMacro(vtkImageMathematics, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the Operation to perform.
   */
  vtkSetMacro(Operation, int);
  vtkGetMacro(Operation, int);
  ///@}

  /**
   * Set each pixel in the output image to the sum of the corresponding pixels
   * in Input1 and Input2.
   */
  void SetOperationToAdd() { this->SetOperation(VTK_ADD); }

  /**
   * Set each pixel in the output image to the difference of the corresponding pixels
   * in Input1 and Input2 (output = Input1 - Input2).
   */
  void SetOperationToSubtract() { this->SetOperation(VTK_SUBTRACT); }

  /**
   * Set each pixel in the output image to the product of the corresponding pixels
   * in Input1 and Input2.
   */
  void SetOperationToMultiply() { this->SetOperation(VTK_MULTIPLY); }

  /**
   * Set each pixel in the output image to the quotient of the corresponding pixels
   * in Input1 and Input2 (Output = Input1 / Input2).
   */
  void SetOperationToDivide() { this->SetOperation(VTK_DIVIDE); }

  void SetOperationToConjugate() { this->SetOperation(VTK_CONJUGATE); }

  void SetOperationToComplexMultiply() { this->SetOperation(VTK_COMPLEX_MULTIPLY); }

  /**
   * Set each pixel in the output image to 1 over the corresponding pixel
   * in Input1 and Input2 (output = 1 / Input1). Input2 is not used.
   */
  void SetOperationToInvert() { this->SetOperation(VTK_INVERT); }

  /**
   * Set each pixel in the output image to the sine of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSin() { this->SetOperation(VTK_SIN); }

  /**
   * Set each pixel in the output image to the cosine of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToCos() { this->SetOperation(VTK_COS); }

  /**
   * Set each pixel in the output image to the exponential of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToExp() { this->SetOperation(VTK_EXP); }

  /**
   * Set each pixel in the output image to the log of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToLog() { this->SetOperation(VTK_LOG); }

  /**
   * Set each pixel in the output image to the absolute value of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToAbsoluteValue() { this->SetOperation(VTK_ABS); }

  /**
   * Set each pixel in the output image to the square of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSquare() { this->SetOperation(VTK_SQR); }

  /**
   * Set each pixel in the output image to the square root of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSquareRoot() { this->SetOperation(VTK_SQRT); }

  /**
   * Set each pixel in the output image to the minimum of the corresponding pixels
   * in Input1 and Input2. (Output = min(Input1, Input2))
   */
  void SetOperationToMin() { this->SetOperation(VTK_MIN); }

  /**
   * Set each pixel in the output image to the maximum of the corresponding pixels
   * in Input1 and Input2. (Output = max(Input1, Input2))
   */
  void SetOperationToMax() { this->SetOperation(VTK_MAX); }

  /**
   * Set each pixel in the output image to the arctangent of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToATAN() { this->SetOperation(VTK_ATAN); }

  void SetOperationToATAN2() { this->SetOperation(VTK_ATAN2); }

  /**
   * Set each pixel in the output image to the product of ConstantK with the
   * corresponding pixel in Input1. Input2 is not used.
   */
  void SetOperationToMultiplyByK() { this->SetOperation(VTK_MULTIPLYBYK); }

  /**
   * Set each pixel in the output image to the product of ConstantC with the
   * corresponding pixel in Input1. Input2 is not used.
   */
  void SetOperationToAddConstant() { this->SetOperation(VTK_ADDC); }

  /**
   * Find every pixel in Input1 that equals ConstantC and set the corresponding pixels
   * in the Output to ConstantK. Input2 is not used.
   */
  void SetOperationToReplaceCByK() { this->SetOperation(VTK_REPLACECBYK); }

  ///@{
  /**
   * A constant used by some operations (typically multiplicative). Default is 1.
   */
  vtkSetMacro(ConstantK, double);
  vtkGetMacro(ConstantK, double);
  ///@}

  ///@{
  /**
   * A constant used by some operations (typically additive). Default is 0.
   */
  vtkSetMacro(ConstantC, double);
  vtkGetMacro(ConstantC, double);
  ///@}

  ///@{
  /**
   * How to handle divide by zero. Default is 0.
   */
  vtkSetMacro(DivideByZeroToC, vtkTypeBool);
  vtkGetMacro(DivideByZeroToC, vtkTypeBool);
  vtkBooleanMacro(DivideByZeroToC, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the inputs to this filter. For some operations, the second input
   * is not used.
   */
  virtual void SetInput1Data(vtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(vtkDataObject* in) { this->AddInputData(0, in); }
  void SetInputConnection(int idx, vtkAlgorithmOutput* input) override;
  void SetInputConnection(vtkAlgorithmOutput* input) override
  {
    this->SetInputConnection(0, input);
  }
  ///@}

  /**
   * Replace one of the input connections with a new input.  You can
   * only replace input connections that you previously created with
   * AddInputConnection() or, in the case of the first input,
   * with SetInputConnection().
   */
  virtual void ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input);

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(int idx, vtkDataObject* input);
  void SetInputData(vtkDataObject* input) { this->SetInputData(0, input); }
  ///@}

  ///@{
  /**
   * Get one input to this filter. This method is only for support of
   * old-style pipeline connections.  When writing new code you should
   * use vtkAlgorithm::GetInputConnection(0, num).
   */
  vtkDataObject* GetInput(int idx);
  vtkDataObject* GetInput() { return this->GetInput(0); }
  ///@}

  /**
   * Get the number of inputs to this filter. This method is only for
   * support of old-style pipeline connections.  When writing new code
   * you should use vtkAlgorithm::GetNumberOfInputConnections(0).
   */
  int GetNumberOfInputs() { return this->GetNumberOfInputConnections(0); }

protected:
  vtkImageMathematics();
  ~vtkImageMathematics() override = default;

  int Operation;
  double ConstantK;
  double ConstantC;
  vtkTypeBool DivideByZeroToC;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int threadId) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkImageMathematics(const vtkImageMathematics&) = delete;
  void operator=(const vtkImageMathematics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
