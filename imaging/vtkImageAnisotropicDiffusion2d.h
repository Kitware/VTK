/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2d.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageAnisotropicDiffusion2d - edge preserving smoothing.
// .SECTION Description
// vtkImageAnisotropicDiffusion2d  diffuses if pixel
// difference is below a threshold.  It diffuses with all 8 neighbors.
// Input and output can be any type.


#ifndef __vtkImageAnisotropicDiffusion2d_h
#define __vtkImageAnisotropicDiffusion2d_h


#include "vtkImageSpatialFilter.h"

class vtkImageAnisotropicDiffusion2d : public vtkImageSpatialFilter
{
public:
  vtkImageAnisotropicDiffusion2d();
  char *GetClassName() {return "vtkImageAnisotropicDiffusion2d";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetNumberOfIterations(int num);
  // Description:
  // Get the number of iterations.
  vtkGetMacro(NumberOfIterations,int);

  // Description:
  // Set/Get the difference threshold that stops diffusion.
  vtkSetMacro(DiffusionThreshold,float);
  vtkGetMacro(DiffusionThreshold,float);
  
  
  // Description:
  // Set/Get the difference factor
  vtkSetMacro(DiffusionFactor,float);
  vtkGetMacro(DiffusionFactor,float);
  
  
protected:
  int NumberOfIterations;
  float DiffusionThreshold;
  float DiffusionFactor;  
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Iterate(vtkImageRegion *in, vtkImageRegion *out, float ar0, float ar1);
};

#endif



