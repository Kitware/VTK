/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButterflySubdivisionFilter.h
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
// .NAME vtkButterflySubdivisionFilter - generate a subdivision surface using the Butterfly Scheme
// .SECTION Description
// vtkButterflySubdivisionFilter is an interpolating subdivision scheme
// that creates four new triangles for each triangle in the mesh. The
// user can specifiy the NumberOfSubdivisions. This filter implements the
// 8-point butterfly scheme described in: Zorin, D., Schroder, P., and
// Sweldens, W., "Interpolating Subdivisions for Meshes with Arbitrary
// Topology," Computer Graphics Proceedings, Annual Conference Series,
// 1996, ACM SIGGRAPH, pp.189-192. This scheme improves previous
// butterfly subdivisions with special treatment of vertices with valence
// other than 6.
// <P>
// Currently, the filter only operates on triangles. Users should use the
// vtkTriangleFilter to triangulate meshes that contain polygons or
// triangle stiprs.
// <P>
// The filter interpolates point data using the same scheme. New
// triangles created at a subdivision step will have the cell data of
// their parent cell.

// .SECTION See Also
// vtkInterpolatingSubdivisionFilter vtkLinearSubdivisionFilter

#ifndef __vtkButterflySubdivisionFilter_h
#define __vtkButterflySubdivisionFilter_h

#include "vtkInterpolatingSubdivisionFilter.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellArray.h"

class VTK_EXPORT vtkButterflySubdivisionFilter : public vtkInterpolatingSubdivisionFilter
{
public:
  // Description:
  // Construct object with NumberOfSubdivisions set to 1.
  static vtkButterflySubdivisionFilter *New();
  vtkTypeMacro(vtkButterflySubdivisionFilter,vtkInterpolatingSubdivisionFilter);

 protected:
  vtkButterflySubdivisionFilter () {};
  ~vtkButterflySubdivisionFilter () {};
  vtkButterflySubdivisionFilter(const vtkButterflySubdivisionFilter&) {};
  void operator=(const vtkButterflySubdivisionFilter&) {};

 private:
  void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD);
  void GenerateButterflyStencil (int p1, int p2, vtkPolyData *polys, vtkIdList *stencilIds, float *weights);
  void GenerateLoopStencil (int p1, int p2, vtkPolyData *polys, vtkIdList *stencilIds, float *weights);
  void GenerateBoundaryStencil (int p1, int p2, vtkPolyData *polys, vtkIdList *stencilIds, float *weights);
};

#endif


