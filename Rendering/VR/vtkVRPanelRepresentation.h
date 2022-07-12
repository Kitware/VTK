/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRPanelRepresentation.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVRPanelRepresentation
 * @brief   Widget representation for vtkVRPanelWidget
 * Implementation of the popup panel representation for the
 * vtkVRPanelWidget.
 * This representation is rebuilt every time the hovered prop changes.
 * Its position is set according to the camera orientation and is placed at a
 * distance defined in meters in the BuildRepresentation() method.
 *
 * WARNING: The panel might be occluded by other props.
 */

#ifndef vtkVRPanelRepresentation_h
#define vtkVRPanelRepresentation_h

#include "vtkDeprecation.h"       // For VTK_DEPRECATED_IN_9_2_0
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

#include <string> // for ivar

class vtkTextActor3D;

class VTKRENDERINGVR_EXPORT vtkVRPanelRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkVRPanelRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkVRPanelRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget. Note that ComputeInteractionState() and several other methods
  // must be implemented by subclasses.
  enum InteractionStateType
  {
    Outside = 0,
    Moving
  };
#if !defined(VTK_LEGACY_REMOVE)
  VTK_DEPRECATED_IN_9_2_0("because leading underscore is reserved")
  typedef InteractionStateType _InteractionState;
#endif

  ///@{
  /**
   * Methods to interface with the vtkVRPanelWidget.
   */
  void BuildRepresentation() override;
  void PlaceWidget(double bounds[6]) override;
  void StartComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  void EndComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  ///@}

  // Place the widget with a few more options
  // This method allows you to place the panel
  // and provides more options so that you can get
  // the exact positioning you want.
  // Bounds are the bounds that you want the panel to
  // fit within. For World coordinates they should be in
  // world coordinates. For all others they should be in
  // physical meters relative to the HMD or controller origin.
  // The normal is the direction the planel should face.
  // The coordinate system for the controller is X right
  // Y up and Z towards the handle. Upvec specifies the
  // vector to use as up for the panel. Note that upvec
  // has priority over normal, if they are not orthogonal
  // normal will be modified to be orthogonal to upvec.
  // Scale is the physical scale from the RenderWindow
  // and is used to position/scale the panel correctly.
  //
  // Note that you should set the Text on the panel
  // before calling this method as the positioning
  // and scaling is done based on the current text.
  //
  // All vectors will be normalized prior to use.
  void PlaceWidgetExtended(
    const double* bounds, const double* normal, const double* upvec, double scale);

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  ///@{
  /**
   * Set panel text
   */
  void SetText(const char* str);
  ///@}

  // allow access to the underlying text actor
  // so that properties can be set
  vtkGetObjectMacro(TextActor, vtkTextActor3D);

  // Set the coordinate system to use for this prop
  void SetCoordinateSystemToWorld();
  void SetCoordinateSystemToHMD();
  void SetCoordinateSystemToLeftController();
  void SetCoordinateSystemToRightController();

  ///@{
  /**
   * Can the panel be relocated by the user
   */
  vtkSetMacro(AllowAdjustment, bool);
  vtkGetMacro(AllowAdjustment, bool);
  vtkBooleanMacro(AllowAdjustment, bool);
  ///@}

protected:
  vtkVRPanelRepresentation();
  ~vtkVRPanelRepresentation() override;

  // Keep track of event positions
  double LastEventPosition[3];
  double LastEventOrientation[4];
  double StartEventOrientation[4];

  double LastScale;

  bool AllowAdjustment;

  void UpdatePose(double* p1, double* d1, double* p2, double* d2);

  void ComputeMatrix(vtkRenderer* ren);

  enum CoordinateSystems
  {
    World = 0,
    HMD = 1,
    LeftController = 2,
    RightController = 3,
  };

  CoordinateSystems CoordinateSystem;

  // The text
  vtkTextActor3D* TextActor;
  std::string Text;

private:
  vtkVRPanelRepresentation(const vtkVRPanelRepresentation&) = delete;
  void operator=(const vtkVRPanelRepresentation&) = delete;
};

#endif
