/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkGeoInteractorStyle
 * @brief   Interaction for a globe
 *
 *
 * vtkGeoInteractorStyle contains interaction capabilities for a geographic
 * view including orbit, zoom, and tilt. It also includes a compass widget
 * for changing view parameters.
 *
 * @sa
 * vtkCompassWidget vtkInteractorStyle
*/

#ifndef vtkGeoInteractorStyle_h
#define vtkGeoInteractorStyle_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h" // for SP

class vtkCamera;
class vtkCommand;
class vtkCompassWidget;
class vtkGeoCamera;
class vtkUnsignedCharArray;

class VTKGEOVISCORE_EXPORT vtkGeoInteractorStyle :
  public vtkInteractorStyleTrackballCamera
{
public:
  static vtkGeoInteractorStyle *New();
  vtkTypeMacro(vtkGeoInteractorStyle,
                       vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings
   */
  void OnEnter() VTK_OVERRIDE;
  void OnLeave() VTK_OVERRIDE;
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnChar() VTK_OVERRIDE;
  //@}

  virtual void RubberBandZoom();
  void Pan() VTK_OVERRIDE;
  void Dolly() VTK_OVERRIDE;

  // Public for render callback.
  void RedrawRectangle();

  // See cxx for description of why we need this method.
  void StartState(int newstate) VTK_OVERRIDE;

  // Used for updating the terrain.
  vtkGeoCamera* GetGeoCamera();

  /**
   * This can be used to set the camera to the standard view of the earth.
   */
  void ResetCamera();

  //! Called when the sub widgets have an interaction
  void WidgetInteraction(vtkObject *caller);

  /**
   * Set/Get the Interactor wrapper being controlled by this object.
   * (Satisfy superclass API.)
   */
  void SetInteractor(vtkRenderWindowInteractor *interactor) VTK_OVERRIDE;

  int ViewportToWorld(double x, double y,
                      double &wx, double &wy, double &wz);
  void WorldToLongLat(double wx, double wy, double wz,
                      double &lon, double &lat);
  void ViewportToLongLat(double x, double y,
                         double &lon, double &lat);
  int GetRayIntersection(double origin[3],
                         double direction[3],
                         double intersection[3]);

  /**
   * Override to make the renderer use this camera subclass
   */
  void SetCurrentRenderer(vtkRenderer*) VTK_OVERRIDE;

  //@{
  /**
   * Whether to lock the heading a particular value during pan.
   */
  vtkGetMacro(LockHeading, bool);
  vtkSetMacro(LockHeading, bool);
  vtkBooleanMacro(LockHeading, bool);
  //@}

  /**
   * Called after camera properties are modified
   */
  void ResetCameraClippingRange();

protected:
  vtkGeoInteractorStyle();
  ~vtkGeoInteractorStyle() VTK_OVERRIDE;

  // To avoid a warning.
  // We should really inherit directy from vtkInteractorStyle
  void Dolly(double) VTK_OVERRIDE;

  void OnTimer() VTK_OVERRIDE;
  // Used to get a constant speed regardless of frame rate.
  double LastTime;

  // Rubberband zoom has a verification stage.
  int RubberBandExtent[4];
  int RubberBandExtentEnabled;
  int RenderCallbackTag;
  void EnableRubberBandRedraw();
  void DisableRubberBandRedraw();
  bool InRubberBandRectangle(int x, int y);
  void DrawRectangle();

  void KeepCameraAboveGround(vtkCamera* camera);
  void UpdateLights();
  void GetPanCenter(double &px, double &py);

  int StartPosition[2];
  int EndPosition[2];
  int DraggingRubberBandBoxState;
  double MotionFactor;
  vtkUnsignedCharArray *PixelArray;
  int PixelDims[2];
  bool LockHeading;

  vtkSmartPointer<vtkGeoCamera> GeoCamera;

  // widget handling members
  vtkSmartPointer<vtkCompassWidget> CompassWidget;
  vtkSmartPointer<vtkCommand> EventCommand;

private:
  vtkGeoInteractorStyle(const vtkGeoInteractorStyle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoInteractorStyle&) VTK_DELETE_FUNCTION;
};

#endif
