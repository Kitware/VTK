/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTetra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTetra
 * @brief   a 3D cell that represents a tetrahedron
 *
 * vtkTetra is a concrete implementation of vtkCell to represent a 3D
 * tetrahedron. vtkTetra uses the standard isoparametric shape functions
 * for a linear tetrahedron. The tetrahedron is defined by the four points
 * (0-3); where (0,1,2) is the base of the tetrahedron which, using the
 * right hand rule, forms a triangle whose normal points in the direction
 * of the fourth point.
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkVoxel vtkWedge
*/

#ifndef vtkTetra_h
#define vtkTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkTriangle;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkTetra : public vtkCell3D
{
public:
  static vtkTetra *New();
  vtkTypeMacro(vtkTetra,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int edgeId, int* &pts) VTK_OVERRIDE;
  void GetFacePoints(int faceId, int* &pts) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_TETRA;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 6;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 4;}
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  /**
   * Returns the set of points that are on the boundary of the tetrahedron that
   * are closest parametrically to the point specified. This may include faces,
   * edges, or vertices.
   */
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;

  /**
   * Return the center of the tetrahedron in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Compute the center of the tetrahedron,
   */
  static void TetraCenter(double p1[3], double p2[3], double p3[3], double p4[3],
                          double center[3]);

  /**
   * Compute the circumcenter (center[3]) and radius squared (method
   * return value) of a tetrahedron defined by the four points x1, x2,
   * x3, and x4.
   */
  static double Circumsphere(double  p1[3], double p2[3], double p3[3],
                             double p4[3], double center[3]);

  /**
   * Compute the center (center[3]) and radius (method return value) of
   * a sphere that just fits inside the faces of a tetrahedron defined
   * by the four points x1, x2, x3, and x4.
   */
  static double Insphere(double  p1[3], double p2[3], double p3[3],
                         double p4[3], double center[3]);

  /**
   * Given a 3D point x[3], determine the barycentric coordinates of the point.
   * Barycentric coordinates are a natural coordinate system for simplices that
   * express a position as a linear combination of the vertices. For a
   * tetrahedron, there are four barycentric coordinates (because there are
   * four vertices), and the sum of the coordinates must equal 1. If a
   * point x is inside a simplex, then all four coordinates will be strictly
   * positive.  If three coordinates are zero (so the fourth =1), then the
   * point x is on a vertex. If two coordinates are zero, the point x is on an
   * edge (and so on). In this method, you must specify the vertex coordinates
   * x1->x4. Returns 0 if tetrahedron is degenerate.
   */
  static int BarycentricCoords(double x[3], double  x1[3], double x2[3],
                               double x3[3], double x4[3], double bcoords[4]);

  /**
   * Compute the volume of a tetrahedron defined by the four points
   * p1, p2, p3, and p4.
   */
  static double ComputeVolume(double  p1[3], double p2[3], double p3[3],
                              double p4[3]);

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives. Returns 0 if no inverse exists.
   */
  int JacobianInverse(double **inverse, double derivs[12]);

  /**
   * @deprecated Replaced by vtkTetra::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[4]);
  /**
   * @deprecated Replaced by vtkTetra::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[12]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[4]) VTK_OVERRIDE
  {
    vtkTetra::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[12]) VTK_OVERRIDE
  {
    vtkTetra::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);
  //@}

protected:
  vtkTetra();
  ~vtkTetra() VTK_OVERRIDE;

  vtkLine *Line;
  vtkTriangle *Triangle;

private:
  vtkTetra(const vtkTetra&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTetra&) VTK_DELETE_FUNCTION;
};

inline int vtkTetra::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

#endif



