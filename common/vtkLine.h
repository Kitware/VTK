/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLine.h
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
// .NAME vtkLine - cell represents a 1D line
// .SECTION Description
// vtkLine is a concrete implementation of vtkCell to represent a 1D line.

#ifndef __vtkLine_h
#define __vtkLine_h

#include "vtkCell.h"

class VTK_EXPORT vtkLine : public vtkCell
{
public:
  vtkLine();
  static vtkLine *New() {return new vtkLine;};
  const char *GetClassName() {return "vtkLine";};

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, int cellId, vtkCellData *outCd);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3], 
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int Triangulate(int index, vtkIdList &ptIds, vtkPoints &pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Clip this line using scalar value provided. Like contouring, except
  // that it cuts the line to produce other lines.
  void Clip(float value, vtkScalars *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, int cellId, vtkCellData *outCd, int insideOut);

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
  //
  static float DistanceToLine(float x[3], float p1[3], float p2[3]);


};

#endif


