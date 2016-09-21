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
// .NAME vtkHexahedron - a cell that represents a linear 3D hexahedron
// .SECTION Description
// vtkHexahedron is a concrete implementation of vtkCell to represent a
// linear, 3D rectangular hexahedron (e.g., "brick" topology). vtkHexahedron
// uses the standard isoparametric shape functions for a linear
// hexahedron. The hexahedron is defined by the eight points (0-7) where
// (0,1,2,3) is the base of the hexahedron which, using the right hand rule,
// forms a quadrilaterial whose normal points in the direction of the
// opposite face (4,5,6,7).

// .SECTION See Also
// vtkConvexPointSet vtkPyramid vtkTetra vtkVoxel vtkWedge

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // See vtkCell3D API for description of these methods.
  void GetEdgePoints(int edgeId, int* &pts) VTK_OVERRIDE;
  void GetFacePoints(int faceId, int* &pts) VTK_OVERRIDE;

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() VTK_OVERRIDE {return VTK_HEXAHEDRON;}
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
  double *GetParametricCoords() VTK_OVERRIDE;

  // Description:
  // @deprecated Replaced by vtkHexahedron::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[8]);
  // Description:
  // @deprecated Replaced by vtkHexahedron::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[24]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  void InterpolateFunctions(double pcoords[3], double weights[8]) VTK_OVERRIDE
    {
    vtkHexahedron::InterpolationFunctions(pcoords,weights);
    }
  void InterpolateDerivs(double pcoords[3], double derivs[24]) VTK_OVERRIDE
    {
    vtkHexahedron::InterpolationDerivs(pcoords,derivs);
    }

  // Description:
  // Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
  // Ids are related to the cell, not to the dataset.
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[24]);

protected:
  vtkHexahedron();
  ~vtkHexahedron();

  vtkLine *Line;
  vtkQuad *Quad;

private:
  vtkHexahedron(const vtkHexahedron&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHexahedron&) VTK_DELETE_FUNCTION;
};

#endif


