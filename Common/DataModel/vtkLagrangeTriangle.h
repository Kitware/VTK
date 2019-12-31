/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangeTriangle
 * @brief   A 2D cell that represents an arbitrary order Lagrange triangle
 *
 * vtkLagrangeTriangle is a concrete implementation of vtkCell to represent a
 * 2D triangle using Lagrange shape functions of user specified order.
 *
 * The number of points in a Lagrange cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
 */

#ifndef vtkLagrangeTriangle_h
#define vtkLagrangeTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderTriangle.h"

#include <vector> // For caching

class vtkDoubleArray;
class vtkLagrangeCurve;
class vtkTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeTriangle : public vtkHigherOrderTriangle
{
public:
  static vtkLagrangeTriangle* New();
  vtkTypeMacro(vtkLagrangeTriangle, vtkHigherOrderTriangle);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_TRIANGLE; }

  vtkCell* GetEdge(int edgeId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  virtual vtkHigherOrderCurve* getEdgeCell() override;

protected:
  vtkLagrangeTriangle();
  ~vtkLagrangeTriangle() override;

  vtkIdType GetNumberOfSubtriangles() const { return this->NumberOfSubtriangles; }
  vtkIdType ComputeNumberOfSubtriangles();
  vtkNew<vtkLagrangeCurve> EdgeCell;

private:
  vtkLagrangeTriangle(const vtkLagrangeTriangle&) = delete;
  void operator=(const vtkLagrangeTriangle&) = delete;
};

#endif
