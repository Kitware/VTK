// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMagnifierWidget
 * @brief   create a moving, magnifying renderer that can inspect the contents
 *          of an encapsulating renderer.
 *
 * This class provides a small, interactive, overlaid viewport (i.e.,
 * renderer) that follows the mouse while inside another, larger
 * renderer. Inside this viewport effects like magnification (via zoom or
 * adjustment of the view angle), or other rendering effects are possible.
 *
 * To use this widget, pair it with a representation (which provides options
 * like a border and so on). In the representation, indicate what renderer
 * with which the magnifier is associated. Optionally, a list of actors can
 * be provided to the magnifier so that it only renders the actors specified
 * (this is useful for removing objects like widgets from the scene, or
 * creating separate pipelines for creating special effects).
 *
 * By default, the magnifier responds to the keypress "m" to toggle between
 * showing/hiding the magnifier. Internally, it also follows mouse move events
 * to track the magnifier with the mouse motion. Finally, using the "-" or "+"
 * keys can be used to decrease/increase the magnification factor by observing
 * the "vtkCommand::WidgetValueChangedEvent".
 *
 * @sa
 * vtkMagnifierRepresentation
 */

#ifndef vtkMagnifierWidget_h
#define vtkMagnifierWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkMagnifierRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkMagnifierWidget : public vtkAbstractWidget
{
public:
  /**
   * Method to instantiate class.
   */
  static vtkMagnifierWidget* New();

  ///@{
  /**
   * Standard methods for class.
   */
  vtkTypeMacro(vtkMagnifierWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkMagnifierRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkMagnifierRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkBorderRepresentation.
   */
  vtkMagnifierRepresentation* GetMagnifierRepresentation()
  {
    return reinterpret_cast<vtkMagnifierRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Override superclasses' SetEnabled() method because the this widget
   * must activate the representation.
   */
  void SetEnabled(int enabling) override;

  ///@{
  /**
   * By default, the keypress "+" increases magnification, and the keypress
   * "-" decreases magnification. These can be changed to other key press
   * values. Note that in either case, these keypresses cause the widget to
   * emit the "vtkCommand::WidgetValueChangedEvent".
   */
  vtkSetMacro(KeyPressIncreaseValue, char);
  vtkGetMacro(KeyPressIncreaseValue, char);
  vtkSetMacro(KeyPressDecreaseValue, char);
  vtkGetMacro(KeyPressDecreaseValue, char);
  ///@}

protected:
  vtkMagnifierWidget();
  ~vtkMagnifierWidget() override;

  // Keypresses to change value
  char KeyPressIncreaseValue;
  char KeyPressDecreaseValue;

  // process the registered events
  static void MoveAction(vtkAbstractWidget*);
  static void CharAction(vtkAbstractWidget*);

  int WidgetState;
  enum WidgetStateType
  {
    Invisible = 0,
    Visible
  };

private:
  vtkMagnifierWidget(const vtkMagnifierWidget&) = delete;
  void operator=(const vtkMagnifierWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
