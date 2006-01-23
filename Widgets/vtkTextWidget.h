/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextWidget - widget for placing text on overlay plane
// .SECTION Description
// This class provides support for interactively placing text on the 2D
// overlay plane. The text is defined by an instance of vtkTextActor. It uses
// the event bindings of its superclass (vtkBorderWidget). In addition, when
// the text is selected, the widget emits a WidgetActivateEvent that
// observers can watch for. This is useful for opening GUI dialogoues to
// adjust font characteristics, etc. (Please see the superclass for a
// description of event bindings.)

// .SECTION See Also
// vtkBorderWidget vtkCaptionWidget


#ifndef __vtkTextWidget_h
#define __vtkTextWidget_h

class vtkTextRepresentation;
class vtkTextActor;

#include "vtkBorderWidget.h"

class VTK_WIDGETS_EXPORT vtkTextWidget : public vtkBorderWidget
{
public:
  // Description:
  // Instantiate class.
  static vtkTextWidget *New();

  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkTextWidget,vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkTextRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));}
  
  // Description:
  // Specify the vtkTextActor to manage. This is convenient, alternative
  // method to SetRepresentation(). It internally create a vtkTextRepresentation
  // and then invokes vtkTextRepresentation::SetTextActor().
  void SetTextActor(vtkTextActor *textActor);
  vtkTextActor *GetTextActor();

  // Description:
  // Create the default widget representation if one is not set. 
  virtual void CreateDefaultRepresentation();

protected:
  vtkTextWidget();
  ~vtkTextWidget();

private:
  vtkTextWidget(const vtkTextWidget&);  //Not implemented
  void operator=(const vtkTextWidget&);  //Not implemented
};

#endif
