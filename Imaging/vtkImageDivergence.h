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

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageDivergence : public vtkImageAlgorithm
{
public:
  static vtkImageDivergence *New();
  vtkTypeRevisionMacro(vtkImageDivergence,vtkImageAlgorithm);

protected:
  vtkImageDivergence();
  ~vtkImageDivergence() {};

  void ComputeInputUpdateExtent (vtkInformation *, vtkInformationVector *, 
                                 vtkInformationVector *);
  void ExecuteInformation (vtkInformation *, vtkInformationVector *, 
                           vtkInformationVector *);
  void ThreadedExecute (vtkImageData ***inData, vtkImageData **outData,
                       int ext[6], int id);

private:
  vtkImageDivergence(const vtkImageDivergence&);  // Not implemented.
  void operator=(const vtkImageDivergence&);  // Not implemented.
};

#endif



