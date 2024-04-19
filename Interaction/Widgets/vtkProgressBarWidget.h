// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProgressBarWidget
 * @brief   2D widget for placing and manipulating a progress bar
 *
 * This class provides support for interactively displaying and manipulating
 * a progress bar.A Progress bar is defined by a progress rate and the color of the bar and
 * its background.
 * This widget allows you to interactively place and resize the progress bar.
 * To use this widget, simply create a vtkProgressBarRepresentation (or subclass)
 * and associate it with a vtkProgressBarWidget instance.
 *
 * @sa
 * vtkBorderWidget
 */

#ifndef vtkProgressBarWidget_h
#define vtkProgressBarWidget_h

#include "vtkBorderWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkProgressBarRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkProgressBarWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkProgressBarWidget* New();

  ///@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkProgressBarWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkProgressBarRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkProgressBarWidget();
  ~vtkProgressBarWidget() override;

private:
  vtkProgressBarWidget(const vtkProgressBarWidget&) = delete;
  void operator=(const vtkProgressBarWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
