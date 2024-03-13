// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTensorRepresentation
 * @brief   class defining a representation for the vtkTensorWidget
 *
 * This class is a concrete representation for the vtkTensorWidget. In
 * summary, it allows the editing of a tensor glyph (by modifying the
 * underlying tensor value). This includes controlling the position, scaling,
 * and rotation of the glyph. The representation is simply an oriented,
 * scaled box which can be manipulated to transform the tensor. Optionally,
 * an ellipsoid defined by the tensor eigenvectors can be shown for
 * informational purposes.
 *
 * To use this representation, specify a 3x3 real, symmetric matrix defining
 * the tensor. (This implicitly defines an orthogonal basis from the three
 * tensor eigenvectors.) Then use PlaceWidget() to define a bounding box: the
 * bounding box defines a position for the tensor from its center point, and
 * the representation is scaled to fit in the bounding box.
 *
 * Note: typical usage is to place a tensor glyph inside of the
 * representation (i.e., the box) which is updated as the representation is
 * manipulated by the user. The built-in ellipsoid can be used for this;
 * alternatively through callbacks and such, it is possible to place
 * other glyph types such as superquadrics.
 *
 * @sa
 * vtkTensorWidget vtkBoxRepresentation
 */

#ifndef vtkTensorRepresentation_h
#define vtkTensorRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkPolyDataMapper;
class vtkLineSource;
class vtkSphereSource;
class vtkCellPicker;
class vtkProperty;
class vtkPolyData;
class vtkPoints;
class vtkPolyDataAlgorithm;
class vtkPointHandleRepresentation3D;
class vtkTransform;
class vtkMatrix4x4;
class vtkPlane;
class vtkPlanes;
class vtkBox;
class vtkDoubleArray;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkTensorRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkTensorRepresentation* New();
  vtkTypeMacro(vtkTensorRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * These are the basic methods used to define the tensor (these methods
   * coordinate with the overloaded PlaceWidget() method). The methods enable
   * specification of a 3x3 symmetric tensor. This information is used to
   * construct an oriented, appropriately ellipsoid that is (initially)
   * centered and fits inside the bounding box defined by PlaceWidget(). As
   * this widget is modified during user interaction, the tensor data member
   * is continuously updated and can be queried. Note that a symmetric tensor
   * can be defined with only six components. If a full 3x3 tensor is
   * specified, only the symmetrical part of the tensor is used since the
   * extracted eigenvalues/eigenvecters are required to be real valued. When
   * a tensor is specified, the derived information (e.g.,
   * eigenvalues/vectors and position) are immediately updated.
   */
  void SetTensor(double tensor[9]);
  void SetSymmetricTensor(double symTensor[6]);
  void GetTensor(double tensor[9]) { std::copy(this->Tensor, this->Tensor + 9, tensor); }
  void GetSymmetricTensor(double symTensor[6])
  {
    symTensor[0] = this->Tensor[0];
    symTensor[1] = this->Tensor[4];
    symTensor[2] = this->Tensor[8];
    symTensor[3] = this->Tensor[1];
    symTensor[4] = this->Tensor[2];
    symTensor[5] = this->Tensor[5];
  }
  ///@}

  ///@{
  /**
   * These are methods used to retrieve derived information about the tensor.
   * Specify (0<=i<3) to retrieve the ith eigenvector. The eigenvalues and
   * associated eigenvectors are sorted in decreasing order.
   */
  void GetEigenvalues(double evals[3])
  {
    std::copy(this->Eigenvalues, this->Eigenvalues + 3, evals);
  }
  void GetEigenvector(int n, double ev[3])
  {
    n = (n < 0 ? 0 : (n > 2 ? 2 : n));
    std::copy(this->Eigenvectors[n], this->Eigenvectors[n] + 3, ev);
  }
  ///@}

  ///@{
  /**
   * Set/Get a position for the location of the tensor. Of course a tensor
   * inherently has no position, but this is for the purpose of placing
   * this widget representation.
   */
  void SetPosition(double pos[3]);
  void GetPosition(double pos[3])
  {
    std::copy(this->TensorPosition, this->TensorPosition + 3, pos);
  }
  ///@}

  /**
   * Grab the polydata (including points) that define the representation. The
   * polydata consists of 6 quadrilateral faces and 15 points. The first
   * eight points define the eight corner vertices; the next six define the
   * -x,+x, -y,+y, -z,+z face points; and the final point (the 15th out of 15
   * points) defines the center of the box. These point values are guaranteed
   * to be up-to-date when either the widget's corresponding InteractionEvent
   * or EndInteractionEvent events are invoked. The user provides the
   * vtkPolyData and the points and cells are added to it.
   */
  void GetPolyData(vtkPolyData* pd);

  ///@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles, when selected or normal, can be
   * specified.
   */
  vtkGetObjectMacro(HandleProperty, vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the face properties (the faces of the box). The
   * properties of the face when selected and normal can be
   * set.
   */
  vtkGetObjectMacro(FaceProperty, vtkProperty);
  vtkGetObjectMacro(SelectedFaceProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the outline properties (the outline of the box). The
   * properties of the outline when selected and normal can be
   * set.
   */
  vtkGetObjectMacro(OutlineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Get the tensor ellipsoid properties. If visibility is enabled,
   * the ellipsoid will be rendered with this property.
   */
  vtkGetObjectMacro(EllipsoidProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Control the representation of the outline. This flag enables
   * face wires. By default face wires are off.
   */
  void SetOutlineFaceWires(bool);
  vtkGetMacro(OutlineFaceWires, bool);
  void OutlineFaceWiresOn() { this->SetOutlineFaceWires(true); }
  void OutlineFaceWiresOff() { this->SetOutlineFaceWires(false); }
  ///@}

  ///@{
  /**
   * Control the representation of the outline. This flag enables
   * the cursor lines running between the handles. By default cursor
   * wires are on.
   */
  void SetOutlineCursorWires(bool);
  vtkGetMacro(OutlineCursorWires, bool);
  void OutlineCursorWiresOn() { this->SetOutlineCursorWires(true); }
  void OutlineCursorWiresOff() { this->SetOutlineCursorWires(false); }
  ///@}

  ///@{
  /**
   * Switches handles (the spheres) on or off by manipulating the underlying
   * actor visibility.
   */
  virtual void HandlesOn();
  virtual void HandlesOff();
  ///@}

  ///@{
  /**
   * Indicate whether to show the tensor ellipsoid. By default it is on.
   */
  void SetTensorEllipsoid(bool);
  vtkGetMacro(TensorEllipsoid, bool);
  void TensorEllipsoidOn() { this->SetTensorEllipsoid(true); }
  void TensorEllipsoidOff() { this->SetTensorEllipsoid(false); }
  ///@}

  /**
   * This is a specialized place widget method for a tensor. Specify the
   * tensor (an array of 9 components) and the position to place the tensor.
   * Note that the PlaceFactor (defined in superclass) can be used to
   * scale the representation when placed.
   */
  void PlaceTensor(double tensor[9], double position[3]);

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
  void StartComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  void EndComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
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

  // Used to manage the state of the widget
  enum
  {
    Outside = 0,
    MoveF0,
    MoveF1,
    MoveF2,
    MoveF3,
    MoveF4,
    MoveF5,
    Translating,
    Rotating,
    Scaling
  };

  /**
   * The interaction state may be set from a widget (e.g., vtkTensorWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  void SetInteractionState(int state);

  ///@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  vtkGetMacro(SnapToAxes, bool);
  vtkSetMacro(SnapToAxes, bool);
  ///@}

  ///@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  void StepForward();
  void StepBackward();
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  ///@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  vtkGetMacro(TranslationAxis, int);
  vtkSetClampMacro(TranslationAxis, int, -1, 2);
  ///@}

  ///@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  ///@}

  ///@{
  /**
   * Returns true if ConstrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  ///@}

protected:
  vtkTensorRepresentation();
  ~vtkTensorRepresentation() override;

  // Core data
  double Tensor[9]; // stored as 3x3 symmetric matrix
  double Eigenvalues[3];
  double Eigenvectors[3][3];
  double TensorPosition[3];

  // Manage how the representation appears
  double LastEventPosition[3];
  double LastEventOrientation[4];
  double StartEventOrientation[4];
  double SnappedEventOrientations[3][4];
  bool SnappedOrientation[3];
  bool SnapToAxes;

  // Constraint axis translation
  int TranslationAxis;

  // the hexahedron (6 faces)
  vtkActor* HexActor;
  vtkPolyDataMapper* HexMapper;
  vtkPolyData* HexPolyData;
  vtkPoints* Points; // used by others as well
  double N[6][3];    // the normals of the faces

  // A face of the hexahedron
  vtkActor* HexFace;
  vtkPolyDataMapper* HexFaceMapper;
  vtkPolyData* HexFacePolyData;

  // glyphs representing hot spots (e.g., handles)
  vtkActor** Handle;
  vtkPolyDataMapper** HandleMapper;
  vtkSphereSource** HandleGeometry;
  virtual void PositionHandles();
  int HighlightHandle(vtkProp* prop); // returns cell id
  void HighlightFace(int cellId);
  void HighlightOutline(int highlight);
  virtual void ComputeNormals();
  virtual void SizeHandles();

  // wireframe outline
  vtkActor* HexOutline;
  vtkPolyDataMapper* OutlineMapper;
  vtkPolyData* OutlinePolyData;

  // the tensor ellipsoid and transforms
  vtkActor* EllipsoidActor;
  vtkTransform* EllipsoidTransform;
  vtkMatrix4x4* EllipsoidMatrix;
  vtkPolyDataMapper* EllipsoidMapper;
  vtkSphereSource* EllipsoidSource;

  // Do the picking
  vtkCellPicker* HandlePicker;
  vtkCellPicker* HexPicker;
  vtkActor* CurrentHandle;
  int CurrentHexFace;
  vtkCellPicker* LastPicker;

  // Transform the hexahedral points (used for rotations)
  vtkTransform* Transform;
  vtkMatrix4x4* Matrix;
  vtkPoints* TmpPoints;

  // Support GetBounds() method
  vtkBox* BoundingBox;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty* HandleProperty;
  vtkProperty* SelectedHandleProperty;
  vtkProperty* FaceProperty;
  vtkProperty* SelectedFaceProperty;
  vtkProperty* OutlineProperty;
  vtkProperty* SelectedOutlineProperty;
  vtkProperty* EllipsoidProperty;
  virtual void CreateDefaultProperties();

  // Control the orientation of the normals
  bool InsideOut;
  bool OutlineFaceWires;
  bool OutlineCursorWires;
  void GenerateOutline();
  bool TensorEllipsoid;
  void UpdateTensorFromWidget(); // tensor information updated from widget state
  void UpdateWidgetFromTensor(); // widget state updated from tensor specification
  void UpdateTensorEigenfunctions(double tensor[3][3]);

  // Helper methods
  virtual void Translate(const double* p1, const double* p2);
  virtual void Scale(const double* p1, const double* p2, int X, int Y);
  virtual void Rotate(int X, int Y, const double* p1, const double* p2, const double* vpn);
  void MovePlusXFace(const double* p1, const double* p2, bool entry);
  void MoveMinusXFace(const double* p1, const double* p2, bool entry);
  void MovePlusYFace(const double* p1, const double* p2, bool entry);
  void MoveMinusYFace(const double* p1, const double* p2, bool entry);
  void MovePlusZFace(const double* p1, const double* p2, bool entry);
  void MoveMinusZFace(const double* p1, const double* p2, bool entry);
  void UpdatePose(const double* p1, const double* d1, const double* p2, const double* d2);

  // Internal ivars for performance
  vtkPoints* PlanePoints;
  vtkDoubleArray* PlaneNormals;

  // The actual planes which are being manipulated
  vtkPlane* Planes[6];

  //"dir" is the direction in which the face can be moved i.e. the axis passing
  // through the center
  void MoveFace(const double* p1, const double* p2, const double* dir, double* x1, double* x2,
    double* x3, double* x4, double* x5);

  // Helper method to obtain the direction in which the face is to be moved.
  // Handles special cases where some of the scale factors are 0.
  void GetDirection(const double Nx[3], const double Ny[3], const double Nz[3], double dir[3]);

private:
  vtkTensorRepresentation(const vtkTensorRepresentation&) = delete;
  void operator=(const vtkTensorRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
