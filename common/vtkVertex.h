/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertex.h
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
// .NAME vtkVertex - a cell that represents a 3D point
// .SECTION Description
// vtkVertex is a concrete implementation of vtkCell to represent a 3D point.

#ifndef __vtkVertex_h
#define __vtkVertex_h

#include "vtkCell.h"

class VTK_EXPORT vtkVertex : public vtkCell
{
public:
  vtkVertex();
  static vtkVertex *New() {return new vtkVertex;};
  const char *GetClassName() {return "vtkVertex";};
  
  // Description:
  // Make a new vtkVertex object with the same information as this object.
  vtkCell *MakeObject();

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_VERTEX;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  void Clip(float value, vtkScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *pts,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, int cellId, vtkCellData *outCd, int insideOut);
  int EvaluatePosition(float x[3], float closestPoint[3], 
                       int& subId, float pcoords[3], 
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);

  // Description:
  // Given parametric coordinates of a point, return the closest cell
  // boundary, and whether the point is inside or outside of the cell. The
  // cell boundary is defined by a list of points (pts) that specify a vertex
  // (1D cell).  If the return value of the method is != 0, then the point is
  // inside the cell.
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);

  // Description:
  // Generate contouring primitives. The scalar list cellScalars are
  // scalar values at each cell point. The point locator is essentially a 
  // points list that merges points as they are inserted (i.e., prevents 
  // duplicates). 
  void Contour(float value, vtkScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts1, 
               vtkCellArray *lines, vtkCellArray *verts2, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, int cellId, vtkCellData *outCd);

  // Description:
  // Intersect with a ray. Return parametric coordinates (both line and cell)
  // and global intersection coordinates, given ray definition and tolerance. 
  // The method returns non-zero value if intersection occurs.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  
  // Description:
  // Triangulate the vertex. This method fills pts and ptIds with information
  // from the only point in the vertex.
  int Triangulate(int index, vtkIdList &ptIds, vtkPoints &pts);

  // Description:
  // Get the derivative of the vertex. Returns (0.0, 0.0, 0.0) for all 
  // dimensions.
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // For legacy compatability. Do not use.
  int CellBoundary(int subId, float pcoords[3], vtkIdList &pts)
    {return this->CellBoundary(subId, pcoords, &pts);}
};

#endif


