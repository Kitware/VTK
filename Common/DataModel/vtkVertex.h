/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertex.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVertex - a cell that represents a 3D point
// .SECTION Description
// vtkVertex is a concrete implementation of vtkCell to represent a 3D point.

#ifndef vtkVertex_h
#define vtkVertex_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkVertex : public vtkCell
{
public:
  static vtkVertex *New();
  vtkTypeMacro(vtkVertex,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Make a new vtkVertex object with the same information as this object.

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_VERTEX;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *pts,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  virtual double *GetParametricCoords();

  // Description:
  // Given parametric coordinates of a point, return the closest cell
  // boundary, and whether the point is inside or outside of the cell. The
  // cell boundary is defined by a list of points (pts) that specify a vertex
  // (1D cell).  If the return value of the method is != 0, then the point is
  // inside the cell.
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);

  // Description:
  // Generate contouring primitives. The scalar list cellScalars are
  // scalar values at each cell point. The point locator is essentially a
  // points list that merges points as they are inserted (i.e., prevents
  // duplicates).
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts1,
               vtkCellArray *lines, vtkCellArray *verts2,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);

  // Description:
  // Return the center of the triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Intersect with a ray. Return parametric coordinates (both line and cell)
  // and global intersection coordinates, given ray definition and tolerance.
  // The method returns non-zero value if intersection occurs.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);

  // Description:
  // Triangulate the vertex. This method fills pts and ptIds with information
  // from the only point in the vertex.
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);

  // Description:
  // Get the derivative of the vertex. Returns (0.0, 0.0, 0.0) for all
  // dimensions.
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);

  // Description:
  // @deprecated Replaced by vtkVertex::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[1]);
  // Description:
  // @deprecated Replaced by vtkVertex::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[3]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[1])
    {
    vtkVertex::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[3])
    {
    vtkVertex::InterpolationDerivs(pcoords,derivs);
    }

protected:
  vtkVertex();
  ~vtkVertex() {}

private:
  vtkVertex(const vtkVertex&);  // Not implemented.
  void operator=(const vtkVertex&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkVertex::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

#endif


