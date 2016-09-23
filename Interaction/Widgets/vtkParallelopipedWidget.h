/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelopipedWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParallelopipedWidget
 * @brief   a widget to manipulate 3D parallelopipeds
 *
 *
 * This widget was designed with the aim of visualizing / probing cuts on
 * a skewed image data / structured grid.
 *
 * @par Interaction:
 * The widget allows you to create a parallelopiped (defined by 8 handles).
 * The widget is initially placed by using the "PlaceWidget" method in the
 * representation class. After the widget has been created, the following
 * interactions may be used to manipulate it :
 * 1) Click on a handle and drag it around moves the handle in space, while
 *    keeping the same axis alignment of the parallelopiped
 * 2) Dragging a handle with the shift button pressed resizes the piped
 *    along an axis.
 * 3) Control-click on a handle creates a chair at that position. (A chair
 *    is a depression in the piped that allows you to visualize cuts in the
 *    volume).
 * 4) Clicking on a chair and dragging it around moves the chair within the
 *    piped.
 * 5) Shift-click on the piped enables you to translate it.
 *
*/

#ifndef vtkParallelopipedWidget_h
#define vtkParallelopipedWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkParallelopipedRepresentation;
class vtkHandleWidget;
class vtkWidgetSet;

class VTKINTERACTIONWIDGETS_EXPORT vtkParallelopipedWidget : public vtkAbstractWidget
{

  friend class vtkWidgetSet;

public:
  /**
   * Instantiate the object.
   */
  static vtkParallelopipedWidget *New();

  vtkTypeMacro(vtkParallelopipedWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Override the superclass method. This is a composite widget, (it internally
   * consists of handle widgets). We will override the superclass method, so
   * that we can pass the enabled state to the internal widgets as well.
   */
  virtual void SetEnabled(int);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkParallelopipedRepresentation *r)
  {
    this->Superclass::SetWidgetRepresentation(
        reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkParallelopipedRepresentation.
   */
  vtkParallelopipedRepresentation *GetParallelopipedRepresentation()
    {return reinterpret_cast<vtkParallelopipedRepresentation*>(this->WidgetRep);}

  //@{
  /**
   * Enable/disable the creation of a chair on this widget. If off,
   * chairs cannot be created.
   */
  vtkSetMacro(EnableChairCreation,int);
  vtkGetMacro(EnableChairCreation,int);
  vtkBooleanMacro(EnableChairCreation,int);
  //@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation();

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  virtual void SetProcessEvents(int);

protected:
  vtkParallelopipedWidget();
  ~vtkParallelopipedWidget();

  static void RequestResizeCallback             (vtkAbstractWidget* );
  static void RequestResizeAlongAnAxisCallback  (vtkAbstractWidget* );
  static void RequestChairModeCallback          (vtkAbstractWidget* );
  static void TranslateCallback                 (vtkAbstractWidget* );
  static void OnMouseMoveCallback               (vtkAbstractWidget* );
  static void OnLeftButtonUpCallback            (vtkAbstractWidget* );

  // Control whether chairs can be created
  int EnableChairCreation;

  //@{
  void BeginTranslateAction ( vtkParallelopipedWidget *dispatcher);
  void TranslateAction ( vtkParallelopipedWidget *dispatcher);
  //@}

  // helper methods for cursor management
  void SetCursor(int state);

  // To break reference count loops
  void ReportReferences(vtkGarbageCollector* collector) VTK_OVERRIDE;

  // The positioning handle widgets
  vtkHandleWidget **HandleWidgets;

  /**
   * Events invoked by this widget
   */
  enum WidgetEventIds
  {
    RequestResizeEvent = 10000,
    RequestResizeAlongAnAxisEvent,
    RequestChairModeEvent
  };

  vtkWidgetSet* WidgetSet;

private:
  vtkParallelopipedWidget(const vtkParallelopipedWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParallelopipedWidget&) VTK_DELETE_FUNCTION;
};

#endif
