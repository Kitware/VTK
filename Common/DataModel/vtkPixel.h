/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPixel - a cell that represents an orthogonal quadrilateral
// .SECTION Description
// vtkPixel is a concrete implementation of vtkCell to represent a 2D
// orthogonal quadrilateral. Unlike vtkQuad, the corners are at right angles,
// and aligned along x-y-z coordinate axes leading to large increases in
// computational efficiency.

#ifndef __vtkPixel_h
#define __vtkPixel_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkLine;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPixel : public vtkCell
{
public:
  static vtkPixel *New();
  vtkTypeMacro(vtkPixel,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_PIXEL;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 4;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);

  // Description:
  // Return the center of the triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // @deprecated Replaced by vtkPixel::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[4]);
  // Description:
  // @deprecated Replaced by vtkPixel::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[8]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[4])
    {
    vtkPixel::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[8])
    {
    vtkPixel::InterpolationDerivs(pcoords,derivs);
    }

protected:
  vtkPixel();
  ~vtkPixel();

  vtkLine *Line;

private:
  vtkPixel(const vtkPixel&);  // Not implemented.
  void operator=(const vtkPixel&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkPixel::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.0;
  return 0;
}

#endif


