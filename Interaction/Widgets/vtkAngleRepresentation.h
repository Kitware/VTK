/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAngleRepresentation - represent the vtkAngleWidget
// .SECTION Description
// The vtkAngleRepresentation is a superclass for classes representing the
// vtkAngleWidget. This representation consists of two rays and three
// vtkHandleRepresentations to place and manipulate the three points defining
// the angle representation. (Note: the three points are referred to as Point1,
// Center, and Point2, at the two end points (Point1 and Point2) and Center
// (around which the angle is measured).

// .SECTION See Also
// vtkAngleWidget vtkHandleRepresentation vtkAngleRepresentation2D


#ifndef __vtkAngleRepresentation_h
#define __vtkAngleRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkAngleRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkAngleRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This representation and all subclasses must keep an angle (in degrees)
  // consistent with the state of the widget.
  virtual double GetAngle() = 0;

  // Description:
  // Methods to Set/Get the coordinates of the three points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  virtual void GetPoint1WorldPosition(double pos[3]) = 0;
  virtual void GetCenterWorldPosition(double pos[3]) = 0;
  virtual void GetPoint2WorldPosition(double pos[3]) = 0;
  virtual void SetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void SetCenterDisplayPosition(double pos[3]) = 0;
  virtual void SetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void GetCenterDisplayPosition(double pos[3]) = 0;
  virtual void GetPoint2DisplayPosition(double pos[3]) = 0;

  // Description:
  // This method is used to specify the type of handle representation to use
  // for the three internal vtkHandleWidgets within vtkAngleRepresentation.
  // To use this method, create a dummy vtkHandleRepresentation (or
  // subclass), and then invoke this method with this dummy. Then the
  // vtkAngleRepresentation uses this dummy to clone three
  // vtkHandleRepresentations of the same type. Make sure you set the handle
  // representation before the widget is enabled. (The method
  // InstantiateHandleRepresentation() is invoked by the vtkAngle widget.)
  void SetHandleRepresentation(vtkHandleRepresentation *handle);
  void InstantiateHandleRepresentation();

  // Description:
  // Set/Get the handle representations used for the vtkAngleRepresentation.
  vtkGetObjectMacro(Point1Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(CenterRepresentation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point2Representation,vtkHandleRepresentation);

  // Description:
  // The tolerance representing the distance to the representation (in
  // pixels) in which the cursor is considered near enough to the end points
  // of the representation to be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // Specify the format to use for labelling the angle. Note that an empty
  // string results in no label, or a format string without a "%" character
  // will not print the angle value.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Special methods for turning off the rays and arc that define the cone
  // and arc of the angle.
  vtkSetMacro(Ray1Visibility,int);
  vtkGetMacro(Ray1Visibility,int);
  vtkBooleanMacro(Ray1Visibility,int);
  vtkSetMacro(Ray2Visibility,int);
  vtkGetMacro(Ray2Visibility,int);
  vtkBooleanMacro(Ray2Visibility,int);
  vtkSetMacro(ArcVisibility,int);
  vtkGetMacro(ArcVisibility,int);
  vtkBooleanMacro(ArcVisibility,int);

//BTX -- used to communicate about the state of the representation
  enum {Outside=0,NearP1,NearCenter,NearP2};
//ETX

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void CenterWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);

protected:
  vtkAngleRepresentation();
  ~vtkAngleRepresentation();

  // The handle and the rep used to close the handles
  vtkHandleRepresentation *HandleRepresentation;
  vtkHandleRepresentation *Point1Representation;
  vtkHandleRepresentation *CenterRepresentation;
  vtkHandleRepresentation *Point2Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Visibility of the various pieces of the representation
  int Ray1Visibility;
  int Ray2Visibility;
  int ArcVisibility;

  // Format for the label
  char *LabelFormat;

private:
  vtkAngleRepresentation(const vtkAngleRepresentation&);  //Not implemented
  void operator=(const vtkAngleRepresentation&);  //Not implemented
};

#endif
