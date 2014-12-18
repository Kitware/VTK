/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraticQuad - cell represents a parabolic, 8-node isoparametric quad
// .SECTION Description
// vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
// represent a two-dimensional, 8-node isoparametric parabolic quadrilateral
// element. The interpolation is the standard finite element, quadratic
// isoparametric shape function. The cell includes a mid-edge node for each
// of the four edges of the cell. The ordering of the eight points defining
// the cell are point ids (0-3,4-7) where ids 0-3 define the four corner
// vertices of the quad; ids 4-7 define the midedge nodes (0,1), (1,2),
// (2,3), (3,0).

// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
// vtkQuadraticHexahedron vtkQuadraticWedge vtkQuadraticPyramid


#ifndef vtkQuadraticQuad_h
#define vtkQuadraticQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuad;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticQuad : public vtkNonLinearCell
{
public:
  static vtkQuadraticQuad *New();
  vtkTypeMacro(vtkQuadraticQuad,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions
  // of these methods.
  int GetCellType() {return VTK_QUADRATIC_QUAD;};
  int GetCellDimension() {return 2;}
  int GetNumberOfEdges() {return 4;}
  int GetNumberOfFaces() {return 0;}
  vtkCell *GetEdge(int);
  vtkCell *GetFace(int) {return 0;}

  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // Clip this quadratic quad using scalar value provided. Like contouring,
  // except that it cuts the quad to produce linear triangles.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);


  // Description:
  // Return the center of the pyramid in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkQuadraticQuad::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[8]);
  // Description:
  // @deprecated Replaced by vtkQuadraticQuad::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[16]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[8])
    {
    vtkQuadraticQuad::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[16])
    {
    vtkQuadraticQuad::InterpolationDerivs(pcoords,derivs);
    }

protected:
  vtkQuadraticQuad();
  ~vtkQuadraticQuad();

  vtkQuadraticEdge *Edge;
  vtkQuad          *Quad;
  vtkPointData     *PointData;
  vtkDoubleArray   *Scalars;

  // In order to achieve some functionality we introduce a fake center point
  // which require to have some extra functionalities compare to other non-linar
  // cells
  vtkCellData      *CellData;
  vtkDoubleArray   *CellScalars;
  void Subdivide(double *weights);
  void InterpolateAttributes(vtkPointData *inPd, vtkCellData *inCd, vtkIdType cellId,
    vtkDataArray *cellScalars);

private:
  vtkQuadraticQuad(const vtkQuadraticQuad&);  // Not implemented.
  void operator=(const vtkQuadraticQuad&);  // Not implemented.
};
//----------------------------------------------------------------------------
inline int vtkQuadraticQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.;
  return 0;
}

#endif


