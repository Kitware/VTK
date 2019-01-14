/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVoxel
 * @brief   a cell that represents a 3D orthogonal parallelepiped
 *
 * vtkVoxel is a concrete implementation of vtkCell to represent a 3D
 * orthogonal parallelepiped. Unlike vtkHexahedron, vtkVoxel has interior
 * angles of 90 degrees, and sides are parallel to coordinate axes. This
 * results in large increases in computational performance.
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkTetra vtkWedge
*/

#ifndef vtkVoxel_h
#define vtkVoxel_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkPixel;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkVoxel : public vtkCell3D
{
public:
  static vtkVoxel *New();
  vtkTypeMacro(vtkVoxel,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int edgeId, int* &pts) override;
  void GetFacePoints(int faceId, int* &pts) override;
  double *GetParametricCoords() override;
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override {return VTK_VOXEL;}
  int GetCellDimension() override {return 3;}
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
  //@}

  /**
   * @deprecated Replaced by vtkVoxel::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[24]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[8]) override
  {
    vtkVoxel::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[24]) override
  {
    vtkVoxel::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  /**
   * Compute the interpolation functions.
   * This static method is for convenience. Use the member function
   * if you already have an instance of a voxel.
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[8]);

  /**
   * Return the case table for table-based isocontouring (aka marching cubes
   * style implementations). A linear 3D cell with N vertices will have 2**N
   * cases. The returned case array lists three edges in order to produce one
   * output triangle which may be repeated to generate multiple triangles. The
   * list of cases terminates with a -1 entry.
   */
  static int* GetTriangleCases(int caseId);

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId) VTK_SIZEHINT(2);
  static int *GetFaceArray(int faceId) VTK_SIZEHINT(4);
  //@}

protected:
  vtkVoxel();
  ~vtkVoxel() override;

private:
  vtkVoxel(const vtkVoxel&) = delete;
  void operator=(const vtkVoxel&) = delete;

  vtkLine *Line;
  vtkPixel *Pixel;
};

#endif
