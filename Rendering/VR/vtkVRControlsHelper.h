/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRControlsHelper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkVRPanelRepresentation
 */

#ifndef vtkVRControlsHelper_h
#define vtkVRControlsHelper_h

#include "vtkEventData.h" // for vtkEventDataDevice
#include "vtkNew.h"       // for iVar
#include "vtkProp.h"
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkWeakPointer.h"       // needed for vtkWeakPointer iVar.
#include <string>                 // for std::string

class vtkActor;
class vtkCallbackCommand;
class vtkLineSource;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkTextActor3D;
class vtkTransform;

class VTKRENDERINGVR_EXPORT vtkVRControlsHelper : public vtkProp
{
public:
  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkVRControlsHelper, vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  enum ButtonSides
  {
    Back = -1,
    Front = 1
  };

  enum DrawSides
  {
    Left = -1,
    Right = 1
  };

  ///@{
  /**
   * Methods to interface with the vtkVRPanelWidget.
   */
  void BuildRepresentation();
  void UpdateRepresentation();
  ///@}

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
   * Set Tooltip text (used by TextActor)
   */
  void SetText(const std::string& str);
  ///@}

  void SetTooltipInfo(const char* s, int buttonSide, int drawSide, const char* txt)
  {
    if (!s || !txt)
    {
      return;
    }
    this->ComponentName = std::string(s);
    this->DrawSide = drawSide;
    this->ButtonSide = buttonSide;
    this->SetText(std::string(txt));
  }

  void SetEnabled(bool enabled);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

  void SetDevice(vtkEventDataDevice val);

  virtual void SetRenderer(vtkRenderer* ren);
  virtual vtkRenderer* GetRenderer();

protected:
  vtkVRControlsHelper();
  ~vtkVRControlsHelper() override;

  double FrameSize[2];

  // The text
  vtkTextActor3D* TextActor;
  std::string Text;

  // The line
  vtkLineSource* LineSource;
  vtkPolyDataMapper* LineMapper;
  vtkActor* LineActor;

  vtkEventDataDevice Device;

  // Tooltip parameters
  std::string ComponentName;
  int DrawSide;   // Left/Right
  int ButtonSide; // Front/Back

  bool Enabled;

  double ControlPositionLC[3];

  // The renderer in which this widget is placed
  vtkWeakPointer<vtkRenderer> Renderer;

  vtkCallbackCommand* MoveCallbackCommand;
  unsigned long ObserverTag;
  static void MoveEvent(vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  /**
   * Must be overridden in subclasses to init the member variable
   * ControlPositionLC to position the tooltip on the component.
   */
  virtual void InitControlPosition() = 0;

  vtkNew<vtkTransform> TempTransform;
  double LastPhysicalTranslation[3];
  double LastEventPosition[3];
  double LastEventOrientation[4];
  bool NeedUpdate;
  bool LabelVisible;

private:
  vtkVRControlsHelper(const vtkVRControlsHelper&) = delete;
  void operator=(const vtkVRControlsHelper&) = delete;
};

#endif
