/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetCallbackMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWidgetCallbackMapper
 * @brief   map widget events into callbacks
 *
 * vtkWidgetCallbackMapper maps widget events (defined in vtkWidgetEvent.h)
 * into static class methods, and provides facilities to invoke the methods.
 * This class is templated and meant to be used as an internal helper class
 * by the widget classes. The class works in combination with the class
 * vtkWidgetEventTranslator, which translates VTK events into widget events.
 *
 * @sa
 * vtkWidgetEvent vtkWidgetEventTranslator
*/

#ifndef vtkWidgetCallbackMapper_h
#define vtkWidgetCallbackMapper_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

class vtkWidgetEvent;
class vtkAbstractWidget;
class vtkWidgetEventTranslator;
class vtkCallbackMap; // PIMPL encapsulation of STL map


class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetCallbackMapper : public vtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static vtkWidgetCallbackMapper *New();

  //@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkWidgetCallbackMapper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Specify the vtkWidgetEventTranslator to coordinate with.
   */
  void SetEventTranslator(vtkWidgetEventTranslator *t);
  vtkGetObjectMacro(EventTranslator,vtkWidgetEventTranslator);
  //@}

  /**
   * Convenient typedef for working with callbacks.
   */
  typedef void (*CallbackType)(vtkAbstractWidget*);

  //@{
  /**
   * This class works with the class vtkWidgetEventTranslator to set up the
   * initial coorespondence between VTK events, widget events, and callbacks.
   * Different flavors of the SetCallbackMethod() are available depending on
   * what sort of modifiers are to be associated with a particular event.
   * Typically the widgets should use this method to set up their event
   * callbacks. If modifiers are not provided (i.e., the VTKEvent is a
   * unsigned long eventId) then modifiers are ignored. Otherwise, a vtkEvent
   * instance is used to fully quality the events.
   */
  void SetCallbackMethod(unsigned long VTKEvent, unsigned long widgetEvent,
                         vtkAbstractWidget *w, CallbackType f);
  void SetCallbackMethod(unsigned long VTKEvent, int modifiers, char keyCode,
                         int repeatCount, const char* keySym,
                         unsigned long widgetEvent,
                         vtkAbstractWidget *w, CallbackType f);
  //void SetCallbackMethod(vtkWidgetEvent *vtkEvent, unsigned long widgetEvent,
  //                       vtkAbstractWidget *w, CallbackType f);
  //@}

  /**
   * This method invokes the callback given a widget event. A non-zero value
   * is returned if the listed event is registered.
   */
  void InvokeCallback(unsigned long widgetEvent);

protected:
  vtkWidgetCallbackMapper();
  ~vtkWidgetCallbackMapper() VTK_OVERRIDE;

  // Translates VTK events into widget events
  vtkWidgetEventTranslator *EventTranslator;

  // Invoke the method associated with a particular widget event
  vtkCallbackMap *CallbackMap;

  /**
   * This method is used to assign a callback (implemented as a static class
   * method) to a particular widget event. This is an internal method used by
   * widgets to map widget events into invocations of class methods.
   */
  void SetCallbackMethod(unsigned long widgetEvent,
                         vtkAbstractWidget *w, CallbackType f);


private:
  vtkWidgetCallbackMapper(const vtkWidgetCallbackMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWidgetCallbackMapper&) VTK_DELETE_FUNCTION;

};


#endif /* vtkWidgetCallbackMapper_h */

