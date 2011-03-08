/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundedPlanePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBoundedPlanePointPlacer - a placer that constrains a handle to a finite plane
// .SECTION Description
// vtkBoundedPlanePointPlacer is a type of point placer that constrains its
// points to a finite (i.e., bounded) plance.
//
// .SECTION See Also
// vtkPointPlacer vtkHandleWidget vtkHandleRepresentation

#ifndef __vtkBoundedPlanePointPlacer_h
#define __vtkBoundedPlanePointPlacer_h

#include "vtkPointPlacer.h"

class vtkPlane;
class vtkPlaneCollection;
class vtkPlanes;
class vtkRenderer;


class VTK_WIDGETS_EXPORT vtkBoundedPlanePointPlacer : public vtkPointPlacer
{
public:
  // Description:
  // Instantiate this class.
  static vtkBoundedPlanePointPlacer *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkBoundedPlanePointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the projection normal to lie along the x, y, or z axis,
  // or to be oblique. If it is oblique, then the plane is 
  // defined in the ObliquePlane ivar.
  vtkSetClampMacro(ProjectionNormal,int,
                   vtkBoundedPlanePointPlacer::XAxis,
                   vtkBoundedPlanePointPlacer::Oblique);
  vtkGetMacro(ProjectionNormal,int);
  void SetProjectionNormalToXAxis()
    { this->SetProjectionNormal(vtkBoundedPlanePointPlacer::XAxis); }
  void SetProjectionNormalToYAxis()
    { this->SetProjectionNormal(vtkBoundedPlanePointPlacer::YAxis); }
  void SetProjectionNormalToZAxis()
    { this->SetProjectionNormal(vtkBoundedPlanePointPlacer::ZAxis); }
  void SetProjectionNormalToOblique()
    { this->SetProjectionNormal(vtkBoundedPlanePointPlacer::Oblique); }

  // Description:
  // If the ProjectionNormal is set to Oblique, then this is the 
  // oblique plane used to constrain the handle position.
  void SetObliquePlane(vtkPlane *);

  // Description:
  // The position of the bounding plane from the origin along the
  // normal. The origin and normal are defined in the oblique plane
  // when the ProjectionNormal is oblique. For the X, Y, and Z
  // axes projection normals, the normal is the axis direction, and
  // the origin is (0,0,0).
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);

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
  
//BTX
  enum
  {
    XAxis=0,
    YAxis,
    ZAxis,
    Oblique
  };
//ETX
  
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
  // Given a renderer, a display position, and a reference world
  // position, compute the new world position and orientation 
  // of this point. This method is typically used by the 
  // representation to move the point.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2], 
                                    double refWorldPos[3],
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
  
  // Description:
  // If the constraints on this placer are changed, then
  // this method will be called by the representation on
  // each of its points. For this placer, the world
  // position will be converted to a display position, then
  // ComputeWorldPosition will be used to update the 
  // point.
  virtual int UpdateWorldPosition( vtkRenderer *ren,
                                   double worldPos[3],
                                   double worldOrient[9] );
  

protected:
  vtkBoundedPlanePointPlacer();
  ~vtkBoundedPlanePointPlacer();

  // Indicates the projection normal as lying along the
  // XAxis, YAxis, ZAxis, or Oblique. For X, Y, and Z axes,
  // the projection normal is assumed to be anchored at
  // (0,0,0)
  int             ProjectionNormal;

  // Indicates a distance from the origin of the projection
  // normal where the project plane will be placed
  double          ProjectionPosition;
  
  // If the ProjectionNormal is oblique, this is the oblique
  // plane
  vtkPlane        *ObliquePlane;

  // A collection of planes used to bound the projection
  // plane
  vtkPlaneCollection *BoundingPlanes;

  // Internal method for getting the project normal as a vector
  void GetProjectionNormal( double normal[3] );
  
  // Internal method for getting the origin of the 
  // constraining plane as a 3-tuple
  void GetProjectionOrigin( double origin[3] );

  // Internal method for getting the orientation of
  // the projection plane
  void GetCurrentOrientation( double worldOrient[9] );
  
  // Calculate the distance of a point from the Object. Negative 
  // values imply that the point is outside. Positive values imply that it is
  // inside. The closest point to the object is returned in closestPt. 
  static double GetDistanceFromObject( double               pos[3],
                                       vtkPlaneCollection * pc,
                                       double               closestPt[3]);
  
private:
  vtkBoundedPlanePointPlacer(const vtkBoundedPlanePointPlacer&);  //Not implemented
  void operator=(const vtkBoundedPlanePointPlacer&);  //Not implemented
};

#endif
