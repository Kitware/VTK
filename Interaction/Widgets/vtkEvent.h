/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEvent
 * @brief   a complete specification of a VTK event including all modifiers
 *
 * vtkEvent is a class that fully describes a VTK event. It is used by the
 * widgets to help specify the mapping between VTK events and widget events.
*/

#ifndef vtkEvent_h
#define vtkEvent_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderWindowInteractor;

class VTKINTERACTIONWIDGETS_EXPORT vtkEvent : public vtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static vtkEvent *New();

  //@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkEvent,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Ways to specify modifiers to VTK events. These can be logical OR'd to
   * produce combinations of modifiers.
   */
  enum EventModifiers {
    AnyModifier = -1,
    NoModifier = 0,
    ShiftModifier = 1,
    ControlModifier = 2,
    AltModifier = 4
  };

  //@{
  /**
   * Set the modifier for the event.
   */
  vtkSetMacro(EventId,unsigned long);
  vtkGetMacro(EventId,unsigned long);
  //@}

  //@{
  /**
   * Set the modifier for the event.
   */
  vtkSetMacro(Modifier,int);
  vtkGetMacro(Modifier,int);
  //@}

  //@{
  /**
   * Set the KeyCode for the event.
   */
  vtkSetMacro(KeyCode,char);
  vtkGetMacro(KeyCode,char);
  //@}

  //@{
  /**
   * Set the repease count for the event.
   */
  vtkSetMacro(RepeatCount,int);
  vtkGetMacro(RepeatCount,int);
  //@}

  //@{
  /**
   * Set the complex key symbol (compound key strokes) for the event.
   */
  vtkSetStringMacro(KeySym);
  vtkGetStringMacro(KeySym);
  //@}

  /**
   * Convenience method computes the event modifier from an interactor.
   */
  static int GetModifier(vtkRenderWindowInteractor*);

  /**
   * Used to compare whether two events are equal. Takes into account
   * the EventId as well as the various modifiers.
   */
  bool operator==(vtkEvent*);
  bool operator==(unsigned long VTKEvent);  //event with no modifiers

protected:
  vtkEvent();
  virtual ~vtkEvent();

  unsigned long EventId;
  int           Modifier;
  char          KeyCode;
  int           RepeatCount;
  char*         KeySym;

private:
  vtkEvent(const vtkEvent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEvent&) VTK_DELETE_FUNCTION;

};

#endif
