/*========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.h
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
// .NAME vtkPolyLine - cell represents a set of 1D lines
// .SECTION Description
// vtkPolyLine is a concrete implementation of vtkCell to represent a set
// of 1D lines.

#ifndef __vtkPolyLine_h
#define __vtkPolyLine_h

#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkNormals.h"
#include "vtkLine.h"

class VTK_EXPORT vtkPolyLine : public vtkCell
{
public:
  vtkPolyLine();
  ~vtkPolyLine();
  static vtkPolyLine *New() {return new vtkPolyLine;};
  const char *GetClassName() {return "vtkPolyLine";};

  // Description:
  // Given points and lines, compute normals to lines. These are not true 
  // normals, they are "orientation" normals used by classes like vtkTubeFilter
  // that control the rotation around the line. The normals try to stay pointing
  // in the same direction as much as possible (i.e., minimal rotation).
  int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkNormals *);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_POLY_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int vtkNotUsed(edgeId)) {return 0;};
  vtkCell *GetFace(int vtkNotUsed(faceId)) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, int cellId, vtkCellData *outCd);
  void Clip(float value, vtkScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *lines,
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

  // Description:
  // Return the center of the point cloud in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // For legacy compatability. Do not use.
  int CellBoundary(int subId, float pcoords[3], vtkIdList &pts)
    {return this->CellBoundary(subId, pcoords, &pts);}

protected:
  vtkLine *Line;

};

#endif


