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
// .NAME 
// .SECTION Description
// 
//
// .SECTION See Also

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
  vtkTypeRevisionMacro(vtkBoundedPlanePointPlacer,vtkPointPlacer);
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
  // oblique plane used to constrain the handle position
  void SetObliquePlane(vtkPlane *);

  // Description:
  // The position of the bounding plane from the origin along the
  // normal. The origin and normal are defined in the oblique plane
  // when the ProjectionNormal is Oblique. For the X, Y, and Z
  // axes projection normals, the normal is the axis direction, and
  // the origin is (0,0,0).
  void SetProjectionPosition(double position);
  vtkGetMacro(ProjectionPosition, double);

  // Description:
  // A collection of plane equations used to bound the position of the point.
  // This is in addition to confining the point to a plane - these contraints
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
  
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2], 
                            double worldPos[3],
                            double worldOrient[9] );
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2], 
                            double refWorldPos[2],
                            double worldPos[3],
                            double worldOrient[9] );
  int ValidateWorldPosition( double worldPos[3] );
  int ValidateWorldPosition( double worldPos[3],
                             double worldOrient[9]);
  
protected:
  vtkBoundedPlanePointPlacer();
  ~vtkBoundedPlanePointPlacer();

  // Controlling vars
  int             ProjectionNormal;
  double          ProjectionPosition;
  int             ProjectToPlane;
  vtkPlane        *ObliquePlane;

  vtkPlaneCollection *BoundingPlanes;

  // Internal method for getting the project normal as a vector
  void GetProjectionNormal( double normal[3] );
  
  // Internal method for getting the origin of the 
  // constraining plane as a 3-tuple
  void GetProjectionOrigin( double origin[3] );

  void GetCurrentOrientation( double worldOrient[9] );
  
private:
  vtkBoundedPlanePointPlacer(const vtkBoundedPlanePointPlacer&);  //Not implemented
  void operator=(const vtkBoundedPlanePointPlacer&);  //Not implemented
};

#endif
