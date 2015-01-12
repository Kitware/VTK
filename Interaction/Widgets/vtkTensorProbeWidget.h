/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorProbeWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTensorProbeWidget - a widget to probe tensors on a polyline
// .SECTION Description
// The class is used to probe tensors on a trajectory. The representation
// (vtkTensorProbeRepresentation) is free to choose its own method of
// rendering the tensors. For instance vtkEllipsoidTensorProbeRepresentation
// renders the tensors as ellipsoids. The interactions of the widget are
// controlled by the left mouse button. A left click on the tensor selects
// it. It can dragged around the trajectory to probe the tensors on it.
//
// For instance dragging the ellipsoid around with
// vtkEllipsoidTensorProbeRepresentation will manifest itself with the
// ellipsoid shape changing as needed along the trajectory.

#ifndef vtkTensorProbeWidget_h
#define vtkTensorProbeWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkTensorProbeRepresentation;
class vtkPolyData;

class VTKINTERACTIONWIDGETS_EXPORT vtkTensorProbeWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkTensorProbeWidget *New();

  // Description:
  // Standard VTK class macros.
  vtkTypeMacro(vtkTensorProbeWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkTensorProbeRepresentation *r)
    {
    this->Superclass::SetWidgetRepresentation(
        reinterpret_cast<vtkWidgetRepresentation*>(r));
    }

  // Description:
  // Return the representation as a vtkTensorProbeRepresentation.
  vtkTensorProbeRepresentation *GetTensorProbeRepresentation()
    {return reinterpret_cast<vtkTensorProbeRepresentation*>(this->WidgetRep);}

  // Description:
  // See vtkWidgetRepresentation for details.
  virtual void CreateDefaultRepresentation();

protected:
  vtkTensorProbeWidget();
  ~vtkTensorProbeWidget();

  // 1 when the probe has been selected, for instance when dragging it around
  int           Selected;

  int           LastEventPosition[2];

  // Callback interface to capture events and respond
  static void SelectAction    (vtkAbstractWidget*);
  static void MoveAction      (vtkAbstractWidget*);
  static void EndSelectAction (vtkAbstractWidget*);

private:
  vtkTensorProbeWidget(
      const vtkTensorProbeWidget&);  //Not implemented
  void operator=(const vtkTensorProbeWidget&);  //Not implemented

};

#endif

