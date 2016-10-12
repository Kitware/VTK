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
/**
 * @class   vtkLine
 * @brief   cell represents a 1D line
 *
 * vtkLine is a concrete implementation of vtkCell to represent a 1D line.
*/

#ifndef vtkLine_h
#define vtkLine_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkLine : public vtkCell
{
public:
  static vtkLine *New();
  vtkTypeMacro(vtkLine,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_LINE;};
  int GetCellDimension() VTK_OVERRIDE {return 1;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int) VTK_OVERRIDE  {return 0;};
  vtkCell *GetFace(int) VTK_OVERRIDE  {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  /**
   * Clip this line using scalar value provided. Like contouring, except
   * that it cuts the line to produce other lines.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Line-line intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;


  /**
   * Performs intersection of the projection of two finite 3D lines onto a 2D
   * plane. An intersection is found if the projection of the two lines onto
   * the plane perpendicular to the cross product of the two lines intersect.
   * The parameters (u,v) are the parametric coordinates of the lines at the
   * position of closest approach.
   */
  static int Intersection(double p1[3], double p2[3],
                          double x1[3], double x2[3],
                          double& u, double& v);


  /**
   * Performs intersection of two finite 3D lines. An intersection is found if
   * the projection of the two lines onto the plane perpendicular to the cross
   * product of the two lines intersect, and if the distance between the
   * closest points of approach are within a relative tolerance. The parameters
   * (u,v) are the parametric coordinates of the lines at the position of
   * closest approach.

   * NOTE: "Unlike Intersection(), which determines whether the projections of
   * two lines onto a plane intersect, Intersection3D() determines whether the
   * lines themselves in 3D space intersect, within a tolerance.
   */
  static int Intersection3D(double p1[3], double p2[3],
                            double x1[3], double x2[3],
                            double& u, double& v);


  /**
   * Compute the distance of a point x to a finite line (p1,p2). The method
   * computes the parametric coordinate t and the point location on the
   * line. Note that t is unconstrained (i.e., it may lie outside the range
   * [0,1]) but the closest point will lie within the finite line [p1,p2], if
   * it is defined. Also, the method returns the distance squared between x and
   * the line (p1,p2).
   */
  static double DistanceToLine(double x[3], double p1[3], double p2[3],
                              double &t, double* closestPoint=NULL);


  /**
   * Determine the distance of the current vertex to the edge defined by
   * the vertices provided.  Returns distance squared. Note: line is assumed
   * infinite in extent.
   */
  static double DistanceToLine(double x[3], double p1[3], double p2[3]);

  /**
   * Computes the shortest distance squared between two infinite lines, each
   * defined by a pair of points (l0,l1) and (m0,m1).
   * Upon return, the closest points on the two line segments will be stored
   * in closestPt1 and closestPt2. Their parametric coords
   * (-inf <= t0, t1 <= inf) will be stored in t0 and t1. The return value is
   * the shortest distance squared between the two line-segments.
   */
  static double DistanceBetweenLines(
                double l0[3], double l1[3],
                double m0[3], double m1[3],
                double closestPt1[3], double closestPt2[3],
                double &t1, double &t2 );

  /**
   * Computes the shortest distance squared between two finite line segments
   * defined by their end points (l0,l1) and (m0,m1).
   * Upon return, the closest points on the two line segments will be stored
   * in closestPt1 and closestPt2. Their parametric coords (0 <= t0, t1 <= 1)
   * will be stored in t0 and t1. The return value is the shortest distance
   * squared between the two line-segments.
   */
  static double DistanceBetweenLineSegments(
                double l0[3], double l1[3],
                double m0[3], double m1[3],
                double closestPt1[3], double closestPt2[3],
                double &t1, double &t2 );

  /**
   * @deprecated Replaced by vtkLine::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[2]);
  /**
   * @deprecated Replaced by vtkLine::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[2]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[2]) VTK_OVERRIDE
  {
    vtkLine::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[2]) VTK_OVERRIDE
  {
    vtkLine::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkLine();
  ~vtkLine() VTK_OVERRIDE {}

private:
  vtkLine(const vtkLine&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLine&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkLine::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5;
  pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

#endif
