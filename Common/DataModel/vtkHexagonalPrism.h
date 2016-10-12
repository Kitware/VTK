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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int edgeId, int* &pts) VTK_OVERRIDE;
  void GetFacePoints(int faceId, int* &pts) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_HEXAGONAL_PRISM;};
  int GetCellDimension() VTK_OVERRIDE {return 3;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 18;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 8;};
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  //@}

  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;

  /**
   * Return the center of the wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkHexagonalPrism::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[12]);
  /**
   * @deprecated Replaced by vtkHexagonalPrism::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[36]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[12]) VTK_OVERRIDE
  {
    vtkHexagonalPrism::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[36]) VTK_OVERRIDE
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[36]);

protected:
  vtkHexagonalPrism();
  ~vtkHexagonalPrism() VTK_OVERRIDE;

  vtkLine          *Line;
  vtkQuad          *Quad;
  vtkPolygon       *Polygon;

private:
  vtkHexagonalPrism(const vtkHexagonalPrism&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHexagonalPrism&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkHexagonalPrism::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.5;
  return 0;
}
#endif


