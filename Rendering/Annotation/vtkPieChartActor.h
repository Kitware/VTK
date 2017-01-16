/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieChartActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPieChartActor
 * @brief   create a pie chart from an array
 *
 * vtkPieChartActor generates a pie chart from an array of numbers defined in
 * field data (a vtkDataObject). To use this class, you must specify an input
 * data object. You'll probably also want to specify the position of the plot
 * be setting the Position and Position2 instance variables, which define a
 * rectangle in which the plot lies.  There are also many other instance
 * variables that control the look of the plot includes its title,
 * and legend.
 *
 * Set the text property/attributes of the title and the labels through the
 * vtkTextProperty objects associated with these components.
 *
 * @sa
 * vtkParallelCoordinatesActor vtkXYPlotActor2D vtkSpiderPlotActor
*/

#ifndef vtkPieChartActor_h
#define vtkPieChartActor_h

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
class vtkPieChartActorConnection;
class vtkPieceLabelArray;

class VTKRENDERINGANNOTATION_EXPORT vtkPieChartActor : public vtkActor2D
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkPieChartActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Instantiate this class.
   */
  static vtkPieChartActor *New();

  //@{
  /**
   * Set the input to the pie chart actor. SetInputData()
   * does not connect the pipeline whereas SetInputConnection()
   * does.
   */
  virtual void SetInputData(vtkDataObject*);
  virtual void SetInputConnection(vtkAlgorithmOutput*);
  //@}

  /**
   * Get the input data object to this actor.
   */
  virtual vtkDataObject* GetInput();

  //@{
  /**
   * Enable/Disable the display of a plot title.
   */
  vtkSetMacro(TitleVisibility, int);
  vtkGetMacro(TitleVisibility, int);
  vtkBooleanMacro(TitleVisibility, int);
  //@}

  //@{
  /**
   * Set/Get the title of the pie chart.
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
   * Enable/Disable the display of pie piece labels.
   */
  vtkSetMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkBooleanMacro(LabelVisibility, int);
  //@}

  //@{
  /**
   * Set/Get the labels text property. This controls the appearance
   * of all pie piece labels.
   */
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Specify colors for each piece of pie. If not specified, they are
   * automatically generated.
   */
  void SetPieceColor(int i, double r, double g, double b);
  void SetPieceColor(int i, const double color[3])
    { this->SetPieceColor(i, color[0], color[1], color[2]); }
  double *GetPieceColor(int i);
  //@}

  //@{
  /**
   * Specify the names for each piece of pie.  not specified, then an integer
   * number is automatically generated.
   */
  void SetPieceLabel(const int i, const char *);
  const char* GetPieceLabel(int i);
  //@}

  //@{
  /**
   * Enable/Disable the creation of a legend. If on, the legend labels will
   * be created automatically unless the per plot legend symbol has been
   * set.
   */
  vtkSetMacro(LegendVisibility, int);
  vtkGetMacro(LegendVisibility, int);
  vtkBooleanMacro(LegendVisibility, int);
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
   * Draw the pie plot.
   */
  int RenderOverlay(vtkViewport*) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport* ) VTK_OVERRIDE {return 0;}
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

protected:
  vtkPieChartActor();
  ~vtkPieChartActor() VTK_OVERRIDE;

private:

  vtkPieChartActorConnection* ConnectionHolder;

  vtkIdType ArrayNumber;
  vtkIdType ComponentNumber;
  int TitleVisibility;         // Should I see the title?
  char *Title;                 // The title string
  vtkTextProperty *TitleTextProperty;
  int LabelVisibility;
  vtkTextProperty *LabelTextProperty;
  vtkPieceLabelArray *Labels;
  int LegendVisibility;
  vtkLegendBoxActor *LegendActor;
  vtkGlyphSource2D *GlyphSource;

  // Local variables needed to plot
  vtkIdType N;                 // The number of values
  double    Total;             // The total of all values in the data array
  double   *Fractions;         // The fraction of the pie

  vtkTextMapper    **PieceMappers; //a label for each radial spoke
  vtkActor2D       **PieceActors;

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

  int   LastPosition[2];
  int   LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(vtkViewport *viewport, int *size);
  int BuildPlot(vtkViewport*);

private:
  vtkPieChartActor(const vtkPieChartActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPieChartActor&) VTK_DELETE_FUNCTION;
};


#endif

