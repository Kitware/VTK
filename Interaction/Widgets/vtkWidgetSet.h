/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWidgetSet
 * @brief   Synchronize a collection on vtkWidgets drawn on different renderwindows using the Callback - Dispatch Action mechanism.
 *
 *
 * The class synchronizes a set of vtkAbstractWidget(s). Widgets typically
 * invoke "Actions" that drive the geometry/behaviour of their representations
 * in response to interactor events. Interactor interactions on a render window
 * are mapped into "Callbacks" by the widget, from which "Actions" are
 * dispatched to the entire set. This architecture allows us to tie widgets
 * existing in different render windows together. For instance a HandleWidget
 * might exist on the sagittal view. Moving it around should update the
 * representations of the corresponding handle widget that lies on the axial
 * and coronal and volume views as well.
 *
 * @par User API:
 * A user would use this class as follows.
 * \code
 * vtkWidgetSet *set = vtkWidgetSet::New();
 * vtkParallelopipedWidget *w1 = vtkParallelopipedWidget::New();
 * set->AddWidget(w1);
 * w1->SetInteractor(axialRenderWindow->GetInteractor());
 * vtkParallelopipedWidget *w2 = vtkParallelopipedWidget::New();
 * set->AddWidget(w2);
 * w2->SetInteractor(coronalRenderWindow->GetInteractor());
 * vtkParallelopipedWidget *w3 = vtkParallelopipedWidget::New();
 * set->AddWidget(w3);
 * w3->SetInteractor(sagittalRenderWindow->GetInteractor());
 * set->SetEnabled(1);
 * \endcode
 *
 * @par Motivation:
 * The motivation for this class is really to provide a usable API to tie
 * together multiple widgets of the same kind. To enable this, subclasses
 * of vtkAbstractWidget, must be written as follows:
 *   They will generally have callback methods mapped to some user
 * interaction such as:
 * \code
 * this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
 *                         vtkEvent::NoModifier, 0, 0, nullptr,
 *                         vtkPaintbrushWidget::BeginDrawStrokeEvent,
 *                         this, vtkPaintbrushWidget::BeginDrawCallback);
 * \endcode
 *   The callback invoked when the left button is pressed looks like:
 * \code
 * void vtkPaintbrushWidget::BeginDrawCallback(vtkAbstractWidget *w)
 * {
 *   vtkPaintbrushWidget *self = vtkPaintbrushWidget::SafeDownCast(w);
 *   self->WidgetSet->DispatchAction(self, &vtkPaintbrushWidget::BeginDrawAction);
 * }
 * \endcode
 *   The actual code for handling the drawing is written in the BeginDrawAction
 * method.
 * \code
 * void vtkPaintbrushWidget::BeginDrawAction( vtkPaintbrushWidget *dispatcher)
 * {
 * // Do stuff to draw...
 * // Here dispatcher is the widget that was interacted with, the one that
 * // dispatched an action to all the other widgets in its group. You may, if
 * // necessary find it helpful to get parameters from it.
 * //   For instance for a ResizeAction:
 * //     if (this != dispatcher)
 * //       {
 * //       double *newsize = dispatcher->GetRepresentation()->GetSize();
 * //       this->WidgetRep->SetSize(newsize);
 * //       }
 * //     else
 * //       {
 * //       this->WidgetRep->IncrementSizeByDelta();
 * //       }
 * }
 * \endcode
 *
 * @warning
 * Actions are always dispatched first to the activeWidget, the one calling
 * the set, and then to the other widgets in the set.
 *
*/

#ifndef vtkWidgetSet_h
#define vtkWidgetSet_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"
#include <vector> // Required for vector

class vtkAbstractWidget;

// Pointer to a member function that takes a vtkAbstractWidget (the active
// child) and another vtkAbstractWidget (the widget to dispatch an action)
// to. All "Action" functions in a widget must conform to this signature.
template< class TWidget > struct ActionFunction
{
  typedef void (TWidget::*TActionFunctionPointer)(TWidget *dispatcher);
};

class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetSet : public vtkObject
{
public:
  /**
   * Instantiate this class.
   */
  static vtkWidgetSet *New();

  //@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkWidgetSet,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Method for activating and deactivating all widgets in the group.
   */
  virtual void SetEnabled(vtkTypeBool);
  vtkBooleanMacro(Enabled, vtkTypeBool);
  //@}

  /**
   * Add a widget to the set.
   */
  void AddWidget(vtkAbstractWidget *);

  /**
   * Remove a widget from the set
   */
  void RemoveWidget(vtkAbstractWidget *);

  /**
   * Get number of widgets in the set.
   */
  unsigned int GetNumberOfWidgets();

  /**
   * Get the Nth widget in the set.
   */
  vtkAbstractWidget *GetNthWidget( unsigned int );

  // TODO: Move this to the protected section. The class vtkAbstractWidget
  //       should be a friend of this class.
  typedef std::vector< vtkAbstractWidget * >   WidgetContainerType;
  typedef WidgetContainerType::iterator           WidgetIteratorType;
  typedef WidgetContainerType::const_iterator     WidgetConstIteratorType;
  WidgetContainerType                             Widget;

  //@{
  /**
   * Dispatch an "Action" to every widget in this set. This is meant to be
   * invoked from a "Callback" in a widget.
   */
  template < class TWidget >
  void DispatchAction(TWidget *caller,
                      typename ActionFunction< TWidget >::TActionFunctionPointer action)
  {
    // Dispatch action to the caller first.
    for (WidgetIteratorType it  = this->Widget.begin();
                            it != this->Widget.end()  ; ++it)
    {
      TWidget *w = static_cast<TWidget *>(*it);
      if (caller == w)
      {
        ((*w).*(action))(caller);
        break;
      }
    }
  //@}

    // Dispatch action to all other widgets
    for (WidgetIteratorType it  = this->Widget.begin();
                            it != this->Widget.end()  ; ++it)
    {
      TWidget *w = static_cast<TWidget *>(*it);
      if (caller != w) ((*w).*(action))(caller);
    }
  }

protected:
  vtkWidgetSet();
  ~vtkWidgetSet() override;

private:
  vtkWidgetSet(const vtkWidgetSet&) = delete;
  void operator=(const vtkWidgetSet&) = delete;
};

#endif

