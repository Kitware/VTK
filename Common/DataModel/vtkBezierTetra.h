// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBezierTetra
 * @brief   A 3D cell that represents an arbitrary order Bezier tetrahedron
 *
 * vtkBezierTetra is a concrete implementation of vtkCell to represent a
 * 3D tetrahedron using Bezier shape functions of user specified order.
 *
 * The number of points in a Bezier cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
 */

#ifndef vtkBezierTetra_h
#define vtkBezierTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderTetra.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTetra;
class vtkBezierCurve;
class vtkBezierTriangle;
class vtkDoubleArray;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierTetra : public vtkHigherOrderTetra
{
public:
  static vtkBezierTetra* New();
  vtkTypeMacro(vtkBezierTetra, vtkHigherOrderTetra);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_BEZIER_TETRAHEDRON; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void SetRationalWeightsFromPointData(vtkPointData* point_data, vtkIdType numPts);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  vtkHigherOrderCurve* GetEdgeCell() override;
  vtkHigherOrderTriangle* GetFaceCell() override;

  vtkDoubleArray* GetRationalWeights();

protected:
  vtkBezierTetra();
  ~vtkBezierTetra() override;
  vtkNew<vtkBezierCurve> EdgeCell;
  vtkNew<vtkBezierTriangle> FaceCell;
  vtkNew<vtkDoubleArray> RationalWeights;

private:
  vtkBezierTetra(const vtkBezierTetra&) = delete;
  void operator=(const vtkBezierTetra&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
