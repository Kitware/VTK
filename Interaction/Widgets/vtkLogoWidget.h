// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLogoWidget
 * @brief   2D widget for placing and manipulating a logo
 *
 * This class provides support for interactively displaying and manipulating
 * a logo. Logos are defined by an image; this widget simply allows you to
 * interactively place and resize the image logo. To use this widget, simply
 * create a vtkLogoRepresentation (or subclass) and associate it with the
 * vtkLogoWidget.
 *
 * @sa
 * vtkBorderWidget
 */

#ifndef vtkLogoWidget_h
#define vtkLogoWidget_h

#include "vtkBorderWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLogoRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkLogoWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkLogoWidget* New();

  ///@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkLogoWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkLogoRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkLogoWidget();
  ~vtkLogoWidget() override;

private:
  vtkLogoWidget(const vtkLogoWidget&) = delete;
  void operator=(const vtkLogoWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
