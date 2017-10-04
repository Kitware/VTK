/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWedge
 * @brief   a 3D cell that represents a linear wedge
 *
 * vtkWedge is a concrete implementation of vtkCell to represent a linear 3D
 * wedge. A wedge consists of two triangular and three quadrilateral faces
 * and is defined by the six points (0-5). vtkWedge uses the standard
 * isoparametric shape functions for a linear wedge. The wedge is defined
 * by the six points (0-5) where (0,1,2) is the base of the wedge which,
 * using the right hand rule, forms a triangle whose normal points outward
 * (away from the triangular face (3,4,5)).
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkTetra vtkVoxel
*/

#ifndef vtkWedge_h
#define vtkWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkTriangle;
class vtkQuad;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkWedge : public vtkCell3D
{
public:
  static vtkWedge *New();
  vtkTypeMacro(vtkWedge,vtkCell3D);
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
  int GetCellType() override {return VTK_WEDGE;}
  int GetCellDimension() override {return 3;}
  int GetNumberOfEdges() override {return 9;}
  int GetNumberOfFaces() override {return 5;}
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int faceId) override;
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) override;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) override;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) override;
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) override;
  double *GetParametricCoords() override;
  //@}

  /**
   * Return the center of the wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * @deprecated Replaced by vtkWedge::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[6]);
  /**
   * @deprecated Replaced by vtkWedge::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[18]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[6]) override
  {
    vtkWedge::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[18]) override
  {
    vtkWedge::InterpolationDerivs(pcoords,derivs);
  }
  int JacobianInverse(double pcoords[3], double **inverse, double derivs[18]);
  //@}

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);
  //@}

protected:
  vtkWedge();
  ~vtkWedge() override;

  vtkLine *Line;
  vtkTriangle *Triangle;
  vtkQuad *Quad;

private:
  vtkWedge(const vtkWedge&) = delete;
  void operator=(const vtkWedge&) = delete;
};

inline int vtkWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333;
  pcoords[2] = 0.5;
  return 0;
}

#endif



