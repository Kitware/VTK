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
// .NAME vtkPentagonalPrism - a 3D cell that represents a prism with
// pentagonal base
// .SECTION Description
// vtkPentagonalPrism is a concrete implementation of vtkCell to represent a
// linear 3D prism with pentagonal base. Such prism is defined by the ten points (0-9)
// where (0,1,2,3,4) is the base of the prism which, using the right hand
// rule, forms a pentagon whose normal points is in the direction of the
// opposite face (5,6,7,8,9).

// .SECTION Thanks
// Thanks to Philippe Guerville who developed this class.
// Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
// VTK 4. <br>
// Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
// class in VTK. <br>
// Please address all comments to Jean Favre (jfavre at cscs.ch).
//
// The Interpolation functions and derivatives were changed in June
// 2015 by Bill Lorensen. These changes follow the formulation in:
// http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf
// NOTE: An additional copy of this paper is located at:
// http://www.vtk.org/Wiki/File:ApplicationOfPolygonalFiniteElementsInLinearElasticity.pdf

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);

  // Description:
  // See the vtkCell3D API for descriptions of these methods.
  int GetCellType() {return VTK_PENTAGONAL_PRISM;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 15;};
  int GetNumberOfFaces() {return 7;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);

  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  double *GetParametricCoords();

  // Description:
  // Return the center of the wedge in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkPentagonalPrism::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[10]);
  // Description:
  // @deprecated Replaced by vtkPentagonalPrism::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[30]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[10])
    {
    vtkPentagonalPrism::InterpolationFunctions(pcoords, weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[30])
    {
    vtkPentagonalPrism::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[30]);

protected:
  vtkPentagonalPrism();
  ~vtkPentagonalPrism();

  vtkLine          *Line;
  vtkQuad          *Quad;
  vtkPolygon       *Polygon;
  vtkTriangle      *Triangle;

private:
  vtkPentagonalPrism(const vtkPentagonalPrism&);  // Not implemented.
  void operator=(const vtkPentagonalPrism&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkPentagonalPrism::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.5;

  return 0;
}
#endif
