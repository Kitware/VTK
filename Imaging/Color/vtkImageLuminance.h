/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLuminance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageLuminance
 * @brief   Computes the luminance of the input
 *
 * vtkImageLuminance calculates luminance from an rgb input.
*/

#ifndef vtkImageLuminance_h
#define vtkImageLuminance_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageLuminance : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLuminance *New();
  vtkTypeMacro(vtkImageLuminance,vtkThreadedImageAlgorithm);

protected:
  vtkImageLuminance();
  ~vtkImageLuminance() override {}

  int RequestInformation (vtkInformation *, vtkInformationVector**,
                                  vtkInformationVector *) override;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                        int outExt[6], int id) override;

private:
  vtkImageLuminance(const vtkImageLuminance&) = delete;
  void operator=(const vtkImageLuminance&) = delete;
};

#endif










// VTK-HeaderTest-Exclude: vtkImageLuminance.h
