/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHigherOrderHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHigherOrderHexahedron
 * @brief   A 3D cell that represents an arbitrary order HigherOrder hex
 *
 * vtkHigherOrderHexahedron is a concrete implementation of vtkCell to represent a
 * 3D hexahedron using HigherOrder shape functions of user specified order.
 *
 * @sa
 * vtkHexahedron
 */

#ifndef vtkHigherOrderHexahedron_h
#define vtkHigherOrderHexahedron_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For member variable.
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkHexahedron;
class vtkIdList;
class vtkHigherOrderCurve;
class vtkHigherOrderInterpolation;
class vtkHigherOrderQuadrilateral;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkHigherOrderHexahedron : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkHigherOrderHexahedron, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override = 0;
  int GetCellDimension() override { return 3; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 12; }
  int GetNumberOfFaces() override { return 6; }
  vtkCell* GetEdge(int edgeId) override = 0;
  vtkCell* GetFace(int faceId) override = 0;
  void GetEdgeWithoutRationalWeights(vtkHigherOrderCurve* result, int edgeId);
  void GetFaceWithoutRationalWeights(vtkHigherOrderQuadrilateral* result, int faceId);

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
  virtual void SetOrder(const int s, const int t, const int u);
  virtual const int* GetOrder();
  virtual int GetOrder(int i) { return this->GetOrder()[i]; }

  void InterpolateFunctions(const double pcoords[3], double* weights) override = 0;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override = 0;

  bool SubCellCoordinatesFromId(vtkVector3i& ijk, int subId);
  bool SubCellCoordinatesFromId(int& i, int& j, int& k, int subId);
  static int PointIndexFromIJK(int i, int j, int k, const int* order);
  int PointIndexFromIJK(int i, int j, int k);
  bool TransformApproxToCellParams(int subCell, double* pcoords);
  bool TransformFaceToCellParams(int bdyFace, double* pcoords);
  virtual vtkHigherOrderCurve* getEdgeCell() = 0;
  virtual vtkHigherOrderQuadrilateral* getFaceCell() = 0;
  virtual vtkHigherOrderInterpolation* getInterp() = 0;

protected:
  vtkHigherOrderHexahedron();
  ~vtkHigherOrderHexahedron() override;

  vtkHexahedron* GetApprox();
  void PrepareApproxData(
    vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  virtual vtkHexahedron* GetApproximateHex(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) = 0;

  int Order[4];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkHexahedron> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;
  vtkNew<vtkPoints> TmpPts;
  vtkNew<vtkIdList> TmpIds;

private:
  vtkHigherOrderHexahedron(const vtkHigherOrderHexahedron&) = delete;
  void operator=(const vtkHigherOrderHexahedron&) = delete;
};

inline int vtkHigherOrderHexahedron::GetParametricCenter(double center[3])
{
  center[0] = center[1] = center[2] = 0.5;
  return 0;
}

#endif // vtkHigherOrderHexahedron_h
