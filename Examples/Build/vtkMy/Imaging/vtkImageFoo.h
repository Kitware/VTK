/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFoo.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/
// .NAME vtkImageFoo - foo and scale an input image
// .SECTION Description
// With vtkImageFoo Pixels are foo'ed.

#ifndef __vtkImageFoo_h
#define __vtkImageFoo_h

#include "vtkImageToImageFilter.h"
#include "vtkmyImagingWin32Header.h"

class vtkBar;

class VTK_MY_IMAGING_EXPORT vtkImageFoo : public vtkImageToImageFilter
{
public:
  static vtkImageFoo *New();
  vtkTypeRevisionMacro(vtkImageFoo,vtkImageToImageFilter);
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
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

protected:
  vtkImageFoo();
  ~vtkImageFoo();

  float Foo;
  int OutputScalarType;
  vtkBar* Bar;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageFoo(const vtkImageFoo&);  // Not implemented.
  void operator=(const vtkImageFoo&);  // Not implemented.
};

#endif
