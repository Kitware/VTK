/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangle.h
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
// .NAME vtkTriangle - a cell that represents a triangle
// .SECTION Description
// vtkTriangle is a concrete implementation of vtkCell to represent a triangle
// located in 3-space.

#ifndef __vtkTriangle_h
#define __vtkTriangle_h

#include "vtkCell.h"
#include "vtkMath.h"

class VTK_EXPORT vtkTriangle : public vtkCell
{
public:
  vtkTriangle();
  vtkTriangle(const vtkTriangle& t);
  static vtkTriangle *New() {return new vtkTriangle;};
  const char *GetClassName() {return "vtkTriangle";};

  // cell methods
  vtkCell *MakeObject() {return new vtkTriangle(*this);};
  int GetCellType() {return VTK_TRIANGLE;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 3;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd);
  void Clip(float value, vtkFloatScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd, int insideOut);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList &ptIds, vtkFloatPoints &pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // triangle specific
  static void TriangleCenter(float p1[3], float p2[3], float p3[3], 
                             float center[3]);
  static float TriangleArea(float p1[3], float p2[3], float p3[3]);
  static float Circumcircle(float  p1[2], float p2[2], float p3[2], 
                            float center[2]);
  static int BarycentricCoords(float x[2], float  x1[2], float x2[2], 
                               float x3[2], float bcoords[3]);
  static int ProjectTo2D(float x1[3], float x2[3], float x3[3],
                         float v1[2], float v2[2], float v3[2]);
  static void ComputeNormal(vtkPoints *p, int numPts, int *pts, float n[3]);
  static void ComputeNormal(float v1[3], float v2[3], float v3[3], float n[3]);
  static int PointInTriangle(float x[3], float x1[3], float x2[3], float x3[3], 
                             float tol2);

};

// Description:
// Compute the triangle normal from three points.
inline void vtkTriangle::ComputeNormal(float v1[3], float v2[3], 
                                       float v3[3], float n[3])
{
  float length, ax, ay, az, bx, by, bz;

  // order is important!!! maintain consistency with triangle vertex order 
  ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
  bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

  n[0] = (ay * bz - az * by);
  n[1] = (az * bx - ax * bz);
  n[2] = (ax * by - ay * bx);

  if ( (length = sqrt((n[0]*n[0] + n[1]*n[1] + n[2]*n[2]))) != 0.0 )
    {
    n[0] /= length;
    n[1] /= length;
    n[2] /= length;
    }
  else
    {
    n[0] = 1.0;
    }
}

// Description:
// Compute the center of the triangle.
inline void vtkTriangle::TriangleCenter(float p1[3], float p2[3], float p3[3],
                                       float center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]) / 3.0;
  center[1] = (p1[1]+p2[1]+p3[1]) / 3.0;
  center[2] = (p1[2]+p2[2]+p3[2]) / 3.0;
}

// Description:
// Compute the area of a triangle in 3D.
inline float vtkTriangle::TriangleArea(float p1[3], float p2[3], float p3[3])
{
  float a,b,c;
  a = vtkMath::Distance2BetweenPoints(p1,p2);
  b = vtkMath::Distance2BetweenPoints(p2,p3);
  c = vtkMath::Distance2BetweenPoints(p3,p1);
  return (0.25* sqrt(fabs((double)4.0*a*c - (a-b+c)*(a-b+c))));
} 

#endif


