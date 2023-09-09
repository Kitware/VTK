// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkHigherOrderQuadrilateral
// .SECTION Description
// .SECTION See Also

#ifndef vtkHigherOrderQuadrilateral_h
#define vtkHigherOrderQuadrilateral_h

#include <functional> //For std::function

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For member variable.
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkHigherOrderCurve;
class vtkPointData;
class vtkPoints;
class vtkQuad;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkHigherOrderQuadrilateral : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkHigherOrderQuadrilateral, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override = 0;
  int GetCellDimension() override { return 2; }
  int RequiresInitialization() override { return 0; }
  int GetNumberOfEdges() override { return 4; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int edgeId) override = 0;
  vtkCell* GetFace(int vtkNotUsed(faceId)) override { return nullptr; }
  void SetEdgeIdsAndPoints(int edgeId,
    const std::function<void(const vtkIdType&)>& set_number_of_ids_and_points,
    const std::function<void(const vtkIdType&, const vtkIdType&)>& set_ids_and_points);

  void Initialize() override;

  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int TriangulateLocalIds(int index, vtkIdList* ptId) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  void SetParametricCoords();
  double* GetParametricCoords() override;
  int GetParametricCenter(double center[3]) override;

  double GetParametricDistance(const double pcoords[3]) override;

  virtual void SetOrderFromCellData(vtkCellData* cell_data, vtkIdType numPts, vtkIdType cell_id);
  static void SetOrderFromCellData(
    vtkCellData* cell_data, vtkIdType numPts, vtkIdType cell_id, int* order);
  virtual void SetUniformOrderFromNumPoints(vtkIdType numPts);
  virtual void SetOrder(int s, int t);
  virtual const int* GetOrder();
  virtual int GetOrder(int i) { return this->GetOrder()[i]; }
  /// Return true if the number of points supports a cell of uniform
  /// degree along each axis.
  ///
  /// For quadrilaterals, \a pointsPerCell must be a perfect square >= 4.
  static bool PointCountSupportsUniformOrder(vtkIdType pointsPerCell);

  void InterpolateFunctions(const double pcoords[3], double* weights) override = 0;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override = 0;

  bool SubCellCoordinatesFromId(vtkVector3i& ijk, int subId);
  bool SubCellCoordinatesFromId(int& i, int& j, int& k, int subId);
  int PointIndexFromIJK(int i, int j, int k);
  static int PointIndexFromIJK(int i, int j, const int* order);
  bool TransformApproxToCellParams(int subCell, double* pcoords);

  virtual vtkHigherOrderCurve* GetEdgeCell() = 0;

protected:
  vtkHigherOrderQuadrilateral();
  ~vtkHigherOrderQuadrilateral() override;

  vtkQuad* GetApprox();
  // The version of GetApproximateQuad between Lagrange and Bezier is different because Bezier is
  // non-interpolatory
  void PrepareApproxData(
    vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  virtual vtkQuad* GetApproximateQuad(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) = 0;

  int Order[3];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkQuad> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;

private:
  vtkHigherOrderQuadrilateral(const vtkHigherOrderQuadrilateral&) = delete;
  void operator=(const vtkHigherOrderQuadrilateral&) = delete;
};

inline int vtkHigherOrderQuadrilateral::GetParametricCenter(double center[3])
{
  center[0] = center[1] = 0.5;
  center[2] = 0.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif // vtkHigherOrderQuadrilateral_h
