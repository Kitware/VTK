/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHarrWavelet2D.h
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
// .NAME vtkImageHarrWavelet2D - Har wavelet decomposition.
// .SECTION Description
// vtkImageHarrWavelet2D Uses har wavelets to decompose an image to
// a specified number of resolution levels.  It is written so that
// it uses the whole input, and generates the whole output, when
// any region is requested.


#ifndef __vtkImageHarrWavelet2D_h
#define __vtkImageHarrWavelet2D_h


#include "vtkImageFilter.h"

class vtkImageHarrWavelet2D : public vtkImageFilter
{
public:
  vtkImageHarrWavelet2D();
  char *GetClassName() {return "vtkImageHarrWavelet2D";};

  // Description:
  // Set/Get the number of resolution levels.
  vtkSetMacro(NumberLevels,int);
  vtkGetMacro(NumberLevels,int);
  
  // Description:
  // Set/Get offset for the 3 wavelet quadrents.
  vtkSetMacro(PixelOffset,float);
  vtkGetMacro(PixelOffset,float);

  // Description:
  // Set/Get scale for the 3 wavelet quadrents.
  vtkSetMacro(PixelScale,float);
  vtkGetMacro(PixelScale,float);

  void InterceptCacheUpdate(vtkImageRegion *region);

protected:
  int NumberLevels;
  float PixelScale;
  float PixelOffset;

  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



