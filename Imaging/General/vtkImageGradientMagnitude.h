/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientMagnitude.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageGradientMagnitude
 * @brief   Computes magnitude of the gradient.
 *
 *
 * vtkImageGradientMagnitude computes the gradient magnitude of an image.
 * Setting the dimensionality determines whether the gradient is computed on
 * 2D images, or 3D volumes.  The default is two dimensional XY images.
 *
 * @sa
 * vtkImageGradient vtkImageMagnitude
*/

#ifndef vtkImageGradientMagnitude_h
#define vtkImageGradientMagnitude_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageGradientMagnitude : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageGradientMagnitude *New();
  vtkTypeMacro(vtkImageGradientMagnitude,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * If "HandleBoundariesOn" then boundary pixels are duplicated
   * So central differences can get values.
   */
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);
  //@}

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...)
   */
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  //@}

protected:
  vtkImageGradientMagnitude();
  ~vtkImageGradientMagnitude() {}

  int HandleBoundaries;
  int Dimensionality;

  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageGradientMagnitude(const vtkImageGradientMagnitude&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageGradientMagnitude&) VTK_DELETE_FUNCTION;
};

#endif



