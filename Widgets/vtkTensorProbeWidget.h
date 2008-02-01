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

#ifndef __vtkTensorProbeWidget_h
#define __vtkTensorProbeWidget_h

#include "vtkAbstractWidget.h"

class vtkTensorProbeRepresentation;
class vtkPolyData;

class VTK_WIDGETS_EXPORT vtkTensorProbeWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkTensorProbeWidget *New();

  // Description:
  // Standard VTK class macros.
  vtkTypeRevisionMacro(vtkTensorProbeWidget, vtkAbstractWidget);
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
  // See vtkWidgetRepresentation for details.
  virtual void CreateDefaultRepresentation();

protected:
  vtkTensorProbeWidget();
  ~vtkTensorProbeWidget();

  vtkPolyData * Trajetory;

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

