/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpiderPlotActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpiderPlotActor - create a spider plot from input field
// .SECTION Description
// vtkSpiderPlotActor generates a spider plot from an input field (i.e.,
// vtkDataObject). A spider plot represents N-dimensional data by using a set
// of N axes that originate from the center of a circle, and form the spokes
// of a wheel (like a spider web).  Each N-dimensional point is plotted as a
// polyline that forms a closed polygon; the vertices of the polygon
// are plotted against the radial axes.
//
// To use this class, you must specify an input data object. You'll probably
// also want to specify the position of the plot be setting the Position and
// Position2 instance variables, which define a rectangle in which the plot
// lies. Another important parameter is the IndependentVariables ivar, which
// tells the instance how to interpret the field data (independent variables
// as the rows or columns of the field). There are also many other instance
// variables that control the look of the plot includes its title and legend.
//
// Set the text property/attributes of the title and the labels through the
// vtkTextProperty objects associated with these components.

// .SECTION Caveats
// Field data is not necessarily "rectangular" in shape. In these cases, some
// of the data may not be plotted.
//
// Field data can contain non-numeric arrays (i.e. arrays not subclasses of
// vtkDataArray). Such arrays are skipped.
//
// .SECTION See Also
// vtkParallelCoordinatesActor vtkXYPlotActor2D

#ifndef vtkSpiderPlotActor_h
#define vtkSpiderPlotActor_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkAlgorithmOutput;
class vtkAxisActor2D;
class vtkDataObject;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextMapper;
class vtkTextProperty;
class vtkLegendBoxActor;
class vtkGlyphSource2D;
class vtkAxisLabelArray;
class vtkAxisRanges;
class vtkSpiderPlotActorConnection;


#define VTK_IV_COLUMN 0
#define VTK_IV_ROW    1

class VTKRENDERINGANNOTATION_EXPORT vtkSpiderPlotActor : public vtkActor2D
{
public:
  // Description:
  // Standard methods for type information and printing.
  vtkTypeMacro(vtkSpiderPlotActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate this class.
  static vtkSpiderPlotActor *New();

  // Description:
  // Set the input to the pie chart actor. SetInputData()
  // does not connect the pipeline whereas SetInputConnection()
  // does.
  virtual void SetInputData(vtkDataObject*);
  virtual void SetInputConnection(vtkAlgorithmOutput*);

  // Description:
  // Get the input data object to this actor.
  virtual vtkDataObject* GetInput();

  // Description:
  // Specify whether to use the rows or columns as independent variables.
  // If columns, then each row represents a separate point. If rows, then
  // each column represents a separate point.
  vtkSetClampMacro(IndependentVariables,int,VTK_IV_COLUMN, VTK_IV_ROW);
  vtkGetMacro(IndependentVariables,int);
  void SetIndependentVariablesToColumns()
    {this->SetIndependentVariables(VTK_IV_COLUMN);};
  void SetIndependentVariablesToRows()
    {this->SetIndependentVariables(VTK_IV_ROW);};

  // Description:
  // Enable/Disable the display of a plot title.
  vtkSetMacro(TitleVisibility, int);
  vtkGetMacro(TitleVisibility, int);
  vtkBooleanMacro(TitleVisibility, int);

  // Description:
  // Set/Get the title of the spider plot.
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  // Description:
  // Set/Get the title text property.
  virtual void SetTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);

  // Enable/Disable the display axes titles. These are arranged on the end
  // of each radial axis on the circumference of the spider plot. The label
  // text strings are derived from the names of the data object arrays
  // associated with the input.
  vtkSetMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkBooleanMacro(LabelVisibility, int);

  // Description:
  // Enable/Disable the creation of a legend. If on, the legend labels will
  // be created automatically unless the per plot legend symbol has been
  // set.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);

  // Description:
  // Specify the number of circumferential rings. If set to zero, then
  // none will be shown; otherwise the specified number will be shown.
  vtkSetClampMacro(NumberOfRings,int,0,VTK_INT_MAX);
  vtkGetMacro(NumberOfRings,int);

  // Description:
  // Specify the names of the radial spokes (i.e., the radial axes). If
  // not specified, then an integer number is automatically generated.
  void SetAxisLabel(const int i, const char *);
  const char* GetAxisLabel(int i);

  // Description:
  // Specify the range of data on each radial axis. If not specified,
  // then the range is computed automatically.
  void SetAxisRange(int i, double min, double max);
  void SetAxisRange(int i, double range[2]);
  void GetAxisRange(int i, double range[2]);

  // Description:
  // Specify colors for each plot. If not specified, they are automatically generated.
  void SetPlotColor(int i, double r, double g, double b);
  void SetPlotColor(int i, const double color[3])
    { this->SetPlotColor(i, color[0], color[1], color[2]); }
  double *GetPlotColor(int i);

  // Description:
  // Enable/Disable the creation of a legend. If on, the legend labels will
  // be created automatically unless the per plot legend symbol has been
  // set.
  vtkSetMacro(LegendVisibility, int);
  vtkGetMacro(LegendVisibility, int);
  vtkBooleanMacro(LegendVisibility, int);

  // Description:
  // Retrieve handles to the legend box. This is useful if you would like
  // to manually control the legend appearance.
  vtkGetObjectMacro(LegendActor,vtkLegendBoxActor);

  // Description:
  // Draw the spider plot.
  int RenderOverlay(vtkViewport*);
  int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* ) {return 0;}

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkSpiderPlotActor();
  ~vtkSpiderPlotActor();

private:

  vtkSpiderPlotActorConnection* ConnectionHolder;

  int IndependentVariables;    // Use column or row
  int TitleVisibility;         // Should I see the title?
  char *Title;                 // The title string
  vtkTextProperty *TitleTextProperty;
  int LabelVisibility;
  vtkTextProperty *LabelTextProperty;
  vtkAxisLabelArray *Labels;
  int LegendVisibility;
  vtkLegendBoxActor *LegendActor;
  vtkGlyphSource2D *GlyphSource;
  int NumberOfRings;
  int AutomaticRangeComputation;

  // Local variables needed to plot
  vtkIdType N;                 // The number of independent variables
  double *Mins;                // Minimum data value along this row/column
  double *Maxs;                // Maximum data value along this row/column
  vtkAxisRanges *Ranges;

  vtkTextMapper    **LabelMappers; //a label for each radial spoke
  vtkActor2D       **LabelActors;

  vtkTextMapper    *TitleMapper;
  vtkActor2D       *TitleActor;

  vtkPolyData         *WebData;    // The web of the spider plot
  vtkPolyDataMapper2D *WebMapper;
  vtkActor2D          *WebActor;

  vtkPolyData         *PlotData;    // The lines drawn within the axes
  vtkPolyDataMapper2D *PlotMapper;
  vtkActor2D          *PlotActor;

  vtkTimeStamp  BuildTime;

  double Center[3];
  double Radius;
  double Theta;

  int   LastPosition[2];
  int   LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(vtkViewport *viewport, int *size);
  int BuildPlot(vtkViewport*);

private:
  vtkSpiderPlotActor(const vtkSpiderPlotActor&);  // Not implemented.
  void operator=(const vtkSpiderPlotActor&);  // Not implemented.
};


#endif

