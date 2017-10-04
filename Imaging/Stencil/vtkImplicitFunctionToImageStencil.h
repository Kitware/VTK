/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitFunctionToImageStencil
 * @brief   clip an image with a function
 *
 * vtkImplicitFunctionToImageStencil will convert a vtkImplicitFunction into
 * a stencil that can be used with vtkImageStencil or with other classes
 * that apply a stencil to an image.
 * @sa
 * vtkImplicitFunction vtkImageStencil vtkPolyDataToImageStencil
*/

#ifndef vtkImplicitFunctionToImageStencil_h
#define vtkImplicitFunctionToImageStencil_h


#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageStencilSource.h"

class vtkImplicitFunction;

class VTKIMAGINGSTENCIL_EXPORT vtkImplicitFunctionToImageStencil : public vtkImageStencilSource
{
public:
  static vtkImplicitFunctionToImageStencil *New();
  vtkTypeMacro(vtkImplicitFunctionToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the implicit function to convert into a stencil.
   */
  virtual void SetInput(vtkImplicitFunction*);
  vtkGetObjectMacro(Input, vtkImplicitFunction);
  //@}

  //@{
  /**
   * Set the threshold value for the implicit function.
   */
  vtkSetMacro(Threshold, double);
  vtkGetMacro(Threshold, double);
  //@}

  /**
   * Override GetMTime() to account for the implicit function.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkImplicitFunctionToImageStencil();
  ~vtkImplicitFunctionToImageStencil() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;

  vtkImplicitFunction *Input;
  double Threshold;

private:
  vtkImplicitFunctionToImageStencil(const vtkImplicitFunctionToImageStencil&) = delete;
  void operator=(const vtkImplicitFunctionToImageStencil&) = delete;
};

#endif

