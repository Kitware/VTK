/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygon.h
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
// .NAME vtkPolygon - a cell that represents an n-sided polygon
// .SECTION Description
// vtkPolygon is a concrete implementation of vtkCell to represent a 2D 
// n-sided polygon. The polygons cannot have any internal holes, and cannot
// self-intersect.

#ifndef __vtkPolygon_h
#define __vtkPolygon_h

#include "vtkCell.h"
#include "vtkPoints.h"

class vtkPolygon : public vtkCell
{
public:
  vtkPolygon() {};
  vtkPolygon(const vtkPolygon& p);
  char *GetClassName() {return "vtkPolygon";};

  // Cell interface
  vtkCell *MakeObject() {return new vtkPolygon(*this);};
  int GetCellType() {return VTK_POLYGON;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkPointLocator *locator,vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkFloatPoints &pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Polygon specific
  static void ComputeNormal(vtkPoints *p, int numPts, int *pts, float n[3]);
  static void ComputeNormal(vtkFloatPoints *p, float n[3]);

  void ComputeWeights(float x[3], float *weights);

  int ParameterizePolygon(float p0[3], float p10[3], float &l10, 
                          float p20[3], float &l20, float n[3]);

  int PointInPolygon(float bounds[6], float x[3], float n[3]);  

  int Triangulate(vtkIdList &outTris);
  int FastTriangulate(int numVerts, int *verts, vtkIdList& Tris);
  int SlowTriangulate(int numVerts, int *verts, float planeNormal[3],
                      vtkIdList& Tris);
  int CanSplitLoop(int fedges[2], int numVerts, int *verts, int& n1, int *l1,
                   int& n2, int *l2, float& ar);
  void SplitLoop (int fedges[2], int numVerts, int *verts, int& n1, int *l1, 
                  int& n2, int* l2);

};

#endif

