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
// .NAME vtkImageFoo - foo and scale an input image
// .SECTION Description
// With vtkImageFoo Pixels are foo'ed.

#ifndef vtkImageFoo_h
#define vtkImageFoo_h

#include "vtkThreadedImageAlgorithm.h"
#include "vtkmyImagingWin32Header.h"

class vtkBar;

class VTK_MY_IMAGING_EXPORT vtkImageFoo : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageFoo *New();
  vtkTypeMacro(vtkImageFoo,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the foo value.
  vtkSetMacro(Foo,float);
  vtkGetMacro(Foo,float);

  // Description:
  // Set the desired output scalar type.
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToLong()
    {this->SetOutputScalarType(VTK_LONG);}
  void SetOutputScalarTypeToUnsignedLong()
    {this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
  void SetOutputScalarTypeToInt()
    {this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToUnsignedInt()
    {this->SetOutputScalarType(VTK_UNSIGNED_INT);}
  void SetOutputScalarTypeToShort()
    {this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToChar()
    {this->SetOutputScalarType(VTK_CHAR);}
  void SetOutputScalarTypeToSignedChar()
    {this->SetOutputScalarType(VTK_SIGNED_CHAR);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

protected:
  vtkImageFoo();
  ~vtkImageFoo();

  float Foo;
  int OutputScalarType;
  vtkBar* Bar;

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector* outputVector);
  void ThreadedRequestData(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector,
                           vtkImageData*** inData, vtkImageData** outData,
                           int outExt[6], int id);
private:
  vtkImageFoo(const vtkImageFoo&);  // Not implemented.
  void operator=(const vtkImageFoo&);  // Not implemented.
};

#endif
