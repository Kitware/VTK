/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPyramid.h
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
// .NAME vtkPyramid - a 3D cell that represents a pyramid
// .SECTION Description
// vtkPyramid is a concrete implementation of vtkCell to represent a 3D
// pyramid. A pyramid consists of a rectangular base with four triangular
// faces.

#ifndef __vtkPyramid_h
#define __vtkPyramid_h

#include "vtkCell3D.h"
#include "vtkLine.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"

class vtkUnstructuredGrid;

class VTK_EXPORT vtkPyramid : public vtkCell3D
{
public:
  vtkTypeMacro(vtkPyramid,vtkCell);

  // Description:
  // Create an instance of this class.
  static vtkPyramid *New();

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_PYRAMID;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 8;}
  int GetNumberOfFaces() {return 5;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
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
  // Return the center of the pyramid in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // Pyramid specific methods for computing interpolation functions and
  // derivatives.
  static void InterpolationFunctions(float pcoords[3], float weights[5]);
  static void InterpolationDerivs(float pcoords[3], float derivs[15]);
  int JacobianInverse(float pcoords[3], double **inverse, float derivs[15]);
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

protected:
  vtkPyramid();
  ~vtkPyramid();
  vtkPyramid(const vtkPyramid&) {};
  void operator=(const vtkPyramid&) {};

  vtkLine *Line;
  vtkTriangle *Triangle;
  vtkQuad *Quad;

};

inline int vtkPyramid::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.333333;
  return 0;
}

#endif



