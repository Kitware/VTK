/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygon.h
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
// .NAME vtkPolygon - a cell that represents an n-sided polygon
// .SECTION Description
// vtkPolygon is a concrete implementation of vtkCell to represent a 2D 
// n-sided polygon. The polygons cannot have any internal holes, and cannot
// self-intersect.

#ifndef __vtkPolygon_h
#define __vtkPolygon_h

#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkLine.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"

class VTK_EXPORT vtkPolygon : public vtkCell
{
public:
  static vtkPolygon *New();
  vtkTypeMacro(vtkPolygon,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_POLYGON;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator,vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  virtual void Contour(float value, vtkScalars *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *verts, 
                       vtkCellArray *lines, vtkCellArray *polys, 
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId,
                       vtkCellData *outCd)
    {
      VTK_LEGACY_METHOD("Contour", "4.0");
      this->Contour(value, cellScalars->GetData(), locator, verts, 
		    lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *tris,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  virtual void Clip(float value, vtkScalars *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut)
    {
      vtkWarningMacro("The use of this method has been deprecated.You should use vtkGenericCell::Clip(float, vtkDataArray*, vtkPointLocator*, vtkCellArray*, vtkPointData*, vtkPointData*, vtkCellData*, vtkIdType, vtkCellData*, int) instead.");
      this->Clip(value, cellScalars->GetData(), locator, connectivity, 
		 inPd, outPd, inCd, cellId, outCd, insideOut);
    }
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Polygon specific
  static void ComputeNormal(vtkPoints *p, int numPts, vtkIdType *pts,
                            float n[3]);
  static void ComputeNormal(vtkPoints *p, float n[3]);
  
  // Description:
  // Compute the polygon normal from an array of points. This version assumes
  // that the polygon is convex, and looks for the first valid normal.
  static void ComputeNormal(int numPts, float *pts, float n[3]);

  // Description:
  // Compute interpolation weights using 1/r**2 normalized sum.
  void ComputeWeights(float x[3], float *weights);


  // Description:
  // Create a local s-t coordinate system for a polygon. The point p0 is
  // the origin of the local system, p10 is s-axis vector, and p20 is the 
  // t-axis vector. (These are expressed in the modeling coordinate system and
  // are vectors of dimension [3].) The values l20 and l20 are the lengths of
  // the vectors p10 and p20, and n is the polygon normal.
  int ParameterizePolygon(float p0[3], float p10[3], float &l10, 
                          float p20[3], float &l20, float n[3]);
  
  // Description:
  // Determine whether point is inside polygon. Function uses ray-casting
  // to determine if point is inside polygon. Works for arbitrary polygon shape
  // (e.g., non-convex). Returns 0 if point is not in polygon; 1 if it is.
  // Can also return -1 to indicate degenerate polygon.
  static int PointInPolygon(float x[3], int numPts, float *pts, 
			    float bounds[6], float n[3]);  

  // Description:
  // Triangulate this polygon. The user must provide the vtkIdList outTris.
  // On output, the outTris list contains the ids of the points defining 
  // the triangulation. The ids are ordered into groups of three: each 
  // three-group defines one triangle.
  int Triangulate(vtkIdList *outTris);
  
  // Description:
  // Method intersects two polygons. You must supply the number of points and
  // point coordinates (npts, *pts) and the bounding box (bounds) of the two
  // polygons. Also supply a tolerance squared for controlling
  // error. The method returns 1 if there is an intersection, and 0 if
  // not. A single point of intersection x[3] is also returned if there
  // is an intersection.
  static int IntersectPolygonWithPolygon(int npts, float *pts, float bounds[6],
                                         int npts2, float *pts2, 
                                         float bounds2[3], float tol,
                                         float x[3]);

protected:
  vtkPolygon();
  ~vtkPolygon();
  vtkPolygon(const vtkPolygon&) {};
  void operator=(const vtkPolygon&) {};

  // variables used by instances of this class
  float   Tolerance; // Intersection tolerance
  int     SuccessfulTriangulation; // Stops recursive tri. if necessary
  float   Normal[3]; //polygon normal
  vtkIdList *Tris;
  vtkTriangle *Triangle;
  vtkQuad *Quad;
  vtkFloatArray *TriScalars;
  vtkLine *Line;

  // Helper methods for triangulation------------------------------
  // Description: 
  // A fast triangulation method. Uses recursive divide and 
  // conquer based on plane splitting  to reduce loop into triangles.  
  // The cell (e.g., triangle) is presumed properly initialized (i.e., 
  // Points and PointIds).
  int EarCutTriangulation();

};

#endif

