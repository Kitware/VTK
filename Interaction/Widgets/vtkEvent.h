// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindowInteractor;

class VTKINTERACTIONWIDGETS_EXPORT vtkEvent : public vtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static vtkEvent* New();

  ///@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkEvent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Ways to specify modifiers to VTK events. These can be logical OR'd to
   * produce combinations of modifiers.
   */
  enum EventModifiers
  {
    AnyModifier = -1,
    NoModifier = 0,
    ShiftModifier = 1,
    ControlModifier = 2,
    AltModifier = 4
  };

  ///@{
  /**
   * Set the modifier for the event.
   */
  vtkSetMacro(EventId, unsigned long);
  vtkGetMacro(EventId, unsigned long);
  ///@}

  ///@{
  /**
   * Set the modifier for the event.
   */
  vtkSetMacro(Modifier, int);
  vtkGetMacro(Modifier, int);
  ///@}

  ///@{
  /**
   * Set the KeyCode for the event.
   * Default is 0.
   */
  vtkSetMacro(KeyCode, char);
  vtkGetMacro(KeyCode, char);
  ///@}

  ///@{
  /**
   * Set the repease count for the event.
   */
  vtkSetMacro(RepeatCount, int);
  vtkGetMacro(RepeatCount, int);
  ///@}

  ///@{
  /**
   * Set the complex key symbol (compound key strokes) for the event.
   * Default is nullptr.
   */
  vtkSetStringMacro(KeySym);
  vtkGetStringMacro(KeySym);
  ///@}

  /**
   * Convenience method computes the event modifier from an interactor.
   */
  static int GetModifier(vtkRenderWindowInteractor*);

  /**
   * Used to compare whether two events are equal. Takes into account
   * the EventId as well as the various modifiers.
   */
  bool operator==(vtkEvent*) const;
  bool operator==(unsigned long VTKEvent) const; // event with no modifiers

protected:
  vtkEvent();
  ~vtkEvent() override;

  unsigned long EventId;
  int Modifier;
  char KeyCode;
  int RepeatCount;
  char* KeySym;

private:
  vtkEvent(const vtkEvent&) = delete;
  void operator=(const vtkEvent&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
