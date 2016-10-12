/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompassRepresentation.h

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

/**
 * @class   vtkCompassRepresentation
 * @brief   provide a compass
 *
 * This class is used to represent and render a compass.
*/

#ifndef vtkCompassRepresentation_h
#define vtkCompassRepresentation_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkContinuousValueWidgetRepresentation.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro
#include "vtkCenteredSliderRepresentation.h" // to use in a SP
#include "vtkSmartPointer.h" // used for SmartPointers

class vtkActor2D;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkCoordinate;
class vtkProperty2D;
class vtkPropCollection;
class vtkWindow;
class vtkViewport;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkTextProperty;
class vtkTextActor;


class VTKGEOVISCORE_EXPORT vtkCompassRepresentation :
  public vtkContinuousValueWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCompassRepresentation *New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkCompassRepresentation,
                       vtkContinuousValueWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint1Coordinate();

  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint2Coordinate();

  //@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(RingProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the
   * appearance of selected objects (e.g., the slider).
   */
  vtkGetObjectMacro(SelectedProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the properties for the label and title text.
   */
  vtkGetObjectMacro(LabelProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Methods to interface with the vtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual void TiltWidgetInteraction(double eventPos[2]);
  virtual void DistanceWidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void Highlight(int);
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  virtual void GetActors(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  //@}

  virtual void SetHeading(double value);
  virtual double GetHeading();
  virtual void SetTilt(double value);
  virtual double GetTilt();
  virtual void UpdateTilt(double time);
  virtual void EndTilt();
  virtual void SetDistance(double value);
  virtual double GetDistance();
  virtual void UpdateDistance(double time);
  virtual void EndDistance();
  virtual void SetRenderer(vtkRenderer *ren);

  // Enums are used to describe what is selected
  enum _InteractionState
  {
    Outside=0,
    Inside,
    Adjusting,
    TiltDown,
    TiltUp,
    TiltAdjusting,
    DistanceOut,
    DistanceIn,
    DistanceAdjusting
  };

protected:
  vtkCompassRepresentation();
  ~vtkCompassRepresentation();

  // Positioning the widget
  vtkCoordinate *Point1Coordinate;
  vtkCoordinate *Point2Coordinate;

  // radius values
  double InnerRadius;
  double OuterRadius;

  // tilt and distance rep

  vtkSmartPointer<vtkCenteredSliderRepresentation> TiltRepresentation;
  vtkSmartPointer<vtkCenteredSliderRepresentation> DistanceRepresentation;

  // Define the geometry. It is constructed in canaonical position
  // along the x-axis and then rotated into position.
  vtkTransform        *XForm;
  vtkPoints           *Points;

  vtkPolyData         *Ring;
  vtkTransformPolyDataFilter *RingXForm;
  vtkPolyDataMapper2D *RingMapper;
  vtkActor2D          *RingActor;
  vtkProperty2D       *RingProperty;

  vtkPolyDataMapper2D *BackdropMapper;
  vtkActor2D          *Backdrop;

  vtkTextProperty     *LabelProperty;
  vtkTextActor        *LabelActor;
  vtkTextProperty     *StatusProperty;
  vtkTextActor        *StatusActor;

  vtkProperty2D       *SelectedProperty;

  // build the tube geometry
  void BuildRing();
  void BuildBackdrop();

  // used for positioning etc
  void GetCenterAndUnitRadius(int center[2], double &radius);

  int HighlightState;

  double Heading;
  double Tilt;
  double Distance;

private:
  vtkCompassRepresentation(const vtkCompassRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompassRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
