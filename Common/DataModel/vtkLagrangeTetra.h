/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTetra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLagrangeTetra
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeTetra_h
#define vtkLagrangeTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

#define VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER 6

#define MAX_POINTS ((VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1) *  \
                    (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 2) *  \
                    (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 3)/6)

#define MAX_SUBTETRAHEDRA (((VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER - 2) *  \
                            (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER - 1) *  \
                            (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER)) +     \
                           4*((VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER - 1) * \
                              (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER) *    \
                              (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1)) + \
                           ((VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER) *      \
                            (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1) *  \
                            (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 2))/6)

class vtkTetra;
class vtkLagrangeCurve;
class vtkLagrangeTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeTetra : public vtkNonLinearCell
{
public:
  static vtkLagrangeTetra *New();
  vtkTypeMacro(vtkLagrangeTetra,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_LAGRANGE_TETRAHEDRON; }
  int GetCellDimension() override { return 3; }
  int RequiresInitialization() override { return 1; }
  int GetNumberOfEdges() override { return 6; }
  int GetNumberOfFaces() override { return 4; }
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int faceId) override;

  void Initialize() override;

  static int MaximumOrder() { return VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER; }
  static int MaximumNumberOfPoints()
  {
    return ((vtkLagrangeTetra::MaximumOrder() + 1) *
            (vtkLagrangeTetra::MaximumOrder() + 2)/2);
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

protected:
  vtkLagrangeTetra();
  ~vtkLagrangeTetra() override;

  vtkIdType GetNumberOfSubtetras() const { return this->NumberOfSubtetras; }
  vtkIdType ComputeNumberOfSubtetras();

  // Description:
  // Given the index of the subtriangle, compute the barycentric indices of
  // the subtriangle's vertices.
  void SubtetraBarycentricPointIndices(vtkIdType cellIndex,
                                       vtkIdType (&pointBIndices)[4][4]);
  void TetraFromOctahedron(vtkIdType cellIndex,
                           const vtkIdType (&octBIndices)[6][4],
                           vtkIdType (&tetraBIndices)[4][4]);

  vtkLagrangeCurve *Edge;
  vtkLagrangeTriangle *Face;
  vtkTetra *Tetra;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping
  vtkIdType Order;
  vtkIdType NumberOfSubtetras;
  double* ParametricCoordinates;

  vtkIdType EdgeIds[VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1];
  vtkIdType BarycentricIndexMap[4*MAX_POINTS];
  vtkIdType IndexMap[(VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1) *
                     (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1) *
                     (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1)];
  vtkIdType SubtetraIndexMap[16*MAX_SUBTETRAHEDRA];

private:
  vtkLagrangeTetra(const vtkLagrangeTetra&) = delete;
  void operator=(const vtkLagrangeTetra&) = delete;
};

#undef MAX_POINTS
#undef MAX_SUBTETRAHEDRA

#endif
