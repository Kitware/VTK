/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVariance3D.h
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
// .NAME vtkImageVariance3D - Variance in a neighborhood..
// .SECTION Description
// vtkImageVariance3D replaces each pixel with a measurement of 
// pixel variance in a eliptical neighborhood centered on that pixel.
// The value computed is not exactly the variance.
// The difference between the neighbor values and center value is computed
// and squared for each neighbor.  These values are summed and divided by
// the total number of neighbors to produce the output value.


#ifndef __vtkImageVariance3D_h
#define __vtkImageVariance3D_h


#include "vtkImageSpatialFilter.h"

class VTK_EXPORT vtkImageVariance3D : public vtkImageSpatialFilter
{
public:
  vtkImageVariance3D();
  static vtkImageVariance3D *New() 
    {return new vtkImageVariance3D;};
  const char *GetClassName() {return "vtkImageVariance3D";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetFilteredAxes(int axis0, int axis1, int axis2);

  // Set/Get the size of the neighood.
  void SetKernelSize(int size0, int size1, int size2);
  
  // Description:
  // Get the Mask used as a footprint.
  vtkGetObjectMacro(Mask, vtkImageRegion);
  
protected:
  vtkImageRegion *Mask;
    
  void ExecuteCenter(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void ComputeMask();
};

#endif



