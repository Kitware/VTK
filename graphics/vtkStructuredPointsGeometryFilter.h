/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsGeometryFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStructuredPointsGeometryFilter - extract geometry for structured points
// .SECTION Description
// vtkStructuredPointsGeometryFilter is a filter that extracts geometry from a
// structured points dataset. By specifying appropriate i-j-k indices (via the 
// "Extent" instance variable), it is possible to extract a point, a line, a 
// plane (i.e., image), or a "volume" from dataset. (Since the output is 
// of type polydata, the volume is actually a (n x m x o) region of points.)
//
// The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 volume is given by (0,49, 0,49, 0,0).
// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.
// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridFilter

#ifndef __vtkStructuredPointsGeometryFilter_h
#define __vtkStructuredPointsGeometryFilter_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_EXPORT vtkStructuredPointsGeometryFilter : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkStructuredPointsGeometryFilter();
  static vtkStructuredPointsGeometryFilter *New() {return new vtkStructuredPointsGeometryFilter;};
  const char *GetClassName() {return "vtkStructuredPointsGeometryFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  void SetExtent(int *extent);
  int *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int Extent[6];
};

#endif


