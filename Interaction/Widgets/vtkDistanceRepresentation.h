/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDistanceRepresentation
 * @brief   represent the vtkDistanceWidget
 *
 * The vtkDistanceRepresentation is a superclass for various types of
 * representations for the vtkDistanceWidget. Logically subclasses consist of
 * an axis and two handles for placing/manipulating the end points.
 *
 * @sa
 * vtkDistanceWidget vtkHandleRepresentation vtkDistanceRepresentation2D vtkDistanceRepresentation
*/

#ifndef vtkDistanceRepresentation_h
#define vtkDistanceRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkDistanceRepresentation : public vtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkDistanceRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * This representation and all subclasses must keep a distance
   * consistent with the state of the widget.
   */
  virtual double GetDistance() = 0;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  virtual void GetPoint1WorldPosition(double pos[3]) = 0;
  virtual void GetPoint2WorldPosition(double pos[3]) = 0;
  virtual double* GetPoint1WorldPosition() VTK_SIZEHINT(3) = 0;
  virtual double* GetPoint2WorldPosition() VTK_SIZEHINT(3) = 0;
  virtual void SetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint1WorldPosition(double pos[3])=0;
  virtual void SetPoint2WorldPosition(double pos[3])=0;
  //@}

  //@{
  /**
   * This method is used to specify the type of handle representation to
   * use for the two internal vtkHandleWidgets within vtkDistanceWidget.
   * To use this method, create a dummy vtkHandleWidget (or subclass),
   * and then invoke this method with this dummy. Then the
   * vtkDistanceRepresentation uses this dummy to clone two vtkHandleWidgets
   * of the same type. Make sure you set the handle representation before
   * the widget is enabled. (The method InstantiateHandleRepresentation()
   * is invoked by the vtkDistance widget.)
   */
  void SetHandleRepresentation(vtkHandleRepresentation *handle);
  void InstantiateHandleRepresentation();
  //@}

  //@{
  /**
   * Set/Get the two handle representations used for the vtkDistanceWidget. (Note:
   * properties can be set by grabbing these representations and setting the
   * properties appropriately.)
   */
  vtkGetObjectMacro(Point1Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point2Representation,vtkHandleRepresentation);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels) in
   * which the cursor is considered near enough to the end points of
   * the widget to be active.
   */
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);
  //@}

  //@{
  /**
   * Specify the format to use for labelling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the distance value.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set the scale factor from VTK world coordinates. The ruler marks and label
   * will be defined in terms of the scaled space. For example, if the VTK world
   * coordinates are assumed to be in inches, but the desired distance units
   * should be defined in terms of centimeters, the scale factor should be set
   * to 2.54. The ruler marks will then be spaced in terms of centimeters, and
   * the label will show the measurement in centimeters.
   */
  vtkSetMacro(Scale,double);
  vtkGetMacro(Scale,double);
  //@}

  //@{
  /**
   * Enable or disable ruler mode. When enabled, the ticks on the distance widget
   * are separated by the amount specified by RulerDistance. Otherwise, the ivar
   * NumberOfRulerTicks is used to draw the tick marks.
   */
  vtkSetMacro(RulerMode,vtkTypeBool);
  vtkGetMacro(RulerMode,vtkTypeBool);
  vtkBooleanMacro(RulerMode,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify the RulerDistance which indicates the spacing of the major ticks.
   * This ivar only has effect when the RulerMode is on.
   */
  vtkSetClampMacro(RulerDistance,double,0,VTK_FLOAT_MAX);
  vtkGetMacro(RulerDistance,double);
  //@}

  //@{
  /**
   * Specify the number of major ruler ticks. This overrides any subclasses
   * (e.g., vtkDistanceRepresentation2D) that have alternative methods to
   * specify the number of major ticks. Note: the number of ticks is the
   * number between the two handle endpoints. This ivar only has effect
   * when the RulerMode is off.
   */
  vtkSetClampMacro(NumberOfRulerTicks,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfRulerTicks,int);
  //@}

  // Used to communicate about the state of the representation
  enum {Outside=0,NearP1,NearP2};

  //@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify=0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  void StartComplexInteraction(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata) override;
  void ComplexInteraction(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata) override;
  int ComputeComplexInteractionState(
    vtkRenderWindowInteractor *iren,
    vtkAbstractWidget *widget,
    unsigned long event, void *calldata, int modify = 0) override;
  //@}

protected:
  vtkDistanceRepresentation();
  ~vtkDistanceRepresentation() override;

  // The handle and the rep used to close the handles
  vtkHandleRepresentation *HandleRepresentation;
  vtkHandleRepresentation *Point1Representation;
  vtkHandleRepresentation *Point2Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char *LabelFormat;

  // Scale to change from the VTK world coordinates to the desired coordinate
  // system.
  double Scale;

  // Ruler related stuff
  vtkTypeBool RulerMode;
  double RulerDistance;
  int NumberOfRulerTicks;

private:
  vtkDistanceRepresentation(const vtkDistanceRepresentation&) = delete;
  void operator=(const vtkDistanceRepresentation&) = delete;
};

#endif
