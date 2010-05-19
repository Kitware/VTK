/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiDimensionalRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBiDimensionalRepresentation2D - represent the vtkBiDimensionalWidget
// .SECTION Description
// The vtkBiDimensionalRepresentation2D is used to represent the
// bi-dimensional measure in a 2D (overlay) context. This representation
// consists of two perpendicular lines defined by four
// vtkHandleRepresentations. The four handles can be independently
// manipulated consistent with the orthogonal constraint on the lines. (Note:
// the four points are referred to as Point1, Point2, Point3 and
// Point4. Point1 and Point2 define the first line; and Point3 and Point4
// define the second orthogonal line.)
//
// To create this widget, you click to place the first two points. The third
// point is mirrored with the fourth point; when you place the third point
// (which is orthogonal to the lined defined by the first two points), the
// fourth point is dropped as well. After definition, the four points can be
// moved (in constrained fashion, preserving orthogonality). Further, the
// entire widget can be translated by grabbing the center point of the widget;
// each line can be moved along the other line; and the entire widget can be
// rotated around its center point.

// .SECTION See Also
// vtkAngleWidget vtkHandleRepresentation vtkBiDimensionalRepresentation2D


#ifndef __vtkBiDimensionalRepresentation2D_h
#define __vtkBiDimensionalRepresentation2D_h

#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;
class vtkCellArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextMapper;
class vtkActor2D;
class vtkProperty2D;
class vtkTextProperty;


class VTK_WIDGETS_EXPORT vtkBiDimensionalRepresentation2D : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkBiDimensionalRepresentation2D *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkBiDimensionalRepresentation2D,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to Set/Get the coordinates of the four points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  virtual void SetPoint1WorldPosition(double pos[3]);
  virtual void SetPoint2WorldPosition(double pos[3]);
  virtual void SetPoint3WorldPosition(double pos[3]);
  virtual void SetPoint4WorldPosition(double pos[3]);
  virtual void GetPoint1WorldPosition(double pos[3]);
  virtual void GetPoint2WorldPosition(double pos[3]);
  virtual void GetPoint3WorldPosition(double pos[3]);
  virtual void GetPoint4WorldPosition(double pos[3]);
  virtual void SetPoint1DisplayPosition(double pos[3]);
  virtual void SetPoint2DisplayPosition(double pos[3]);
  virtual void SetPoint3DisplayPosition(double pos[3]);
  virtual void SetPoint4DisplayPosition(double pos[3]);
  virtual void GetPoint1DisplayPosition(double pos[3]);
  virtual void GetPoint2DisplayPosition(double pos[3]);
  virtual void GetPoint3DisplayPosition(double pos[3]);
  virtual void GetPoint4DisplayPosition(double pos[3]);

  // Description:
  // Special methods for turning off the lines that define the bi-dimensional
  // measure. Generally these methods are used by the vtkBiDimensionalWidget to
  // control the appearance of the widget. Note: turning off Line1 actually turns
  // off Line1 and Line2.
  vtkSetMacro(Line1Visibility,int);
  vtkGetMacro(Line1Visibility,int);
  vtkBooleanMacro(Line1Visibility,int);
  vtkSetMacro(Line2Visibility,int);
  vtkGetMacro(Line2Visibility,int);
  vtkBooleanMacro(Line2Visibility,int);

  // Description:
  // This method is used to specify the type of handle representation to use
  // for the four internal vtkHandleRepresentations within
  // vtkBiDimensionalRepresentation2D.  To use this method, create a dummy
  // vtkHandleRepresentation (or subclass), and then invoke this method with
  // this dummy. Then the vtkBiDimensionalRepresentation2D uses this dummy to
  // clone four vtkHandleRepresentations of the same type. Make sure you set the
  // handle representation before the widget is enabled. (The method
  // InstantiateHandleRepresentation() is invoked by the vtkBiDimensionalWidget
  // for the purposes of cloning.)
  void SetHandleRepresentation(vtkHandleRepresentation *handle);
  void InstantiateHandleRepresentation();

  // Description:
  // Set/Get the handle representations used within the
  // vtkBiDimensionalRepresentation2D. (Note: properties can be set by
  // grabbing these representations and setting the properties
  // appropriately.)
  vtkGetObjectMacro(Point1Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point2Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point3Representation,vtkHandleRepresentation);
  vtkGetObjectMacro(Point4Representation,vtkHandleRepresentation);

  // Description:
  // Retrieve the property used to control the appearance of the two
  // orthogonal lines.
  vtkGetObjectMacro(LineProperty,vtkProperty2D);
  vtkGetObjectMacro(SelectedLineProperty,vtkProperty2D);

  // Description:
  // Retrieve the property used to control the appearance of the text
  // labels.
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  // Description:
  // The tolerance representing the distance to the representation (in
  // pixels) in which the cursor is considered near enough to the
  // representation to be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // Return the length of the line defined by (Point1,Point2). This is the
  // distance in the world coordinate system.
  virtual double GetLength1();

  // Description:
  // Return the length of the line defined by (Point3,Point4). This is the
  // distance in the world coordinate system.
  virtual double GetLength2();

  // Description:
  // Specify the format to use for labelling the distance. Note that an empty
  // string results in no label, or a format string without a "%" character
  // will not print the distance value.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

//BTX -- used to communicate about the state of the representation
  enum {Outside=0,NearP1,NearP2,NearP3,NearP4,OnL1Inner,OnL1Outer,OnL2Inner,OnL2Outer,OnCenter};
//ETX

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetDefinition(double e[2]);
  virtual void Point2WidgetInteraction(double e[2]);
  virtual void Point3WidgetInteraction(double e[2]);
  virtual void StartWidgetManipulation(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual void Highlight(int highlightOn);

  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);

  // Description:
  // Toggle whether to display the label above or below the widget.
  // Defaults to 1.
  vtkSetMacro(ShowLabelAboveWidget, int);
  vtkGetMacro(ShowLabelAboveWidget, int);
  vtkBooleanMacro(ShowLabelAboveWidget, int);

  // Description:
  // Set/get the id to display in the label.
  void SetID(unsigned long id);
  vtkGetMacro(ID, unsigned long);

  // Description:
  // Get the text shown in the widget's label.
  char* GetLabelText();

  // Description:
  // Get the position of the widget's label in display coordinates.
  double* GetLabelPosition();
  void GetLabelPosition(double pos[3]);
  void GetWorldLabelPosition(double pos[3]);

protected:
  vtkBiDimensionalRepresentation2D();
  ~vtkBiDimensionalRepresentation2D();

  // Keep track if modifier is set
  int Modifier;

  // The handle and the rep used to close the handles
  vtkHandleRepresentation *HandleRepresentation;
  vtkHandleRepresentation *Point1Representation;
  vtkHandleRepresentation *Point2Representation;
  vtkHandleRepresentation *Point3Representation;
  vtkHandleRepresentation *Point4Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Visibility of the lines
  int Line1Visibility;
  int Line2Visibility;

  // Geometry of the lines
  vtkCellArray        *LineCells;
  vtkPoints           *LinePoints;
  vtkPolyData         *LinePolyData;
  vtkPolyDataMapper2D *LineMapper;
  vtkActor2D          *LineActor;
  vtkProperty2D       *LineProperty;
  vtkProperty2D       *SelectedLineProperty;

  // The labels for the line lengths
  vtkTextProperty *TextProperty;
  vtkTextMapper   *TextMapper;
  vtkActor2D      *TextActor;

  unsigned long ID;
  int IDInitialized;

  // Internal variables
  double P1World[3];
  double P2World[3];
  double P3World[3];
  double P4World[3];
  double P21World[3];
  double P43World[3];
  double T21;
  double T43;
  double CenterWorld[3];
  double StartEventPositionWorld[4];

  // Format for printing the distance
  char *LabelFormat;

  // toggle to determine whether to place text above or below widget
  int ShowLabelAboveWidget;

  // Helper method
  void ProjectOrthogonalPoint(double x[4], double y[3], double x1[3], double x2[3], double x21[3], 
                              double dir, double xP[3]);

private:
  vtkBiDimensionalRepresentation2D(const vtkBiDimensionalRepresentation2D&);  //Not implemented
  void operator=(const vtkBiDimensionalRepresentation2D&);  //Not implemented
};

#endif
