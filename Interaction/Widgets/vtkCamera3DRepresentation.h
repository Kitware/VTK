// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCamera3DRepresentation
 * @brief   a class defining the representation for the vtkCamera3DWidget
 *
 * This class is a concrete representation for the vtkCamera3DWidget.
 * The camera is represented by a box and a cone. The first one allows
 * camera movement, the second allows view angle update. There are three
 * more handles to rotate the view up, and move the target position.
 * It also has a frustum representation.
 *
 * To use this representation, you can use the PlaceWidget() method
 * to position the widget looking at a specified region in space. This
 * is optional as you may want to not move the camera at setup.
 *
 * @sa
 * vtkCamera3DWidget
 */

#ifndef vtkCamera3DRepresentation_h
#define vtkCamera3DRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"                      // For vtkNew
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <array>

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkBox;
class vtkCamera;
class vtkCamera3DWidget;
class vtkCameraActor;
class vtkCellPicker;
class vtkLineSource;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCamera3DRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCamera3DRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkCamera3DRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
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

  ///@{
  /**
   * Set/Get the camera.
   */
  virtual void SetCamera(vtkCamera* camera);
  vtkCamera* GetCamera();
  ///@}

  ///@{
  /**
   * Set/Get the distance between camera position and the front handle.
   * Note that the distance is scaled with view to keep the widget the same size.
   * Default: 2.5.
   **/
  vtkSetClampMacro(FrontHandleDistance, double, 1.5, VTK_DOUBLE_MAX);
  vtkGetMacro(FrontHandleDistance, double);
  ///@}

  ///@{
  /**
   * Set/Get the distance between camera position and the up handle.
   * Note that the distance is scaled with view to keep the widget the same size.
   * Default: 1.5.
   **/
  vtkSetClampMacro(UpHandleDistance, double, 0.5, VTK_DOUBLE_MAX);
  vtkGetMacro(UpHandleDistance, double);
  ///@}

  ///@{
  /**
   * Set/Get the constraint axis for translations.
   * Default: Axis::NONE (-1).
   **/
  void SetTranslationAxisToXAxis() { this->SetTranslationAxis(Axis::XAxis); }
  void SetTranslationAxisToYAxis() { this->SetTranslationAxis(Axis::YAxis); }
  void SetTranslationAxisToZAxis() { this->SetTranslationAxis(Axis::ZAxis); }
  void SetTranslationAxisToNone() { this->SetTranslationAxis(Axis::NONE); }
  vtkSetClampMacro(TranslationAxis, int, Axis::NONE, Axis::ZAxis);
  vtkGetMacro(TranslationAxis, int);
  ///@}

  ///@{
  /**
   * Set/Get whether to translate both position and target or not.
   * Default: false.
   **/
  vtkSetMacro(TranslatingAll, bool);
  vtkGetMacro(TranslatingAll, bool);
  vtkBooleanMacro(TranslatingAll, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to show camera frustum.
   * Default: true.
   **/
  void SetFrustumVisibility(bool visible);
  vtkGetMacro(FrustumVisibility, bool);
  vtkBooleanMacro(FrustumVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to show secondary handles (spheres and lines).
   * Default: true.
   **/
  void SetSecondaryHandlesVisibility(bool visible);
  vtkGetMacro(SecondaryHandlesVisibility, bool);
  vtkBooleanMacro(SecondaryHandlesVisibility, bool);
  ///@}

  /**
   * The interaction state may be set from a widget (e.g., vtkCamera3DWidget)
   * or other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  void SetInteractionState(int state);

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp (i.e., support rendering).
   * GetActors adds all the internal props used by this representation to the supplied collection.
   */
  void GetActors(vtkPropCollection*) override;

  // Used to manage the state of the widget
  enum
  {
    Outside = 0,
    Translating,
    TranslatingPosition,
    TranslatingNearTarget,
    TranslatingTarget,
    TranslatingUp,
    Scaling
  };

protected:
  vtkCamera3DRepresentation();
  ~vtkCamera3DRepresentation() override;

  virtual void CreateDefaultProperties();
  virtual void UpdateGeometry();
  void HighlightHandle(vtkProp* prop);

  // Helper methods
  virtual void TranslateAll(const double p1[4], const double p2[4]);
  virtual void TranslatePosition(const double p1[4], const double p2[4]);
  virtual void TranslateTarget(const double p1[4], const double p2[4]);
  virtual void TranslateNearTarget(const double p1[4], const double p2[4]);
  virtual void TranslateUp(const double p1[4], const double p2[4]);
  virtual void Scale(const double p1[4], const double p2[4], int X, int Y);
  void GetTranslation(const double p1[4], const double p2[4], double v[3]);

  // Manage how the representation appears
  double LastEventPosition[3] = { 0.0 };
  double LastEventOrientation[4] = { 0.0 };
  double StartEventOrientation[4] = { 0.0 };

  // the camera object
  vtkSmartPointer<vtkCamera> Camera;
  vtkNew<vtkCameraActor> CameraFrustumActor;
  vtkNew<vtkTransform> CameraTransform;
  vtkNew<vtkTransform> FrontTransform;
  vtkNew<vtkTransform> UpTransform;
  vtkNew<vtkActor> CameraBoxActor;
  vtkNew<vtkActor> CameraConeActor;

  // secondary handles
  double FrontHandleDistance = 2.5;
  double UpHandleDistance = 1.5;
  std::array<vtkNew<vtkActor>, 3> HandleSphereActor;
  std::array<vtkNew<vtkSphereSource>, 3> HandleSphereGeometry;
  std::array<vtkNew<vtkActor>, 2> HandleLineActor;
  std::array<vtkNew<vtkLineSource>, 2> HandleLineGeometry;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkNew<vtkProperty> HandleProperty;
  vtkNew<vtkProperty> SelectedHandleProperty;

  // Do the picking
  vtkNew<vtkCellPicker> HandlePicker;
  vtkProp* CurrentHandle = nullptr;

  // Support GetBounds() method
  vtkNew<vtkBox> BoundingBox;

  int TranslationAxis = Axis::NONE;
  bool TranslatingAll = false;
  bool FrustumVisibility = true;
  bool SecondaryHandlesVisibility = true;

private:
  vtkCamera3DRepresentation(const vtkCamera3DRepresentation&) = delete;
  void operator=(const vtkCamera3DRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
