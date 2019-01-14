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
/**
 * @class   vtkImageOrthoPlanes
 * @brief   Connect three vtkImagePlaneWidgets together
 *
 * vtkImageOrthoPlanes is an event observer class that listens to the
 * events from three vtkImagePlaneWidgets and keeps their orientations
 * and scales synchronized.
 * @sa
 * vtkImagePlaneWidget
 * @par Thanks:
 * Thanks to Atamai Inc. for developing and contributing this class.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * You must set three planes for the widget.
   */
  void SetPlane(int i, vtkImagePlaneWidget *imagePlaneWidget);
  vtkImagePlaneWidget* GetPlane(int i);
  //@}

  /**
   * Reset the planes to original scale, rotation, and location.
   */
  void ResetPlanes();

  /**
   * Get the transform for the planes.
   */
  vtkTransform *GetTransform() { return this->Transform; };

  /**
   * A public method to be used only by the event callback.
   */
  void HandlePlaneEvent(vtkImagePlaneWidget *imagePlaneWidget);

protected:
  vtkImageOrthoPlanes();
  ~vtkImageOrthoPlanes() override;

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
  vtkImageOrthoPlanes(const vtkImageOrthoPlanes&) = delete;
  void operator=(const vtkImageOrthoPlanes&) = delete;
};

#endif


