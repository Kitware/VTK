/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPentagonalPrism.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPentagonalPrism
 * @brief   a 3D cell that represents a convex prism with
 * pentagonal base
 *
 * vtkPentagonalPrism is a concrete implementation of vtkCell to represent a
 * linear convex 3D prism with pentagonal base. Such prism is defined by the
 * ten points (0-9), where (0,1,2,3,4) is the base of the prism which, using
 * the right hand rule, forms a pentagon whose normal points is in the direction
 * of the opposite face (5,6,7,8,9).
 *
 * @par Thanks:
 * Thanks to Philippe Guerville who developed this class.
 * Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
 * VTK 4. <br>
 * Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
 * class in VTK. <br>
 * Please address all comments to Jean Favre (jfavre at cscs.ch).
 *
 * @par Thanks:
 * The Interpolation functions and derivatives were changed in June
 * 2015 by Bill Lorensen. These changes follow the formulation in:
 * http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf
 * NOTE: An additional copy of this paper is located at:
 * http://www.vtk.org/Wiki/File:ApplicationOfPolygonalFiniteElementsInLinearElasticity.pdf
*/

#ifndef vtkPentagonalPrism_h
#define vtkPentagonalPrism_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkPolygon;
class vtkQuad;
class vtkTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkPentagonalPrism : public vtkCell3D
{
public:
  static vtkPentagonalPrism *New();
  vtkTypeMacro(vtkPentagonalPrism,vtkCell3D);
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
   * See the vtkCell3D API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_PENTAGONAL_PRISM;};
  int GetCellDimension() VTK_OVERRIDE {return 3;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 15;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 7;};
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
   * @deprecated Replaced by vtkPentagonalPrism::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[10]);
  /**
   * @deprecated Replaced by vtkPentagonalPrism::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[30]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[10]) VTK_OVERRIDE
  {
    vtkPentagonalPrism::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[30]) VTK_OVERRIDE
  {
    vtkPentagonalPrism::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[30]);

protected:
  vtkPentagonalPrism();
  ~vtkPentagonalPrism() VTK_OVERRIDE;

  vtkLine          *Line;
  vtkQuad          *Quad;
  vtkPolygon       *Polygon;
  vtkTriangle      *Triangle;

private:
  vtkPentagonalPrism(const vtkPentagonalPrism&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPentagonalPrism&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkPentagonalPrism::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.5;

  return 0;
}
#endif
