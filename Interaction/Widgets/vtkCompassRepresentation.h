// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkCompassRepresentation
 * @brief   provide a compass and distance, tilt sliders
 *
 * This class is used to represent and render a compass to represent a heading, and two vertical
 * sliders to manipulate distance and tilt.
 *
 * If distance or tilt sliders are not required then their Visibility can be set to off when
 * subclassing it.
 *
 * Override the GetStatusText() method if you require a customized status text.
 */

#ifndef vtkCompassRepresentation_h
#define vtkCompassRepresentation_h

#include "vtkCenteredSliderRepresentation.h" // to use in a SP
#include "vtkContinuousValueWidgetRepresentation.h"
#include "vtkCoordinate.h"               // For vtkViewportCoordinateMacro
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkSmartPointer.h"             // used for SmartPointers
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
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

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCompassRepresentation
  : public vtkContinuousValueWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCompassRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkCompassRepresentation, vtkContinuousValueWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate* GetPoint1Coordinate();

  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate* GetPoint2Coordinate();

  ///@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(RingProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Get the selection property. This property is used to modify the
   * appearance of selected objects (e.g., the slider).
   */
  vtkGetObjectMacro(SelectedProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Set/Get the properties for the label and title text.
   */
  vtkGetObjectMacro(LabelProperty, vtkTextProperty);
  ///@}

  ///@{
  /**
   * Methods to interface with the vtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  virtual void TiltWidgetInteraction(double eventPos[2]);
  virtual void DistanceWidgetInteraction(double eventPos[2]);
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void Highlight(int) override;
  ///@}

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors(vtkPropCollection* propCollection) override;
  void ReleaseGraphicsResources(vtkWindow* window) override;
  int RenderOverlay(vtkViewport* viewPort) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  ///@}

  ///@{
  /**
   * Get/Set the heading in degrees. The methods ensure that the heading is in the range [0, 360)
   * degrees.
   */
  virtual void SetHeading(double heading);
  virtual double GetHeading();
  ///@}

  ///@{
  /**
   * Get/Set the tilt in degrees. The methods ensure that the tilt is in the range set by
   * SetMaximumTiltAngle() and SetMinimumTiltAngle().
   */
  virtual void SetTilt(double tilt);
  virtual double GetTilt();
  ///@}

  ///@{
  /**
   * Get/Set the tilt range. These default range is [-90, 90] degrees.
   */
  void SetMaximumTiltAngle(double angle);
  double GetMaximumTiltAngle();
  void SetMinimumTiltAngle(double angle);
  double GetMinimumTiltAngle();
  ///@}

  ///@{
  /**
   * Update the tilt by the given delta in degrees.
   */
  virtual void UpdateTilt(double deltaTilt = 0);
  ///@}

  virtual void EndTilt();

  ///@{
  /**
   * Get/Set the distance. These methods ensure that the distance is in the range set by
   * SetMaximumDistance() and SetMinimumDistance().
   */
  virtual void SetDistance(double distance);
  virtual double GetDistance();
  ///@}

  ///@{
  /**
   * Get/Set the distance range. The default is [0.0, 2.0].
   */
  void SetMaximumDistance(double distance);
  double GetMaximumDistance();
  void SetMinimumDistance(double distance);
  double GetMinimumDistance();
  ///@}

  ///@{
  /**
   * Update the distance by the given delta.
   */
  virtual void UpdateDistance(double deltaDistance = 0);
  ///@}

  virtual void EndDistance();

  void SetRenderer(vtkRenderer* renderer) override;

  // Enums are used to describe what is selected
  enum InteractionStateType
  {
    Outside = 0,
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
  ~vtkCompassRepresentation() override;

  // Positioning the widget
  vtkCoordinate* Point1Coordinate;
  vtkCoordinate* Point2Coordinate;

  // radius values
  double InnerRadius;
  double OuterRadius;

  // tilt and distance rep

  vtkSmartPointer<vtkCenteredSliderRepresentation> TiltRepresentation;
  vtkSmartPointer<vtkCenteredSliderRepresentation> DistanceRepresentation;

  // Define the geometry. It is constructed in canaonical position
  // along the x-axis and then rotated into position.
  vtkTransform* XForm;
  vtkPoints* Points;

  vtkPolyData* Ring;
  vtkTransformPolyDataFilter* RingXForm;
  vtkPolyDataMapper2D* RingMapper;
  vtkActor2D* RingActor;
  vtkProperty2D* RingProperty;

  vtkPolyDataMapper2D* BackdropMapper;
  vtkActor2D* Backdrop;

  vtkTextProperty* LabelProperty;
  vtkTextActor* LabelActor;
  vtkTextProperty* StatusProperty;
  vtkTextActor* StatusActor;

  vtkProperty2D* SelectedProperty;

  // build the tube geometry
  void BuildRing();
  void BuildBackdrop();

  // used for positioning etc
  void GetCenterAndUnitRadius(int center[2], double& radius);

  ///@{
  /**
   * Return the text used for the status label. Subclasses can override this method to customize the
   * status text, for example when using unit conversions.
   */
  virtual std::string GetStatusText();
  ///@}

  int HighlightState;

  double Heading;
  double Tilt;
  double Distance;

private:
  vtkCompassRepresentation(const vtkCompassRepresentation&) = delete;
  void operator=(const vtkCompassRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
