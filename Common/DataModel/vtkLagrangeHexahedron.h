/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangeHexahedron
 * @brief   A 3D cell that represents an arbitrary order Lagrange hex
 *
 * vtkLagrangeHexahedron is a concrete implementation of vtkCell to represent a
 * 3D hexahedron using Lagrange shape functions of user specified order.
 *
 * @sa
 * vtkHexahedron
*/

#ifndef vtkLagrangeHexahedron_h
#define vtkLagrangeHexahedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.
#include "vtkCellType.h" // For GetCellType.
#include "vtkNew.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkHexahedron;
class vtkIdList;
class vtkLagrangeCurve;
class vtkLagrangeInterpolation;
class vtkLagrangeQuadrilateral;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeHexahedron : public vtkNonLinearCell
{
public:
  static vtkLagrangeHexahedron* New();
  vtkTypeMacro(vtkLagrangeHexahedron,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_LAGRANGE_HEXAHEDRON; }
  int GetCellDimension() override { return 3; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 12; }
  int GetNumberOfFaces() override { return 6; }
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

protected:
  vtkLagrangeHexahedron();
  ~vtkLagrangeHexahedron() override;

  vtkHexahedron* GetApprox();
  void PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars);
  vtkHexahedron* GetApproximateHex(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr);

  int Order[4];
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;
  vtkSmartPointer<vtkHexahedron> Approx;
  vtkSmartPointer<vtkPointData> ApproxPD;
  vtkSmartPointer<vtkCellData> ApproxCD;
  vtkNew<vtkDoubleArray> CellScalars;
  vtkNew<vtkDoubleArray> Scalars;
  vtkNew<vtkPoints> TmpPts;
  vtkNew<vtkIdList> TmpIds;
  vtkNew<vtkLagrangeQuadrilateral> FaceCell;
  vtkNew<vtkLagrangeCurve> EdgeCell;
  vtkNew<vtkLagrangeInterpolation> Interp;

private:
  vtkLagrangeHexahedron(const vtkLagrangeHexahedron&) = delete;
  void operator=(const vtkLagrangeHexahedron&) = delete;
};

inline int vtkLagrangeHexahedron::GetParametricCenter(double center[3])
{
  center[0] = center[1] = center[2] = 0.5;
  return 0;
}

#endif // vtkLagrangeHexahedron_h
