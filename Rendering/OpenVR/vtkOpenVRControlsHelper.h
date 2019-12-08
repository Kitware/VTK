/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRControlsHelper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkOpenVRPanelRepresentation
 */

#ifndef vtkOpenVRControlsHelper_h
#define vtkOpenVRControlsHelper_h

#include "vtkEventData.h" // for enums
#include "vtkNew.h"       // for iVar
#include "vtkProp.h"
#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkStdString.h"             // needed for vtkStdString iVar.
#include "vtkWeakPointer.h"           // needed for vtkWeakPointer iVar.

class vtkActor;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkCellArray;
class vtkPoints;
class vtkTextActor3D;
class vtkTransform;

class vtkLineSource;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkCallbackCommand;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRControlsHelper : public vtkProp
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenVRControlsHelper* New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkOpenVRControlsHelper, vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

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

  //@{
  /**
   * Methods to interface with the vtkOpenVRPanelWidget.
   */
  void BuildRepresentation();
  void UpdateRepresentation();
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set Tooltip text (used by TextActor)
   */
  void SetText(vtkStdString str);
  //@}

  void SetTooltipInfo(const char* s, int buttonSide, int drawSide, const char* txt)
  {
    if (!s || !txt)
    {
      return;
    }
    this->ComponentName = vtkStdString(s);
    this->DrawSide = drawSide;
    this->ButtonSide = buttonSide;
    this->SetText(vtkStdString(txt));
  }

  void SetEnabled(bool enabled);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

  void SetDevice(vtkEventDataDevice val);

  virtual void SetRenderer(vtkRenderer* ren);
  virtual vtkRenderer* GetRenderer();

protected:
  vtkOpenVRControlsHelper();
  ~vtkOpenVRControlsHelper() override;

  double FrameSize[2];

  // The text
  vtkTextActor3D* TextActor;
  vtkStdString Text;

  // The line
  vtkLineSource* LineSource;
  vtkPolyDataMapper* LineMapper;
  vtkActor* LineActor;

  vtkEventDataDevice Device;

  // Tooltip parameters
  vtkStdString ComponentName;
  int DrawSide;   // Left/Right
  int ButtonSide; // Front/Back

  bool Enabled;

  double ControlPositionLC[3];

  // The renderer in which this widget is placed
  vtkWeakPointer<vtkRenderer> Renderer;

  vtkCallbackCommand* MoveCallbackCommand;
  unsigned long ObserverTag;
  static void MoveEvent(vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  void InitControlPosition();

  vtkNew<vtkTransform> TempTransform;
  double LastPhysicalTranslation[3];
  double LastEventPosition[3];
  double LastEventOrientation[4];
  bool NeedUpdate;
  bool LabelVisible;

private:
  vtkOpenVRControlsHelper(const vtkOpenVRControlsHelper&) = delete;
  void operator=(const vtkOpenVRControlsHelper&) = delete;
};

#endif
