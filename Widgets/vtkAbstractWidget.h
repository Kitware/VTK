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
// .NAME vtkAbstractWidget - define API for widget / widget representation
// .SECTION Description
// The vtkAbstractWidget defines an API and implements methods common to all
// widgets using the interaction/representation design. In this case, the
// interaction means that part of the widget that performs event handling,
// while the representation means the vtkProp (or subclasses) used to
// represent the widget. This class also implements some methods common to
// all subclasses.
//
// Note that vtkAbstractWidget provides access to the
// vtkWidgetEventTranslator.  This class is responsible for translating VTK
// events (defined in vtkCommand.h) into widget events (defined in
// vtkWidgetEvent.h).  This class can be manipulated so that different
// VTK events can be mapped into widget events, thereby allowing the
// modification of event bindings. Each subclass of vtkAbstractWidget
// defines the events to which it responds.

//
// .SECTION Caveats
// The separation of the widget event handling and representation enables
// users and developers to create new appearances for the widget. It also
// facilitates parallel processing, where the client application handles
// events, and remote representations of the widget are slaves to the 
// client (and do not handle events).

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
  // Standard macros.
  vtkTypeRevisionMacro(vtkAbstractWidget,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods for activating this widget. Note that the widget representation
  // must be specified or the widget will not appear.
  //
  // SetEnabled disables the widget from receiving events. SetVisibility
  // toggles the visibility of the widget and the enabled state.
  virtual void SetEnabled(int);
  virtual void SetVisibility(int);
  vtkGetMacro(Visibility, int);

  // Description:
  // Get the event translator. Careful manipulation of this class enables
  // the user to specify their own event bindings.
  vtkWidgetEventTranslator *GetEventTranslator()
    {return this->EventTranslator;}
  
  // Description:
  // Create the default widget representation if one is not set. 
  virtual void CreateDefaultRepresentation() = 0;

  // Description:
  // This method is called by subclasses to reduce the number of renders that
  // are invoked.
  void Render();

  // Description:
  // Specifying a parent to this widget is used when creating composite
  // widgets. It is an internal method not meant to be used by the public.
  void SetParent(vtkAbstractWidget *parent) {this->Parent = parent;}
  vtkGetObjectMacro(Parent,vtkAbstractWidget);

  // Description:
  // Return an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  vtkWidgetRepresentation *GetRepresentation()
    {
      this->CreateDefaultRepresentation();
      return this->WidgetRep;
    }
  
  // Description:
  // Turn on or off the management of the cursor. Some classes (like
  // handles) the managing of the cursor may be taken over by an
  // owning superclass.
  vtkSetMacro(ManagesCursor,int);
  vtkGetMacro(ManagesCursor,int);
  vtkBooleanMacro(ManagesCursor,int);

protected:
  vtkAbstractWidget();
  ~vtkAbstractWidget();

  // Handles the events
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // The representation for the widget. This is typically called by the
  // SetRepresentation() methods particular to each widget (i.e. subclasses
  // of this class). This method does the actual work; the SetRepresentation()
  // methods constrain the type that can be set.
  void SetWidgetRepresentation(vtkWidgetRepresentation *r);
  vtkWidgetRepresentation *WidgetRep;

  virtual void SetVisibilityAndEnabledState();

  // helper methods for cursor management
  int ManagesCursor;
  virtual void SetCursor(int vtkNotUsed(state)) {}

  // For translating and invoking events
  vtkWidgetEventTranslator *EventTranslator;
  vtkWidgetCallbackMapper  *CallbackMapper;

  // The parent, if any, for this widget
  vtkAbstractWidget *Parent;

  // Call data which can be retrieved by the widget
  void *CallData;

  // Note: visibility and enabled are two different things
  int Visibility;

private:
  vtkAbstractWidget(const vtkAbstractWidget&);  //Not implemented
  void operator=(const vtkAbstractWidget&);  //Not implemented
};

#endif
