/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnitude.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMagnitude
 * @brief   Colapses components with magnitude function..
 *
 * vtkImageMagnitude takes the magnitude of the components.
*/

#ifndef vtkImageMagnitude_h
#define vtkImageMagnitude_h


#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageMagnitude : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMagnitude *New();
  vtkTypeMacro(vtkImageMagnitude,vtkThreadedImageAlgorithm);

protected:
  vtkImageMagnitude();
  ~vtkImageMagnitude() override {}

  int RequestInformation (vtkInformation *, vtkInformationVector**,
                                  vtkInformationVector *) override;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                        int outExt[6], int id) override;

private:
  vtkImageMagnitude(const vtkImageMagnitude&) = delete;
  void operator=(const vtkImageMagnitude&) = delete;
};

#endif










// VTK-HeaderTest-Exclude: vtkImageMagnitude.h
