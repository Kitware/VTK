/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTetra.h
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
// .NAME vtkTetra - a 3D cell that represents a tetrahedron
// .SECTION Description
// vtkTetra is a concrete implementation of vtkCell to represent a 3D
// tetrahedron.

#ifndef __vtkTetra_h
#define __vtkTetra_h

#include "vtkCell3D.h"
#include "vtkLine.h"
#include "vtkTriangle.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkTetra : public vtkCell3D
{
public:
  static vtkTetra *New();
  vtkTypeMacro(vtkTetra,vtkCell);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_TETRA;}
  int GetNumberOfEdges() {return 6;}
  int GetNumberOfFaces() {return 4;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
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
            vtkPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
            int insideOut);
  virtual void Clip(float value, vtkScalars *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut)
    {
      VTK_LEGACY_METHOD("Clip", "4.0");
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
  // Returns the set of points that are on the boundary of the tetrahedron that
  // are closest parametrically to the point specified. This may include faces,
  // edges, or vertices.
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  
  // Description:
  // Return the center of the tetrahedron in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // Compute the center of the tetrahedron,
  static void TetraCenter(float p1[3], float p2[3], float p3[3], float p4[3], 
                          float center[3]);

  // Description:
  // Compute the circumcenter (center[3]) and radius (method return value) of
  // a tetrahedron defined by the four points x1, x2, x3, and x4.
  static double Circumsphere(double  p1[3], double p2[3], double p3[3], 
                             double p4[3], double center[3]);

  // Description:
  // Given a 3D point x[3], determine the barycentric coordinates of the point.
  // Barycentric coordinates are a natural coordinate system for simplices that
  // express a position as a linear combination of the vertices. For a 
  // tetrahedron, there are four barycentric coordinates (because there are
  // four vertices), and the sum of the coordinates must equal 1. If a 
  // point x is inside a simplex, then all four coordinates will be strictly 
  // positive.  If three coordinates are zero (so the fourth =1), then the 
  // point x is on a vertex. If two coordinates are zero, the point x is on an 
  // edge (and so on). In this method, you must specify the vertex coordinates
  // x1->x4. Returns 0 if tetrahedron is degenerate.
  static int BarycentricCoords(double x[3], double  x1[3], double x2[3], 
                               double x3[3], double x4[3], double bcoords[4]);
  
  // Description:
  // Compute the volume of a tetrahedron defined by the four points
  // p1, p2, p3, and p4.
  static double ComputeVolume(double  p1[3], double p2[3], double p3[3], 
                              double p4[3]);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives. Returns 0 if no inverse exists.
  int JacobianInverse(double **inverse, float derivs[12]);

  // Description:
  // Tetra specific methods.
  static void InterpolationFunctions(float pcoords[3], float weights[4]);
  static void InterpolationDerivs(float derivs[12]);
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

protected:
  vtkTetra();
  ~vtkTetra();
  vtkTetra(const vtkTetra&) {};
  void operator=(const vtkTetra&) {};

  vtkLine *Line;
  vtkTriangle *Triangle;

};

inline int vtkTetra::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

#endif



