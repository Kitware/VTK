/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeCurve.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLagrangeCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeCurve_h
#define vtkLagrangeCurve_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.
#include "vtkCellType.h" // For GetCellType.
#include "vtkNew.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLine;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeCurve : public vtkNonLinearCell
{
public:
  static vtkLagrangeCurve* New();
  vtkTypeMacro(vtkLagrangeCurve,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_LAGRANGE_CURVE; }
  int GetCellDimension() override { return 1; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 0; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int) override { return nullptr; }
  vtkCell* GetFace(int) override { return nullptr; }


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
  bool SubCellCoordinatesFromId(int& i, int subId);
  int PointIndexFromIJK(int i, int, int);
  bool TransformApproxToCellParams(int subCell, double* pcoords);

protected:
  vtkLagrangeCurve();
  ~vtkLagrangeCurve() override;

  vtkLine* GetApprox();
  void PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr);

  int Order[2];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkLine> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;
  vtkNew<vtkPoints> TmpPts;
  vtkNew<vtkIdList> TmpIds;

private:
  vtkLagrangeCurve(const vtkLagrangeCurve&) = delete;
  void operator=(const vtkLagrangeCurve&) = delete;
};

inline int vtkLagrangeCurve::GetParametricCenter(double center[3])
{
  center[0] = 0.5;
  center[1] = center[2] = 0.0;
  return 0;
}

#endif // vtkLagrangeCurve_h
