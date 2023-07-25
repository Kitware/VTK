// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlaybackWidget
 * @brief   2D widget for controlling a playback stream
 *
 * This class provides support for interactively controlling the playback of
 * a serial stream of information (e.g., animation sequence, video, etc.).
 * Controls for play, stop, advance one step forward, advance one step backward,
 * jump to beginning, and jump to end are available.
 *
 * @sa
 * vtkBorderWidget
 */

#ifndef vtkPlaybackWidget_h
#define vtkPlaybackWidget_h

#include "vtkBorderWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPlaybackRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkPlaybackWidget : public vtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPlaybackWidget* New();

  ///@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkPlaybackWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkPlaybackRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkPlaybackRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkPlaybackWidget();
  ~vtkPlaybackWidget() override;

  /**
   * When selecting the interior of this widget, special operations occur
   * (i.e., operating the playback controls).
   */
  void SelectRegion(double eventPos[2]) override;

private:
  vtkPlaybackWidget(const vtkPlaybackWidget&) = delete;
  void operator=(const vtkPlaybackWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
