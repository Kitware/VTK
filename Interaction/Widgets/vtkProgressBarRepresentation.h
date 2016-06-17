/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProgressBarRepresentation - represent a vtkProgressBarWidget
// .SECTION Description
// This class is used to represent a vtkProgressBarWidget.

// .SECTION See Also
// vtkProgressBarWidget

#ifndef vtkProgressBarRepresentation_h
#define vtkProgressBarRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkActor2D;
class vtkPoints;
class vtkPolyData;
class vtkProperty2D;
class vtkUnsignedCharArray;

class VTKINTERACTIONWIDGETS_EXPORT vtkProgressBarRepresentation : public vtkBorderRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkProgressBarRepresentation *New();

  // Description:
  // Standard VTK class methods.
  vtkTypeMacro(vtkProgressBarRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // By obtaining this property you can specify the properties of the
  // representation.
  vtkGetObjectMacro(Property, vtkProperty2D);

  // Description:
  // Set/Get the progress rate of the progress bar, between 0 and 1
  // default is 0
  vtkSetClampMacro(ProgressRate, double, 0, 1);
  vtkGetMacro(ProgressRate, double);

  // Description:
  // Set/Get the progress bar color
  // Default is pure green
  vtkSetVector3Macro(ProgressBarColor, double);
  vtkGetVector3Macro(ProgressBarColor, double);

  // Description:
  // Set/Get the background color
  // Default is white
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);

  // Description:
  // Set/Get background visibility
  // Default is off
  vtkSetMacro(DrawBackground, bool);
  vtkGetMacro(DrawBackground, bool);
  vtkBooleanMacro(DrawBackground, bool);

  // Description:
  // Satisfy the superclasses' API.
  virtual void BuildRepresentation();
  virtual void GetSize(double size[2]);

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  vtkProgressBarRepresentation();
  ~vtkProgressBarRepresentation();

  double ProgressRate;
  double ProgressBarColor[3];
  double BackgroundColor[3];
  bool DrawBackground;

  vtkPoints      *Points;
  vtkUnsignedCharArray  *BackgroundData;
  vtkUnsignedCharArray  *ProgressBarData;
  vtkProperty2D  *Property;
  vtkActor2D     *Actor;
  vtkActor2D     *BackgroundActor;

private:
  vtkProgressBarRepresentation(const vtkProgressBarRepresentation&);  //Not implemented
  void operator=(const vtkProgressBarRepresentation&);  //Not implemented
};

#endif
