/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLoopSubdivisionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLoopSubdivisionFilter - generate a subdivision surface using the Loop Scheme
// .SECTION Description
// vtkLoopSubdivisionFilter is an approximating subdivision scheme that
// creates four new triangles for each triangle in the mesh. The user can
// specify the NumberOfSubdivisions. Loop's subdivision scheme is
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

class VTK_GRAPHICS_EXPORT vtkLoopSubdivisionFilter : public vtkApproximatingSubdivisionFilter
{
public:
  // Description:
  // Construct object with NumberOfSubdivisions set to 1.
  static vtkLoopSubdivisionFilter *New();
  vtkTypeRevisionMacro(vtkLoopSubdivisionFilter,vtkApproximatingSubdivisionFilter);

protected:
  vtkLoopSubdivisionFilter () {};
  ~vtkLoopSubdivisionFilter () {};

  void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData,
                                  vtkPoints *outputPts,
                                  vtkPointData *outputPD);
  void GenerateEvenStencil (vtkIdType p1, vtkPolyData *polys,
                            vtkIdList *stencilIds, float *weights);
  void GenerateOddStencil (vtkIdType p1, vtkIdType p2, vtkPolyData *polys,
                           vtkIdList *stencilIds, float *weights);

  void ComputeInputUpdateExtents(vtkDataObject *output);
private:
  vtkLoopSubdivisionFilter(const vtkLoopSubdivisionFilter&);  // Not implemented.
  void operator=(const vtkLoopSubdivisionFilter&);  // Not implemented.
};

#endif


