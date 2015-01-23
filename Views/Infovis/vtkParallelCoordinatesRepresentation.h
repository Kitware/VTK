/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkParallelCoordinatesRepresentation - Data representation that
//  takes generic multivariate data and produces a parallel coordinates plot.
//
// .SECTION Description
//  A parallel coordinates plot represents each variable in a multivariate
//  data set as a separate axis.  Individual samples of that data set are
//  represented as a polyline that pass through each variable axis at
//  positions that correspond to data values.  vtkParallelCoordinatesRepresentation
//  generates this plot when added to a vtkParallelCoordinatesView, which handles
//  interaction and highlighting.  Sample polylines can alternatively
//  be represented as s-curves by enabling the UseCurves flag.
//
//  There are three selection modes: lasso, angle, and function. Lasso selection
//  picks sample lines that pass through a polyline.  Angle selection picks sample
//  lines that have similar slope to a line segment.  Function selection picks
//  sample lines that are near a linear function defined on two variables.  This
//  function specified by passing two (x,y) variable value pairs.
//
//  All primitives are plotted in normalized view coordinates [0,1].
//
// .SECTION See Also
//  vtkParallelCoordinatesView vtkParallelCoordinatesHistogramRepresentation
//  vtkSCurveSpline
//
// .SECTION Thanks
//  Developed by David Feng at Sandia National Laboratories

#ifndef vtkParallelCoordinatesRepresentation_h
#define vtkParallelCoordinatesRepresentation_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkRenderedRepresentation.h"

class vtkActor;
class vtkActor2D;
class vtkArrayData;
class vtkAxisActor2D;
class vtkBivariateLinearTableThreshold;
class vtkCollection;
class vtkCoordinate;
class vtkExtractSelectedPolyDataIds;
class vtkFieldData;
class vtkDataArray;
class vtkDataObject;
class vtkDoubleArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkIntArray;
class vtkLookupTable;
class vtkOutlineCornerSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkPropCollection;
class vtkSelection;
class vtkSelectionNode;
class vtkTextMapper;
class vtkTimeStamp;
class vtkUnsignedIntArray;
class vtkViewport;
class vtkWindow;

class VTKVIEWSINFOVIS_EXPORT vtkParallelCoordinatesRepresentation : public vtkRenderedRepresentation
{
public:
  static vtkParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkParallelCoordinatesRepresentation, vtkRenderedRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the theme to this view.  CellColor is used for line coloring
  // and titles.  EdgeLabelColor is used for axis color. CellOpacity is
  // used for line opacity.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Returns the hover text at an x,y location.
  virtual const char* GetHoverText(vtkView* view, int x, int y);

  // Description:
  // Change the position of the plot
  int SetPositionAndSize(double* position, double* size);
  int GetPositionAndSize(double* position, double* size);

  // Description:
  // Set/Get the axis titles
  void SetAxisTitles(vtkStringArray*);
  void SetAxisTitles(vtkAlgorithmOutput*);

  // Description:
  // Set the title for the entire plot
  void SetPlotTitle(const char*);

  // Description:
  // Get the number of axes in the plot
  vtkGetMacro(NumberOfAxes,int);

  //Description:
  // Get the number of samples in the plot
  vtkGetMacro(NumberOfSamples,int);

  // Description:
  // Set/Get the number of labels to display on each axis
  void SetNumberOfAxisLabels(int num);
  vtkGetMacro(NumberOfAxisLabels,int);

  // Description:
  // Move an axis to a particular screen position.  Using these
  // methods requires an Update() before they will work properly.
  virtual int SwapAxisPositions(int position1, int position2);
  int SetXCoordinateOfPosition(int position, double xcoord);
  double GetXCoordinateOfPosition(int axis);
  void GetXCoordinatesOfPositions(double* coords);
  int GetPositionNearXCoordinate(double xcoord);

  // Description:
  // Whether or not to display using curves
  vtkSetMacro(UseCurves,int);
  vtkGetMacro(UseCurves,int);
  vtkBooleanMacro(UseCurves,int);

  // Description:
  // Resolution of the curves displayed, enabled by setting UseCurves
  vtkSetMacro(CurveResolution,int);
  vtkGetMacro(CurveResolution,int);

  // Description:
  // Access plot properties
  vtkGetMacro(LineOpacity,double)
  vtkGetMacro(FontSize,double);
  vtkGetVector3Macro(LineColor,double);
  vtkGetVector3Macro(AxisColor,double);
  vtkGetVector3Macro(AxisLabelColor,double);
  vtkSetMacro(LineOpacity,double);
  vtkSetMacro(FontSize,double);
  vtkSetVector3Macro(LineColor,double);
  vtkSetVector3Macro(AxisColor,double);
  vtkSetVector3Macro(AxisLabelColor,double);

  // Description:
  // Maximum angle difference (in degrees) of selection using angle/function brushes
  vtkSetMacro(AngleBrushThreshold,double);
  vtkGetMacro(AngleBrushThreshold,double);

  // Description:
  // Maximum angle difference (in degrees) of selection using angle/function brushes
  vtkSetMacro(FunctionBrushThreshold,double);
  vtkGetMacro(FunctionBrushThreshold,double);

  // Description:
  // Set/get the value range of the axis at a particular screen position
  int GetRangeAtPosition(int position, double range[2]);
  virtual int SetRangeAtPosition(int position, double range[2]);

  // Description:
  // Reset the axes to their default positions and orders
  void ResetAxes();

  // Description:
  // Do a selection of the lines.  See the main description for how to use these functions.
  // RangeSelect is currently stubbed out.
  virtual void LassoSelect(int brushClass, int brushOperator, vtkPoints* brushPoints);
  virtual void AngleSelect(int brushClass, int brushOperator, double *p1, double *p2);
  virtual void FunctionSelect(int brushClass, int brushOperator, double *p1, double *p2, double *q1, double *q2);
  virtual void RangeSelect(int brushClass, int brushOperator, double *p1, double *p2);

//BTX
  enum InputPorts
  {
    INPUT_DATA=0,
    INPUT_TITLES,
    NUM_INPUT_PORTS
  };
//ETX

protected:
  vtkParallelCoordinatesRepresentation();
  virtual ~vtkParallelCoordinatesRepresentation();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  // Description:
  // Add/remove the props and actors to/from a view
  virtual bool AddToView(vtkView* view);
  virtual bool RemoveFromView(vtkView* view);
  virtual void PrepareForRendering(vtkRenderView* view);

  // Description:
  // This function is not actually used, but as left as a stub in case
  // it becomes useful at some point.
  void UpdateHoverHighlight(vtkView* view, int x, int y);

  // Description:
  // Allocate the cells/points/scalars for a vtkPolyData
  virtual int AllocatePolyData(vtkPolyData* polyData,
                               int numLines,
                               int numPointsPerLine,
                               int numStrips,
                               int numPointsPerStrip,
                               int numQuads,
                               int numPoints,
                               int numCellScalars,
                               int numPointScalars);

  // Description:
  // Put the axis actors in their correct positions.
  int PlaceAxes();

  // Description:
  // Place line primitives into a vtkPolyData from the input data.  idsToPlot
  // is a list of which rows/samples should be plotted.  If NULL, all
  // rows/samples are plotted.
  virtual int PlaceLines(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot);
  virtual int PlaceCurves(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot);

  // Description:
  // Takes the selection list (assumed to be a vtkIdTypeArray) from a vtkSelectionNode
  // and plots lines/curves into polyData for just those row/sample ids.
  virtual int PlaceSelection(vtkPolyData* polyData, vtkTable* data, vtkSelectionNode* selectionNode);

  // Description:
  // Compute the number of axes and their individual ranges
  virtual int ComputeDataProperties();

  // Description:
  // Set plot actor properties (line thickness, opacity, etc)
  virtual int UpdatePlotProperties(vtkStringArray* inputTitles);

  // Description:
  // Delete and reallocate the internals, resetting to default values
  virtual int ReallocateInternals();

  // Description:
  // Compute which screen position a point belongs to (returns the left position)
  int ComputePointPosition(double* p);
  int ComputeLinePosition(double* p1, double* p2);

  // Description:
  // Select a set of points using the prescribed operator (add, subtract, etc.) and class
  virtual void SelectRows(vtkIdType brushClass, vtkIdType brushOperator, vtkIdTypeArray* rowIds);
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);
  virtual void BuildInverseSelection();
  virtual vtkPolyDataMapper2D* InitializePlotMapper(vtkPolyData* input, vtkActor2D* actor, bool forceStandard=false);

  // Description:
  // Build an s-curve passing through (0,0) and (1,1) with a specified number of
  // values.  This is used as a lookup table when plotting curved primitives.
  void BuildDefaultSCurve(vtkDoubleArray* array, int numValues);

  // Description:
  // same as public version, but assumes that the brushpoints coming in
  // are all within two neighboring axes.
  virtual void LassoSelectInternal(vtkPoints* brushPoints, vtkIdTypeArray* outIds);

  // Description:
  // todo
  virtual void UpdateSelectionActors();

  vtkPolyDataMapper2D* GetSelectionMapper(int idx);
  int GetNumberOfSelections();


  //BTX
  vtkSmartPointer<vtkPolyData>         PlotData;
  vtkSmartPointer<vtkPolyDataMapper2D> PlotMapper;
  vtkSmartPointer<vtkActor2D>          PlotActor;
  vtkSmartPointer<vtkTextMapper> PlotTitleMapper;
  vtkSmartPointer<vtkActor2D> PlotTitleActor;
  vtkSmartPointer<vtkTextMapper> FunctionTextMapper;
  vtkSmartPointer<vtkActor2D> FunctionTextActor;

  vtkSmartPointer<vtkSelection> InverseSelection;
  vtkSmartPointer<vtkBivariateLinearTableThreshold> LinearThreshold;

  class Internals;
  Internals* I;
  //ETX

  int NumberOfAxes;
  int NumberOfAxisLabels;
  int NumberOfSamples;
  double YMin;
  double YMax;

  int CurveResolution;
  int UseCurves;
  double AngleBrushThreshold;
  double FunctionBrushThreshold;
  double SwapThreshold;

  // Indexed by screen position
  double* Xs;
  double* Mins;
  double* Maxs;
  double* MinOffsets;
  double* MaxOffsets;

  //BTX
  vtkSmartPointer<vtkAxisActor2D>* Axes;
  vtkSmartPointer<vtkTable> InputArrayTable;
  vtkSmartPointer<vtkStringArray> AxisTitles;
  //ETX

  vtkTimeStamp BuildTime;

  double LineOpacity;
  double FontSize;
  double LineColor[3];
  double AxisColor[3];
  double AxisLabelColor[3];

  vtkGetStringMacro(InternalHoverText);
  vtkSetStringMacro(InternalHoverText);
  char* InternalHoverText;

private:
  vtkParallelCoordinatesRepresentation(const vtkParallelCoordinatesRepresentation&); // Not implemented
  void operator=(const vtkParallelCoordinatesRepresentation&);   // Not implemented
};

#endif

