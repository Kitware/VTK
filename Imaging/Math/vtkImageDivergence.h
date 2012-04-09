/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDivergence - Divergence of a vector field.
// .SECTION Description
// vtkImageDivergence takes a 3D vector field
// and creates a scalar field which
// which represents the rate of change of the vector field.
// The definition of Divergence:
// Given V = P(x,y,z), Q(x,y,z), R(x,y,z),
// Divergence = dP/dx + dQ/dy + dR/dz.

#ifndef __vtkImageDivergence_h
#define __vtkImageDivergence_h

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageDivergence : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageDivergence *New();
  vtkTypeMacro(vtkImageDivergence,vtkThreadedImageAlgorithm);

protected:
  vtkImageDivergence();
  ~vtkImageDivergence() {};

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);

private:
  vtkImageDivergence(const vtkImageDivergence&);  // Not implemented.
  void operator=(const vtkImageDivergence&);  // Not implemented.
};

#endif



// VTK-HeaderTest-Exclude: vtkImageDivergence.h
