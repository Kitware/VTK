/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlanesIntersection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkPlanesIntersection - A vtkPlanesIntersection object is a
//    vtkPlanes object that can compute whether the arbitrary convex region
//    bounded by it's planes intersects an axis-aligned box.
//
// .SECTION Description
//    A subclass of vtkPlanes, this class determines whether it
//    intersects an axis aligned box.   This is motivated by the
//    need to intersect the axis aligned region of a spacial
//    decomposition of volume data with various other regions.
//    It uses the algorithm from Graphics Gems IV, page 81.
//
// .SECTION Caveat
//    An instance of vtkPlanes can be redefined by changing the planes,
//    but this subclass then will not know if the region vertices are
//    up to date.  (Region vertices can be specified in SetRegionVertices
//    or computed by the subclass.)  So Delete and recreate if you want
//    to change the set of planes.
//

#ifndef __vtkPlanesIntersection_h
#define __vtkPlanesIntersection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPlanes.h"

class vtkPoints;
class vtkPointsProjectedHull;
class vtkCell;

class VTKCOMMONDATAMODEL_EXPORT vtkPlanesIntersection : public vtkPlanes
{
  vtkTypeMacro(vtkPlanesIntersection, vtkPlanes);

public:
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPlanesIntersection *New();

  // Description:
  //   It helps if you know the vertices of the convex region.
  //   If you don't, we will calculate them.  Region vertices
  //   are 3-tuples.

  void SetRegionVertices(vtkPoints *pts);
  void SetRegionVertices(double *v, int nvertices);
  int GetNumRegionVertices();
  int GetRegionVertices(double *v, int nvertices);

  // Description:
  //   Return 1 if the axis aligned box defined by R intersects
  //   the region defined by the planes, or 0 otherwise.

  int IntersectsRegion(vtkPoints *R);

  // Description:
  //   A convenience function provided by this class, returns
  //   1 if the polygon defined in pts intersects the bounding
  //   box defined in bounds, 0 otherwise.
  //
  //   The points must define a planar polygon.

  static int PolygonIntersectsBBox(double bounds[6], vtkPoints *pts);

  // Description:
  //   Another convenience function provided by this class, returns
  //   the vtkPlanesIntersection object representing a 3D
  //   cell.  The point IDs for each face must be given in
  //   counter-clockwise order from the outside of the cell.

  static vtkPlanesIntersection *Convert3DCell(vtkCell *cell);

protected:

  static void ComputeNormal(double *p1, double *p2, double *p3, double normal[3]);
  static double EvaluatePlaneEquation(double *x, double *p);
  static void PlaneEquation(double *n, double *x, double *p);
  static int GoodNormal(double *n);
  static int Invert3x3(double M[3][3]);

  vtkPlanesIntersection();
  ~vtkPlanesIntersection();

private:

  int IntersectsBoundingBox(vtkPoints *R);
  int EnclosesBoundingBox(vtkPoints *R);
  int EvaluateFacePlane(int plane, vtkPoints *R);
  int IntersectsProjection(vtkPoints *R, int direction);

  void SetPlaneEquations();
  void ComputeRegionVertices();

  void planesMatrix(int p1, int p2, int p3, double M[3][3]) const;
  int duplicate(double testv[3]) const;
  void planesRHS(int p1, int p2, int p3, double r[3]) const;
  int outsideRegion(double v[3]) ;

  // plane equations
  double *Plane;

  // vertices of convex regions enclosed by the planes, also
  //    the ccw hull of that region projected in 3 orthog. directions
  vtkPointsProjectedHull *regionPts;

  vtkPlanesIntersection(const vtkPlanesIntersection&); // Not implemented
  void operator=(const vtkPlanesIntersection&); // Not implemented
};
#endif


