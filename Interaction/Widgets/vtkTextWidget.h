/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTextWidget
 * @brief   widget for placing text on overlay plane
 *
 * This class provides support for interactively placing text on the 2D
 * overlay plane. The text is defined by an instance of vtkTextActor. It uses
 * the event bindings of its superclass (vtkBorderWidget). In addition, when
 * the text is selected, the widget emits a WidgetActivateEvent that
 * observers can watch for. This is useful for opening GUI dialogues to
 * adjust font characteristics, etc. (Please see the superclass for a
 * description of event bindings.)
 *
 * @sa
 * vtkBorderWidget vtkCaptionWidget
*/

#ifndef vtkTextWidget_h
#define vtkTextWidget_h

class vtkTextRepresentation;
class vtkTextActor;

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderWidget.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkTextWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate class.
   */
  static vtkTextWidget *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkTextWidget,vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkTextRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));}

  //@{
  /**
   * Specify a vtkTextActor to manage. This is a convenient, alternative
   * method to specify the representation for the widget (i.e., used instead
   * of SetRepresentation()). It internally creates a vtkTextRepresentation
   * and then invokes vtkTextRepresentation::SetTextActor().
   */
  void SetTextActor(vtkTextActor *textActor);
  vtkTextActor *GetTextActor();
  //@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkTextWidget();
  ~vtkTextWidget() override;

private:
  vtkTextWidget(const vtkTextWidget&) = delete;
  void operator=(const vtkTextWidget&) = delete;
};

#endif
