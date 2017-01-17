/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressBarWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderWidget.h"

class vtkProgressBarRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkProgressBarWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkProgressBarWidget *New();

  //@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkProgressBarWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkProgressBarRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() VTK_OVERRIDE;

protected:
  vtkProgressBarWidget();
  ~vtkProgressBarWidget() VTK_OVERRIDE;

private:
  vtkProgressBarWidget(const vtkProgressBarWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProgressBarWidget&) VTK_DELETE_FUNCTION;
};

#endif
