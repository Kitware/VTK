/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBorderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBorderWidget
 * @brief   place a border around a 2D rectangular region
 *
 * This class is a superclass for 2D widgets that may require a rectangular
 * border. Besides drawing a border, the widget provides methods for resizing
 * and moving the rectangular region (and associated border). The widget
 * provides methods and internal data members so that subclasses can take
 * advantage of this widgets capabilities, requiring only that the subclass
 * defines a "representation", i.e., some combination of props or actors
 * that can be managed in the 2D rectangular region.
 *
 * The class defines basic positioning functionality, including the ability
 * to size the widget with locked x/y proportions. The area within the border
 * may be made "selectable" as well, meaning that a selection event interior
 * to the widget invokes a virtual SelectRegion() method, which can be used
 * to pick objects or otherwise manipulate data interior to the widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * On the boundary of the widget:
 *   LeftButtonPressEvent - select boundary
 *   LeftButtonReleaseEvent - deselect boundary
 *   MouseMoveEvent - move/resize widget depending on which portion of the
 *                    boundary was selected.
 * On the interior of the widget:
 *   LeftButtonPressEvent - invoke SelectButton() callback (if the ivar
 *                          Selectable is on)
 * Anywhere on the widget:
 *   MiddleButtonPressEvent - move the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkBorderWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Translate -- the widget is to be translated
 *   vtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, this widget invokes the
 * following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 * @sa
 * vtkInteractorObserver vtkCameraInterpolator
*/

#ifndef vtkBorderWidget_h
#define vtkBorderWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkBorderRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkBorderWidget : public vtkAbstractWidget
{
public:
  /**
   * Method to instantiate class.
   */
  static vtkBorderWidget *New();

  //@{
  /**
   * Standard methods for class.
   */
  vtkTypeMacro(vtkBorderWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Indicate whether the interior region of the widget can be selected or
   * not. If not, then events (such as left mouse down) allow the user to
   * "move" the widget, and no selection is possible. Otherwise the
   * SelectRegion() method is invoked.
   */
  vtkSetMacro(Selectable,vtkTypeBool);
  vtkGetMacro(Selectable,vtkTypeBool);
  vtkBooleanMacro(Selectable,vtkTypeBool);
  //@}


  //@{
  /**
   * Indicate whether the boundary of the widget can be resized.
   * If not, the cursor will not change to "resize" type when mouse
   * over the boundary.
   */
  vtkSetMacro(Resizable,vtkTypeBool);
  vtkGetMacro(Resizable,vtkTypeBool);
  vtkBooleanMacro(Resizable,vtkTypeBool);
  //@}


  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkBorderRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkBorderRepresentation.
   */
  vtkBorderRepresentation *GetBorderRepresentation()
    {return reinterpret_cast<vtkBorderRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkBorderWidget();
  ~vtkBorderWidget() override;

  /**
   * Subclasses generally implement this method. The SelectRegion() method
   * offers a subclass the chance to do something special if the interior
   * of the widget is selected.
   */
  virtual void SelectRegion(double eventPos[2]);

  //enable the selection of the region interior to the widget
  vtkTypeBool Selectable;
  vtkTypeBool Resizable;

  //processes the registered events
  static void SelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // Special internal methods to support subclasses handling events.
  // If a non-zero value is returned, the subclass is handling the event.
  virtual int SubclassSelectAction() {return 0;}
  virtual int SubclassTranslateAction() {return 0;}
  virtual int SubclassEndSelectAction() {return 0;}
  virtual int SubclassMoveAction() {return 0;}

  // helper methods for cursoe management
  void SetCursor(int State) override;

  //widget state
  int WidgetState;
  enum _WidgetState{Start=0,Define,Manipulate,Selected};

private:
  vtkBorderWidget(const vtkBorderWidget&) = delete;
  void operator=(const vtkBorderWidget&) = delete;
};

#endif
