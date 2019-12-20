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

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderHexahedron.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

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

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeHexahedron : public vtkHigherOrderHexahedron
{
public:
  static vtkLagrangeHexahedron* New();
  vtkTypeMacro(vtkLagrangeHexahedron, vtkHigherOrderHexahedron);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_HEXAHEDRON; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  virtual vtkHigherOrderCurve* getEdgeCell() override;
  virtual vtkHigherOrderQuadrilateral* getFaceCell() override;
  virtual vtkHigherOrderInterpolation* getInterp() override;

protected:
  vtkHexahedron* GetApproximateHex(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkLagrangeHexahedron();
  ~vtkLagrangeHexahedron() override;

  vtkNew<vtkLagrangeQuadrilateral> FaceCell;
  vtkNew<vtkLagrangeCurve> EdgeCell;
  vtkNew<vtkLagrangeInterpolation> Interp;

private:
  vtkLagrangeHexahedron(const vtkLagrangeHexahedron&) = delete;
  void operator=(const vtkLagrangeHexahedron&) = delete;
};

#endif // vtkLagrangeHexahedron_h
