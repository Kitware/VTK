/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBezierTriangle
 * @brief   A 2D cell that represents an arbitrary order Bezier triangle
 *
 * vtkBezierTriangle is a concrete implementation of vtkCell to represent a
 * 2D triangle using Bezier shape functions of user specified order.
 *
 * The number of points in a Bezier cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
 */

#ifndef vtkBezierTriangle_h
#define vtkBezierTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderTriangle.h"

class vtkDoubleArray;
class vtkBezierCurve;
class vtkTriangle;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierTriangle : public vtkHigherOrderTriangle
{
public:
  static vtkBezierTriangle* New();
  vtkTypeMacro(vtkBezierTriangle, vtkHigherOrderTriangle);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_BEZIER_TRIANGLE; }
  vtkCell* GetEdge(int edgeId) override;
  void EvaluateLocationProjectedNode(
    int& subId, const vtkIdType point_id, double x[3], double* weights);
  void SetRationalWeightsFromPointData(vtkPointData* point_data, const vtkIdType numPts);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  virtual vtkHigherOrderCurve* getEdgeCell() override;

  vtkDoubleArray* GetRationalWeights();

protected:
  vtkBezierTriangle();
  ~vtkBezierTriangle() override;

  vtkNew<vtkBezierCurve> EdgeCell;
  vtkNew<vtkDoubleArray> RationalWeights;

private:
  vtkBezierTriangle(const vtkBezierTriangle&) = delete;
  void operator=(const vtkBezierTriangle&) = delete;
};

#endif
