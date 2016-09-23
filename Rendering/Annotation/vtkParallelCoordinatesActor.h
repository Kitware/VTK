/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParallelCoordinatesActor
 * @brief   create parallel coordinate display from input field
 *
 * vtkParallelCoordinatesActor generates a parallel coordinates plot from an
 * input field (i.e., vtkDataObject). Parallel coordinates represent
 * N-dimensional data by using a set of N parallel axes (not orthogonal like
 * the usual x-y-z Cartesian axes). Each N-dimensional point is plotted as a
 * polyline, were each of the N components of the point lie on one of the
 * N axes, and the components are connected by straight lines.
 *
 * To use this class, you must specify an input data object. You'll probably
 * also want to specify the position of the plot be setting the Position and
 * Position2 instance variables, which define a rectangle in which the plot
 * lies. Another important parameter is the IndependentVariables ivar, which
 * tells the instance how to interpret the field data (independent variables
 * as the rows or columns of the field). There are also many other instance
 * variables that control the look of the plot includes its title,
 * attributes, number of ticks on the axes, etc.
 *
 * Set the text property/attributes of the title and the labels through the
 * vtkTextProperty objects associated to this actor.
 *
 * @warning
 * Field data is not necessarily "rectangular" in shape. In these cases, some
 * of the data may not be plotted.
 *
 * @warning
 * Field data can contain non-numeric arrays (i.e. arrays not subclasses of
 * vtkDataArray). Such arrays are skipped.
 *
 * @warning
 * The early implementation lacks many features that could be added in the
 * future. This includes the ability to "brush" data (choose regions along an
 * axis and highlight any points/lines passing through the region);
 * efficiency is really bad; more control over the properties of the plot
 * (separate properties for each axes,title,etc.; and using the labels found
 * in the field to label each of the axes.
 *
 * @sa
 * vtkAxisActor3D can be used to create axes in world coordinate space.
 * vtkActor2D vtkTextMapper vtkPolyDataMapper2D vtkScalarBarActor
 * vtkCoordinate vtkTextProperty
*/

#ifndef vtkParallelCoordinatesActor_h
#define vtkParallelCoordinatesActor_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkAlgorithmOutput;
class vtkAxisActor2D;
class vtkDataObject;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextMapper;
class vtkTextProperty;
class vtkParallelCoordinatesActorConnection;

#define VTK_IV_COLUMN 0
#define VTK_IV_ROW    1

class VTKRENDERINGANNOTATION_EXPORT vtkParallelCoordinatesActor : public vtkActor2D
{
public:
  vtkTypeMacro(vtkParallelCoordinatesActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Instantiate object with autorange computation;
   * the number of labels set to 5 for the x and y axes;
   * a label format of "%-#6.3g"; and x coordinates computed from point
   * ids.
   */
  static vtkParallelCoordinatesActor *New();

  //@{
  /**
   * Specify whether to use the rows or columns as independent variables.
   * If columns, then each row represents a separate point. If rows, then
   * each column represents a separate point.
   */
  vtkSetClampMacro(IndependentVariables,int,VTK_IV_COLUMN, VTK_IV_ROW);
  vtkGetMacro(IndependentVariables,int);
  void SetIndependentVariablesToColumns()
    {this->SetIndependentVariables(VTK_IV_COLUMN);};
  void SetIndependentVariablesToRows()
    {this->SetIndependentVariables(VTK_IV_ROW);};
  //@}

  //@{
  /**
   * Set/Get the title of the parallel coordinates plot.
   */
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the number of annotation labels to show along each axis.
   * This values is a suggestion: the number of labels may vary depending
   * on the particulars of the data.
   */
  vtkSetClampMacro(NumberOfLabels, int, 0, 50);
  vtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the axes.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the title text property.
   */
  virtual void SetTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property.
   */
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Draw the parallel coordinates plot.
   */
  int RenderOpaqueGeometry(vtkViewport*);
  int RenderOverlay(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *) {return 0;}
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  virtual int HasTranslucentPolygonalGeometry();

  /**
   * Set the input to the parallel coordinates actor. Creates
   * a pipeline connection.
   */
  virtual void SetInputConnection(vtkAlgorithmOutput*);

  /**
   * Set the input to the parallel coordinates actor. Does not
   * create a pipeline connection.
   */
  virtual void SetInputData(vtkDataObject*);

  /**
   * Remove a dataset from the list of data to append.
   */
  vtkDataObject* GetInput();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkParallelCoordinatesActor();
  ~vtkParallelCoordinatesActor();

private:

  vtkParallelCoordinatesActorConnection* ConnectionHolder;

  int IndependentVariables;    // Use column or row
  vtkIdType N;                 // The number of independent variables
  double *Mins;                 // Minimum data value along this row/column
  double *Maxs;                 // Maximum data value along this row/column
  int   *Xs;                   // Axes x-values (in viewport coordinates)
  int   YMin;                  // Axes y-min-value (in viewport coordinates)
  int   YMax;                  // Axes y-max-value (in viewport coordinates)
  int   NumberOfLabels;        // Along each axis
  char  *LabelFormat;
  char  *Title;

  vtkAxisActor2D **Axes;
  vtkTextMapper  *TitleMapper;
  vtkActor2D     *TitleActor;

  vtkTextProperty *TitleTextProperty;
  vtkTextProperty *LabelTextProperty;

  vtkPolyData         *PlotData;    // The lines drawn within the axes
  vtkPolyDataMapper2D *PlotMapper;
  vtkActor2D          *PlotActor;

  vtkTimeStamp  BuildTime;

  int   LastPosition[2];
  int   LastPosition2[2];

  void Initialize();
  int PlaceAxes(vtkViewport *viewport, int *size);

private:
  vtkParallelCoordinatesActor(const vtkParallelCoordinatesActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParallelCoordinatesActor&) VTK_DELETE_FUNCTION;
};


#endif

