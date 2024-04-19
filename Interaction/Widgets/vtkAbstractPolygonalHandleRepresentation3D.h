// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D while maintaining a fixed orientation
 * w.r.t the camera.
 *
 * This class serves as the geometrical representation of a vtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (vtkPolyData),
 * set via SetHandle(vtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired. This class differs from
 * vtkPolygonalHandleRepresentation3D in that the handle will always remain
 * front facing, ie it maintains a fixed orientation with respect to the
 * camera. This is done by using vtkFollowers internally to render the actors.
 * @sa
 * vtkPolygonalHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget
 */

#ifndef vtkAbstractPolygonalHandleRepresentation3D_h
#define vtkAbstractPolygonalHandleRepresentation3D_h

#include "vtkHandleRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkProperty;
class vtkPolyDataMapper;
class vtkCellPicker;
class vtkTransformPolyDataFilter;
class vtkMatrixToLinearTransform;
class vtkMatrix4x4;
class vtkPolyData;
class vtkAbstractTransform;
class vtkActor;
class vtkFollower;
class vtkVectorText;

class VTKINTERACTIONWIDGETS_EXPORT vtkAbstractPolygonalHandleRepresentation3D
  : public vtkHandleRepresentation
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkAbstractPolygonalHandleRepresentation3D, vtkHandleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  using vtkHandleRepresentation::Translate;

  ///@{
  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) override;
  void SetDisplayPosition(double p[3]) override;
  ///@}

  ///@{
  /**
   * Set/get the handle polydata.
   */
  void SetHandle(vtkPolyData*);
  vtkPolyData* GetHandle();
  ///@}

  ///@{
  /**
   * Set/Get the handle properties when unselected and selected.
   */
  void SetProperty(vtkProperty*);
  void SetSelectedProperty(vtkProperty*);
  vtkGetObjectMacro(Property, vtkProperty);
  vtkGetObjectMacro(SelectedProperty, vtkProperty);
  ///@}

  /**
   * Get the transform used to transform the generic handle polydata before
   * placing it in the render window
   */
  virtual vtkAbstractTransform* GetTransform();

  ///@{
  /**
   * Methods to make this class properly act like a vtkWidgetRepresentation.
   */
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  ///@}

  ///@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void ShallowCopy(vtkProp* prop) override;
  void DeepCopy(vtkProp* prop) override;
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  double* GetBounds() override;
  ///@}

  ///@{
  /**
   * A label may be associated with the seed. The string can be set via
   * SetLabelText. The visibility of the label can be turned on / off.
   */
  vtkSetMacro(LabelVisibility, vtkTypeBool);
  vtkGetMacro(LabelVisibility, vtkTypeBool);
  vtkBooleanMacro(LabelVisibility, vtkTypeBool);
  virtual void SetLabelText(const char* label);
  virtual char* GetLabelText();
  ///@}

  ///@{
  /**
   * Scale text (font size along each dimension).
   */
  virtual void SetLabelTextScale(double scale[3]);
  void SetLabelTextScale(double x, double y, double z)
  {
    double scale[3] = { x, y, z };
    this->SetLabelTextScale(scale);
  }
  virtual double* GetLabelTextScale();
  ///@}

  ///@{
  /**
   * Get the label text actor
   */
  vtkGetObjectMacro(LabelTextActor, vtkFollower);
  ///@}

  /**
   * The handle may be scaled uniformly in all three dimensions using this
   * API. The handle can also be scaled interactively using the right
   * mouse button.
   */
  virtual void SetUniformScale(double scale);

  ///@{
  /**
   * Toggle the visibility of the handle on and off
   */
  vtkSetMacro(HandleVisibility, vtkTypeBool);
  vtkGetMacro(HandleVisibility, vtkTypeBool);
  vtkBooleanMacro(HandleVisibility, vtkTypeBool);
  ///@}

  void Highlight(int highlight) override;

  ///@{
  /**
   * Turn on/off smooth motion of the handle. See the documentation of
   * MoveFocusRequest for details. By default, SmoothMotion is ON. However,
   * in certain applications the user may want to turn it off. For instance
   * when using certain specific PointPlacer's with the representation such
   * as the vtkCellCentersPointPlacer, which causes the representation to
   * snap to the center of cells, or using a vtkPolygonalSurfacePointPlacer
   * which constrains the widget to the surface of a mesh. In such cases,
   * inherent restrictions on handle placement might conflict with a request
   * for smooth motion of the handles.
   */
  vtkSetMacro(SmoothMotion, vtkTypeBool);
  vtkGetMacro(SmoothMotion, vtkTypeBool);
  vtkBooleanMacro(SmoothMotion, vtkTypeBool);
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  vtkAbstractPolygonalHandleRepresentation3D();
  ~vtkAbstractPolygonalHandleRepresentation3D() override;

  vtkActor* Actor;
  vtkPolyDataMapper* Mapper;
  vtkTransformPolyDataFilter* HandleTransformFilter;
  vtkMatrixToLinearTransform* HandleTransform;
  vtkMatrix4x4* HandleTransformMatrix;
  vtkCellPicker* HandlePicker;
  double LastPickPosition[3];
  double LastEventPosition[2];
  int ConstraintAxis;
  vtkProperty* Property;
  vtkProperty* SelectedProperty;
  int WaitingForMotion;
  int WaitCount;
  vtkTypeBool HandleVisibility;

  // Methods to manipulate the cursor
  void Translate(const double* p1, const double* p2) override;
  virtual void Scale(const double* p1, const double* p2, const double eventPos[2]);
  virtual void MoveFocus(const double* p1, const double* p2);

  void CreateDefaultProperties();

  // Given a motion vector defined by p1 --> p2 (p1 and p2 are in
  // world coordinates), the new display position of the handle center is
  // populated into requestedDisplayPos. This is again only a request for the
  // new display position. It is up to the point placer to deduce the
  // appropriate world coordinates that this display position will map into.
  // The placer may even disallow such a movement.
  // If "SmoothMotion" is OFF, the returned requestedDisplayPos is the same
  // as the event position, ie the location of the mouse cursor. If its OFF,
  // incremental offsets as described above are used to compute it.
  void MoveFocusRequest(
    const double* p1, const double* p2, const double eventPos[2], double requestedDisplayPos[3]);

  int DetermineConstraintAxis(int constraint, double* x, double* startPickPos);

  /**
   * Update the actor position. Different subclasses handle this differently.
   * For instance vtkPolygonalHandleRepresentation3D updates the handle
   * transformation and sets this on the handle.
   * vtkOrientedPolygonalHandleRepresentation3D, which uses a vtkFollower to
   * keep the handle geometry facinig the camera handles this differently. This
   * is an opportunity for subclasses to update the actor's position etc each
   * time the handle is rendered.
   */
  virtual void UpdateHandle();

  /**
   * Opportunity to update the label position and text during each render.
   */
  virtual void UpdateLabel();

  // Handle the label.
  vtkTypeBool LabelVisibility;
  vtkFollower* LabelTextActor;
  vtkPolyDataMapper* LabelTextMapper;
  vtkVectorText* LabelTextInput;
  bool LabelAnnotationTextScaleInitialized;
  vtkTypeBool SmoothMotion;

private:
  vtkAbstractPolygonalHandleRepresentation3D(
    const vtkAbstractPolygonalHandleRepresentation3D&) = delete;
  void operator=(const vtkAbstractPolygonalHandleRepresentation3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
