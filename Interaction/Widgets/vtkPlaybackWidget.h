/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaybackWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlaybackWidget - 2D widget for controlling a playback stream
// .SECTION Description
// This class provides support for interactively controlling the playback of
// a serial stream of information (e.g., animation sequence, video, etc.).
// Controls for play, stop, advance one step forward, advance one step backward,
// jump to beginning, and jump to end are available.

// .SECTION See Also
// vtkBorderWidget


#ifndef vtkPlaybackWidget_h
#define vtkPlaybackWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderWidget.h"

class vtkPlaybackRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkPlaybackWidget : public vtkBorderWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkPlaybackWidget *New();

  // Description:
  // Standar VTK class methods.
  vtkTypeMacro(vtkPlaybackWidget,vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkPlaybackRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkPlaybackRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

protected:
  vtkPlaybackWidget();
  ~vtkPlaybackWidget();

  // Description:
  // When selecting the interior of this widget, special operations occur
  // (i.e., operating the playback controls).
  virtual void SelectRegion(double eventPos[2]);

private:
  vtkPlaybackWidget(const vtkPlaybackWidget&);  //Not implemented
  void operator=(const vtkPlaybackWidget&);  //Not implemented
};

#endif
