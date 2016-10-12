/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActorPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageActorPointPlacer
 * @brief   Converts 2D display positions to world positions such that they lie on an ImageActor
 *
 * This PointPlacer is used to constrain the placement of points on the
 * supplied image actor. Additionally, you may set bounds to restrict the
 * placement of the points. The placement of points will then be constrained
 * to lie not only on the ImageActor but also within the bounds specified.
 * If no bounds are specified, they may lie anywhere on the supplied ImageActor.
*/

#ifndef vtkImageActorPointPlacer_h
#define vtkImageActorPointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkBoundedPlanePointPlacer;
class vtkImageActor;
class vtkRenderer;

class VTKINTERACTIONWIDGETS_EXPORT vtkImageActorPointPlacer : public vtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static vtkImageActorPointPlacer *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkImageActorPointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Given and renderer and a display position in pixels,
   * find a world position and orientation. In this class
   * an internal vtkBoundedPlanePointPlacer is used to compute
   * the world position and orientation. The internal placer
   * is set to use the plane of the image actor and the bounds
   * of the image actor as the constraints for placing points.
   */
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double worldPos[3],
                            double worldOrient[9] );

  /**
   * This method is identical to the one above since the
   * reference position is ignored by the bounded plane
   * point placer.
   */
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double refWorldPos[2],
                            double worldPos[3],
                            double worldOrient[9] );

  /**
   * This method validates a world position by checking to see
   * if the world position is valid according to the constraints
   * of the internal placer (essentially - is this world position
   * on the image?)
   */
  int ValidateWorldPosition( double worldPos[3] );

  /**
   * This method is identical to the one above since the bounded
   * plane point placer ignores orientation
   */
  int ValidateWorldPosition( double worldPos[3],
                             double worldOrient[9]);


  /**
   * Update the world position and orientation according the
   * the current constraints of the placer. Will be called
   * by the representation when it notices that this placer
   * has been modified.
   */
  int UpdateWorldPosition( vtkRenderer *ren,
                           double worldPos[3],
                           double worldOrient[9]);

  /**
   * A method for configuring the internal placer according
   * to the constraints of the image actor.
   * Called by the representation to give the placer a chance
   * to update itself, which may cause the MTime to change,
   * which would then cause the representation to update
   * all of its points
   */
  int UpdateInternalState();

  //@{
  /**
   * Set / get the reference vtkImageActor used to place the points.
   * An image actor must be set for this placer to work. An internal
   * bounded plane point placer is created and set to match the bounds
   * of the displayed image.
   */
  void SetImageActor( vtkImageActor * );
  vtkGetObjectMacro( ImageActor, vtkImageActor );
  //@}

  //@{
  /**
   * Optionally, you may set bounds to restrict the placement of the points.
   * The placement of points will then be constrained to lie not only on
   * the ImageActor but also within the bounds specified. If no bounds are
   * specified, they may lie anywhere on the supplied ImageActor.
   */
  vtkSetVector6Macro( Bounds, double );
  vtkGetVector6Macro( Bounds, double );
  //@}

  /**
   * Set the world tolerance. This propagates it to the internal
   * BoundedPlanePointPlacer.
   */
  virtual void SetWorldTolerance( double s );

protected:
  vtkImageActorPointPlacer();
  ~vtkImageActorPointPlacer();


  // The reference image actor. Must be configured before this placer
  // is used.
  vtkImageActor *ImageActor;

  // The internal placer.
  vtkBoundedPlanePointPlacer *Placer;

  // Used to keep track of whether the bounds of the
  // input image have changed
  double SavedBounds[6];

  // See the SetBounds method
  double Bounds[6];

private:
  vtkImageActorPointPlacer(const vtkImageActorPointPlacer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageActorPointPlacer&) VTK_DELETE_FUNCTION;
};

#endif
