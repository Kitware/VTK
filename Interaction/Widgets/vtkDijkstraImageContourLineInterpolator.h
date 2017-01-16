/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraImageContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDijkstraImageContourLineInterpolator
 * @brief   Contour interpolator for placing points on an image.
 *
 *
 * vtkDijkstraImageContourLineInterpolator interpolates and places
 * contour points on images. The class interpolates nodes by
 * computing a graph laying on the image data. By graph, we mean
 * that the line interpolating the two end points traverses along
 * pixels so as to form a shortest path. A Dijkstra algorithm is
 * used to compute the path.
 *
 * The class is meant to be used in conjunction with
 * vtkImageActorPointPlacer. One reason for this coupling is a
 * performance issue: both classes need to perform a cell pick, and
 * coupling avoids multiple cell picks (cell picks are slow).  Another
 * issue is that the interpolator may need to set the image input to
 * its vtkDijkstraImageGeodesicPath ivar.
 *
 * @sa
 * vtkContourWidget vtkContourLineInterpolator vtkDijkstraImageGeodesicPath
*/

#ifndef vtkDijkstraImageContourLineInterpolator_h
#define vtkDijkstraImageContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkContourLineInterpolator.h"

class vtkDijkstraImageGeodesicPath;
class vtkImageData;

class VTKINTERACTIONWIDGETS_EXPORT vtkDijkstraImageContourLineInterpolator
                       : public vtkContourLineInterpolator
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkDijkstraImageContourLineInterpolator,
                              vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  static vtkDijkstraImageContourLineInterpolator *New();

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance vtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 ) VTK_OVERRIDE;

  //@{
  /**
   * Set the image data for the vtkDijkstraImageGeodesicPath.
   * If not set, the interpolator uses the image data input to the image actor.
   * The image actor is obtained from the expected vtkImageActorPointPlacer.
   */
  virtual void SetCostImage( vtkImageData* );
  vtkGetObjectMacro( CostImage, vtkImageData );
  //@}

  //@{
  /**
   * access to the internal dijkstra path
   */
  vtkGetObjectMacro( DijkstraImageGeodesicPath, vtkDijkstraImageGeodesicPath );
  //@}

protected:
  vtkDijkstraImageContourLineInterpolator();
  ~vtkDijkstraImageContourLineInterpolator() VTK_OVERRIDE;

  vtkImageData *CostImage;
  vtkDijkstraImageGeodesicPath *DijkstraImageGeodesicPath;

private:
  vtkDijkstraImageContourLineInterpolator(
    const vtkDijkstraImageContourLineInterpolator&) VTK_DELETE_FUNCTION;
  void operator=(const
    vtkDijkstraImageContourLineInterpolator&) VTK_DELETE_FUNCTION;
};

#endif
