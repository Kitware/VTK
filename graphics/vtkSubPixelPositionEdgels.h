/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1997 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkSubPixelPositionEdgels - adjust edgel locations based on gradients.
// .SECTION Description
// vtkSubPixelPositionEdgels is a filter that takes a series of linked
// edgels (digital curves) and gradient maps as input. It then adjusts
// the edgel locations based on the gradient data. Specifically, the
// algorithm first determines the neighboring gradient magnitudes of
// an edgel using simple interpolation of its neighbors. It then fits
// the following three data points: negative gradient direction
// gradient magnitude, edgel gradient magnitude and positive gradient
// direction gradient magnitude to a quadratic function. It then
// solves this quadratic to find the maximum gradient location along
// the gradient orientation.  It then modifes the edgels location
// along the gradient orientation to the calculated maximum
// location. This algorithm does not adjust an edgel in the direction
// orthogonal to its gradient vector.

// .SECTION see also
// vtkImage vtkImageGradient vtkLinkEdgels

#ifndef __vtkSubPixelPositionEdgels_h
#define __vtkSubPixelPositionEdgels_h

#include "vtkPolyToPolyFilter.h"
#include "vtkStructuredPoints.h"

class VTK_EXPORT vtkSubPixelPositionEdgels : public vtkPolyToPolyFilter
{
public:
  vtkSubPixelPositionEdgels();
  static vtkSubPixelPositionEdgels *New() {return new vtkSubPixelPositionEdgels;};
  char *GetClassName() {return "vtkSubPixelPositionEdgels";};

  void Update();

  // Description:
  // Set/Get the gradient data for doing the position adjustments.
  vtkSetObjectMacro(GradMaps,vtkStructuredPoints);
  vtkGetObjectMacro(GradMaps,vtkStructuredPoints);

protected:
  // Usual data generation method
  void Execute();
  void Move(int xdim, int ydim, int zdim, int x, int y,
	    float *img, vtkVectors *inVecs, 
	    float *result, int z, float *aspect, float *resultNormal);
  vtkStructuredPoints *GradMaps;  
};

#endif


