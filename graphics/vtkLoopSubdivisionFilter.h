/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLoopSubdivisionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources

Copyright (c) 1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkLoopSubdivisionFilter - generate a subdivision surface using the Loop Scheme
// .SECTION Description
// vtkLoopSubdivisionFilter is an approximating subdivision scheme that
// creates four new triangles for each triangle in the mesh. The user can
// specifiy the NumberOfSubdivisions. Loop's subdivision scheme is
// described in: Loop, C., "Smooth Subdivision surfaces based on
// triangles,", Masters Thesis, University of Utah, August 1987.
// For a nice summary of the technique see, Hoppe, H., et. al,
// "Piecewise Smooth Surface Reconstruction,:, Proceedings of Siggraph 94
// (Orlando, Florida, July 24-29, 1994). In COmputer Graphics
// Proceedings, Annual COnference Series, 1994, ACM SIGGRAPH,
// pp. 295-302.
// <P>
// The filter only operates on triangles. Users should use the
// vtkTriangleFilter to triangulate meshes that contain polygons or
// triangle strips.
// <P>
// The filter approximates point data using the same scheme. New
// triangles create at a subdivision step will have the cell data of
// their parent cell.

// .SECTION See Also
// vtkApproximatingSubdivisionFilter

#ifndef __vtkLoopSubdivisionFilter_h
#define __vtkLoopSubdivisionFilter_h

#include "vtkApproximatingSubdivisionFilter.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellArray.h"

class VTK_EXPORT vtkLoopSubdivisionFilter : public vtkApproximatingSubdivisionFilter
{
public:
  // Description:
  // Construct object with NumberOfSubdivisions set to 1.
  static vtkLoopSubdivisionFilter *New();
  vtkTypeMacro(vtkLoopSubdivisionFilter,vtkApproximatingSubdivisionFilter);

protected:
  vtkLoopSubdivisionFilter () {};
  ~vtkLoopSubdivisionFilter () {};
  vtkLoopSubdivisionFilter(const vtkLoopSubdivisionFilter&) {};
  void operator=(const vtkLoopSubdivisionFilter&) {};

  void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD);
  void GenerateEvenStencil (int p1, vtkPolyData *polys, vtkIdList *stencilIds, float *weights);
  void GenerateOddStencil (int p1, int p2, vtkPolyData *polys, vtkIdList *stencilIds, float *weights);
};

#endif


