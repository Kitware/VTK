/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageStencilToImage
 * @brief   Convert an image stencil into an image
 *
 * vtkImageStencilToImage will convert an image stencil into a binary
 * image.  The default output will be an 8-bit image with a value of 1
 * inside the stencil and 0 outside.  When used in combination with
 * vtkPolyDataToImageStencil or vtkImplicitFunctionToImageStencil, this
 * can be used to create a binary image from a mesh or a function.
 * @sa
 * vtkImplicitModeller
*/

#ifndef vtkImageStencilToImage_h
#define vtkImageStencilToImage_h

#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGSTENCIL_EXPORT vtkImageStencilToImage : public vtkImageAlgorithm
{
public:
  static vtkImageStencilToImage *New();
  vtkTypeMacro(vtkImageStencilToImage, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The value to use outside the stencil.  The default is 0.
   */
  vtkSetMacro(OutsideValue, double);
  vtkGetMacro(OutsideValue, double);
  //@}

  //@{
  /**
   * The value to use inside the stencil.  The default is 1.
   */
  vtkSetMacro(InsideValue, double);
  vtkGetMacro(InsideValue, double);
  //@}

  //@{
  /**
   * The desired output scalar type.  The default is unsigned char.
   */
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToFloat(){this->SetOutputScalarType(VTK_FLOAT);};
  void SetOutputScalarTypeToDouble(){this->SetOutputScalarType(VTK_DOUBLE);};
  void SetOutputScalarTypeToInt(){this->SetOutputScalarType(VTK_INT);};
  void SetOutputScalarTypeToUnsignedInt()
    {this->SetOutputScalarType(VTK_UNSIGNED_INT);};
  void SetOutputScalarTypeToLong(){this->SetOutputScalarType(VTK_LONG);};
  void SetOutputScalarTypeToUnsignedLong()
    {this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
  void SetOutputScalarTypeToShort(){this->SetOutputScalarType(VTK_SHORT);};
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);};
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);};
  void SetOutputScalarTypeToChar()
    {this->SetOutputScalarType(VTK_CHAR);};
  //@}

protected:
  vtkImageStencilToImage();
  ~vtkImageStencilToImage() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *) VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  double OutsideValue;
  double InsideValue;
  int OutputScalarType;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkImageStencilToImage(const vtkImageStencilToImage&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageStencilToImage&) VTK_DELETE_FUNCTION;
};

#endif
