/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPlotBox
 * @brief   Class for drawing box plots.
 *
 *
 * Plots to draw box plots given columns from a vtkTable that may contains
 * 5 lines with quartiles and median.
*/

#ifndef vtkPlotBox_h
#define vtkPlotBox_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkStdString.h"       // For vtkStdString ivars

class vtkBrush;
class vtkTextProperty;
class vtkTable;
class vtkStdString;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT vtkPlotBox : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotBox, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a box plot.
   */
  static vtkPlotBox* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the plot, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  //@{
  /**
   * This is a convenience function to set the input table.
   */
  virtual void SetInputData(vtkTable *table);
  virtual void SetInputData(vtkTable *table, const vtkStdString&,
                            const vtkStdString&)
  {
    this->SetInputData(table);
  }
  //@}

  /**
   * Get the plot labels. If this array has a length greater than 1 the index
   * refers to the stacked objects in the plot.
   */
  virtual vtkStringArray *GetLabels();

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated
   * or -1.
   */
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();
  //@}

  /**
   * Helper function to set the color of a given column.
   */
  void SetColumnColor(const vtkStdString& colName, double *rgb);

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  //@{
  /**
   * Get/Set the width of boxes.
   */
  vtkGetMacro(BoxWidth, float);
  vtkSetMacro(BoxWidth, float);
  //@}

  //@{
  /**
   * Get the vtkTextProperty that governs how the plot title is displayed.
   */
  vtkGetObjectMacro(TitleProperties, vtkTextProperty);
  //@}

protected:
  vtkPlotBox();
  ~vtkPlotBox();

  void DrawBoxPlot(int, unsigned char*, double, vtkContext2D*);

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(vtkTable *table);

  //@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  class Private;
  Private* Storage;
  //@}

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  vtkTimeStamp BuildTime;

  /**
   * Width of boxes.
   */
  float BoxWidth;

  /**
   * Lookup Table for coloring points by scalar value
   */
  vtkScalarsToColors *LookupTable;

  /**
   * Text properties for the plot title
   */
  vtkTextProperty* TitleProperties;

private:
  vtkPlotBox(const vtkPlotBox &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotBox &) VTK_DELETE_FUNCTION;

};

#endif //vtkPlotBox_h
