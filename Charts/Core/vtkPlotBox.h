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

// .NAME vtkPlotBox - Class for drawing box plots.
//
// .SECTION Description
// Plots to draw box plots given columns from a vtkTable that may contains
// 5 lines with quartiles and median.

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

  // Description:
  // Creates a box plot.
  static vtkPlotBox* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                           int legendIndex);

  // Description:
  // This is a convenience function to set the input table.
  virtual void SetInputData(vtkTable *table);
  virtual void SetInputData(vtkTable *table, const vtkStdString&,
                            const vtkStdString&)
  {
    this->SetInputData(table);
  }

  // Description:
  // Get the plot labels. If this array has a length greater than 1 the index
  // refers to the stacked objects in the plot.
  virtual vtkStringArray *GetLabels();

  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // Returns the index of the data series with which the point is associated
  // or -1.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location);

  // Description:
  // Specify a lookup table for the mapper to use.
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();

  // Description:
  // Helper function to set the color of a given column.
  void SetColumnColor(const vtkStdString& colName, double *rgb);

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Get/Set the width of boxes.
  vtkGetMacro(BoxWidth, float);
  vtkSetMacro(BoxWidth, float);

  // Description:
  // Get the vtkTextProperty that governs how the plot title is displayed.
  vtkGetObjectMacro(TitleProperties, vtkTextProperty);

//BTX
protected:
  vtkPlotBox();
  ~vtkPlotBox();

  void DrawBoxPlot(int, unsigned char*, double, vtkContext2D*);

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  class Private;
  Private* Storage;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // Width of boxes.
  float BoxWidth;

  // Description:
  // Lookup Table for coloring points by scalar value
  vtkScalarsToColors *LookupTable;

  // Description:
  // Text properties for the plot title
  vtkTextProperty* TitleProperties;

private:
  vtkPlotBox(const vtkPlotBox &); // Not implemented.
  void operator=(const vtkPlotBox &); // Not implemented.

//ETX
};

#endif //vtkPlotBox_h
