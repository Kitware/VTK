/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHexahedron
 * @brief   a cell that represents a linear 3D hexahedron
 *
 * vtkHexahedron is a concrete implementation of vtkCell to represent a
 * linear, 3D rectangular hexahedron (e.g., "brick" topology). vtkHexahedron
 * uses the standard isoparametric shape functions for a linear
 * hexahedron. The hexahedron is defined by the eight points (0-7) where
 * (0,1,2,3) is the base of the hexahedron which, using the right hand rule,
 * forms a quadrilaterial whose normal points in the direction of the
 * opposite face (4,5,6,7).
 *
 * @sa
 * vtkConvexPointSet vtkPyramid vtkTetra vtkVoxel vtkWedge
*/

#ifndef vtkHexahedron_h
#define vtkHexahedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkQuad;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkHexahedron : public vtkCell3D
{
public:
  static vtkHexahedron *New();
  vtkTypeMacro(vtkHexahedron,vtkCell3D);
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
  int GetCellType() override {return VTK_HEXAHEDRON;}
  int GetNumberOfEdges() override {return 12;}
  int GetNumberOfFaces() override {return 6;}
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int faceId) override;
  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;
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
   * Return the case table for table-based isocontouring (aka marching cubes
   * style implementations). A linear 3D cell with N vertices will have 2**N
   * cases. The returned case array lists three edges in order to produce one
   * output triangle which may be repeated to generate multiple triangles. The
   * list of cases terminates with a -1 entry.
   */
  static int* GetTriangleCases(int caseId);

  /**
   * @deprecated Replaced by vtkHexahedron::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[8]);
  /**
   * @deprecated Replaced by vtkHexahedron::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[24]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[8]) override
  {
    vtkHexahedron::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[24]) override
  {
    vtkHexahedron::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId) VTK_SIZEHINT(2);
  static int *GetFaceArray(int faceId) VTK_SIZEHINT(4);
  //@}

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives.
   */
  void JacobianInverse(const double pcoords[3], double **inverse, double derivs[24]);

protected:
  vtkHexahedron();
  ~vtkHexahedron() override;

  vtkLine *Line;
  vtkQuad *Quad;

private:
  vtkHexahedron(const vtkHexahedron&) = delete;
  void operator=(const vtkHexahedron&) = delete;
};

#endif
