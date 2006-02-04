/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLine - cell represents a 1D line
// .SECTION Description
// vtkLine is a concrete implementation of vtkCell to represent a 1D line.

#ifndef __vtkLine_h
#define __vtkLine_h

#include "vtkCell.h"

class VTK_FILTERING_EXPORT vtkLine : public vtkCell
{
public:
  static vtkLine *New();
  vtkTypeRevisionMacro(vtkLine,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3], 
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values, 
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // Clip this line using scalar value provided. Like contouring, except
  // that it cuts the line to produce other lines.
  void Clip(double value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-line intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);


  // Description:
  // Performs intersection of two finite 3D lines. An intersection is found if
  // the projection of the two lines onto the plane perpendicular to the cross
  // product of the two lines intersect. The parameters (u,v) are the 
  // parametric coordinates of the lines at the position of closest approach.
  static int Intersection(double p1[3], double p2[3], 
                          double x1[3], double x2[3],
                          double& u, double& v);

  
  // Description:
  // Compute the distance of a point x to a finite line (p1,p2). The method
  // computes the parametric coordinate t and the point location on the
  // line. Note that t is unconstrained (i.e., it may lie outside the range
  // [0,1]) but the closest point will lie within the finite line
  // [p1,p2]. Also, the method returns the distance squared between x and the
  // line (p1,p2).
  static double DistanceToLine(double x[3], double p1[3], double p2[3], 
                              double &t, double closestPoint[3]);
  
  
  // Description:
  // Determine the distance of the current vertex to the edge defined by
  // the vertices provided.  Returns distance squared. Note: line is assumed
  // infinite in extent.
  static double DistanceToLine(double x[3], double p1[3], double p2[3]);

  // Description:
  // Line specific methods.
  static void InterpolationFunctions(double pcoords[3], double weights[2]);

protected:
  vtkLine();
  ~vtkLine() {};

private:
  vtkLine(const vtkLine&);  // Not implemented.
  void operator=(const vtkLine&);  // Not implemented.
};

#endif


