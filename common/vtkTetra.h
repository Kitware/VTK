/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTetra.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkTetra - a 3D cell that represents a tetrahedron
// .SECTION Description
// vtkTetra is a concrete implementation of vtkCell to represent a 3D
// tetrahedron.

#ifndef __vtkTetra_h
#define __vtkTetra_h

#include "vtkCell.h"
#include "vtkLine.h"
#include "vtkTriangle.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkTetra : public vtkCell
{
public:

// Description:
// Construct the tetra with four points.
  vtkTetra();

  static vtkTetra *New() {return new vtkTetra;};
  const char *GetClassName() {return "vtkTetra";};

  // cell methods
  vtkCell *MakeObject();
  int GetCellType() {return VTK_TETRA;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 6;};
  int GetNumberOfFaces() {return 4;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);


// Description:
// Returns the set of points that are on the boundary of the tetrahedron that
// are closest parametrically to the point specified. This may include faces,
// edges, or vertices.
  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);

  void Contour(float value, vtkScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, int cellId, vtkCellData *outCd);

// Description:
// Clip this tetra using scalar value provided. Like contouring, except
// that it cuts the tetra to produce other tetrahedra.
  void Clip(float value, vtkScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *tetras,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, int cellId, vtkCellData *outCd, int insideOut);

  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList &ptIds, vtkPoints &pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);
  int GetParametricCenter(float pcoords[3]);

  // tetrahedron specific

// Description:
// Compute the center of the tetrahedron,
  static void TetraCenter(float p1[3], float p2[3], float p3[3], float p4[3], 
                          float center[3]);


// Description:
// Compute the circumcenter (center[3]) and radius (method return value) of
// a tetrahedron defined by the four points x1, x2, x3, and x4.
  static float Circumsphere(float  p1[3], float p2[3], float p3[3], 
                            float p4[3], float center[3]);


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
  static int BarycentricCoords(float x[3], float  x1[3], float x2[3], 
                               float x3[3], float x4[3], float bcoords[4]);

  
  static void InterpolationFunctions(float pcoords[3], float weights[4]);
  static void InterpolationDerivs(float derivs[12]);

// Description:
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives. Returns 0 if no inverse exists.
  int JacobianInverse(double **inverse, float derivs[12]);


protected:
  vtkLine Line;
  vtkTriangle Triangle;

};

// Description:
// Return the center of the triangle in parametric coordinates.
inline int vtkTetra::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

#endif



