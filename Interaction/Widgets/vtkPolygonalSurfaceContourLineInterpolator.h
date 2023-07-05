// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolygonalSurfaceContourLineInterpolator
 * @brief   Contour interpolator for to place points on polygonal surfaces.
 *
 *
 * vtkPolygonalSurfaceContourLineInterpolator interpolates and places
 * contour points on polygonal surfaces. The class interpolates nodes by
 * computing a \em graph \em geodesic laying on the polygonal data. By \em
 * graph \em Geodesic, we mean that the line interpolating the two end
 * points traverses along on the mesh edges so as to form the shortest
 * path. A Dijkstra algorithm is used to compute the path. See
 * vtkDijkstraGraphGeodesicPath.
 *
 * The class is mean to be used in conjunction with
 * vtkPolygonalSurfacePointPlacer. The reason for this weak coupling is a
 * performance issue, both classes need to perform a cell pick, and
 * coupling avoids multiple cell picks (cell picks are slow).
 *
 * @warning
 * You should have computed cell normals for the input polydata.
 *
 * @sa
 * vtkDijkstraGraphGeodesicPath, vtkPolyDataNormals
 */

#ifndef vtkPolygonalSurfaceContourLineInterpolator_h
#define vtkPolygonalSurfaceContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataContourLineInterpolator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDijkstraGraphGeodesicPath;
class vtkIdList;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolygonalSurfaceContourLineInterpolator
  : public vtkPolyDataContourLineInterpolator
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkPolygonalSurfaceContourLineInterpolator, vtkPolyDataContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  static vtkPolygonalSurfaceContourLineInterpolator* New();

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance vtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine(vtkRenderer* ren, vtkContourRepresentation* rep, int idx1, int idx2) override;

  /**
   * The interpolator is given a chance to update the node.
   * vtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(vtkRenderer*, vtkContourRepresentation*, double* vtkNotUsed(node),
    int vtkNotUsed(idx)) override;

  ///@{
  /**
   * Height offset at which points may be placed on the polygonal surface.
   * If you specify a non-zero value here, be sure to have computed vertex
   * normals on your input polygonal data. (easily done with
   * vtkPolyDataNormals).
   */
  vtkSetMacro(DistanceOffset, double);
  vtkGetMacro(DistanceOffset, double);
  ///@}

  /**
   * Get the contour point ids. These point ids correspond to those on the
   * polygonal surface
   */
  void GetContourPointIds(vtkContourRepresentation* rep, vtkIdList* ids);

protected:
  vtkPolygonalSurfaceContourLineInterpolator();
  ~vtkPolygonalSurfaceContourLineInterpolator() override;

  /**
   * Draw the polyline at a certain height (in the direction of the vertex
   * normal) above the polydata.
   */
  double DistanceOffset;

private:
  vtkPolygonalSurfaceContourLineInterpolator(
    const vtkPolygonalSurfaceContourLineInterpolator&) = delete;
  void operator=(const vtkPolygonalSurfaceContourLineInterpolator&) = delete;

  // Cache the last used vertex id's (start and end).
  // If they are the same, don't recompute.
  vtkIdType LastInterpolatedVertexIds[2];

  vtkDijkstraGraphGeodesicPath* DijkstraGraphGeodesicPath;
};

VTK_ABI_NAMESPACE_END
#endif
