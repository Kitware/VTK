/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButterflySubdivisionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkButterflySubdivisionFilter - generate a subdivision surface using the Butterfly Scheme
// .SECTION Description
// vtkButterflySubdivisionFilter is an interpolating subdivision scheme
// that creates four new triangles for each triangle in the mesh. The
// user can specify the NumberOfSubdivisions. This filter implements the
// 8-point butterfly scheme described in: Zorin, D., Schroder, P., and
// Sweldens, W., "Interpolating Subdivisions for Meshes with Arbitrary
// Topology," Computer Graphics Proceedings, Annual Conference Series,
// 1996, ACM SIGGRAPH, pp.189-192. This scheme improves previous
// butterfly subdivisions with special treatment of vertices with valence
// other than 6.
// <P>
// Currently, the filter only operates on triangles. Users should use the
// vtkTriangleFilter to triangulate meshes that contain polygons or
// triangle strips.
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
  vtkButterflySubdivisionFilter(const vtkButterflySubdivisionFilter&);
  void operator=(const vtkButterflySubdivisionFilter&);

 private:
  void GenerateSubdivisionPoints(vtkPolyData *inputDS, vtkIntArray *edgeData,
                                 vtkPoints *outputPts, vtkPointData *outputPD);
  void GenerateButterflyStencil(vtkIdType p1, vtkIdType p2, vtkPolyData *polys,
                                vtkIdList *stencilIds, float *weights);
  void GenerateLoopStencil(vtkIdType p1, vtkIdType p2, vtkPolyData *polys,
                           vtkIdList *stencilIds, float *weights);
  void GenerateBoundaryStencil(vtkIdType p1, vtkIdType p2, vtkPolyData *polys,
                               vtkIdList *stencilIds, float *weights);
};

#endif


