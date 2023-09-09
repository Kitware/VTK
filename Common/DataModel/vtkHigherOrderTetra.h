// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHigherOrderTetra
 * @brief   A 3D cell that represents an arbitrary order HigherOrder tetrahedron
 *
 * vtkHigherOrderTetra is a concrete implementation of vtkCell to represent a
 * 3D tetrahedron using HigherOrder shape functions of user specified order.
 *
 * The number of points in a HigherOrder cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
 */

#ifndef vtkHigherOrderTetra_h
#define vtkHigherOrderTetra_h

#include <functional> //For std::function

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For member variable.
#include "vtkNonLinearCell.h"
#include "vtkSmartPointer.h" // For member variable.

#include <vector> //For caching

VTK_ABI_NAMESPACE_BEGIN
class vtkTetra;
class vtkHigherOrderCurve;
class vtkHigherOrderTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkHigherOrderTetra : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkHigherOrderTetra, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override = 0;
  int GetCellDimension() override { return 3; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 6; }
  int GetNumberOfFaces() override { return 4; }
  vtkCell* GetEdge(int edgeId) override = 0;
  vtkCell* GetFace(int faceId) override = 0;
  void SetEdgeIdsAndPoints(int edgeId,
    const std::function<void(const vtkIdType&)>& set_number_of_ids_and_points,
    const std::function<void(const vtkIdType&, const vtkIdType&)>& set_ids_and_points);
  static void SetFaceIdsAndPoints(int faceId, int order, vtkIdType numPts,
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
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void JacobianInverse(const double pcoords[3], double** inverse, double* derivs);
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  void SetParametricCoords();
  double* GetParametricCoords() override;

  int GetParametricCenter(double pcoords[3]) override;
  double GetParametricDistance(const double pcoords[3]) override;

  void InterpolateFunctions(const double pcoords[3], double* weights) override = 0;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override = 0;

  vtkIdType GetOrder() const { return this->Order; }
  vtkIdType ComputeOrder();
  static vtkIdType ComputeOrder(vtkIdType nPoints);
  /// Return true if the number of points supports a cell of uniform
  /// degree along each axis.
  static bool PointCountSupportsUniformOrder(vtkIdType pointsPerCell);

  void ToBarycentricIndex(vtkIdType index, vtkIdType* bindex);
  vtkIdType ToIndex(const vtkIdType* bindex);

  static void BarycentricIndex(vtkIdType index, vtkIdType* bindex, vtkIdType order);
  static vtkIdType Index(const vtkIdType* bindex, vtkIdType order);
  virtual vtkHigherOrderCurve* GetEdgeCell() = 0;
  virtual vtkHigherOrderTriangle* GetFaceCell() = 0;

protected:
  vtkHigherOrderTetra();
  ~vtkHigherOrderTetra() override;

  vtkIdType GetNumberOfSubtetras() const { return this->NumberOfSubtetras; }
  vtkIdType ComputeNumberOfSubtetras();

  // Description:
  // Given the index of the subtriangle, compute the barycentric indices of
  // the subtriangle's vertices.
  void SubtetraBarycentricPointIndices(vtkIdType cellIndex, vtkIdType (&pointBIndices)[4][4]);
  void TetraFromOctahedron(
    vtkIdType cellIndex, const vtkIdType (&octBIndices)[6][4], vtkIdType (&tetraBIndices)[4][4]);

  vtkTetra* Tetra;
  vtkDoubleArray* Scalars; // used to avoid New/Delete in contouring/clipping
  vtkIdType Order;
  vtkIdType NumberOfSubtetras;
  vtkSmartPointer<vtkPoints> PointParametricCoordinates;

  std::vector<vtkIdType> EdgeIds;
  std::vector<vtkIdType> BarycentricIndexMap;
  std::vector<vtkIdType> IndexMap;
  std::vector<vtkIdType> SubtetraIndexMap;

private:
  vtkHigherOrderTetra(const vtkHigherOrderTetra&) = delete;
  void operator=(const vtkHigherOrderTetra&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
