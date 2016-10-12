/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNormalize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageNormalize
 * @brief   Normalizes that scalar components for each point.
 *
 * For each point, vtkImageNormalize normalizes the vector defined by the
 * scalar components.  If the magnitude of this vector is zero, the output
 * vector is zero also.
*/

#ifndef vtkImageNormalize_h
#define vtkImageNormalize_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageNormalize : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageNormalize *New();
  vtkTypeMacro(vtkImageNormalize,vtkThreadedImageAlgorithm);

protected:
  vtkImageNormalize();
  ~vtkImageNormalize() {}

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageNormalize(const vtkImageNormalize&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageNormalize&) VTK_DELETE_FUNCTION;
};

#endif










// VTK-HeaderTest-Exclude: vtkImageNormalize.h
