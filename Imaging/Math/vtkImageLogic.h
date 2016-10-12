/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageLogic
 * @brief   And, or, xor, nand, nor, not.
 *
 * vtkImageLogic implements basic logic operations.
 * SetOperation is used to select the filter's behavior.
 * The filter can take two or one input. Inputs must have the same type.
*/

#ifndef vtkImageLogic_h
#define vtkImageLogic_h


// Operation options.
#define VTK_AND          0
#define VTK_OR           1
#define VTK_XOR          2
#define VTK_NAND         3
#define VTK_NOR          4
#define VTK_NOT          5
#define VTK_NOP          6



#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageLogic : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLogic *New();
  vtkTypeMacro(vtkImageLogic,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set/Get the Operation to perform.
   */
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);
  void SetOperationToAnd() {this->SetOperation(VTK_AND);};
  void SetOperationToOr() {this->SetOperation(VTK_OR);};
  void SetOperationToXor() {this->SetOperation(VTK_XOR);};
  void SetOperationToNand() {this->SetOperation(VTK_NAND);};
  void SetOperationToNor() {this->SetOperation(VTK_NOR);};
  void SetOperationToNot() {this->SetOperation(VTK_NOT);};
  //@}

  //@{
  /**
   * Set the value to use for true in the output.
   */
  vtkSetMacro(OutputTrueValue, double);
  vtkGetMacro(OutputTrueValue, double);
  //@}

    /**
     * Set the Input1 of this filter.
     */
  virtual void SetInput1Data(vtkDataObject *input) { this->SetInputData(0,input);};

  /**
   * Set the Input2 of this filter.
   */
  virtual void SetInput2Data(vtkDataObject *input) { this->SetInputData(1,input);};

protected:
  vtkImageLogic();
  ~vtkImageLogic() {}

  int Operation;
  double OutputTrueValue;

  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkImageLogic(const vtkImageLogic&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageLogic&) VTK_DELETE_FUNCTION;
};

#endif













