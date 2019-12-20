/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierQuadrilateral.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBezierQuadrilateral
// .SECTION Description
// .SECTION See Also

#ifndef vtkBezierQuadrilateral_h
#define vtkBezierQuadrilateral_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkBezierCurve;
class vtkPointData;
class vtkPoints;
class vtkQuad;
class vtkVector3d;
class vtkVector3i;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierQuadrilateral : public vtkHigherOrderQuadrilateral
{
public:
  static vtkBezierQuadrilateral* New();
  vtkTypeMacro(vtkBezierQuadrilateral, vtkHigherOrderQuadrilateral);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_BEZIER_QUADRILATERAL; }

  vtkCell* GetEdge(int edgeId) override;
  void EvaluateLocationProjectedNode(
    int& subId, const vtkIdType point_id, double x[3], double* weights);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  void SetRationalWeightsFromPointData(vtkPointData* point_data, const vtkIdType numPts);
  vtkDoubleArray* GetRationalWeights();
  virtual vtkHigherOrderCurve* getEdgeCell() override;

protected:
  // The verion of GetApproximateQuad between Lagrange and Bezier is different because Bezier is
  // non-interpolatory
  vtkQuad* GetApproximateQuad(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;

  vtkBezierQuadrilateral();
  ~vtkBezierQuadrilateral() override;

  vtkNew<vtkDoubleArray> RationalWeights;
  vtkNew<vtkBezierCurve> EdgeCell;

private:
  vtkBezierQuadrilateral(const vtkBezierQuadrilateral&) = delete;
  void operator=(const vtkBezierQuadrilateral&) = delete;
};

#endif // vtkBezierQuadrilateral_h
