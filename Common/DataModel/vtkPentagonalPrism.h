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
   * See the vtkCell3D API for descriptions of these methods.
   */
  int GetCellType() override {return VTK_PENTAGONAL_PRISM;};
  int GetCellDimension() override {return 3;};
  int GetNumberOfEdges() override {return 15;};
  int GetNumberOfFaces() override {return 7;};
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
   * @deprecated Replaced by vtkPentagonalPrism::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[10]);
  /**
   * @deprecated Replaced by vtkPentagonalPrism::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[30]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[10]) override
  {
    vtkPentagonalPrism::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[30]) override
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
  void JacobianInverse(const double pcoords[3], double **inverse, double derivs[30]);

protected:
  vtkPentagonalPrism();
  ~vtkPentagonalPrism() override;

  vtkLine          *Line;
  vtkQuad          *Quad;
  vtkPolygon       *Polygon;
  vtkTriangle      *Triangle;

private:
  vtkPentagonalPrism(const vtkPentagonalPrism&) = delete;
  void operator=(const vtkPentagonalPrism&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkPentagonalPrism::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.5;

  return 0;
}
#endif
