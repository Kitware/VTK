// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkImageAlgorithm.h"
#include "vtkImagingStencilModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGSTENCIL_EXPORT vtkImageStencilToImage : public vtkImageAlgorithm
{
public:
  static vtkImageStencilToImage* New();
  vtkTypeMacro(vtkImageStencilToImage, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The value to use outside the stencil.  The default is 0.
   */
  vtkSetMacro(OutsideValue, double);
  vtkGetMacro(OutsideValue, double);
  ///@}

  ///@{
  /**
   * The value to use inside the stencil.  The default is 1.
   */
  vtkSetMacro(InsideValue, double);
  vtkGetMacro(InsideValue, double);
  ///@}

  ///@{
  /**
   * The desired output scalar type.  The default is unsigned char.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(VTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(VTK_DOUBLE); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(VTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(VTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(VTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(VTK_CHAR); }
  ///@}

protected:
  vtkImageStencilToImage();
  ~vtkImageStencilToImage() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double OutsideValue;
  double InsideValue;
  int OutputScalarType;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkImageStencilToImage(const vtkImageStencilToImage&) = delete;
  void operator=(const vtkImageStencilToImage&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
