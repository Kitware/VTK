/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayBounder.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkRayBounder - An abstract class for bounding rays
// .SECTION Description
// vtkRayBounder is an abstract class for bounding view rays during
// ray casting. Each concrete subclass must have a GetRayBounds 
// method which, given a renderer, produces a floating point array
// that is w*h*2 (w = width of viewport, h = height of viewport).
// For each pixel, this array contains the near and far clipping
// values for the corresponding ray. A value of -1 means no clipping
// should be performed on that ray.

// .SECTION see also
// 

#ifndef __vtkRayBounder_h
#define __vtkRayBounder_h

#include "vtkObject.h"
#include "vtkRenderer.h"

class VTK_EXPORT vtkRayBounder : public vtkObject
{
public:
  const char *GetClassName() {return "vtkRayBounder";};

  // Description:
  // Get the ray bounds - a floating point array with two
  // values for every pixel in the viewport. These values
  // indicate near and far clipping distances, or -1 for
  // no clipping.
  virtual float *GetRayBounds( vtkRenderer *ren )=0;

protected:
};

#endif

