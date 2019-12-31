/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHigherOrderQuadrilateral.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHigherOrderQuadrilateral
// .SECTION Description
// .SECTION See Also

#ifndef vtkHigherOrderQuadrilateral_h
#define vtkHigherOrderQuadrilateral_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For member variable.
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.

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
  void GetEdgeWithoutRationalWeights(vtkHigherOrderCurve* result, int edgeId);

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
  int Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  void SetParametricCoords();
  double* GetParametricCoords() override;
  int GetParametricCenter(double center[3]) override;

  double GetParametricDistance(const double pcoords[3]) override;

  virtual void SetOrderFromCellData(
    vtkCellData* cell_data, const vtkIdType numPts, const vtkIdType cell_id);
  virtual void SetUniformOrderFromNumPoints(const vtkIdType numPts);
  virtual void SetOrder(const int s, const int t);
  virtual const int* GetOrder();
  virtual int GetOrder(int i) { return this->GetOrder()[i]; }

  void InterpolateFunctions(const double pcoords[3], double* weights) override = 0;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override = 0;

  bool SubCellCoordinatesFromId(vtkVector3i& ijk, int subId);
  bool SubCellCoordinatesFromId(int& i, int& j, int& k, int subId);
  int PointIndexFromIJK(int i, int j, int k);
  static int PointIndexFromIJK(int i, int j, const int* order);
  bool TransformApproxToCellParams(int subCell, double* pcoords);

  virtual vtkHigherOrderCurve* getEdgeCell() = 0;

protected:
  vtkHigherOrderQuadrilateral();
  ~vtkHigherOrderQuadrilateral() override;

  vtkQuad* GetApprox();
  // The verion of GetApproximateQuad between Lagrange and Bezier is different because Bezier is
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
  vtkNew<vtkPoints> TmpPts;
  vtkNew<vtkIdList> TmpIds;

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

#endif // vtkHigherOrderQuadrilateral_h
