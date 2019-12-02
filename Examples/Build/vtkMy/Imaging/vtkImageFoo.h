/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFoo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageFoo
 * @brief   foo and scale an input image
 *
 * With vtkImageFoo Pixels are foo'ed.
 */

#ifndef vtkImageFoo_h
#define vtkImageFoo_h

#include "vtkThreadedImageAlgorithm.h"
#include "vtkmyImagingModule.h" // For export macro

class vtkBar;

class VTKMYIMAGING_EXPORT vtkImageFoo : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageFoo* New();
  vtkTypeMacro(vtkImageFoo, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the foo value.
   */
  vtkSetMacro(Foo, float);
  vtkGetMacro(Foo, float);
  //@}

  //@{
  /**
   * Set the desired output scalar type.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(VTK_DOUBLE); }
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(VTK_FLOAT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(VTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(VTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(VTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(VTK_CHAR); }
  void SetOutputScalarTypeToSignedChar() { this->SetOutputScalarType(VTK_SIGNED_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  //@}

protected:
  vtkImageFoo();
  ~vtkImageFoo() override;

  float Foo;
  int OutputScalarType;
  vtkBar* Bar;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageFoo(const vtkImageFoo&) = delete;
  void operator=(const vtkImageFoo&) = delete;
};

#endif
