/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangeTriangle
 * @brief   A 2D cell that represents an arbitrary order Lagrange triangle
 *
 * vtkLagrangeTriangle is a concrete implementation of vtkCell to represent a
 * 2D triangle using Lagrange shape functions of user specified order.
 *
 * The number of points in a Lagrange cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
*/

#ifndef vtkLagrangeTriangle_h
#define vtkLagrangeTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

#define VTK_LAGRANGE_TRIANGLE_MAX_ORDER 6

#define MAX_POINTS ((VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 1) *     \
                    (VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 2)/2)

#define MAX_SUBTRIANGLES (VTK_LAGRANGE_TRIANGLE_MAX_ORDER *     \
                          VTK_LAGRANGE_TRIANGLE_MAX_ORDER)

class vtkDoubleArray;
class vtkLagrangeCurve;
class vtkTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeTriangle : public vtkNonLinearCell
{
public:
  static vtkLagrangeTriangle *New();
  vtkTypeMacro(vtkLagrangeTriangle,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_LAGRANGE_TRIANGLE; }
  int GetCellDimension() override { return 2; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 3; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int) override { return nullptr; }

  void Initialize() override;

  static int MaximumOrder() { return VTK_LAGRANGE_TRIANGLE_MAX_ORDER; }
  static int MaximumNumberOfPoints()
  {
    return ((vtkLagrangeTriangle::MaximumOrder() + 1) *
            (vtkLagrangeTriangle::MaximumOrder() + 2)/2);
  }

  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;
  int EvaluatePosition(const double x[3], double closestPoint[3],
                       int& subId, double pcoords[3],
                       double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3],
                        double *weights) override;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void JacobianInverse(const double pcoords[3], double** inverse, double* derivs);
  void Derivatives(int subId, const double pcoords[3], const double *values,
                   int dim, double *derivs) override;
  double* GetParametricCoords() override;
  static void ComputeParametricCoords(double*,vtkIdType);

  int GetParametricCenter(double pcoords[3]) override;
  double GetParametricDistance(const double pcoords[3]) override;

  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  vtkIdType GetOrder() const { return this->Order; }
  vtkIdType ComputeOrder();

  void ToBarycentricIndex(vtkIdType index, vtkIdType* bindex);
  vtkIdType ToIndex(const vtkIdType* bindex);

  static void BarycentricIndex(vtkIdType index, vtkIdType* bindex,
                               vtkIdType order);
  static vtkIdType Index(const vtkIdType* bindex, vtkIdType order);

  static double eta(vtkIdType n, vtkIdType chi, double sigma);
  static double d_eta(vtkIdType n, vtkIdType chi,double sigma);

protected:
  vtkLagrangeTriangle();
  ~vtkLagrangeTriangle() override;

  vtkIdType GetNumberOfSubtriangles() const {return this->NumberOfSubtriangles;}
  vtkIdType ComputeNumberOfSubtriangles();

  // Description:
  // Given the index of the subtriangle, compute the barycentric indices of
  // the subtriangle's vertices.
  void SubtriangleBarycentricPointIndices(vtkIdType cellIndex,
                                          vtkIdType (&pointBIndices)[3][3]);

  vtkLagrangeCurve *Edge;
  vtkTriangle *Face;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping
  vtkIdType Order;
  vtkIdType NumberOfSubtriangles;
  double* ParametricCoordinates;

  vtkIdType EdgeIds[VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 1];
  vtkIdType BarycentricIndexMap[3*MAX_POINTS];
  vtkIdType IndexMap[(VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 1) *
                     (VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 1)];
  vtkIdType SubtriangleIndexMap[9*MAX_SUBTRIANGLES];

private:
  vtkLagrangeTriangle(const vtkLagrangeTriangle&) = delete;
  void operator=(const vtkLagrangeTriangle&) = delete;
};

#undef MAX_POINTS
#undef MAX_SUBTRIANGLES

#endif
