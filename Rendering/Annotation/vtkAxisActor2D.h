/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAxisActor2D - Create an axis with tick marks and labels
// .SECTION Description
// vtkAxisActor2D creates an axis with tick marks, labels, and/or a title,
// depending on the particular instance variable settings. vtkAxisActor2D is
// a 2D actor; that is, it is drawn on the overlay plane and is not
// occluded by 3D geometry. To use this class, you typically specify two
// points defining the start and end points of the line (x-y definition using
// vtkCoordinate class), the number of labels, and the data range
// (min,max). You can also control what parts of the axis are visible
// including the line, the tick marks, the labels, and the title.  You can
// also specify the label format (a printf style format).
//
// This class decides what font size to use and how to locate the labels. It
// also decides how to create reasonable tick marks and labels. The number
// of labels and the range of values may not match the number specified, but
// should be close.
//
// Labels are drawn on the "right" side of the axis. The "right" side is
// the side of the axis on the right as you move from Position to Position2.
// The way the labels and title line up with the axis and tick marks depends on
// whether the line is considered horizontal or vertical.
//
// The vtkActor2D instance variables Position and Position2 are instances of
// vtkCoordinate. Note that the Position2 is an absolute position in that
// class (it was by default relative to Position in vtkActor2D).
//
// What this means is that you can specify the axis in a variety of coordinate
// systems. Also, the axis does not have to be either horizontal or vertical.
// The tick marks are created so that they are perpendicular to the axis.
//
// Set the text property/attributes of the title and the labels through the
// vtkTextProperty objects associated to this actor.
//
// .SECTION See Also
// vtkCubeAxesActor2D can be used to create axes in world coordinate space.
//
// vtkActor2D vtkTextMapper vtkPolyDataMapper2D vtkScalarBarActor
// vtkCoordinate vtkTextProperty

#ifndef __vtkAxisActor2D_h
#define __vtkAxisActor2D_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkPolyDataMapper2D;
class vtkPolyData;
class vtkTextMapper;
class vtkTextProperty;

#define VTK_MAX_LABELS 25

class VTKRENDERINGANNOTATION_EXPORT vtkAxisActor2D : public vtkActor2D
{
public:
  vtkTypeMacro(vtkAxisActor2D,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object.
  static vtkAxisActor2D *New();

  // Description:
  // Specify the position of the first point defining the axis.
  // Note: backward compatibility only, use vtkActor2D's Position instead.
  virtual vtkCoordinate *GetPoint1Coordinate()
    { return this->GetPositionCoordinate(); };
  virtual void SetPoint1(double x[2]) { this->SetPosition(x); };
  virtual void SetPoint1(double x, double y) { this->SetPosition(x,y); };
  virtual double *GetPoint1() { return this->GetPosition(); };

  // Description:
  // Specify the position of the second point defining the axis. Note that
  // the order from Point1 to Point2 controls which side the tick marks
  // are drawn on (ticks are drawn on the right, if visible).
  // Note: backward compatibility only, use vtkActor2D's Position2 instead.
  virtual vtkCoordinate *GetPoint2Coordinate()
    { return this->GetPosition2Coordinate(); };
  virtual void SetPoint2(double x[2]) { this->SetPosition2(x); };
  virtual void SetPoint2(double x, double y) { this->SetPosition2(x,y); };
  virtual double *GetPoint2() { return this->GetPosition2(); };

  // Description:
  // Specify the (min,max) axis range. This will be used in the generation
  // of labels, if labels are visible.
  vtkSetVector2Macro(Range,double);
  vtkGetVectorMacro(Range,double,2);

  // Description:
  // Specify whether this axis should act like a measuring tape (or ruler) with
  // specified major tick spacing. If enabled, the distance between major ticks
  // is controlled by the RulerDistance ivar.
  vtkSetMacro(RulerMode,int);
  vtkGetMacro(RulerMode,int);
  vtkBooleanMacro(RulerMode,int);

  // Description:
  // Specify the RulerDistance which indicates the spacing of the major ticks.
  // This ivar only has effect when the RulerMode is on.
  vtkSetClampMacro(RulerDistance,double,0,VTK_LARGE_FLOAT);
  vtkGetMacro(RulerDistance,double);

  // Description:
  // Set/Get the number of annotation labels to show. This also controls the
  // number of major ticks shown. Note that this ivar only holds meaning if
  // the RulerMode is off.
  vtkSetClampMacro(NumberOfLabels, int, 2, VTK_MAX_LABELS);
  vtkGetMacro(NumberOfLabels, int);

  // Description:
  // Set/Get the format with which to print the labels on the scalar
  // bar.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Set/Get the flag that controls whether the labels and ticks are
  // adjusted for "nice" numerical values to make it easier to read
  // the labels. The adjustment is based in the Range instance variable.
  // Call GetAdjustedRange and GetAdjustedNumberOfLabels to get the adjusted
  // range and number of labels. Note that if RulerMode is on, then the
  // number of labels is a function of the range and ruler distance.
  vtkSetMacro(AdjustLabels, int);
  vtkGetMacro(AdjustLabels, int);
  vtkBooleanMacro(AdjustLabels, int);
  virtual double *GetAdjustedRange()
    {
      this->UpdateAdjustedRange();
      return this->AdjustedRange;
    }
  virtual void GetAdjustedRange(double &_arg1, double &_arg2)
    {
      this->UpdateAdjustedRange();
      _arg1 = this->AdjustedRange[0];
      _arg2 = this->AdjustedRange[1];
    };
  virtual void GetAdjustedRange(double _arg[2])
    {
      this->GetAdjustedRange(_arg[0], _arg[1]);
    }
  virtual int GetAdjustedNumberOfLabels()
    {
      this->UpdateAdjustedRange();
      return this->AdjustedNumberOfLabels;
    }

  // Description:
  // Set/Get the title of the scalar bar actor,
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  // Description:
  // Set/Get the title text property.
  virtual void SetTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);

  // Description:
  // Set/Get the labels text property.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);

  // Description:
  // Set/Get the length of the tick marks (expressed in pixels or display
  // coordinates).
  vtkSetClampMacro(TickLength, int, 0, 100);
  vtkGetMacro(TickLength, int);

  // Description:
  // Number of minor ticks to be displayed between each tick. Default
  // is 0.
  vtkSetClampMacro(NumberOfMinorTicks, int, 0, 20);
  vtkGetMacro(NumberOfMinorTicks, int);

  // Description:
  // Set/Get the length of the minor tick marks (expressed in pixels or
  // display coordinates).
  vtkSetClampMacro(MinorTickLength, int, 0, 100);
  vtkGetMacro(MinorTickLength, int);

  // Description:
  // Set/Get the offset of the labels (expressed in pixels or display
  // coordinates). The offset is the distance of labels from tick marks
  // or other objects.
  vtkSetClampMacro(TickOffset, int, 0, 100);
  vtkGetMacro(TickOffset, int);

  // Description:
  // Set/Get visibility of the axis line.
  vtkSetMacro(AxisVisibility, int);
  vtkGetMacro(AxisVisibility, int);
  vtkBooleanMacro(AxisVisibility, int);

  // Description:
  // Set/Get visibility of the axis tick marks.
  vtkSetMacro(TickVisibility, int);
  vtkGetMacro(TickVisibility, int);
  vtkBooleanMacro(TickVisibility, int);

  // Description:
  // Set/Get visibility of the axis labels.
  vtkSetMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkBooleanMacro(LabelVisibility, int);

  // Description:
  // Set/Get visibility of the axis title.
  vtkSetMacro(TitleVisibility, int);
  vtkGetMacro(TitleVisibility, int);
  vtkBooleanMacro(TitleVisibility, int);

  // Description:
  // Set/Get position of the axis title. 0 is at the start of the
  // axis whereas 1 is at the end.
  vtkSetMacro(TitlePosition, double);
  vtkGetMacro(TitlePosition, double);

  // Description:
  // Set/Get the factor that controls the overall size of the fonts used
  // to label and title the axes. This ivar used in conjunction with
  // the LabelFactor can be used to control font sizes.
  vtkSetClampMacro(FontFactor, double, 0.1, 2.0);
  vtkGetMacro(FontFactor, double);

  // Description:
  // Set/Get the factor that controls the relative size of the axis labels
  // to the axis title.
  vtkSetClampMacro(LabelFactor, double, 0.1, 2.0);
  vtkGetMacro(LabelFactor, double);

  // Description:
  // Draw the axis.
  int RenderOverlay(vtkViewport* viewport);
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *) {return 0;}

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This method computes the range of the axis given an input range.
  // It also computes the number of tick marks given a suggested number.
  // (The number of tick marks includes end ticks as well.)
  // The number of tick marks computed (in conjunction with the output
  // range) will yield "nice" tick values. For example, if the input range
  // is (0.25,96.7) and the number of ticks requested is 10, the output range
  // will be (0,100) with the number of computed ticks to 11 to yield tick
  // values of (0,10,20,...,100).
  static void ComputeRange(double inRange[2],
                           double outRange[2],
                           int inNumTicks,
                           int &outNumTicks,
                           double &interval);

  // Description:
  // General method to computes font size from a representative size on the
  // viewport (given by size[2]). The method returns the font size (in points)
  // and the string height/width (in pixels). It also sets the font size of the
  // instance of vtkTextMapper provided. The factor is used when you're trying
  // to create text of different size-factor (it is usually = 1 but you can
  // adjust the font size by making factor larger or smaller).
  static int SetMultipleFontSize(vtkViewport *viewport,
                                 vtkTextMapper **textMappers,
                                 int nbOfMappers,
                                 int *targetSize,
                                 double factor,
                                 int *stringSize);

  // Description:
  // Specify whether to size the fonts relative to the viewport or relative to
  // length of the axis. By default, fonts are resized relative to the axis.
  vtkSetMacro(SizeFontRelativeToAxis,int);
  vtkGetMacro(SizeFontRelativeToAxis,int);
  vtkBooleanMacro(SizeFontRelativeToAxis,int);

  // Description:
  // Shallow copy of an axis actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkAxisActor2D();
  ~vtkAxisActor2D();

  vtkTextProperty *TitleTextProperty;
  vtkTextProperty *LabelTextProperty;

  char  *Title;
  double Range[2];
  double TitlePosition;
  int    RulerMode;
  double RulerDistance;
  int   NumberOfLabels;
  char  *LabelFormat;
  int   AdjustLabels;
  double FontFactor;
  double LabelFactor;
  int   TickLength;
  int   MinorTickLength;
  int   TickOffset;
  int NumberOfMinorTicks;

  double AdjustedRange[2];
  int   AdjustedNumberOfLabels;
  int   NumberOfLabelsBuilt;

  int   AxisVisibility;
  int   TickVisibility;
  int   LabelVisibility;
  int   TitleVisibility;

  int   LastPosition[2];
  int   LastPosition2[2];

  int   LastSize[2];
  int   LastMaxLabelSize[2];

  int  SizeFontRelativeToAxis;

  virtual void BuildAxis(vtkViewport *viewport);
  static double ComputeStringOffset(double width, double height, double theta);
  static void SetOffsetPosition(double xTick[3], double theta,
                                int stringHeight, int stringWidth,
                                int offset, vtkActor2D *actor);
  virtual void UpdateAdjustedRange();

  vtkTextMapper *TitleMapper;
  vtkActor2D    *TitleActor;

  vtkTextMapper **LabelMappers;
  vtkActor2D    **LabelActors;

  vtkPolyData         *Axis;
  vtkPolyDataMapper2D *AxisMapper;
  vtkActor2D          *AxisActor;

  vtkTimeStamp  AdjustedRangeBuildTime;
  vtkTimeStamp  BuildTime;

private:
  vtkAxisActor2D(const vtkAxisActor2D&);  // Not implemented.
  void operator=(const vtkAxisActor2D&);  // Not implemented.
};


#endif
