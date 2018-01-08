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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Event bindings
   */
  void OnEnter() override;
  void OnLeave() override;
  void OnMouseMove() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonUp() override;
  void OnLeftButtonDown() override;
  void OnMiddleButtonDown() override;
  void OnRightButtonDown() override;
  void OnChar() override;
  //@}

  virtual void RubberBandZoom();
  void Pan() override;
  void Dolly() override;

  // Public for render callback.
  void RedrawRectangle();

  // See cxx for description of why we need this method.
  void StartState(int newstate) override;

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
  void SetInteractor(vtkRenderWindowInteractor *interactor) override;

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
  void SetCurrentRenderer(vtkRenderer*) override;

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
  ~vtkGeoInteractorStyle() override;

  // To avoid a warning.
  // We should really inherit directly from vtkInteractorStyle
  void Dolly(double) override;

  void OnTimer() override;
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
  vtkUnsignedCharArray *PixelArray;
  int PixelDims[2];
  bool LockHeading;

  vtkSmartPointer<vtkGeoCamera> GeoCamera;

  // widget handling members
  vtkSmartPointer<vtkCompassWidget> CompassWidget;
  vtkSmartPointer<vtkCommand> EventCommand;

private:
  vtkGeoInteractorStyle(const vtkGeoInteractorStyle&) = delete;
  void operator=(const vtkGeoInteractorStyle&) = delete;
};

#endif
