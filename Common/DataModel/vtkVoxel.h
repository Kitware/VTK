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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int edgeId, int* &pts) VTK_OVERRIDE;
  void GetFacePoints(int faceId, int* &pts) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_VOXEL;}
  int GetCellDimension() VTK_OVERRIDE {return 3;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 12;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 6;}
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
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
  //@}

  /**
   * @deprecated Replaced by vtkVoxel::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[24]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[8]) VTK_OVERRIDE
  {
    vtkVoxel::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[24]) VTK_OVERRIDE
  {
    vtkVoxel::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  /**
   * Compute the interpolation functions.
   * This static method is for convenience. Use the member function
   * if you already have an instance of a voxel.
   */
  static void InterpolationFunctions(double pcoords[3], double weights[8]);

  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);
  //@}

protected:
  vtkVoxel();
  ~vtkVoxel() VTK_OVERRIDE;

private:
  vtkVoxel(const vtkVoxel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVoxel&) VTK_DELETE_FUNCTION;

  vtkLine *Line;
  vtkPixel *Pixel;
};

#endif


