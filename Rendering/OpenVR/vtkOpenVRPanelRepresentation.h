/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkImplicitPlaneRepresentation.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkOpenVRPanelRepresentation
* @brief   Widget representation for vtkOpenVRPanelWidget
* Implementation of the popup panel representation for the
* vtkOpenVRPanelWidget.
* This representation is rebuilt every time the hovered prop changes.
* Its position is set according to the camera orientation and is placed at a
* distance defined in meters in the BuildRepresentation() method.
*
* WARNING: The panel might be occluded by other props.
*   TODO: Improve placement method.
**/

#ifndef vtkOpenVRPanelRepresentation_h
#define vtkOpenVRPanelRepresentation_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkStdString.h"

class vtkActor;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkCellArray;
class vtkPoints;
class vtkTextActor3D;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRPanelRepresentation : public vtkWidgetRepresentation
{
public:
  /**
  * Instantiate the class.
  */
  static vtkOpenVRPanelRepresentation *New();

  //@{
  /**
  * Standard methods for the class.
  */
  vtkTypeMacro(vtkOpenVRPanelRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
  * Methods to interface with the vtkOpenVRPanelWidget.
  */
  void BuildRepresentation() VTK_OVERRIDE;
  void StartWidgetInteraction(double eventPos[3]) VTK_OVERRIDE;
  void EndWidgetInteraction(double newEventPos[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
  * Methods supporting the rendering process.
  */
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) VTK_OVERRIDE;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;
  //@}

  //@{
  /**
  * Set Tooltip text (used by TextActor)
  */  void SetText(vtkStdString* str);
  //@}

  //@{
  /**
  * Set the vtkProp that is being hovered over
  */
  void SetHoveredProp(vtkProp* prop)
  {
    if (!prop)
    {
      return;
    }
    this->HoveredProp = prop;
  };
  //@}

  vtkGetMacro(PhysicalScale, double);

protected:
  vtkOpenVRPanelRepresentation();
  ~vtkOpenVRPanelRepresentation() VTK_OVERRIDE;

  // The text
  vtkTextActor3D *TextActor;
  vtkStdString Text;

  vtkProp* HoveredProp;

  double PhysicalScale;

private:
  vtkOpenVRPanelRepresentation(const vtkOpenVRPanelRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenVRPanelRepresentation&)VTK_DELETE_FUNCTION;
};

#endif
