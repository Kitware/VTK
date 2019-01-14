/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexagonalPrism.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHexagonalPrism
 * @brief   a 3D cell that represents a prism with
 * hexagonal base
 *
 * vtkHexagonalPrism is a concrete implementation of vtkCell to represent a
 * linear 3D prism with hexagonal base. Such prism is defined by the twelve points
 * (0-12) where (0,1,2,3,4,5) is the base of the prism which, using the right
 * hand rule, forms a hexagon whose normal points is in the direction of the
 * opposite face (6,7,8,9,10,11).
 *
 * @par Thanks:
 * Thanks to Philippe Guerville who developed this class.
 * Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
 * VTK 4.
 * Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
 * class in VTK.
 * Please address all comments to Jean Favre (jfavre at cscs.ch).
*/

#ifndef vtkHexagonalPrism_h
#define vtkHexagonalPrism_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkPolygon;
class vtkQuad;

class VTKCOMMONDATAMODEL_EXPORT vtkHexagonalPrism : public vtkCell3D
{
public:
  static vtkHexagonalPrism *New();
  vtkTypeMacro(vtkHexagonalPrism,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int edgeId, int* &pts) override;
  void GetFacePoints(int faceId, int* &pts) override;
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override {return VTK_HEXAGONAL_PRISM;};
  int GetCellDimension() override {return 3;};
  int GetNumberOfEdges() override {return 18;};
  int GetNumberOfFaces() override {return 8;};
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int faceId) override;
  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;
  //@}

  int EvaluatePosition(const double x[3], double closestPoint[3],
                       int& subId, double pcoords[3],
                       double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3],
                        double *weights) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void Derivatives(int subId, const double pcoords[3], const double *values,
                   int dim, double *derivs) override;
  double *GetParametricCoords() override;

  /**
   * Return the center of the wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * @deprecated Replaced by vtkHexagonalPrism::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[12]);
  /**
   * @deprecated Replaced by vtkHexagonalPrism::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[36]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[12]) override
  {
    vtkHexagonalPrism::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[36]) override
  {
    vtkHexagonalPrism::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);
  //@}

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives.
   */
  void JacobianInverse(const double pcoords[3], double **inverse, double derivs[36]);

protected:
  vtkHexagonalPrism();
  ~vtkHexagonalPrism() override;

  vtkLine          *Line;
  vtkQuad          *Quad;
  vtkPolygon       *Polygon;

private:
  vtkHexagonalPrism(const vtkHexagonalPrism&) = delete;
  void operator=(const vtkHexagonalPrism&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkHexagonalPrism::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.5;
  return 0;
}
#endif


