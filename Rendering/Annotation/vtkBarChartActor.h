/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBarChartActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBarChartActor
 * @brief   create a bar chart from an array
 *
 * vtkBarChartActor generates a bar chart from an array of numbers defined in
 * field data (a vtkDataObject). To use this class, you must specify an input
 * data object. You'll probably also want to specify the position of the plot
 * be setting the Position and Position2 instance variables, which define a
 * rectangle in which the plot lies.  There are also many other instance
 * variables that control the look of the plot includes its title and legend.
 *
 * Set the text property/attributes of the title and the labels through the
 * vtkTextProperty objects associated with these components.
 *
 * @sa
 * vtkParallelCoordinatesActor vtkXYPlotActor vtkSpiderPlotActor
 * vtkPieChartActor
*/

#ifndef vtkBarChartActor_h
#define vtkBarChartActor_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkAxisActor2D;
class vtkDataObject;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextMapper;
class vtkTextProperty;
class vtkLegendBoxActor;
class vtkGlyphSource2D;
class vtkBarLabelArray;

class VTKRENDERINGANNOTATION_EXPORT vtkBarChartActor : public vtkActor2D
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkBarChartActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Instantiate this class.
   */
  static vtkBarChartActor *New();

  /**
   * Set the input to the bar chart actor.
   */
  virtual void SetInput(vtkDataObject*);

  //@{
  /**
   * Get the input data object to this actor.
   */
  vtkGetObjectMacro(Input,vtkDataObject);
  //@}

  //@{
  /**
   * Enable/Disable the display of a plot title.
   */
  vtkSetMacro(TitleVisibility, vtkTypeBool);
  vtkGetMacro(TitleVisibility, vtkTypeBool);
  vtkBooleanMacro(TitleVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the title of the bar chart.
   */
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title text property. The property controls the
   * appearance of the plot title.
   */
  virtual void SetTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Enable/Disable the display of bar labels.
   */
  vtkSetMacro(LabelVisibility, vtkTypeBool);
  vtkGetMacro(LabelVisibility, vtkTypeBool);
  vtkBooleanMacro(LabelVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the labels text property. This controls the appearance
   * of all bar bar labels.
   */
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Specify colors for each bar. If not specified, they are
   * automatically generated.
   */
  void SetBarColor(int i, double r, double g, double b);
  void SetBarColor(int i, const double color[3])
    { this->SetBarColor(i, color[0], color[1], color[2]); }
  double *GetBarColor(int i);
  //@}

  //@{
  /**
   * Specify the names of each bar. If
   * not specified, then an integer number is automatically generated.
   */
  void SetBarLabel(const int i, const char *);
  const char* GetBarLabel(int i);
  //@}

  //@{
  /**
   * Specify the title of the y-axis.
   */
  vtkSetStringMacro(YTitle);
  vtkGetStringMacro(YTitle);
  //@}

  //@{
  /**
   * Enable/Disable the creation of a legend. If on, the legend labels will
   * be created automatically unless the per plot legend symbol has been
   * set.
   */
  vtkSetMacro(LegendVisibility, vtkTypeBool);
  vtkGetMacro(LegendVisibility, vtkTypeBool);
  vtkBooleanMacro(LegendVisibility, vtkTypeBool);
  //@}

  //@{
  /**
   * Retrieve handles to the legend box. This is useful if you would like
   * to manually control the legend appearance.
   */
  vtkGetObjectMacro(LegendActor,vtkLegendBoxActor);
  //@}

  //@{
  /**
   * Draw the bar plot.
   */
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* ) override {return 0;}
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

protected:
  vtkBarChartActor();
  ~vtkBarChartActor() override;

private:
  vtkDataObject *Input;        // List of data sets to plot
  vtkIdType ArrayNumber;
  vtkIdType ComponentNumber;
  vtkTypeBool TitleVisibility;         // Should I see the title?
  char *Title;                 // The title string
  vtkTextProperty *TitleTextProperty;
  vtkTypeBool LabelVisibility;
  vtkTextProperty *LabelTextProperty;
  vtkBarLabelArray *Labels;
  vtkTypeBool LegendVisibility;
  vtkLegendBoxActor *LegendActor;
  vtkGlyphSource2D *GlyphSource;

  // Local variables needed to plot
  vtkIdType N;        // The number of values
  double   *Heights;  // The heights of each bar
  double    MinHeight; //The maximum and minimum height
  double    MaxHeight;
  double    LowerLeft[2];
  double    UpperRight[2];

  vtkTextMapper    **BarMappers; //a label for each bar
  vtkActor2D       **BarActors;

  vtkTextMapper    *TitleMapper;
  vtkActor2D       *TitleActor;

  vtkPolyData         *PlotData;    // The actual bars plus the x-axis
  vtkPolyDataMapper2D *PlotMapper;
  vtkActor2D          *PlotActor;

  vtkAxisActor2D *YAxis;  //The y-axis
  char           *YTitle;

  vtkTimeStamp  BuildTime;

  int    LastPosition[2];
  int    LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(vtkViewport *viewport, int *size);
  int BuildPlot(vtkViewport*);

private:
  vtkBarChartActor(const vtkBarChartActor&) = delete;
  void operator=(const vtkBarChartActor&) = delete;
};


#endif

