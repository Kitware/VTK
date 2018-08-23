/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangeWedge
 * @brief   A 3D cell that represents an arbitrary order Lagrange wedge
 *
 * vtkLagrangeWedge is a concrete implementation of vtkCell to represent a
 * 3D wedge using Lagrange shape functions of user specified order.
 * A wedge consists of two triangular and three quadrilateral faces.
 * The first six points of the wedge (0-5) are the "corner" points
 * where the first three points are the base of the wedge. This wedge
 * point ordering is opposite the vtkWedge ordering though in that
 * the base of the wedge defined by the first three points (0,1,2) form
 * a triangle whose normal points inward (toward the triangular face (3,4,5)).
 * While this is opposite the vtkWedge convention it is consistent with
 * every other cell type in VTK. The first 2 parametric coordinates of the
 * Lagrange wedge or for the triangular base and vary between 0 and 1. The
 * third parametric coordinate is between the two triangular faces and goes
 * from 0 to 1 as well.
*/

#ifndef vtkLagrangeWedge_h
#define vtkLagrangeWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.
#include "vtkCellType.h" // For GetCellType.
#include "vtkNew.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkWedge;
class vtkIdList;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;
class vtkLagrangeCurve;
class vtkLagrangeInterpolation;
class vtkLagrangeQuadrilateral;
class vtkLagrangeTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeWedge : public vtkNonLinearCell
{
public:
  static vtkLagrangeWedge* New();
  vtkTypeMacro(vtkLagrangeWedge,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_LAGRANGE_WEDGE; }
  int GetCellDimension() override { return 3; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 9; }
  int GetNumberOfFaces() override { return 5; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;

  void Initialize() override;

  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  int EvaluatePosition(const double x[3], double closestPoint[3],
    int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(
    int& subId, const double pcoords[3], double x[3],
    double* weights) override;
  void Contour(
    double value, vtkDataArray* cellScalars,
    vtkIncrementalPointLocator* locator, vtkCellArray* verts,
    vtkCellArray* lines, vtkCellArray* polys,
    vtkPointData* inPd, vtkPointData* outPd,
    vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(
    double value, vtkDataArray* cellScalars,
    vtkIncrementalPointLocator* locator, vtkCellArray* polys,
    vtkPointData* inPd, vtkPointData* outPd,
    vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd,
    int insideOut) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values,
    int dim, double* derivs) override;
  double* GetParametricCoords() override;
  int GetParametricCenter(double center[3]) override;

  double GetParametricDistance(const double pcoords[3]) override;

  const int* GetOrder();
  int GetOrder(int i) { return this->GetOrder()[i]; }

  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  bool SubCellCoordinatesFromId(vtkVector3i& ijk, int subId);
  bool SubCellCoordinatesFromId(int& i, int& j, int& k, int subId);
  static int PointIndexFromIJK(int i, int j, int k, const int* order);
  int PointIndexFromIJK(int i, int j, int k);
  bool TransformApproxToCellParams(int subCell, double* pcoords);
  bool TransformFaceToCellParams(int bdyFace, double* pcoords);

  static int GetNumberOfApproximatingWedges(const int* order);
  int GetNumberOfApproximatingWedges()
  { return vtkLagrangeWedge::GetNumberOfApproximatingWedges(this->GetOrder()); }

protected:
  vtkLagrangeWedge();
  ~vtkLagrangeWedge() override;

  vtkWedge* GetApprox();
  void PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  vtkWedge* GetApproximateWedge(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr);

  vtkLagrangeTriangle* GetTriangularFace(int iAxis, int k);
  vtkLagrangeQuadrilateral* GetQuadrilateralFace(int di, int dj);

  int Order[4];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkWedge> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;
  vtkNew<vtkPoints> TmpPts;
  vtkNew<vtkIdList> TmpIds;
  vtkNew<vtkLagrangeQuadrilateral> BdyQuad;
  vtkNew<vtkLagrangeTriangle> BdyTri;
  vtkNew<vtkLagrangeCurve> BdyEdge;
  vtkNew<vtkLagrangeInterpolation> Interp;

private:
  vtkLagrangeWedge(const vtkLagrangeWedge&) = delete;
  void operator=(const vtkLagrangeWedge&) = delete;
};

inline int vtkLagrangeWedge::GetParametricCenter(double center[3])
{
  center[0] = center[1] = 1./3.;
  center[2] = 0.5;
  return 0;
}

#endif // vtkLagrangeWedge_h
