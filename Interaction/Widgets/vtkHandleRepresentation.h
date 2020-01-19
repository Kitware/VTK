// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHandleRepresentation
 * @brief   abstract class for representing widget handles
 *
 * This class defines an API for widget handle representations. These
 * representations interact with vtkHandleWidget. Various representations
 * can be used depending on the nature of the handle. The basic functionality
 * of the handle representation is to maintain a position. The position is
 * represented via a vtkCoordinate, meaning that the position can be easily
 * obtained in a variety of coordinate systems.
 *
 * Optional features for this representation include an active mode (the widget
 * appears only when the mouse pointer is close to it). The active distance is
 * expressed in pixels and represents a circle in display space.
 *
 * The class may be subclassed so that alternative representations can
 * be created.  The class defines an API and a default implementation that
 * the vtkHandleWidget interacts with to render itself in the scene.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
 *
 * @sa
 * vtkRectilinearWipeWidget vtkWidgetRepresentation vtkAbstractWidget
 */

#ifndef vtkHandleRepresentation_h
#define vtkHandleRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCoordinate;
class vtkRenderer;
class vtkPointPlacer;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkHandleRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkHandleRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Handles usually have their coordinates set in display coordinates
   * (generally by an associated widget) and internally maintain the position
   * in world coordinates. (Using world coordinates insures that handles are
   * rendered in the right position when the camera view changes.) These
   * methods are often subclassed because special constraint operations can
   * be used to control the actual positioning.
   */
  virtual void SetDisplayPosition(double pos[2]);
  virtual void GetDisplayPosition(double pos[2]);
  virtual double* GetDisplayPosition() VTK_SIZEHINT(2);
  virtual void SetWorldPosition(double pos[3]);
  virtual void GetWorldPosition(double pos[3]) VTK_FUTURE_CONST;
  virtual double* GetWorldPosition() VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered near enough to the widget to
   * be active.
   */
  vtkSetClampMacro(Tolerance, int, 1, 100);
  vtkGetMacro(Tolerance, int);
  ///@}

  ///@{
  /**
   * Flag controls whether the widget becomes visible when the mouse pointer
   * moves close to it (i.e., the widget becomes active). By default,
   * ActiveRepresentation is off and the representation is always visible.
   */
  vtkSetMacro(ActiveRepresentation, vtkTypeBool);
  vtkGetMacro(ActiveRepresentation, vtkTypeBool);
  vtkBooleanMacro(ActiveRepresentation, vtkTypeBool);
  ///@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget. Note that ComputeInteractionState() and several other methods
  // must be implemented by subclasses.
  enum InteractionStateType
  {
    Outside = 0,
    Nearby,
    Selecting,
    Translating,
    Scaling
  };

  ///@{
  /**
   * The interaction state may be set from a widget (e.g., HandleWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * processwith the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, Scaling);
  ///@}

  ///@{
  /**
   * Specify whether any motions (such as scale, translate, etc.) are
   * constrained in some way (along an axis, etc.) Widgets can use this
   * to control the resulting motion.
   */
  vtkSetMacro(Constrained, vtkTypeBool);
  vtkGetMacro(Constrained, vtkTypeBool);
  vtkBooleanMacro(Constrained, vtkTypeBool);
  ///@}

  /**
   * Method has to be overridden in the subclasses which has
   * constraints on placing the handle
   * (Ex. vtkConstrainedPointHandleRepresentation). It should return 1
   * if the position is within the constraint, else it should return
   * 0. By default it returns 1.
   */
  virtual int CheckConstraint(vtkRenderer* renderer, double pos[2]);

  ///@{
  /**
   * Methods to make this class properly act like a vtkWidgetRepresentation.
   */
  void ShallowCopy(vtkProp* prop) override;
  virtual void DeepCopy(vtkProp* prop);
  void SetRenderer(vtkRenderer* ren) override;
  ///@}

  /**
   * Overload the superclasses' GetMTime() because the internal vtkCoordinates
   * are used to keep the state of the representation.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the point placer. Point placers can be used to dictate constraints
   * on the placement of handles. As an example, see vtkBoundedPlanePointPlacer
   * (constrains the placement of handles to a set of bounded planes)
   * vtkFocalPlanePointPlacer (constrains placement on the focal plane) etc.
   * The default point placer is vtkPointPlacer (which does not apply any
   * constraints, so the handles are free to move anywhere).
   */
  virtual void SetPointPlacer(vtkPointPlacer*);
  vtkGetObjectMacro(PointPlacer, vtkPointPlacer);
  ///@}

  ///@{
  /**
   * Gets the translation vector
   */
  virtual void GetTranslationVector(const double* p1, const double* p2, double* v) const;

  ///@{
  /**
   * Translates world position by vector p1p2 projected on the constraint axis if any.
   */
  virtual void Translate(const double* p1, const double* p2);
  ///@}

  ///@{
  /**
   * Translates world position by vector v projected on the constraint axis if any.
   */
  virtual void Translate(const double* v);
  ///@}

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
  void SetCustomTranslationAxisOn() { this->TranslationAxis = Axis::Custom; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  ///@}

  ///@{
  /**
   * Get/Set the translation axis used when vtkHandleRepresentation::TranslationAxis
   * is set to Axis::Custom.
   *
   * @see SetCustomTranslationAxisOn
   */
  vtkGetVector3Macro(CustomTranslationAxis, double);
  vtkSetVector3Macro(CustomTranslationAxis, double);
  ///@}

  ///@{
  /**
   * Returns true if ConstrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  ///@}

protected:
  vtkHandleRepresentation();
  ~vtkHandleRepresentation() override;

  int Tolerance = 15;
  vtkTypeBool ActiveRepresentation = false;
  vtkTypeBool Constrained = false;

  // Two vtkCoordinates are available to subclasses, one in display
  // coordinates and the other in world coordinates. These facilitate
  // the conversion between these two systems. Note that the WorldPosition
  // is the ultimate maintainer of position.
  vtkNew<vtkCoordinate> DisplayPosition;
  vtkNew<vtkCoordinate> WorldPosition;

  // Keep track of when coordinates were changed
  vtkTimeStamp DisplayPositionTime;
  vtkTimeStamp WorldPositionTime;

  // Constraint the placement of handles.
  vtkPointPlacer* PointPlacer;

  // Constraint axis translation
  int TranslationAxis = Axis::NONE;
  double CustomTranslationAxis[3] = { 1.0, 0.0, 0.0 };

private:
  vtkHandleRepresentation(const vtkHandleRepresentation&) = delete;
  void operator=(const vtkHandleRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
