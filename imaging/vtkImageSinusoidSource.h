/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.h
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
// .NAME vtkImageSinusoidSource - Create an image with sinusoidal pixel values.
// vtkImageSinusoidSource just produces images with pixel values determined 
// by a sinusoid.


#ifndef __vtkImageSinusoidSource_h
#define __vtkImageSinusoidSource_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageSinusoidSource : public vtkImageSource
{
public:
  vtkImageSinusoidSource();
  static vtkImageSinusoidSource *New() {return new vtkImageSinusoidSource;};
  const char *GetClassName() {return "vtkImageSinusoidSource";};
  // void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int dim, int *extent);
  vtkImageSetExtentMacro(WholeExtent);
  
  // Description:
  // Set/Get the direction vector which determines the sinusoidal
  // orientation. The magnitude is ignored.
  void SetDirection(int num, float *direction);
  vtkImageSetMacro(Direction,float);
  vtkGetVector4Macro(Direction, float);
  
  // Description:
  // Set/Get the period of the sinusoid in pixels.
  vtkSetMacro(Period, float);
  vtkGetMacro(Period, float);

  // Description:
  // Set/Get the phase: 0->2Pi.  0 => Cosine, pi/2 => Sine.
  vtkSetMacro(Phase, float);
  vtkGetMacro(Phase, float);

  void UpdateImageInformation();

private:
  float StandardDeviation;
  int WholeExtent[8];
  float Direction[4];
  float Period;
  float Phase;

  void Execute(vtkImageRegion *outRegion);
};


#endif
