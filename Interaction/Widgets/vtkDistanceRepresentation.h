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
// .NAME vtkDistanceRepresentation - represent the vtkDistanceWidget
// .SECTION Description
// The vtkDistanceRepresentation is a superclass for various types of
// representations for the vtkDistanceWidget. Logically subclasses consist of
// an axis and two handles for placing/manipulating the end points.

// .SECTION See Also
// vtkDistanceWidget vtkHandleRepresentation vtkDistanceRepresentation2D vtkDistanceRepresentation


#ifndef __vtkDistanceRepresentation_h
#define __vtkDistanceRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkDistanceRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkDistanceRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This representation and all subclasses must keep a distance
  // consistent with the state of the widget.
  virtual double GetDistance() = 0;

  // Description:
  // Methods to Set/Get the coordinates of the two points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  virtual void GetPoint1WorldPosition(double pos[3]) = 0;
  virtual void GetPoint2WorldPosition(double pos[3]) = 0;
  virtual double* GetPoint1WorldPosition() = 0;
  virtual double* GetPoint2WorldPosition() = 0;
  virtual void SetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint1WorldPosition(double pos[3])=0;
  virtual void SetPoint2WorldPosition(double pos[3])=0;

  // Description:
  // This method is used to specify the type of handle representation to
  // use for the two internal vtkHandleWidgets within vtkDistanceWidget.
  // To use this method, create a dummy vtkHandleWidget (or subclass),
  // and then invoke this method with this dummy. Then the
  // vtkDistanceRepresentation uses this dummy to clone two vtkHandleWidgets
  // of the same type. Make sure you set the handle representation before
  // the widget is enabled. (The method InstantiateHandleRepresentation()
  // is invoked by the vtkDistance widget.)
  void SetHandleRepresentation(vtkHandleRepresentation *handle);
  void InstantiateHandleRepresentation();

  // Description:
  // Set/Get the two handle representations used for the vtkDistanceWidget. (Note:
  // properties can be set by grabbing these representations and setting the
  // properties appropriately.)
  vtkGetObjectMacro(Point1Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point2Representation,vtkHandleRepresentation);

  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the end points of
  // the widget to be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // Specify the format to use for labelling the distance. Note that an empty
  // string results in no label, or a format string without a "%" character
  // will not print the distance value.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Enable or disable ruler mode. When enabled, the ticks on the distance widget
  // are separated by the amount specified by RulerDistance. Otherwise, the ivar
  // NumberOfRulerTicks is used to draw the tick marks.
  vtkSetMacro(RulerMode,int);
  vtkGetMacro(RulerMode,int);
  vtkBooleanMacro(RulerMode,int);

  // Description:
  // Specify the RulerDistance which indicates the spacing of the major ticks.
  // This ivar only has effect when the RulerMode is on.
  vtkSetClampMacro(RulerDistance,double,0,VTK_LARGE_FLOAT);
  vtkGetMacro(RulerDistance,double);

  // Description:
  // Specify the number of major ruler ticks. This overrides any subclasses
  // (e.g., vtkDistanceRepresentation2D) that have alternative methods to
  // specify the number of major ticks. Note: the number of ticks is the
  // number between the two handle endpoints. This ivar only has effect
  // when the RulerMode is off.
  vtkSetClampMacro(NumberOfRulerTicks,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfRulerTicks,int);

//BTX -- used to communicate about the state of the representation
  enum {Outside=0,NearP1,NearP2};
//ETX

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);

protected:
  vtkDistanceRepresentation();
  ~vtkDistanceRepresentation();

  // The handle and the rep used to close the handles
  vtkHandleRepresentation *HandleRepresentation;
  vtkHandleRepresentation *Point1Representation;
  vtkHandleRepresentation *Point2Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char *LabelFormat;

  // Ruler related stuff
  int RulerMode;
  double RulerDistance;
  int NumberOfRulerTicks;

private:
  vtkDistanceRepresentation(const vtkDistanceRepresentation&);  //Not implemented
  void operator=(const vtkDistanceRepresentation&);  //Not implemented
};

#endif
