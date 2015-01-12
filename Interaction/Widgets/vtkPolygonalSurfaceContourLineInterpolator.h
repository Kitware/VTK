/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalSurfaceContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolygonalSurfaceContourLineInterpolator - Contour interpolator for to place points on polygonal surfaces.
//
// .SECTION Description
// vtkPolygonalSurfaceContourLineInterpolator interpolates and places
// contour points on polygonal surfaces. The class interpolates nodes by
// computing a \em graph \em geodesic laying on the polygonal data. By \em
// graph \em Geodesic, we mean that the line interpolating the two end
// points traverses along on the mesh edges so as to form the shortest
// path. A Dijkstra algorithm is used to compute the path. See
// vtkDijkstraGraphGeodesicPath.
//
// The class is mean to be used in conjunction with
// vtkPolygonalSurfacePointPlacer. The reason for this weak coupling is a
// performance issue, both classes need to perform a cell pick, and
// coupling avoids multiple cell picks (cell picks are slow).
//
// .SECTION Caveats
// You should have computed cell normals for the input polydata.
//
// .SECTION See Also
// vtkDijkstraGraphGeodesicPath, vtkPolyDataNormals

#ifndef vtkPolygonalSurfaceContourLineInterpolator_h
#define vtkPolygonalSurfaceContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataContourLineInterpolator.h"

class vtkDijkstraGraphGeodesicPath;
class vtkIdList;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolygonalSurfaceContourLineInterpolator : public vtkPolyDataContourLineInterpolator
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkPolygonalSurfaceContourLineInterpolator, vtkPolyDataContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPolygonalSurfaceContourLineInterpolator *New();

  // Description:
  // Subclasses that wish to interpolate a line segment must implement this.
  // For instance vtkBezierContourLineInterpolator adds nodes between idx1
  // and idx2, that allow the contour to adhere to a bezier curve.
  virtual int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 );

  // Description:
  // The interpolator is given a chance to update the node.
  // vtkImageContourLineInterpolator updates the idx'th node in the contour,
  // so it automatically sticks to edges in the vicinity as the user
  // constructs the contour.
  // Returns 0 if the node (world position) is unchanged.
  virtual int UpdateNode( vtkRenderer *,
                          vtkContourRepresentation *,
                          double * vtkNotUsed(node), int vtkNotUsed(idx) );

  // Description:
  // Height offset at which points may be placed on the polygonal surface.
  // If you specify a non-zero value here, be sure to have computed vertex
  // normals on your input polygonal data. (easily done with
  // vtkPolyDataNormals).
  vtkSetMacro( DistanceOffset, double );
  vtkGetMacro( DistanceOffset, double );

  // Description:
  // Get the contour point ids. These point ids correspond to those on the
  // polygonal surface
  void GetContourPointIds( vtkContourRepresentation *rep, vtkIdList *idList );

protected:
  vtkPolygonalSurfaceContourLineInterpolator();
  ~vtkPolygonalSurfaceContourLineInterpolator();

  // Description:
  // Draw the polyline at a certain height (in the direction of the vertex
  // normal) above the polydata.
  double         DistanceOffset;

private:
  vtkPolygonalSurfaceContourLineInterpolator(const vtkPolygonalSurfaceContourLineInterpolator&);  //Not implemented
  void operator=(const vtkPolygonalSurfaceContourLineInterpolator&);  //Not implemented

  // Cache the last used vertex id's (start and end).
  // If they are the same, don't recompute.
  vtkIdType      LastInterpolatedVertexIds[2];

  vtkDijkstraGraphGeodesicPath* DijkstraGraphGeodesicPath;
};

#endif
