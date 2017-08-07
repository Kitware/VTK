/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkOpenVRPanelWidget
* @brief   3D widget to popup a tooltip when a prop is hovered by a controller.
* This widget checks on every Move3DEvent if the openVR interactor style picker
* is returning a prop3D. If so, it tries to find the corresponding
* tooltip in its map of (vtkProp, vtkStdString) and build its representation.
* Use AddTooltip(vtkProp *prop, vtkStdString* str) to associate a tooltip to a
* vtkProp.
*
* @sa
* vtkBalloonWidget vtkOpenVRPanelRepresentation vtkOpenVRPropPicker
*/

#ifndef vtkOpenVRPanelWidget_h
#define vtkOpenVRPanelWidget_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkAbstractWidget.h"
#include "vtkEventData.h"

class vtkOpenVRPanelRepresentation;
class vtkPropMap;
class vtkProp;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRPanelWidget : public vtkAbstractWidget
{
public:
  /**
  * Instantiate the object.
  */
  static vtkOpenVRPanelWidget *New();

  //@{
  /**
  * Standard vtkObject methods
  */
  vtkTypeMacro(vtkOpenVRPanelWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
  * Specify an instance of vtkWidgetRepresentation used to represent this
  * widget in the scene. Note that the representation is a subclass of vtkProp
  * so it can be added to the renderer independent of the widget.
  */
  void SetRepresentation(vtkOpenVRPanelRepresentation *rep);

  /**
  * Create the default widget representation if one is not set.
  */
  void CreateDefaultRepresentation() VTK_OVERRIDE;

  //@{
  /**
  * Create a tooltip associated to a prop.
  * Note that if the tooltip is already assigned to this prop,
  * its text will be replaced
  */
  void AddTooltip(vtkProp *prop, vtkStdString* str);
  void AddTooltip(vtkProp *prop, const char* str);
  //@}

protected:
  vtkOpenVRPanelWidget();
  ~vtkOpenVRPanelWidget() VTK_OVERRIDE;

  /**
  * Update callback to check for the hovered prop
  */
  static void Update(vtkAbstractWidget*);

  vtkPropMap *PropMap; //PIMPL'd map of (vtkProp,char*)

  //Indicates if a device is hovering a prop
  int HoveringDevice[vtkEventDataNumberOfDevices];

private:
  vtkOpenVRPanelWidget(const vtkOpenVRPanelWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenVRPanelWidget&)VTK_DELETE_FUNCTION;

};
#endif
