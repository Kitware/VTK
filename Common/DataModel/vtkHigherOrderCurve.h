// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkHigherOrderCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkHigherOrderCurve_h
#define vtkHigherOrderCurve_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For member variable.
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLine;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkHigherOrderCurve : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkHigherOrderCurve, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override = 0;
  int GetCellDimension() override { return 1; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 0; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int) override { return nullptr; }
  vtkCell* GetFace(int) override { return nullptr; }

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
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  void SetParametricCoords();
  double* GetParametricCoords() override;
  int GetParametricCenter(double center[3]) override;

  double GetParametricDistance(const double pcoords[3]) override;

  const int* GetOrder();
  int GetOrder(int i) { return this->GetOrder()[i]; }
  /// Return true if the number of points supports a cell of uniform
  /// degree along each axis.
  ///
  /// For curves, this is trivially true when \a pointsPerCell >= 2.
  static bool PointCountSupportsUniformOrder(vtkIdType pointsPerCell);

  void InterpolateFunctions(const double pcoords[3], double* weights) override = 0;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override = 0;

  bool SubCellCoordinatesFromId(vtkVector3i& ijk, int subId);
  bool SubCellCoordinatesFromId(int& i, int subId);
  int PointIndexFromIJK(int i, int, int);
  bool TransformApproxToCellParams(int subCell, double* pcoords);

protected:
  vtkHigherOrderCurve();
  ~vtkHigherOrderCurve() override;

  vtkLine* GetApprox();
  void PrepareApproxData(
    vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  virtual vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) = 0;

  int Order[2];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkLine> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;

private:
  vtkHigherOrderCurve(const vtkHigherOrderCurve&) = delete;
  void operator=(const vtkHigherOrderCurve&) = delete;
};

inline int vtkHigherOrderCurve::GetParametricCenter(double center[3])
{
  center[0] = 0.5;
  center[1] = center[2] = 0.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif // vtkHigherOrderCurve_h
