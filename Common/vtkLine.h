/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLine.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkLine - cell represents a 1D line
// .SECTION Description
// vtkLine is a concrete implementation of vtkCell to represent a 1D line.

#ifndef __vtkLine_h
#define __vtkLine_h

#include "vtkCell.h"

class VTK_COMMON_EXPORT vtkLine : public vtkCell
{
public:
  static vtkLine *New();
  vtkTypeMacro(vtkLine,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3], 
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Clip this line using scalar value provided. Like contouring, except
  // that it cuts the line to produce other lines.
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-line intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);


  // Description:
  // Performs intersection of two finite 3D lines. An intersection is found if
  // the projection of the two lines onto the plane perpendicular to the cross
  // product of the two lines intersect. The parameters (u,v) are the 
  // parametric coordinates of the lines at the position of closest approach.
  static int Intersection(float p1[3], float p2[3], float x1[3], float x2[3],
                          float& u, float& v);

  
  // Description:
  // Compute distance to finite line. Returns parametric coordinate t 
  // and point location on line.
  static float DistanceToLine(float x[3], float p1[3], float p2[3], 
                              float &t, float closestPoint[3]);
  
  
  // Description:
  // Determine the distance of the current vertex to the edge defined by
  // the vertices provided.  Returns distance squared. Note: line is assumed
  // infinite in extent.
  static float DistanceToLine(float x[3], float p1[3], float p2[3]);

  // Description:
  // Line specific methods.
  static void InterpolationFunctions(float pcoords[3], float weights[2]);

protected:
  vtkLine();
  ~vtkLine() {};

private:
  vtkLine(const vtkLine&);  // Not implemented.
  void operator=(const vtkLine&);  // Not implemented.
};

#endif


