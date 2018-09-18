/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCameraRepresentation
 * @brief   represent the vtkCameraWidget
 *
 * This class provides support for interactively saving a series of camera
 * views into an interpolated path (using vtkCameraInterpolator). The class
 * typically works in conjunction with vtkCameraWidget. To use this class
 * simply specify the camera to interpolate and use the methods
 * AddCameraToPath(), AnimatePath(), and InitializePath() to add a new camera
 * view, animate the current views, and initialize the interpolation.
 *
 * @sa
 * vtkCameraWidget vtkCameraInterpolator
*/

#ifndef vtkCameraRepresentation_h
#define vtkCameraRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkCamera;
class vtkCameraInterpolator;
class vtkPoints;
class vtkPolyData;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper2D;
class vtkProperty2D;
class vtkActor2D;

class VTKINTERACTIONWIDGETS_EXPORT vtkCameraRepresentation : public vtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkCameraRepresentation *New();

  //@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkCameraRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the camera to interpolate. This must be specified by
   * the user.
   */
  void SetCamera(vtkCamera *camera);
  vtkGetObjectMacro(Camera,vtkCamera);
  //@}

  //@{
  /**
   * Get the vtkCameraInterpolator used to interpolate and save the
   * sequence of camera views. If not defined, one is created
   * automatically. Note that you can access this object to set
   * the interpolation type (linear, spline) and other instance
   * variables.
   */
  void SetInterpolator(vtkCameraInterpolator *camInt);
  vtkGetObjectMacro(Interpolator,vtkCameraInterpolator);
  //@}

  //@{
  /**
   * Set the number of frames to generate when playback is initiated.
   */
  vtkSetClampMacro(NumberOfFrames,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfFrames,int);
  //@}

  //@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  vtkGetObjectMacro(Property,vtkProperty2D);
  //@}

  //@{
  /**
   * These methods are used to create interpolated camera paths.  The
   * AddCameraToPath() method adds the view defined by the current camera
   * (via SetCamera()) to the interpolated camera path. AnimatePath()
   * interpolates NumberOfFrames along the current path. InitializePath()
   * resets the interpolated path to its initial, empty configuration.
   */
  void AddCameraToPath();
  void AnimatePath(vtkRenderWindowInteractor *rwi);
  void InitializePath();
  //@}

  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
    {size[0]=6.0; size[1]=2.0;}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  vtkCameraRepresentation();
  ~vtkCameraRepresentation() override;

  // the camera and the interpolator
  vtkCamera             *Camera;
  vtkCameraInterpolator *Interpolator;
  int                   NumberOfFrames;
  double                CurrentTime;

  // representation of the camera
  vtkPoints                  *Points;
  vtkPolyData                *PolyData;
  vtkTransformPolyDataFilter *TransformFilter;
  vtkPolyDataMapper2D        *Mapper;
  vtkProperty2D              *Property;
  vtkActor2D                 *Actor;

private:
  vtkCameraRepresentation(const vtkCameraRepresentation&) = delete;
  void operator=(const vtkCameraRepresentation&) = delete;
};

#endif
