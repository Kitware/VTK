/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxel.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkVoxel - a cell that represents a 3D orthogonal parallelepiped
// .SECTION Description
// vtkVoxel is a concrete implementation of vtkCell to represent a 3D
// orthogonal parallelepiped. Unlike vtkHexahedron, vtkVoxel has interior angles
// of 90 degrees, and sides are parallel to coordinate axes. This results 
// in large increases in computational performance.

#ifndef __vtkVoxel_h
#define __vtkVoxel_h

#include "vtkCell.h"

class VTK_EXPORT vtkVoxel : public vtkCell
{
public:
  vtkVoxel();
  vtkVoxel(const vtkVoxel& b);
  static vtkVoxel *New() {return new vtkVoxel;};
  const char *GetClassName() {return "vtkVoxel";};

  // cell methods
  vtkCell *MakeObject() {return new vtkVoxel(*this);};
  int GetCellType() {return VTK_VOXEL;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 12;};
  int GetNumberOfFaces() {return 6;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd);
  void Clip(float value, vtkFloatScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *tetras,
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

  // voxel specific
  static void InterpolationFunctions(float pcoords[3], float weights[8]);
  static void InterpolationDerivs(float pcoords[3], float derivs[24]);

};

#endif


