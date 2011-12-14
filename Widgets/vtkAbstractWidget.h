/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractWidget - define the API for widget / widget representation
// .SECTION Description
// The vtkAbstractWidget defines an API and implements methods common to all
// widgets using the interaction/representation design. In this design, the
// term interaction means that part of the widget that performs event
// handling, while the representation corresponds to a vtkProp (or the
// subclass vtkWidgetRepresentation) used to represent the
// widget. vtkAbstractWidget also implements some methods common to all
// subclasses.
//
// Note that vtkAbstractWidget provides access to the
// vtkWidgetEventTranslator.  This class is responsible for translating VTK
// events (defined in vtkCommand.h) into widget events (defined in
// vtkWidgetEvent.h). This class can be manipulated so that different VTK
// events can be mapped into widget events, thereby allowing the modification
// of event bindings. Each subclass of vtkAbstractWidget defines the events
// to which it responds.
//
// .SECTION Caveats
// Note that the pair ( vtkAbstractWidget / vtkWidgetRepresentation ) is an
// implementation of the second generation VTK Widgets design. In the first
// generation design, widgets were implemented in a single monolithic
// class. This design was problematic because in client-server application
// it was difficult to manage widgets properly. Also, new "representations"
// or look-and-feel, for a widget required a whole new class, with a lot of
// redundant code. The separation of the widget event handling and
// representation enables users and developers to create new appearances for
// the widget. It also facilitates parallel processing, where the client
// application handles events, and remote representations of the widget are
// slaves to the client (and do not handle events).

// .SECTION See Also
// vtkWidgetRepresentation vtkWidgetEventTranslator vtkWidgetCallbackMapper


#ifndef __vtkAbstractWidget_h
#define __vtkAbstractWidget_h

#include "vtkInteractorObserver.h"

class vtkWidgetEventTranslator;
class vtkWidgetCallbackMapper;
class vtkWidgetRepresentation;


class VTK_WIDGETS_EXPORT vtkAbstractWidget : public vtkInteractorObserver
{
public:
  // Description:
  // Standard macros implementing standard VTK methods.
  vtkTypeMacro(vtkAbstractWidget,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods for activating this widget. Note that the widget representation
  // must be specified or the widget will not appear.
  // ProcessEvents (On by default) must be On for Enabled widget to respond 
  // to interaction. If ProcessEvents is Off, enabling/disabling a widget 
  // merely affects the visibility of the representation.
  virtual void SetEnabled(int);

  // Description:
  // Methods to change whether the widget responds to interaction.
  // Set this to Off to disable interaction. On by default.
  // Subclasses must overide SetProcessEvents() to make sure
  // that they pass on the flag to all component widgets.
  vtkSetClampMacro(ProcessEvents, int, 0, 1);
  vtkGetMacro(ProcessEvents, int);
  vtkBooleanMacro(ProcessEvents, int);

  // Description:
  // Get the event translator. Careful manipulation of this class enables
  // the user to override the default event bindings.
  vtkWidgetEventTranslator *GetEventTranslator()
    {return this->EventTranslator;}
  
  // Description:
  // Create the default widget representation if one is not set. The
  // representation defines the geometry of the widget (i.e., how it appears)
  // as well as providing special methods for manipulting the state and
  // appearance of the widget.
  virtual void CreateDefaultRepresentation() = 0;

  // Description:
  // This method is called by subclasses when a render method is to be
  // invoked on the vtkRenderWindowInteractor. This method should be called
  // (instead of vtkRenderWindow::Render() because it has built into it
  // optimizations for minimizing renders and/or speeding renders.
  void Render();

  // Description:
  // Specifying a parent to this widget is used when creating composite
  // widgets. It is an internal method not meant to be used by the public.
  // When a widget has a parent, it defers the rendering to the parent. It
  // may also defer managing the cursor (see ManagesCursor ivar).
  void SetParent(vtkAbstractWidget *parent) {this->Parent = parent;}
  vtkGetObjectMacro(Parent,vtkAbstractWidget);

  // Description:
  // Return an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of
  // vtkProp (typically a subclass of vtkWidgetRepresenation) so it can be
  // added to the renderer independent of the widget.
  vtkWidgetRepresentation *GetRepresentation()
    {
      this->CreateDefaultRepresentation();
      return this->WidgetRep;
    }
  
  // Description:
  // Turn on or off the management of the cursor. Cursor management is
  // typically disabled for subclasses when composite widgets are
  // created. For example, vtkHandleWidgets are often used to create
  // composite widgets, and the parent widget takes over the cursor
  // management.
  vtkSetMacro(ManagesCursor,int);
  vtkGetMacro(ManagesCursor,int);
  vtkBooleanMacro(ManagesCursor,int);

  // Description:
  // Override the superclass method. This will automatically change the
  // priority of the widget. Unlike the superclass documentation, no 
  // methods such as SetInteractor to null and reset it etc. are necessary
  virtual void SetPriority( float );

protected:
  vtkAbstractWidget();
  ~vtkAbstractWidget();

  // Handles the events; centralized here for all widgets.
  static void ProcessEventsHandler(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // The representation for the widget. This is typically called by the
  // SetRepresentation() methods particular to each widget (i.e. subclasses
  // of this class). This method does the actual work; the SetRepresentation()
  // methods constrain the type that can be set.
  void SetWidgetRepresentation(vtkWidgetRepresentation *r);
  vtkWidgetRepresentation *WidgetRep;

  // helper methods for cursor management
  int ManagesCursor;
  virtual void SetCursor(int vtkNotUsed(state)) {}

  // For translating and invoking events
  vtkWidgetEventTranslator *EventTranslator;
  vtkWidgetCallbackMapper  *CallbackMapper;

  // The parent, if any, for this widget
  vtkAbstractWidget *Parent;

  // Call data which can be retrieved by the widget. This data is set
  // by ProcessEvents() if call data is provided during a callback
  // sequence.
  void *CallData;

  // Flag indicating if the widget should handle interaction events.
  // On by default.
  int ProcessEvents;

private:
  vtkAbstractWidget(const vtkAbstractWidget&);  //Not implemented
  void operator=(const vtkAbstractWidget&);  //Not implemented
};

#endif
