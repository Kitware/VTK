// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraOrientationRepresentation
 * @brief   A 3D representation for vtkCameraOrientationWidget.
 *
 * Hover over the representation and drag with LMB to orbit around the view.
 * Clicking on one of the axis labels will snap to that view.
 * Click again on the same axis to switch to the opposite view of that same axis.
 *
 * The representation anchors itself to a corner of the renderer's
 * viewport. See AnchorType.
 *
 * @sa
 * vtkCameraOrientationWidget
 *
 */

#ifndef vtkCameraOrientationRepresentation_h
#define vtkCameraOrientationRepresentation_h

#include "vtkInteractionWidgetsModule.h" // needed for export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkDiskSource;
class vtkDoubleArray;
class vtkEllipticalButtonSource;
class vtkImageData;
class vtkPoints;
class vtkPolyData;
class vtkPropCollection;
class vtkProperty;
class vtkPropPicker;
class vtkTextProperty;
class vtkTexture;
class vtkTubeFilter;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCameraOrientationRepresentation
  : public vtkWidgetRepresentation
{
public:
  static vtkCameraOrientationRepresentation* New();
  vtkTypeMacro(vtkCameraOrientationRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum class InteractionStateType : int
  {
    Outside = 0, // corresponds to vtkCameraOrientationWidget::Inactive
    Hovering,    // corresponds to vtkCameraOrientationWidget::Hot
    Rotating     // corresponds to vtkCameraOrientationWidget::Active
  };

  /**
   * The interaction state may be set from a widget (e.g., vtkCameraOrientationWidget) or
   * other object. This call updates the representation to match the interaction state.
   */
  void ApplyInteractionState(const InteractionStateType& state);
  void ApplyInteractionState(const int& state);

  /**
   * Convenient method to get InteractionState as enum.
   * This method clamps the interaction state to possible values.
   * Hence, it does not raise any exceptions.
   */
  InteractionStateType GetInteractionStateAsEnum() noexcept
  {
    // clamp to 0-2
    this->InteractionState =
      this->InteractionState < 0 ? 0 : (this->InteractionState > 2 ? 2 : this->InteractionState);
    // convert
    return static_cast<InteractionStateType>(this->InteractionState);
  }

  ///@{
  /**
   * Get/Set the widget size in display coordinates.
   */
  vtkSetVector2Macro(Size, int);
  vtkGetVector2Macro(Size, int);
  ///@}

  ///@{
  /**
   * Get/Set the widget padding in display coordinates.
   */
  vtkSetVector2Macro(Padding, int);
  vtkGetVector2Macro(Padding, int);
  ///@}

  enum class AnchorType : int
  {
    LowerLeft = 0,
    UpperLeft,
    LowerRight,
    UpperRight
  };

  ///@{
  /**
   * Get/Set the widget anchor type
   */
  vtkSetEnumMacro(AnchorPosition, AnchorType);
  vtkGetEnumMacro(AnchorPosition, AnchorType);
  void AnchorToLowerLeft()
  {
    this->AnchorPosition = AnchorType::LowerLeft;
    this->Modified();
  }
  void AnchorToUpperLeft()
  {
    this->AnchorPosition = AnchorType::UpperLeft;
    this->Modified();
  }
  void AnchorToLowerRight()
  {
    this->AnchorPosition = AnchorType::LowerRight;
    this->Modified();
  }
  void AnchorToUpperRight()
  {
    this->AnchorPosition = AnchorType::UpperRight;
    this->Modified();
  }
  ///@}

  ///@{
  /**
   * Set the total length of the axes in 3 dimensions.
   * This is basis of normalization. Default value: 1.
   */
  vtkSetMacro(TotalLength, double);
  vtkGetMacro(TotalLength, double);
  ///@}

  ///@{
  /**
   * Set the normalized (0-1) diameter of the Handle.
   * Default value: 0.4
   */
  vtkSetMacro(NormalizedHandleDia, double);
  vtkGetMacro(NormalizedHandleDia, double);
  ///@}

  ///@{
  /**
   * Orientation properties. (read only)
   */
  vtkGetMacro(Azimuth, double);
  vtkGetVector3Macro(Back, double);
  vtkGetMacro(Elevation, double);
  vtkGetVector3Macro(Up, double);
  ///@}

  ///@{
  /**
   * Set shaft's resolution.
   */
  vtkSetClampMacro(ShaftResolution, int, 3, 256);
  vtkGetMacro(ShaftResolution, int);
  ///@}

  ///@{
  /**
   * Set Handle's circumferential resolution.
   */
  vtkSetClampMacro(HandleCircumferentialResolution, int, 3, 256);
  vtkGetMacro(HandleCircumferentialResolution, int);
  ///@}

  ///@{
  /**
   * Set container's circumferential resolution.
   */
  vtkSetClampMacro(ContainerCircumferentialResolution, int, 3, 256);
  vtkGetMacro(ContainerCircumferentialResolution, int);
  ///@}

  ///@{
  /**
   * Set container's radial resolution.
   */
  vtkSetClampMacro(ContainerRadialResolution, int, 3, 256);
  vtkGetMacro(ContainerRadialResolution, int);
  ///@}

  ///@{
  /**
   * Get picked axis, direction
   */
  vtkGetMacro(PickedAxis, int);
  vtkGetMacro(PickedDir, int);
  ///@}

  ///@{
  /**
   * Set/Get the '+' axis label text.
   */
  void SetXPlusLabelText(const std::string& label)
  {
    this->AxisLabelsText[0][0] = label;
    this->Modified();
  }
  std::string GetXPlusLabelText() { return this->AxisLabelsText[0][0]; }
  void SetYPlusLabelText(const std::string& label)
  {
    this->AxisLabelsText[1][0] = label;
    this->Modified();
  }
  std::string GetYPlusLabelText() { return this->AxisLabelsText[1][0]; }
  void SetZPlusLabelText(const std::string& label)
  {
    this->AxisLabelsText[2][0] = label;
    this->Modified();
  }
  std::string GetZPlusLabelText() { return this->AxisLabelsText[2][0]; }
  ///@}

  ///@{
  /**
   * Set/Get the '-' axis label text.
   */
  void SetXMinusLabelText(const std::string& label)
  {
    this->AxisLabelsText[0][1] = label;
    this->Modified();
  }
  std::string GetXMinusLabelText() { return this->AxisLabelsText[0][1]; }
  void SetYMinusLabelText(const std::string& label)
  {
    this->AxisLabelsText[1][1] = label;
    this->Modified();
  }
  std::string GetYMinusLabelText() { return this->AxisLabelsText[1][1]; }
  void SetZMinusLabelText(const std::string& label)
  {
    this->AxisLabelsText[2][1] = label;
    this->Modified();
  }
  std::string GetZMinusLabelText() { return this->AxisLabelsText[2][1]; }
  ///@}

  ///@{
  /**
   * Get the '+' axis label properties.
   */
  vtkTextProperty* GetXPlusLabelProperty();
  vtkTextProperty* GetYPlusLabelProperty();
  vtkTextProperty* GetZPlusLabelProperty();
  ///@}

  ///@{
  /**
   * Get the '-' axis label properties.
   */
  vtkTextProperty* GetXMinusLabelProperty();
  vtkTextProperty* GetYMinusLabelProperty();
  vtkTextProperty* GetZMinusLabelProperty();
  ///@}

  /**
   * Get the container property.
   */
  vtkProperty* GetContainerProperty();

  ///@{
  /**
   * Show container to indicate mouse presence.
   */
  void SetContainerVisibility(bool state);
  vtkBooleanMacro(ContainerVisibility, bool);
  bool GetContainerVisibility();
  ///@}

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(vtkPropCollection*) override;

  /**
   * Retrieve internal transform of this widget representation.
   */
  vtkTransform* GetTransform();

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void PlaceWidget(double*) override {} // this representation is an overlay. Doesn't need this.
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  double* GetBounds() VTK_SIZEHINT(6) override;
  ///@}

  ///@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Shallow copy of an axes actor. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp* prop) override;

  /**
   * Is a grabber button picked.
   */
  bool IsAnyHandleSelected() { return (this->PickedAxis != -1) && (this->PickedDir != -1); }

protected:
  vtkCameraOrientationRepresentation();
  ~vtkCameraOrientationRepresentation() override;

  virtual void CreateDefaultGeometry();
  virtual void CreateDefaultProperties();
  virtual void PositionHandles();
  virtual void HighlightHandle();
  virtual void Rotate(double newEventPos[2]);
  void RegisterPickers() override;
  void FinalizeHandlePicks();

  // description of source shapes.
  vtkNew<vtkDiskSource> ContainerSource;
  vtkNew<vtkEllipticalButtonSource> HandleSources[3][2];
  vtkNew<vtkTubeFilter> ShaftGlyphs;

  // geometries of handles and shafts. (position, color info)
  vtkNew<vtkPolyData> Skeleton;
  vtkNew<vtkPoints> Points; // used to store handle positions, also used by shafts

  // defaults are slight variations of r, y, g
  vtkNew<vtkDoubleArray> AxesColors;

  // props
  vtkNew<vtkActor> Container;
  vtkNew<vtkActor> Handles[3][2];
  vtkNew<vtkActor> Shafts;

  // font-sz, font-type, frame color of the labels.
  vtkNew<vtkTextProperty> AxisVectorTextProperties[3][2];
  vtkNew<vtkImageData> LabelImages[3][2];
  vtkNew<vtkTexture> LabelTextures[3][2];

  vtkNew<vtkPropPicker> HandlePicker;

  // Store rotation of gizmo.
  vtkNew<vtkTransform> Transform;

  // Positioning of the representation within a parent renderer.
  AnchorType AnchorPosition = AnchorType::UpperRight;
  int Padding[2] = { 10, 10 }; // In display coords.
  int Size[2] = { 120, 120 };  // In display coords.

  // Geometrical, textual, interaction description of the representation.
  std::string AxisLabelsText[3][2] = { { "X", "-X" }, { "Y", "-Y" }, { "Z", "-Z" } };
  double Azimuth = 0.;
  double Back[3] = { 0., 0., -1. };
  double Bounds[6] = {};
  double Elevation = 0.;
  double MotionFactor = 1.;
  double NormalizedHandleDia = 0.4;
  double TotalLength = 1.;
  double Up[3] = { 0., 1., 0. };
  int ContainerCircumferentialResolution = 32;
  int ContainerRadialResolution = 1;
  int HandleCircumferentialResolution = 32;
  int ShaftResolution = 10;

  // Picking information.
  int PickedAxis = -1;
  int LastPickedAx = -1;
  int PickedDir = -1;
  int LastPickedDir = -1;

  // Event tracking
  double LastEventPosition[3] = {};

private:
  vtkCameraOrientationRepresentation(const vtkCameraOrientationRepresentation&) = delete;
  void operator=(const vtkCameraOrientationRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
