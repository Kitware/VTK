/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedSizeHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFixedSizeHandleRepresentation - A marker that has the same size in pixels.
//
// .SECTION Description
// This class is a concrete implementation of vtkHandleRepresentation. It is
// meant to be used as a representation for vtkHandleWidget. Unlike the other
// represenations, this can maintain a constant size in pixels, regardless of
// the camera zoom parameters. The size in pixels may be set via
// SetHandleSizeInPixels. This representation renders the markers as spherical
// blobs in 3D space with the width as specified above, defaults to 10 pixels.
// The handles will have the same size in pixels, give or take a certain
// tolerance, as specified by SetHandleSizeToleranceInPixels. The tolerance
// defaults to half a pixel. PointPlacers may be used to specify constraints on
// the placement of markers. For instance a vtkPolygonalSurfacePointPlacer
// will constrain placement of these spherical handles to a surface mesh.
//
// .SECTION See Also
// vtkHandleRepresentation vtkHandleWidget

#ifndef vtkFixedSizeHandleRepresentation3D_h
#define vtkFixedSizeHandleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolygonalHandleRepresentation3D.h"

class vtkSphereSource;

class VTKINTERACTIONWIDGETS_EXPORT vtkFixedSizeHandleRepresentation3D : public vtkPolygonalHandleRepresentation3D
{
public:

  // Description:
  // Instantiate this class.
  static vtkFixedSizeHandleRepresentation3D *New();

  // Description:
  // Standard vtk methods
  vtkTypeMacro(vtkFixedSizeHandleRepresentation3D,
               vtkPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the object used to render the spherical handle marker
  vtkGetObjectMacro( SphereSource, vtkSphereSource );

  // Description:
  // Set/Get the required handle size in pixels. Defaults to a width of
  // 10 pixels.
  vtkSetMacro( HandleSizeInPixels, double );
  vtkGetMacro( HandleSizeInPixels, double );

  // Description:
  // Specify the acceptable handle size tolerance. During each render, the
  // handle 3D source will be updated to automatically match a display size
  // as specified by HandleSizeInPixels. This update will be done if the
  // handle size is larger than a tolerance. Default value of this
  // tolerance is half a pixel.
  vtkSetMacro( HandleSizeToleranceInPixels, double );
  vtkGetMacro( HandleSizeToleranceInPixels, double );

protected:
  vtkFixedSizeHandleRepresentation3D();
  ~vtkFixedSizeHandleRepresentation3D();

  // Description:
  // Recomputes the handle world size based on the set display size.
  virtual void BuildRepresentation();

  // Description:
  // Convenience method to convert from world to display
  void WorldToDisplay( double w[4], double d[4] );

  // Description:
  // Convenience method to convert from display to world
  void DisplayToWorld( double d[4], double w[4] );

  vtkSphereSource *                           SphereSource;
  double                                      HandleSizeInPixels;
  double                                      HandleSizeToleranceInPixels;

private:
  vtkFixedSizeHandleRepresentation3D(const vtkFixedSizeHandleRepresentation3D&);  //Not implemented
  void operator=(const vtkFixedSizeHandleRepresentation3D&);  //Not implemented
};

#endif
