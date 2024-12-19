// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightRepresentation
 * @brief   represent a vtkLight
 *
 * The vtkLightRepresentation is a representation for the vtkLight.
 * This representation consists of a LightPosition sphere with an automatically resized
 * radius so it is always visible, a line between the LightPosition and the FocalPoint and
 * a cone of angle ConeAngle when using Positional.
 *
 * @sa
 * vtkLightWidget vtkSphereWidget vtkSphereRepresentation
 */

#ifndef vtkLightRepresentation_h
#define vtkLightRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // Needed for vtkNew
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkCellPicker;
class vtkConeSource;
class vtkLineSource;
class vtkPointHandleRepresentation3D;
class vtkPolyDataMapper;
class vtkProperty;
class vtkSphereSource;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkLightRepresentation
  : public vtkWidgetRepresentation
{
public:
  static vtkLightRepresentation* New();
  vtkTypeMacro(vtkLightRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the positional flag. When set to on, a cone will be visible.
   */
  vtkSetMacro(Positional, bool);
  vtkGetMacro(Positional, bool);
  vtkBooleanMacro(Positional, bool);
  ///@}

  ///@{
  /**
   * Set/Get the coordinates of the position of the light representation.
   */
  void SetLightPosition(double pos[3]);
  vtkGetVector3Macro(LightPosition, double);
  ///@}

  ///@{
  /**
   * Set/Get the coordinates of the focal point of the light representation.
   */
  void SetFocalPoint(double pos[3]);
  vtkGetVector3Macro(FocalPoint, double);
  ///@}

  ///@{
  /**
   * Set/Get the cone angle, in degrees, for the light.
   * Used only when positional.
   */
  void SetConeAngle(double angle);
  vtkGetMacro(ConeAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the light color.
   */
  void SetLightColor(double* color);
  double* GetLightColor() VTK_SIZEHINT(3);
  ///@}

  /**
   * Enum used to communicate interaction state.
   */
  enum
  {
    Outside = 0,
    MovingLight,
    MovingFocalPoint,
    MovingPositionalFocalPoint,
    ScalingConeAngle
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g., vtkLightWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, ScalingConeAngle);
  ///@}

  ///@{
  /**
   * Get the property used for all the actors
   */
  vtkGetObjectMacro(Property, vtkProperty);
  ///@}

  ///@{
  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double eventPosition[2]) override;
  void WidgetInteraction(double eventPosition[2]) override;
  double* GetBounds() override;
  ///@}

  ///@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  ///@}

  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp (i.e., support rendering).
   * GetActors adds all the internal props used by this representation to the supplied collection.
   */
  void GetActors(vtkPropCollection*) override;

protected:
  vtkLightRepresentation();
  ~vtkLightRepresentation() override;

  virtual void SizeHandles();
  virtual void UpdateSources();
  virtual void ScaleConeAngle(double* pickPoint, double* lastPickPoint);

  vtkNew<vtkProperty> Property;
  vtkNew<vtkBox> BoundingBox;
  vtkCellPicker* LastPicker;
  double LastScalingDistance2 = -1;
  double LastEventPosition[3] = { 0, 0, 0 };

  // the Sphere
  vtkNew<vtkSphereSource> Sphere;
  vtkNew<vtkActor> SphereActor;
  vtkNew<vtkPolyDataMapper> SphereMapper;
  vtkNew<vtkCellPicker> SpherePicker;

  // the Cone
  vtkNew<vtkConeSource> Cone;
  vtkNew<vtkActor> ConeActor;
  vtkNew<vtkPolyDataMapper> ConeMapper;
  vtkNew<vtkCellPicker> ConePicker;

  // the Line
  vtkNew<vtkLineSource> Line;
  vtkNew<vtkActor> LineActor;
  vtkNew<vtkPolyDataMapper> LineMapper;
  vtkNew<vtkCellPicker> LinePicker;

  double LightPosition[3] = { 0, 0, 1 };
  double FocalPoint[3] = { 0, 0, 0 };
  double ConeAngle = 30;
  bool Positional = false;

private:
  vtkLightRepresentation(const vtkLightRepresentation&) = delete;
  void operator=(const vtkLightRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
