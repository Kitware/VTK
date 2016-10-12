/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCheckerboardWidget
 * @brief   interactively set the number of divisions in 2D image checkerboard
 *
 * The vtkCheckerboardWidget is used to interactively control an instance of
 * vtkImageCheckerboard (and an associated vtkImageActor used to display the
 * checkerboard). The user can adjust the number of divisions in each of the
 * i-j directions in a 2D image. A frame appears around the vtkImageActor
 * with sliders along each side of the frame. The user can interactively
 * adjust the sliders to the desired number of checkerboard subdivisions.
 *
 * To use this widget, specify an instance of vtkImageCheckerboard and an
 * instance of vtkImageActor. By default, the widget responds to the
 * following events:
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider (if on slider)
 *   LeftButtonReleaseEvent - release slider
 *   MouseMoveEvent - move slider
 * If the end caps or slider tube of a slider are selected:
 *   LeftButtonPressEvent - jump (or animate) to cap or point on tube;
 * </pre>
 * It is possible to change these event bindings. Please refer to the
 * documentation for vtkSliderWidget for more information. Advanced users may
 * directly access and manipulate the sliders by obtaining the instances of
 * vtkSliderWidget composing the vtkCheckerboard widget.
 *
 * @sa
 * vtkImageCheckerboard vtkImageActor vtkSliderWidget vtkRectilinearWipeWidget
*/

#ifndef vtkCheckerboardWidget_h
#define vtkCheckerboardWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkCheckerboardRepresentation;
class vtkSliderWidget;


class VTKINTERACTIONWIDGETS_EXPORT vtkCheckerboardWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkCheckerboardWidget *New();

  //@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkCheckerboardWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' vtkAbstractWidget::SetEnabled() method.
   */
  virtual void SetEnabled(int);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkCheckerboardRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkCheckerboardRepresentation.
   */
  vtkCheckerboardRepresentation *GetCheckerboardRepresentation()
    {return reinterpret_cast<vtkCheckerboardRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation();

protected:
  vtkCheckerboardWidget();
  ~vtkCheckerboardWidget();

  // The four slider widgets
  vtkSliderWidget *TopSlider;
  vtkSliderWidget *RightSlider;
  vtkSliderWidget *BottomSlider;
  vtkSliderWidget *LeftSlider;

  // Callback interface
  void StartCheckerboardInteraction();
  void CheckerboardInteraction(int sliderNum);
  void EndCheckerboardInteraction();

  friend class vtkCWCallback;

private:
  vtkCheckerboardWidget(const vtkCheckerboardWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCheckerboardWidget&) VTK_DELETE_FUNCTION;
};

#endif
