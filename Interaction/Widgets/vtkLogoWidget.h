/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogoWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLogoWidget - 2D widget for placing and manipulating a logo
// .SECTION Description
// This class provides support for interactively displaying and manipulating
// a logo. Logos are defined by an image; this widget simply allows you to
// interactively place and resize the image logo. To use this widget, simply
// create a vtkLogoRepresentation (or subclass) and associate it with the
// vtkLogoWidget.

// .SECTION See Also
// vtkBorderWidget


#ifndef vtkLogoWidget_h
#define vtkLogoWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderWidget.h"

class vtkLogoRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkLogoWidget : public vtkBorderWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkLogoWidget *New();

  // Description:
  // Standar VTK class methods.
  vtkTypeMacro(vtkLogoWidget,vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkLogoRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

protected:
  vtkLogoWidget();
  ~vtkLogoWidget();

private:
  vtkLogoWidget(const vtkLogoWidget&);  //Not implemented
  void operator=(const vtkLogoWidget&);  //Not implemented
};

#endif
