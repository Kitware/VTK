/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeQuadrilateral.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLagrangeQuadrilateral
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeQuadrilateral_h
#define vtkLagrangeQuadrilateral_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLagrangeCurve;
class vtkPointData;
class vtkPoints;
class vtkQuad;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeQuadrilateral : public vtkHigherOrderQuadrilateral
{
public:
  static vtkLagrangeQuadrilateral* New();
  vtkTypeMacro(vtkLagrangeQuadrilateral, vtkHigherOrderQuadrilateral);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_QUADRILATERAL; }

  vtkCell* GetEdge(int edgeId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  virtual vtkHigherOrderCurve* getEdgeCell() override;

protected:
  // The verion of GetApproximateQuad between Lagrange and Bezier is different because Bezier is
  // non-interpolatory
  vtkQuad* GetApproximateQuad(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;

  vtkLagrangeQuadrilateral();
  ~vtkLagrangeQuadrilateral() override;

  vtkNew<vtkLagrangeCurve> EdgeCell;

private:
  vtkLagrangeQuadrilateral(const vtkLagrangeQuadrilateral&) = delete;
  void operator=(const vtkLagrangeQuadrilateral&) = delete;
};

#endif // vtkLagrangeQuadrilateral_h
