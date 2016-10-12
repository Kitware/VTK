/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkContextTransform
 * @brief   all children of this item are transformed
 * by the vtkTransform2D of this item.
 *
 *
 * This class can be used to transform all child items of this class. The
 * default transform is the identity.
*/

#ifndef vtkContextTransform_h
#define vtkContextTransform_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"
#include "vtkSmartPointer.h" // Needed for SP ivars.
#include "vtkVector.h" // Needed for ivars.

class vtkTransform2D;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextTransform : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextTransform, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a vtkContextTransform object.
   */
  static vtkContextTransform *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Reset the transform to the identity transformation.
   */
  virtual void Identity();

  /**
   * Translate the item by the specified amounts dx and dy in the x and y
   * directions.
   */
  virtual void Translate(float dx, float dy);

  /**
   * Scale the item by the specified amounts dx and dy in the x and y
   * directions.
   */
  virtual void Scale(float dx, float dy);

  /**
   * Rotate the item by the specified angle.
   */
  virtual void Rotate(float angle);

  /**
   * Access the vtkTransform2D that controls object transformation.
   */
  virtual vtkTransform2D* GetTransform();

  /**
   * Transforms a point to the parent coordinate system.
   */
  virtual vtkVector2f MapToParent(const vtkVector2f& point);

  /**
   * Transforms a point from the parent coordinate system.
   */
  virtual vtkVector2f MapFromParent(const vtkVector2f& point);

  //@{
  /**
   * The mouse button from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::LEFT_BUTTON.
   */
  vtkSetMacro(PanMouseButton, int);
  vtkGetMacro(PanMouseButton, int);
  //@}

  //@{
  /**
   * The modifier from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::NO_MODIFIER.
   */
  vtkSetMacro(PanModifier, int);
  vtkGetMacro(PanModifier, int);
  //@}

  //@{
  /**
   * A secondary mouse button from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::NO_BUTTON (disabled).
   */
  vtkSetMacro(SecondaryPanMouseButton, int);
  vtkGetMacro(SecondaryPanMouseButton, int);
  //@}

  //@{
  /**
   * A secondary modifier from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::NO_MODIFIER.
   */
  vtkSetMacro(SecondaryPanModifier, int);
  vtkGetMacro(SecondaryPanModifier, int);
  //@}

  //@{
  /**
   * The mouse button from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::RIGHT_BUTTON.
   */
  vtkSetMacro(ZoomMouseButton, int);
  vtkGetMacro(ZoomMouseButton, int);
  //@}

  //@{
  /**
   * The modifier from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::NO_MODIFIER.
   */
  vtkSetMacro(ZoomModifier, int);
  vtkGetMacro(ZoomModifier, int);
  //@}

  //@{
  /**
   * A secondary mouse button from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::LEFT_BUTTON.
   */
  vtkSetMacro(SecondaryZoomMouseButton, int);
  vtkGetMacro(SecondaryZoomMouseButton, int);
  //@}

  //@{
  /**
   * A secondary modifier from vtkContextMouseEvent to use for panning.
   * Default is vtkContextMouseEvent::SHIFT_MODIFIER.
   */
  vtkSetMacro(SecondaryZoomModifier, int);
  vtkGetMacro(SecondaryZoomModifier, int);
  //@}

  //@{
  /**
   * Whether to zoom on mouse wheels. Default is true.
   */
  vtkSetMacro(ZoomOnMouseWheel, bool);
  vtkGetMacro(ZoomOnMouseWheel, bool);
  vtkBooleanMacro(ZoomOnMouseWheel, bool);
  //@}

  //@{
  /**
   * Whether to pan in the Y direction on mouse wheels. Default is false.
   */
  vtkSetMacro(PanYOnMouseWheel, bool);
  vtkGetMacro(PanYOnMouseWheel, bool);
  vtkBooleanMacro(PanYOnMouseWheel, bool);
  //@}

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Mouse press event. Keep track of zoom anchor position.
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event. Perform pan or zoom as specified by the mouse bindings.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse wheel event. Perform pan or zoom as specified by mouse bindings.
   */
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

protected:
  vtkContextTransform();
  ~vtkContextTransform();

  vtkSmartPointer<vtkTransform2D> Transform;

  int PanMouseButton;
  int PanModifier;
  int ZoomMouseButton;
  int ZoomModifier;
  int SecondaryPanMouseButton;
  int SecondaryPanModifier;
  int SecondaryZoomMouseButton;
  int SecondaryZoomModifier;

  bool ZoomOnMouseWheel;
  bool PanYOnMouseWheel;

  vtkVector2f ZoomAnchor;

private:
  vtkContextTransform(const vtkContextTransform &) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextTransform &) VTK_DELETE_FUNCTION;

};

#endif //vtkContextTransform_h
