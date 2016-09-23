/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFocalPlanePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
//
//
// .SECTION See Also

#ifndef vtkFocalPlanePointPlacer_h
#define vtkFocalPlanePointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkRenderer;

class VTKINTERACTIONWIDGETS_EXPORT vtkFocalPlanePointPlacer : public vtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static vtkFocalPlanePointPlacer *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkFocalPlanePointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  // Descirption:
  // Given a renderer and a display position, compute
  // the world position and orientation. The orientation
  // computed by the placer will always line up with the
  // standard coordinate axes. The world position will be
  // computed by projecting the display position onto the
  // focal plane. This method is typically used to place a
  // point for the first time.
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double worldPos[3],
                            double worldOrient[9] );

  /**
   * Given a renderer, a display position, and a reference
   * world position, compute a new world position. The
   * orientation will be the standard coordinate axes, and the
   * computed world position will be created by projecting
   * the display point onto a plane that is parallel to
   * the focal plane and runs through the reference world
   * position. This method is typically used to move existing
   * points.
   */
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double refWorldPos[3],
                            double worldPos[3],
                            double worldOrient[9] );

  //@{
  /**
   * Validate a world position. All world positions
   * are valid so these methods always return 1.
   */
  int ValidateWorldPosition( double worldPos[3] );
  int ValidateWorldPosition( double worldPos[3],
                             double worldOrient[9]);
  //@}

  //@{
  /**
   * Optionally specify a signed offset from the focal plane for the points to
   * be placed at.  If negative, the constraint plane is offset closer to the
   * camera. If positive, its further away from the camera.
   */
  vtkSetMacro( Offset, double );
  vtkGetMacro( Offset, double );
  //@}

  //@{
  /**
   * Optionally Restrict the points to a set of bounds. The placer will
   * invalidate points outside these bounds.
   */
  vtkSetVector6Macro( PointBounds, double );
  vtkGetVector6Macro( PointBounds, double );
  //@}

protected:
  vtkFocalPlanePointPlacer();
  ~vtkFocalPlanePointPlacer();

  void GetCurrentOrientation( double worldOrient[9] );

  double PointBounds[6];
  double Offset;

private:
  vtkFocalPlanePointPlacer(const vtkFocalPlanePointPlacer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFocalPlanePointPlacer&) VTK_DELETE_FUNCTION;
};

#endif
