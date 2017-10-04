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
// .NAME vtkLagrangeTriangle
// .SECTION Description
// .SECTION See Also

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
  vtkCell *GetFace(int) override { return 0; }

  void Initialize() override;

  static int MaximumOrder() { return VTK_LAGRANGE_TRIANGLE_MAX_ORDER; }
  static int MaximumNumberOfPoints()
  {
    return ((vtkLagrangeTriangle::MaximumOrder() + 1) *
            (vtkLagrangeTriangle::MaximumOrder() + 2)/2);
  }

  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) override;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) override;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
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
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void JacobianInverse(double pcoords[3], double** inverse, double* derivs);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) override;
  double* GetParametricCoords() override;
  static void ComputeParametricCoords(double*,vtkIdType);

  int GetParametricCenter(double pcoords[3]) override;
  double GetParametricDistance(double pcoords[3]) override;

  void InterpolateFunctions(double pcoords[3], double* weights) override;
  void InterpolateDerivs(double pcoords[3], double* derivs) override;

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
