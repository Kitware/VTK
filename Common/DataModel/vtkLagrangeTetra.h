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
/**
 * @class   vtkLagrangeTetra
 * @brief   A 3D cell that represents an arbitrary order Lagrange tetrahedron
 *
 * vtkLagrangeTetra is a concrete implementation of vtkCell to represent a
 * 3D tetrahedron using Lagrange shape functions of user specified order.
 *
 * The number of points in a Lagrange cell determines the order over which they
 * are iterated relative to the parametric coordinate system of the cell. The
 * first points that are reported are vertices. They appear in the same order in
 * which they would appear in linear cells. Mid-edge points are reported next.
 * They are reported in sequence. For two- and three-dimensional (3D) cells, the
 * following set of points to be reported are face points. Finally, 3D cells
 * report points interior to their volume.
 */

#ifndef vtkLagrangeTetra_h
#define vtkLagrangeTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderTetra.h"

#include <vector> // For caching

class vtkTetra;
class vtkLagrangeCurve;
class vtkLagrangeTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeTetra : public vtkHigherOrderTetra
{
public:
  static vtkLagrangeTetra* New();
  vtkTypeMacro(vtkLagrangeTetra, vtkHigherOrderTetra);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_TETRAHEDRON; }

  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  virtual vtkHigherOrderCurve* getEdgeCell() override;
  virtual vtkHigherOrderTriangle* getFaceCell() override;

protected:
  vtkLagrangeTetra();
  ~vtkLagrangeTetra() override;

  vtkNew<vtkLagrangeCurve> EdgeCell;
  vtkNew<vtkLagrangeTriangle> FaceCell;

private:
  vtkLagrangeTetra(const vtkLagrangeTetra&) = delete;
  void operator=(const vtkLagrangeTetra&) = delete;
};

#endif
