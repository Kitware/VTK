/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPlotParallelCoordinates
 * @brief   Class for drawing a parallel coordinate
 * plot given columns from a vtkTable.
 *
 *
 *
*/

#ifndef vtkPlotParallelCoordinates_h
#define vtkPlotParallelCoordinates_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkScalarsToColors.h" // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "vtkStdString.h"       // For vtkStdString ivars

class vtkChartParallelCoordinates;
class vtkTable;
class vtkStdString;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT vtkPlotParallelCoordinates : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotParallelCoordinates, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a parallel coordinates chart
   */
  static vtkPlotParallelCoordinates* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Paint legend event for the XY plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Set the selection criteria on the given axis in normalized space (0.0 - 1.0).
   */
  bool SetSelectionRange(int Axis, float low, float high);

  /**
   * Reset the selection criteria for the chart.
   */
  bool ResetSelectionRange();

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

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();
  //@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility,int);
  vtkGetMacro(ScalarVisibility,int);
  vtkBooleanMacro(ScalarVisibility,int);
  //@}

  //@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which array to use for coloring using these methods.
   * The lookup table will decide how to convert vectors to colors.
   */
  void SelectColorArray(vtkIdType arrayNum);
  void SelectColorArray(const vtkStdString &arrayName);
  //@}

  /**
   * Get the array name to color by.
   */
  vtkStdString GetColorArrayName();

protected:
  vtkPlotParallelCoordinates();
  ~vtkPlotParallelCoordinates();

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

  //@{
  /**
   * Lookup Table for coloring points by scalar value
   */
  vtkScalarsToColors *LookupTable;
  vtkUnsignedCharArray *Colors;
  int ScalarVisibility;
  vtkStdString ColorArrayName;
  //@}

private:
  vtkPlotParallelCoordinates(const vtkPlotParallelCoordinates &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotParallelCoordinates &) VTK_DELETE_FUNCTION;

};

#endif //vtkPlotParallelCoordinates_h
