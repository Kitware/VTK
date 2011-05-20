/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxis.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkAxis - takes care of drawing 2D axes
//
// .SECTION Description
// The vtkAxis is drawn in screen coordinates. It is usually one of the last
// elements of a chart to be drawn. It renders the axis label, tick marks and
// tick labels.

#ifndef __vtkAxis_h
#define __vtkAxis_h

#include "vtkContextItem.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkVector.h"       // For position variables
#include "vtkRect.h"         // For bounding rect
#include "vtkStdString.h"    // For vtkStdString ivars

class vtkContext2D;
class vtkPen;
class vtkFloatArray;
class vtkDoubleArray;
class vtkStringArray;
class vtkTextProperty;

class VTK_CHARTS_EXPORT vtkAxis : public vtkContextItem
{
public:
  vtkTypeMacro(vtkAxis, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Enumeration of the axis locations in a conventional XY chart. Other
  // layouts are possible.
  enum {
    LEFT = 0,
    BOTTOM,
    RIGHT,
    TOP,
    PARALLEL
  };

  // Description:
  // Creates a 2D Chart object.
  static vtkAxis *New();

  // Description:
  // Get/set the position of the axis (LEFT, BOTTOM, RIGHT, TOP, PARALLEL).
  virtual void SetPosition(int position);
  vtkGetMacro(Position, int);

  // Description:
  // Set point 1 of the axis (in pixels), this is usually the origin.
  void SetPoint1(const vtkVector2f& pos);
  void SetPoint1(float x, float y);

  // Description:
  // Get point 1 of the axis (in pixels), this is usually the origin.
  vtkGetVector2Macro(Point1, float);
  vtkVector2f GetPosition1();

  // Description:
  // Set point 2 of the axis (in pixels), this is usually the terminus.
  void SetPoint2(const vtkVector2f& pos);
  void SetPoint2(float x, float y);

  // Description:
  // Get point 2 of the axis (in pixels), this is usually the terminus.
  vtkGetVector2Macro(Point2, float);
  vtkVector2f GetPosition2();

  // Description:
  // Set the number of tick marks for this axis. Default is -1, which leads to
  // automatic calculation of nicely spaced tick marks.
  vtkSetMacro(NumberOfTicks, int);

  // Description:
  // Get the number of tick marks for this axis.
  vtkGetMacro(NumberOfTicks, int);

  // Description:
  // Get the vtkTextProperty that governs how the axis lables are displayed.
  // Note that the alignment properties are not used.
  vtkGetObjectMacro(LabelProperties, vtkTextProperty);

  // Description:
  // Set the logical minimum value of the axis, in plot coordinates.
  virtual void SetMinimum(double minimum);

  // Description:
  // Get the logical minimum value of the axis, in plot coordinates.
  vtkGetMacro(Minimum, double);

  // Description:
  // Set the logical maximum value of the axis, in plot coordinates.
  virtual void SetMaximum(double maximum);

  // Description:
  // Get the logical maximum value of the axis, in plot coordinates.
  vtkGetMacro(Maximum, double);

  // Description:
  // Get the logical range of the axis, in plot coordinates.
  virtual void SetRange(double minimum, double maximum);

  // Description:
  // Set the logical lowest possible value for \a Minimum, in plot coordinates.
  virtual void SetMinimumLimit(double lowest);

  // Description:
  // Get the logical lowest possible value for \a Minimum, in plot coordinates.
  vtkGetMacro(MinimumLimit, double);

  // Description:
  // Set the logical highest possible value for \a Maximum, in plot coordinates.
  virtual void SetMaximumLimit(double highest);

  // Description:
  // Get the logical highest possible value for \a Maximum, in plot coordinates.
  vtkGetMacro(MaximumLimit, double);

  // Description:
  // Get/set the title text of the axis.
  virtual void SetTitle(const vtkStdString &title);
  virtual vtkStdString GetTitle();

  // Description:
  // Get the vtkTextProperty that governs how the axis title is displayed.
  vtkGetObjectMacro(TitleProperties, vtkTextProperty);

  // Description:
  // Get/set whether the axis should use a log scale, default is false.
  vtkSetMacro(LogScale, bool);
  vtkGetMacro(LogScale, bool);

  // Description:
  // Get/set whether the axis grid lines should be drawn, default is true.
  vtkSetMacro(GridVisible, bool);
  vtkGetMacro(GridVisible, bool);

  // Description:
  // Get/set whether the axis labels should be visible.
  vtkSetMacro(LabelsVisible, bool);
  vtkGetMacro(LabelsVisible, bool);

  // Description:
  // Get/set the numerical precision to use, default is 2.
  virtual void SetPrecision(int precision);
  vtkGetMacro(Precision, int);

  // Description:
  // Enumeration of the axis notations available.
  enum {
    STANDARD = 0,
    SCIENTIFIC,
    MIXED
  };

  // Description:
  // Get/set the numerical notation, standard, scientific or mixed (0, 1, 2).
  virtual void SetNotation(int notation);
  vtkGetMacro(Notation, int);

  // Description:
  // Enumeration of the axis behaviors.
  enum {
    AUTO = 0,
    FIXED,
    CUSTOM
  };

  // Description:
  // Get/set the behavior of the axis (auto, fixed, custom). Default is 0 (auto).
  vtkSetMacro(Behavior, int);
  vtkGetMacro(Behavior, int);

  // Description:
  // Get a pointer to the vtkPen object that controls the way this axis is drawn.
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Get a pointer to the vtkPen object that controls the way this axis is drawn.
  vtkGetObjectMacro(GridPen, vtkPen);

  // Description:
  // Update the geometry of the axis. Takes care of setting up the tick mark
  // locations etc. Should be called by the scene before rendering.
  virtual void Update();

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Use this function to autoscale the axes after setting the minimum and
  // maximum values. This will cause the axes to select the nicest numbers
  // that enclose the minimum and maximum values, and to select an appropriate
  // number of tick marks.
  virtual void AutoScale();

  // Description:
  // Recalculate the spacing of the tick marks - typically useful to do after
  // scaling the axis.
  virtual void RecalculateTickSpacing();

  // Description:
  // An array with the positions of the tick marks along the axis line.
  // The positions are specified in the plot coordinates of the axis.
  virtual vtkDoubleArray* GetTickPositions();

  // Description:
  // Set the tick positions (in plot coordinates).
  virtual void SetTickPositions(vtkDoubleArray* positions);

  // Description:
  // An array with the positions of the tick marks along the axis line.
  // The positions are specified in scene coordinates.
  virtual vtkFloatArray* GetTickScenePositions();

  // Description:
  // A string array containing the tick labels for the axis.
  virtual vtkStringArray* GetTickLabels();

  // Description:
  // Set the tick labels for the axis.
  virtual void SetTickLabels(vtkStringArray* labels);

  // Description:
  // Request the space the axes require to be drawn. This is returned as a
  // vtkRectf, with the corner being the offset from Point1, and the width/
  // height being the total width/height required by the axis. In order to
  // ensure the numbers are correct, Update() should be called on the axis.
  vtkRectf GetBoundingRect(vtkContext2D* painter);

//BTX
protected:
  vtkAxis();
  ~vtkAxis();

  // Description:
  // Calculate and assign nice labels/logical label positions.
  void GenerateTickLabels(double min, double max);

  // Description:
  // Generate tick labels from the supplied double array of tick positions.
  void GenerateTickLabels();

  // Description:
  // Calculate the next "nicest" numbers above and below the current minimum.
  // \return the "nice" spacing of the numbers.
  double CalculateNiceMinMax(double &min, double &max);

  // Description:
  // Return a "nice number", often defined as 1, 2 or 5. If roundUp is true then
  // the nice number will be rounded up, false it is rounded down. The supplied
  // number should be between 0.0 and 9.9.
  double NiceNumber(double number, bool roundUp);

  // Description:
  // Return a tick mark for a logarithmic axis.
  // If roundUp is true then the upper tick mark is returned.
  // Otherwise the lower tick mark is returned.
  // Tick marks will be: ... 0.1 0.2 .. 0.9 1 2 .. 9 10 20 .. 90 100 ...
  // Parameter nicevalue will be set to true if tick mark is in:
  // ... 0.1 0.2 0.5 1 2 5 10 20 50 100 ...
  // Parameter order is set to the detected order of magnitude of the number.
  double LogScaleTickMark(double number,
                          bool roundUp,
                          bool &niceValue,
                          int &order);

  // Description:
  // Generate tick marks for logarithmic scale for specific order of magnitude.
  // Mark generation is limited by parameters min and max.
  // Tick marks will be: ... 0.1 0.2 .. 0.9 1 2 .. 9 10 20 .. 90 100 ...
  // Tick labels will be: ... 0.1 0.2 0.5 1 2 5 10 20 50 100 ...
  // If Parameter detaillabels is disabled tick labels will be:
  // ... 0.1 1 10 100 ...
  // If min/max is not in between 1.0 and 9.0 defaults will be used.
  // If min and max do not differ 1 defaults will be used.
  void GenerateLogScaleTickMarks(int order,
                                 double min = 1.0,
                                 double max = 9.0,
                                 bool detailLabels = true);

  int Position;        // The position of the axis (LEFT, BOTTOM, RIGHT, TOP)
  float *Point1;       // The position of point 1 (usually the origin)
  float *Point2;       // The position of point 2 (usually the terminus)
  vtkVector2f Position1, Position2;
  double TickInterval; // Interval between tick marks in plot space
  int NumberOfTicks;   // The number of tick marks to draw
  vtkTextProperty* LabelProperties; // Text properties for the labels.
  double Minimum;      // Minimum value of the axis
  double Maximum;      // Maximum values of the axis
  double MinimumLimit; // Lowest possible value for Minimum
  double MaximumLimit; // Highest possible value for Maximum
  vtkStdString Title;  // The text label drawn on the axis
  vtkTextProperty* TitleProperties; // Text properties for the axis title
  bool LogScale;       // Should the axis use a log scale
  bool GridVisible;    // Whether the grid for the axis should be drawn
  bool LabelsVisible;  // Should the axis labels be visible
  int Precision;       // Numerical precision to use, defaults to 2.
  int Notation;        // The notation to use (standard, scientific, mixed)
  int Behavior;        // The behaviour of the axis (auto, fixed, custom).
  float MaxLabel[2];   // The widest/tallest axis label.

  // Description:
  // This object stores the vtkPen that controls how the axis is drawn.
  vtkPen* Pen;

  // Description:
  // This object stores the vtkPen that controls how the grid lines are drawn.
  vtkPen* GridPen;

  // Description:
  // Position of tick marks in screen coordinates
  vtkSmartPointer<vtkDoubleArray> TickPositions;

  // Description:
  // Position of tick marks in screen coordinates
  vtkSmartPointer<vtkFloatArray> TickScenePositions;

  // Description:
  // The labels for the tick marks
  vtkSmartPointer<vtkStringArray> TickLabels;

  // Description:
  // Hint as to whether a nice min/max was set, otherwise labels may not be
  // present at the top/bottom of the axis.
  bool UsingNiceMinMax;

  // Description:
  // Mark the tick labels as dirty when the min/max value is changed.
  bool TickMarksDirty;

  // Description:
  // Flag to indicate that the axis has been resized.
  bool Resized;

  // Description:
  // Hint as to whether a logarithmic scale is reasonable or not.
  bool LogScaleReasonable;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

private:
  vtkAxis(const vtkAxis &); // Not implemented.
  void operator=(const vtkAxis &);   // Not implemented.
//ETX
};

#endif //__vtkAxis_h
