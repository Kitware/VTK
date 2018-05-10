/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSphereHandleRepresentation
 * @brief   A spherical rendition of point in 3D space
 *
 *
 * This class is a concrete implementation of vtkHandleRepresentation. It
 * renders handles as spherical blobs in 3D space.
 *
 * @sa
 * vtkHandleRepresentation vtkHandleWidget vtkSphereSource
*/

#ifndef vtkSphereHandleRepresentation_h
#define vtkSphereHandleRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkHandleRepresentation.h"
#include "vtkSphereSource.h" // Needed for delegation to sphere

class vtkSphereSource;
class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkSphereHandleRepresentation
                        : public vtkHandleRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSphereHandleRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkSphereHandleRepresentation,vtkHandleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the position of the point in world and display coordinates. Note
   * that if the position is set outside of the bounding box, it will be
   * clamped to the boundary of the bounding box. This method overloads
   * the superclasses' SetWorldPosition() and SetDisplayPosition() in
   * order to set the focal point of the cursor properly.
   */
  void SetWorldPosition(double p[3]) override;
  void SetDisplayPosition(double p[3]) override;
  //@}

  //@{
  /**
   * If translation mode is on, as the widget is moved the bounding box,
   * shadows, and cursor are all translated simultaneously as the point moves
   * (i.e., the left and middle mouse buttons act the same).  Otherwise, only
   * the cursor focal point moves, which is constrained by the bounds of the
   * point representation. (Note that the bounds can be scaled up using the
   * right mouse button.)
   */
  vtkSetMacro(TranslationMode,vtkTypeBool);
  vtkGetMacro(TranslationMode,vtkTypeBool);
  vtkBooleanMacro(TranslationMode,vtkTypeBool);
  //@}

  void SetSphereRadius(double);
  double GetSphereRadius();

  //@{
  /**
   * Set/Get the handle properties when unselected and selected.
   */
  void SetProperty(vtkProperty*);
  void SetSelectedProperty(vtkProperty*);
  vtkGetObjectMacro(Property,vtkProperty);
  vtkGetObjectMacro(SelectedProperty,vtkProperty);
  //@}

  //@{
  /**
   * Set the "hot spot" size; i.e., the region around the focus, in which the
   * motion vector is used to control the constrained sliding action. Note the
   * size is specified as a fraction of the length of the diagonal of the
   * point widget's bounding box.
   */
  vtkSetClampMacro(HotSpotSize,double,0.0,1.0);
  vtkGetMacro(HotSpotSize,double);
  //@}

  /**
   * Overload the superclasses SetHandleSize() method to update internal
   * variables.
   */
  void SetHandleSize(double size) override;

  //@{
  /**
   * Methods to make this class properly act like a vtkWidgetRepresentation.
   */
  double *GetBounds() VTK_SIZEHINT(6) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify=0) override;
  void PlaceWidget(double bounds[6]) override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void ShallowCopy(vtkProp *prop) override;
  void DeepCopy(vtkProp *prop) override;
  void GetActors(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  void Highlight(int highlight) override;

  /*
  * Register internal Pickers within PickingManager
  */
  void RegisterPickers() override;

protected:
  vtkSphereHandleRepresentation();
  ~vtkSphereHandleRepresentation() override;

  // the cursor3D
  vtkActor          *Actor;
  vtkPolyDataMapper *Mapper;
  vtkSphereSource   *Sphere;
  // void Highlight(int highlight);

  // Do the picking
  vtkCellPicker *CursorPicker;
  double LastPickPosition[3];
  double LastEventPosition[2];

  // Methods to manipulate the cursor
  int  ConstraintAxis;
  void Translate(double *p1, double *p2);
  void Scale(double *p1, double *p2, double eventPos[2]);
  void MoveFocus(double *p1, double *p2);
  void SizeBounds();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *Property;
  vtkProperty *SelectedProperty;
  void         CreateDefaultProperties();

  // The size of the hot spot.
  double HotSpotSize;
  int    DetermineConstraintAxis(int constraint, double *x);
  int    WaitingForMotion;
  int    WaitCount;

  // Current handle sized (may reflect scaling)
  double CurrentHandleSize;

  // Control how translation works
  vtkTypeBool TranslationMode;

private:
  vtkSphereHandleRepresentation(const vtkSphereHandleRepresentation&) = delete;
  void operator=(const vtkSphereHandleRepresentation&) = delete;
};

#endif
