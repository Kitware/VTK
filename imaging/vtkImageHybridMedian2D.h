/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.h
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
// .NAME vtkImageHybridMedian2D - Median filter that preseverse lines and 
// corners.
// .SECTION Description
// vtkImageHybridMedian2D is a median filter that preserves thin lines and 
// corners.
// It operates on a 5x5 pixel neighborhood. 
// It computes two values initially: the median
// of the + neighbors and the median of the x neighbors.  
// It the computes the median of these
// two values plus the center pixel.  
// This result of this second median is the output pixel value.



#ifndef __vtkImageHybridMedian2D_h
#define __vtkImageHybridMedian2D_h


#include "vtkImageMedianFilter.h"

class VTK_EXPORT vtkImageHybridMedian2D : public vtkImageMedianFilter
{
public:
  vtkImageHybridMedian2D();
  ~vtkImageHybridMedian2D();
  static vtkImageHybridMedian2D *New() {return new vtkImageHybridMedian2D;};
  const char *GetClassName() {return "vtkImageHybridMedian2D";};

  void SetFilteredAxes(int axis0, int axis1);
  
protected:
  // stuff for sorting the pixels
  int NumNeighborhood;
  double *Sort;
  double *Median;
  int UpMax;
  int DownMax;
  int UpNum;
  int DownNum;

  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



