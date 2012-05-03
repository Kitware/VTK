/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCenteredSliderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkCenteredSliderRepresentation - provide the representation for a vtkCenteredSliderWidget
// .SECTION Description
// This class is used to represent and render a vtkCenteredSliderWidget. To use this
// class, you must at a minimum specify the end points of the
// slider. Optional instance variable can be used to modify the appearance of
// the widget.
//

// .SECTION See Also
// vtkSliderWidget


#ifndef __vtkCenteredSliderRepresentation_h
#define __vtkCenteredSliderRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkSliderRepresentation.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkCoordinate;
class vtkProperty2D;
class vtkPropCollection;
class vtkWindow;
class vtkViewport;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkTextProperty;
class vtkTextMapper;
class vtkTextActor;


class VTKINTERACTIONWIDGETS_EXPORT vtkCenteredSliderRepresentation : public vtkSliderRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkCenteredSliderRepresentation *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkCenteredSliderRepresentation,
                       vtkSliderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Position the first end point of the slider. Note that this point is an
  // instance of vtkCoordinate, meaning that Point 1 can be specified in a
  // variety of coordinate systems, and can even be relative to another
  // point. To set the point, you'll want to get the Point1Coordinate and
  // then invoke the necessary methods to put it into the correct coordinate
  // system and set the correct initial value.
  vtkCoordinate *GetPoint1Coordinate();

  // Description:
  // Position the second end point of the slider. Note that this point is an
  // instance of vtkCoordinate, meaning that Point 1 can be specified in a
  // variety of coordinate systems, and can even be relative to another
  // point. To set the point, you'll want to get the Point2Coordinate and
  // then invoke the necessary methods to put it into the correct coordinate
  // system and set the correct initial value.
  vtkCoordinate *GetPoint2Coordinate();

  // Description:
  // Specify the label text for this widget. If the value is not set, or set
  // to the empty string "", then the label text is not displayed.
  virtual void SetTitleText(const char*);
  virtual const char* GetTitleText();

  // Description:
  // Get the properties for the tube and slider
  vtkGetObjectMacro(TubeProperty,vtkProperty2D);
  vtkGetObjectMacro(SliderProperty,vtkProperty2D);

  // Description:
  // Get the selection property. This property is used to modify the
  // appearance of selected objects (e.g., the slider).
  vtkGetObjectMacro(SelectedProperty,vtkProperty2D);

  // Description:
  // Set/Get the properties for the label and title text.
  vtkGetObjectMacro(LabelProperty,vtkTextProperty);

  // Description:
  // Methods to interface with the vtkSliderWidget. The PlaceWidget() method
  // assumes that the parameter bounds[6] specifies the location in display
  // space where the widget should be placed.
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual void Highlight(int);

  // Decsription:
  // Methods supporting the rendering process.
  virtual void GetActors(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);

protected:
  vtkCenteredSliderRepresentation();
  ~vtkCenteredSliderRepresentation();

  // Positioning the widget
  vtkCoordinate *Point1Coordinate;
  vtkCoordinate *Point2Coordinate;

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double x, double y);

  // Define the geometry. It is constructed in canaonical position
  // along the x-axis and then rotated into position.
  vtkTransform        *XForm;
  vtkPoints           *Points;

  vtkCellArray        *SliderCells;
  vtkPolyData         *Slider;
  vtkTransformPolyDataFilter *SliderXForm;
  vtkPolyDataMapper2D *SliderMapper;
  vtkActor2D          *SliderActor;
  vtkProperty2D       *SliderProperty;

  vtkCellArray        *TubeCells;
  vtkPolyData         *Tube;
  vtkTransformPolyDataFilter *TubeXForm;
  vtkPolyDataMapper2D *TubeMapper;
  vtkActor2D          *TubeActor;
  vtkProperty2D       *TubeProperty;

  vtkTextProperty     *LabelProperty;
  vtkTextActor        *LabelActor;

  vtkProperty2D       *SelectedProperty;
  int HighlightState;

  // build the tube geometry
  void BuildTube();

private:

  // how many points along the tube
  int ArcCount;
  double ArcStart;
  double ArcEnd;
  double ButtonSize;
  double TubeSize;

  vtkCenteredSliderRepresentation
    (const vtkCenteredSliderRepresentation&); // Not implemented
  void operator=(const vtkCenteredSliderRepresentation&);  //Not implemented
};

#endif
