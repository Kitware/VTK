/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dFftFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImage1dFftFilter - 1d Fast Fourier Transform.
// .SECTION Description
// vtkImage1dFftFilter implements a 1d fast Fourier transform.  The input
// can have real or imaginary data in any components and data types, but
// the output is always float with real values in component0, and
// imaginary values in component1.


#ifndef __vtkImage1dFftFilter_h
#define __vtkImage1dFftFilter_h


#include "vtkImageFourierFilter.hh"

class vtkImage1dFftFilter : public vtkImageFourierFilter
{
public:
  vtkImage1dFftFilter();
  char *GetClassName() {return "vtkImage1dFftFilter";};

  void SetAxis1d(int axis);
  void InterceptCacheUpdate(vtkImageRegion *region);

  // Description:
  // Tell the filter which input component is real and which is imaginary.
  // If imaginary is out of the image bounds, it is ignored.
  vtkSetMacro(InputRealComponent, int);
  vtkGetMacro(InputRealComponent, int);
  vtkSetMacro(InputImaginaryComponent, int);
  vtkGetMacro(InputImaginaryComponent, int);
  
protected:
  // which input components are real and which are imaginary.
  int InputRealComponent;
  int InputImaginaryComponent;
    
  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionBounds(vtkImageRegion *outRegion, 
					vtkImageRegion *inRegion);
  void Execute2d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif










