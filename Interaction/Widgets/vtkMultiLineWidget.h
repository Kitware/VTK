// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiLineWidget
 * @brief   3D widget for manipulating multiple lines
 *
 * This 3D widget defines a configurable number of lines that can be interactively placed in
 * a scene.
 *
 * To use this widget, you generally pair it with a vtkMultiLineRepresentation.
 * Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.

 * @sa
 * vtkMultiLineRepresentation vtkLineWidget2
 */

#ifndef vtkMultiLineWidget_h
#define vtkMultiLineWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkLineWidget2;
class vtkMultiLineRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkMultiLineWidget : public vtkAbstractWidget
{
public:
  static vtkMultiLineWidget* New();

  vtkTypeMacro(vtkMultiLineWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override superclasses' SetEnabled() method because the multi line
   * widget must enable its internal vtkLineWidget2
   */
  void SetEnabled(int enabling) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the Line Count of the representation is set
   * to the Line Count of this widget.
   */
  void SetRepresentation(vtkMultiLineRepresentation* repr);

  /**
   * Return this object's modified time by checking the modified time of the
   * superclass and the modified time of each vtkLineWidget2 in this widget.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   *  These methods handle events
   */
  static void SelectAction(vtkAbstractWidget* widget);
  static void EndSelectAction(vtkAbstractWidget* widget);
  static void MoveAction(vtkAbstractWidget* widget);
  ///@}

  ///@{
  /**
   * Set the number of vtkLineWidget2 in this widget.
   */
  void SetLineCount(int count);
  vtkGetMacro(LineCount, int)
  ///@}

  /**
   * Return the representation as a vtkMultiLineRepresentation.
   */
  vtkMultiLineRepresentation* GetMultiLineRepresentation();

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  void SetProcessEvents(vtkTypeBool enabled) override;

  /**
   * Override superclasses' SetInteractor() method because the multi line
   * widget must set the interactor for each one of his vtkLineWidget2
   */
  void SetInteractor(vtkRenderWindowInteractor* interactor) override;

protected:
  vtkMultiLineWidget();
  ~vtkMultiLineWidget() override;

private:
  vtkMultiLineWidget(const vtkMultiLineWidget&) = delete;
  void operator=(const vtkMultiLineWidget&) = delete;

  std::vector<vtkSmartPointer<vtkLineWidget2>> LineWidgetVector;
  int LineCount = 0;

  // Manage the state of the widget (and his corresponding enum in vtkLineWidget2)
  enum WidgetStateType
  {
    NOT_SELECTED = 0, // Start
    ACTIVE_SELECTION  // Active
  };
  int WidgetState = vtkMultiLineWidget::NOT_SELECTED;
};

VTK_ABI_NAMESPACE_END
#endif
