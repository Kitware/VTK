/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeClipCutPointsGrabber.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperOctreeClipCutPointsGrabber
 * @brief   A concrete implementation of
 * vtkHyperOctreePointsGrabber used by vtkClipHyperOctree and
 * vtkHyperOctreeCutter.
 * @sa
 * vtkHyperOctreeClipCut, vtkHyperOctreeClipCutClipCutPointsGrabber,
 * vtkClipHyperOctree, vtkHyperOctreeClipCutCutter
*/

#ifndef vtkHyperOctreeClipCutPointsGrabber_h
#define vtkHyperOctreeClipCutPointsGrabber_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperOctreePointsGrabber.h"

class vtkHyperOctreeIdSet; // Pimpl idiom
class vtkPolygon;
class vtkOrderedTriangulator;


class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeClipCutPointsGrabber : public vtkHyperOctreePointsGrabber
{
public:
  static vtkHyperOctreeClipCutPointsGrabber *New();

  vtkTypeMacro(vtkHyperOctreeClipCutPointsGrabber,vtkHyperOctreePointsGrabber);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the dimension of the hyperoctree.
   * \pre valid_dim: (dim==2 || dim==3)
   * \post is_set: GetDimension()==dim
   */
  void SetDimension(int dim) VTK_OVERRIDE;

  /**
   * Initialize the points insertion scheme.
   * Actually, it is just a trick to initialize the IdSet from the filter.
   * The IdSet class cannot be shared with the filter because it is a Pimpl.
   * It is used by clip,cut and contour filters to build the points
   * that lie on an hyperoctant.
   * \pre only_in_3d: GetDimension()==3
   */
  void InitPointInsertion() VTK_OVERRIDE;

  /**
   * Insert a point, assuming the point is unique and does not require a
   * locator. Tt does not mean it does not use a locator. It just mean that
   * some implementation may skip the use of a locator.
   */
  void InsertPoint(vtkIdType ptId,
                           double pt[3],
                           double pcoords[3],
                           int ijk[3]) VTK_OVERRIDE;

  /**
   * Insert a point using a locator.
   */
  void InsertPointWithMerge(vtkIdType ptId,
                                    double pt[3],
                                    double pcoords[3],
                                    int ijk[3]) VTK_OVERRIDE;

  /**
   * Insert a point in the quadtree case.
   */
  void InsertPoint2D(double pt[3],
                             int ijk[3]) VTK_OVERRIDE;

  /**
   * Return the ordered triangulator.
   */
  vtkOrderedTriangulator *GetTriangulator();

  /**
   * Return the polygon.
   */
  vtkPolygon *GetPolygon();


protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperOctreeClipCutPointsGrabber();
  ~vtkHyperOctreeClipCutPointsGrabber() VTK_OVERRIDE;

  vtkOrderedTriangulator *Triangulator;
  vtkPolygon *Polygon;
  vtkHyperOctreeIdSet *IdSet;

private:
  vtkHyperOctreeClipCutPointsGrabber(const vtkHyperOctreeClipCutPointsGrabber&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperOctreeClipCutPointsGrabber&) VTK_DELETE_FUNCTION;
};

#endif
