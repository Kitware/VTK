/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOrthoPlanes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageOrthoPlanes - Connect three vtkImagePlaneWidgets together
// .SECTION Description
// vtkImageOrthoPlanes is an event observer class that listens to the
// events from three vtkImagePlaneWidgets and keeps their orientations
// and scales synchronized.
// .SECTION See Also
// vtkImagePlaneWidget
// .SECTION Thanks
// Thanks to Atamai Inc. for developing and contributing this class.

#ifndef vtkImageOrthoPlanes_h
#define vtkImageOrthoPlanes_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

class vtkImagePlaneWidget;
class vtkTransform;
class vtkMatrix4x4;

class VTKINTERACTIONWIDGETS_EXPORT vtkImageOrthoPlanes : public vtkObject
{
public:
  static vtkImageOrthoPlanes *New();
  vtkTypeMacro(vtkImageOrthoPlanes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // You must set three planes for the widget.
  void SetPlane(int i, vtkImagePlaneWidget *imagePlaneWidget);
  vtkImagePlaneWidget* GetPlane(int i);

  // Description:
  // Reset the planes to original scale, rotation, and location.
  void ResetPlanes();

  // Description:
  // Get the transform for the planes.
  vtkTransform *GetTransform() { return this->Transform; };

  // Description:
  // A public method to be used only by the event callback.
  void HandlePlaneEvent(vtkImagePlaneWidget *imagePlaneWidget);

protected:
  vtkImageOrthoPlanes();
  ~vtkImageOrthoPlanes();

  void HandlePlaneRotation(vtkImagePlaneWidget *imagePlaneWidget,
                           int indexOfModifiedPlane);
  void HandlePlanePush(vtkImagePlaneWidget *imagePlaneWidget,
                       int indexOfModifiedPlane);
  void HandlePlaneTranslate(vtkImagePlaneWidget *imagePlaneWidget,
                            int indexOfModifiedPlane);
  void HandlePlaneScale(vtkImagePlaneWidget *imagePlaneWidget,
                       int indexOfModifiedPlane);

  void SetTransformMatrix(vtkMatrix4x4 *matrix,
                          vtkImagePlaneWidget *currentImagePlane,
                          int indexOfModifiedPlane);

  void GetBounds(double bounds[3]);

  // The plane definitions prior to any rotations or scales
  double Origin[3][3];
  double Point1[3][3];
  double Point2[3][3];

  // The current position and orientation of the bounding box with
  // respect to the origin.
  vtkTransform *Transform;

  // An array to hold the planes
  vtkImagePlaneWidget** Planes;

  // The number of planes.
  int NumberOfPlanes;

  // The observer tags for these planes
  long *ObserverTags;

private:
  vtkImageOrthoPlanes(const vtkImageOrthoPlanes&);  // Not implemented.
  void operator=(const vtkImageOrthoPlanes&);  // Not implemented.
};

#endif


