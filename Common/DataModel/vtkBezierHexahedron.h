/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBezierHexahedron
 * @brief   A 3D cell that represents an arbitrary order Bezier hex
 *
 * vtkBezierHexahedron is a concrete implementation of vtkCell to represent a
 * 3D hexahedron using Bezier shape functions of user specified order.
 *
 * @sa
 * vtkHexahedron
 */

#ifndef vtkBezierHexahedron_h
#define vtkBezierHexahedron_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderHexahedron.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkHexahedron;
class vtkIdList;
class vtkBezierCurve;
class vtkBezierInterpolation;
class vtkBezierQuadrilateral;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierHexahedron : public vtkHigherOrderHexahedron
{
public:
  static vtkBezierHexahedron* New();
  vtkTypeMacro(vtkBezierHexahedron, vtkHigherOrderHexahedron);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_BEZIER_HEXAHEDRON; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void EvaluateLocationProjectedNode(
    int& subId, const vtkIdType point_id, double x[3], double* weights);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  void SetRationalWeightsFromPointData(vtkPointData* point_data, const vtkIdType numPts);

  vtkDoubleArray* GetRationalWeights();
  virtual vtkHigherOrderCurve* getEdgeCell() override;
  virtual vtkHigherOrderQuadrilateral* getFaceCell() override;
  virtual vtkHigherOrderInterpolation* getInterp() override;

protected:
  vtkHexahedron* GetApproximateHex(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkBezierHexahedron();
  ~vtkBezierHexahedron() override;

  vtkNew<vtkDoubleArray> RationalWeights;
  vtkNew<vtkBezierQuadrilateral> FaceCell;
  vtkNew<vtkBezierCurve> EdgeCell;
  vtkNew<vtkBezierInterpolation> Interp;

private:
  vtkBezierHexahedron(const vtkBezierHexahedron&) = delete;
  void operator=(const vtkBezierHexahedron&) = delete;
};

#endif // vtkBezierHexahedron_h
