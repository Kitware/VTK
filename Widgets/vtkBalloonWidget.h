/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBalloonWidget - popup text balloons above instance of vtkProp when hovering occurs
// .SECTION Description 
// The vtkBalloonWidget is used to popup text when the mouse hovers over
// an instance of vtkProp. The widget keeps track of (vtkProp,vtkStdString)
// pairs, and when the mouse stops moving for a user-specified period of time
// over the vtkProp, then the text string (represented by vtkStdString) is 
// rendered over the vtkProp. Note that an instance of vtkBalloonRepresentation
// is used to draw the text.
// 
// To use this widget, specify an instance of vtkBalloonWidget and a
// representation (e.g., vtkBalloonRepresentation). Then list all instances
// of vtkProp and a text string to go along with each vtkProp. You may also
// wish to specify the hover delay (i.e., set in the superclass).
//
// .SECTION Event Bindings
// By default, the widget observes the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   MouseMoveEvent - occurs when mouse is moved in render window.
//   TimerEvent - occurs when the time between events (e.g., mouse move)
//                is greater than TimerDuration.
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events 
// into the vtkBalloonWidget's widget events:
// <pre>
//   vtkWidgetEvent::Move -- start the timer
//   vtkWidgetEvent::TimedOut -- when hovering occurs,
// </pre>
//
// This widget invokes the following VTK events on itself (which observers
// can listen for):
// <pre>
//   vtkCommand::TimerEvent (when hovering is determined to occur)
//   vtkCommand::EndInteractionEvent (after a hover has occured and the
//                                    mouse begins moving again).
// </pre>

// .SECTION See Also
// vtkAbstractWidget


#ifndef __vtkBalloonWidget_h
#define __vtkBalloonWidget_h

#include "vtkHoverWidget.h"

class vtkBalloonRepresentation;
class vtkProp;
class vtkAbstractPropPicker;
class vtkStdString;
class vtkPropMap;


class VTK_WIDGETS_EXPORT vtkBalloonWidget : public vtkHoverWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkBalloonWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeRevisionMacro(vtkBalloonWidget,vtkHoverWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The method for activiating and deactiviating this widget. This method
  // must be overridden because it performs special timer-related operations.
  virtual void SetEnabled(int);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkBalloonRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}
  
  // Description:
  // Create the default widget representation if one is not set. 
  void CreateDefaultRepresentation();

  // Description:
  // Add and remove vtkProp/vtkStdString pairs over which balloons can appear.
  void AddBalloonText(vtkProp *prop, vtkStdString *str);
  void AddBalloonText(vtkProp *prop, const char *str);
  void RemoveBalloonText(vtkProp *prop);
  
  // Description:
  // Return the current vtkProp that is being hovered over. Note that the
  // value may be NULL (if hovering over nothing or the mouse is moving).
  virtual vtkProp *GetCurrentProp()
    {return this->CurrentProp;}
  
  // Description:
  // Set/Get the object used to perform pick operations. Since the
  // vtkBalloonWidget operates on vtkProps, the picker must be a subclass of
  // vtkAbstractPropPicker. (Note: if not specified, an instance of
  // vtkPropPicker is used.)
  void SetPicker(vtkAbstractPropPicker*);
  vtkGetObjectMacro(Picker,vtkAbstractPropPicker);

protected:
  vtkBalloonWidget();
  ~vtkBalloonWidget();

  // This class implements the method called from its superclass.
  virtual int SubclassEndHoverAction();
  virtual int SubclassHoverAction();
  
  // Classes for managing balloons
  vtkPropMap *PropMap; //PIMPL'd map of (vtkProp,vtkStdString)

  // Support for picking
  vtkAbstractPropPicker *Picker;

  // The vtkProp that is being hovered over
  vtkProp *CurrentProp;
  
private:
  vtkBalloonWidget(const vtkBalloonWidget&);  //Not implemented
  void operator=(const vtkBalloonWidget&);  //Not implemented
};

#endif
