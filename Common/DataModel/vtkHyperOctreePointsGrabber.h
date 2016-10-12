/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreePointsGrabber.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperOctreePointsGrabber
 * @brief   An object used by filters to
 * store points computed on face or edge of an hyperoctant. It is an
 * abstract class. vtkClipHyperOctree and vtkHyperOctreeCutter use
 * vtkHyperOctreeClipCutPointsGrabber
 * vtkHyperOctreeContourFilter use an internal one:
 * vtkHyperOctreeContourFilterPointsGrabber.
 *
 * @sa
 * vtkHyperOctree, vtkHyperOctreeClipCutPointsGrabber,
 * vtkClipHyperOctree, vtkHyperOctreeCutter
*/

#ifndef vtkHyperOctreePointsGrabber_h
#define vtkHyperOctreePointsGrabber_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONDATAMODEL_EXPORT vtkHyperOctreePointsGrabber : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperOctreePointsGrabber,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the dimension of the hyperoctree.
   * \post valid_result: (result==2 || result==3)
   */
  int GetDimension();

  /**
   * Set the dimension of the hyperoctree.
   * \pre valid_dim: (dim==2 || dim==3)
   * \post is_set: GetDimension()==dim
   */
  virtual void SetDimension(int dim)=0;

  /**
   * Initialize the points insertion scheme.
   * Actually, it is just a trick to initialize the IdSet from the filter.
   * The IdSet class cannot be shared with the filter because it is a Pimpl.
   * It is used by clip,cut and contour filters to build the points
   * that lie on an hyperoctant.
   * \pre only_in_3d: GetDimension()==3
   */
  virtual void InitPointInsertion()=0;


  /**
   * Insert a point, assuming the point is unique and does not require a
   * locator. Tt does not mean it does not use a locator. It just mean that
   * some implementation may skip the use of a locator.
   */
  virtual void InsertPoint(vtkIdType ptId,
                           double pt[3],
                           double pcoords[3],
                           int ijk[3])=0;

  /**
   * Insert a point using a locator.
   */
  virtual void InsertPointWithMerge(vtkIdType ptId,
                                    double pt[3],
                                    double pcoords[3],
                                    int ijk[3])=0;

  /**
   * Insert a point in the quadtree case.
   */
  virtual void InsertPoint2D(double pt[3],
                             int ijk[3])=0;

protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperOctreePointsGrabber();
  ~vtkHyperOctreePointsGrabber() VTK_OVERRIDE;

  int Dimension;

private:
  vtkHyperOctreePointsGrabber(const vtkHyperOctreePointsGrabber&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperOctreePointsGrabber&) VTK_DELETE_FUNCTION;
};

#endif
