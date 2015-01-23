/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClosedSurfacePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClosedSurfacePointPlacer - PointPlacer to constrain validity within a set of convex planes
// .SECTION Description
// This placer takes a set of boudning planes and constraints the validity
// within the supplied convex planes. It is used by the
// ParallelopPipedRepresentation to place constraints on the motion the
// handles within the parallelopiped.
//
// .SECTION See Also
// vtkParallelopipedRepresentation

#ifndef vtkClosedSurfacePointPlacer_h
#define vtkClosedSurfacePointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkPlane;
class vtkPlaneCollection;
class vtkPlanes;
class vtkRenderer;

class VTKINTERACTIONWIDGETS_EXPORT vtkClosedSurfacePointPlacer : public vtkPointPlacer
{
public:
  // Description:
  // Instantiate this class.
  static vtkClosedSurfacePointPlacer *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkClosedSurfacePointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A collection of plane equations used to bound the position of the point.
  // This is in addition to confining the point to a plane - these constraints
  // are meant to, for example, keep a point within the extent of an image.
  // Using a set of plane equations allows for more complex bounds (such as
  // bounding a point to an oblique reliced image that has hexagonal shape)
  // than a simple extent.
  void AddBoundingPlane(vtkPlane *plane);
  void RemoveBoundingPlane(vtkPlane *plane);
  void RemoveAllBoundingPlanes();
  virtual void SetBoundingPlanes(vtkPlaneCollection*);
  vtkGetObjectMacro(BoundingPlanes,vtkPlaneCollection);
  void SetBoundingPlanes(vtkPlanes *planes);

  // Description:
  // Given a renderer and a display position, compute the
  // world position and world orientation for this point.
  // A plane is defined by a combination of the
  // ProjectionNormal, ProjectionOrigin, and ObliquePlane
  // ivars. The display position is projected onto this
  // plane to determine a world position, and the
  // orientation is set to the normal of the plane. If
  // the point cannot project onto the plane or if it
  // falls outside the bounds imposed by the
  // BoundingPlanes, then 0 is returned, otherwise 1 is
  // returned to indicate a valid return position and
  // orientation.
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double worldPos[3],
                            double worldOrient[9] );

  // Description:
  // Given a renderer, a display position and a reference position, "worldPos"
  // is calculated as :
  //   Consider the line "L" that passes through the supplied "displayPos" and
  // is parallel to the direction of projection of the camera. Clip this line
  // segment with the parallelopiped, let's call it "L_segment". The computed
  // world position, "worldPos" will be the point on "L_segment" that is
  // closest to refWorldPos.
  // NOTE: Note that a set of bounding planes must be supplied. The Oblique
  //       plane, if supplied is ignored.
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2],
                            double refWorldPos[2],
                            double worldPos[3],
                            double worldOrient[9] );

  // Description:
  // Give a world position check if it is valid - does
  // it lie on the plane and within the bounds? Returns
  // 1 if it is valid, 0 otherwise.
  int ValidateWorldPosition( double worldPos[3] );

  // Descrption:
  // Orientationation is ignored, and the above method
  // is called instead.
  int ValidateWorldPosition( double worldPos[3],
                             double worldOrient[9]);

  // Descrption:
  // The minimum distance the object should be from the faces of the object.
  // Must be greater than 0. Default is 0.
  vtkSetClampMacro( MinimumDistance, double, 0.0, VTK_DOUBLE_MAX );
  vtkGetMacro( MinimumDistance, double );

protected:
  vtkClosedSurfacePointPlacer();
  ~vtkClosedSurfacePointPlacer();

  // A collection of planes used to bound the projection
  // plane
  vtkPlaneCollection *BoundingPlanes;

  // Calculate the distance of a point from the Object. Negative
  // values imply that the point is outside. Positive values imply that it is
  // inside. The closest point to the object is returned in closestPt.
  static double GetDistanceFromObject( double               pos[3],
                                       vtkPlaneCollection * pc,
                                       double               closestPt[3]);

  void BuildPlanes();

  double               MinimumDistance;
  vtkPlaneCollection * InnerBoundingPlanes;

private:
  vtkClosedSurfacePointPlacer(const vtkClosedSurfacePointPlacer&);  //Not implemented
  void operator=(const vtkClosedSurfacePointPlacer&);  //Not implemented
};

#endif
