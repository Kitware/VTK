/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageDivergence : public vtkImageToImageFilter
{
public:
  static vtkImageDivergence *New();
  vtkTypeRevisionMacro(vtkImageDivergence,vtkImageToImageFilter);

protected:
  vtkImageDivergence() {};
  ~vtkImageDivergence() {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();}
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);

private:
  vtkImageDivergence(const vtkImageDivergence&);  // Not implemented.
  void operator=(const vtkImageDivergence&);  // Not implemented.
};

#endif



