/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetEventTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWidgetEventTranslator - map VTK events into widget events
// .SECTION Description
// vtkWidgetEventTranslator maps VTK events (defined on vtkCommand) into
// widget events (defined in vtkWidgetEvent.h). This class is typically used
// in combination with vtkWidgetCallbackMapper, which is responsible for
// translating widget events into method callbacks, and then invoking the
// callbacks.
//
// This class can be used to define different mappings of VTK events into
// the widget events. Thus widgets can be reconfigured to use different
// event bindings.

// .SECTION See Also
// vtkWidgetEvent vtkCommand vtkInteractorObserver

#ifndef __vtkWidgetEventTranslator_h
#define __vtkWidgetEventTranslator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

// Support PIMPL encapsulation of internal STL map
class vtkEventMap;
class vtkRenderWindowInteractor;
class vtkCallbackCommand;
class vtkEvent;
class vtkAbstractWidget;


// This is a lightweight class that should be used internally by the widgets
class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetEventTranslator : public vtkObject
{
public:
  // Description:
  // Instantiate the object.
  static vtkWidgetEventTranslator *New();

  // Description:
  // Standard macros.
  vtkTypeMacro(vtkWidgetEventTranslator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Use these methods to create the translation from a VTK event to a widget
  // event. Specifying vtkWidgetEvent::NoEvent or an empty
  // string for the (toEvent) erases the mapping for the event.
  void SetTranslation(unsigned long VTKEvent, unsigned long widgetEvent);
  void SetTranslation(const char *VTKEvent, const char *widgetEvent);
  void SetTranslation(unsigned long VTKEvent, int modifier, char keyCode,
                      int repeatCount, const char* keySym, unsigned long widgetEvent);
  void SetTranslation(vtkEvent *VTKevent, unsigned long widgetEvent);

  // Description:
  // Translate a VTK event into a widget event. If no event mapping is found,
  // then the methods return vtkWidgetEvent::NoEvent or a NULL string.
  unsigned long GetTranslation(unsigned long VTKEvent);
  const char *GetTranslation(const char *VTKEvent);
  unsigned long GetTranslation(unsigned long VTKEvent, int modifier, char keyCode,
                               int repeatCount, char* keySym);
  unsigned long GetTranslation(vtkEvent *VTKEvent);

  // Description:
  // Remove translations for a binding.
  // Returns the number of translations removed.
  int RemoveTranslation( unsigned long VTKEvent,
                         int modifier,    char keyCode,
                         int repeatCount, char* keySym);
  int RemoveTranslation( vtkEvent *e );
  int RemoveTranslation(unsigned long VTKEvent);
  int RemoveTranslation(const char *VTKEvent);

  // Description:
  // Clear all events from the translator (i.e., no events will be
  // translated).
  void ClearEvents();

//BTX
  // Description:
  // Add the events in the current translation table to the interactor.
  void AddEventsToParent(vtkAbstractWidget*, vtkCallbackCommand*, float priority);
  void AddEventsToInteractor(vtkRenderWindowInteractor*, vtkCallbackCommand*,
                             float priority);
//ETX

protected:
  // Constructors/destructors made public for widgets to use
  vtkWidgetEventTranslator();
  ~vtkWidgetEventTranslator();

  // Map VTK events to widget events
  vtkEventMap *EventMap;

  // Used for performance reasons to avoid object construction/deletion
  vtkEvent *Event;

private:
  vtkWidgetEventTranslator(const vtkWidgetEventTranslator&);  //Not implemented
  void operator=(const vtkWidgetEventTranslator&);  //Not implemented

};

#endif /* __vtkWidgetEventTranslator_h */

