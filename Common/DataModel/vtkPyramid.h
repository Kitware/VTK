/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPyramid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPyramid
 * @brief   a 3D cell that represents a linear pyramid
 *
 * vtkPyramid is a concrete implementation of vtkCell to represent a 3D
 * pyramid. A pyramid consists of a rectangular base with four triangular
 * faces. vtkPyramid uses the standard isoparametric shape functions for
 * a linear pyramid. The pyramid is defined by the five points (0-4) where
 * (0,1,2,3) is the base of the pyramid which, using the right hand rule,
 * forms a quadrilaterial whose normal points in the direction of the
 * pyramid apex at vertex #4. The parametric location of vertex #4 is [0, 0, 1].
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkTetra vtkVoxel vtkWedge
*/

#ifndef vtkPyramid_h
#define vtkPyramid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkQuad;
class vtkTriangle;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPyramid : public vtkCell3D
{
public:
  static vtkPyramid *New();
  vtkTypeMacro(vtkPyramid,vtkCell3D);
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
  int GetCellType() override {return VTK_PYRAMID;}
  int GetCellDimension() override {return 3;}
  int GetNumberOfEdges() override {return 8;}
  int GetNumberOfFaces() override {return 5;}
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int faceId) override;
  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;
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
  //@}

  /**
   * Return the case table for table-based isocontouring (aka marching cubes
   * style implementations). A linear 3D cell with N vertices will have 2**N
   * cases. The returned case array lists three edges in order to produce one
   * output triangle which may be repeated to generate multiple triangles. The
   * list of cases terminates with a -1 entry.
   */
  static int* GetTriangleCases(int caseId);

  /**
   * Return the center of the pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * @deprecated Replaced by vtkPyramid::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[5]);
  /**
   * @deprecated Replaced by vtkPyramid::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[15]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[5]) override
  {
    vtkPyramid::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[15]) override
  {
    vtkPyramid::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  int JacobianInverse(const double pcoords[3], double **inverse, double derivs[15]);

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId) VTK_SIZEHINT(2);
  static int *GetFaceArray(int faceId) VTK_SIZEHINT(4);
  //@}

protected:
  vtkPyramid();
  ~vtkPyramid() override;

  vtkLine *Line;
  vtkTriangle *Triangle;
  vtkQuad *Quad;

private:
  vtkPyramid(const vtkPyramid&) = delete;
  void operator=(const vtkPyramid&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkPyramid::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.4;
  pcoords[2] = 0.2;
  return 0;
}

#endif
