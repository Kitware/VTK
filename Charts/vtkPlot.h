/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlot - Abstract class for 2D plots.
//
// .SECTION Description
//

#ifndef __vtkPlot_h
#define __vtkPlot_h

#include "vtkContextItem.h"

class vtkVariant;
class vtkTable;
class vtkIdTypeArray;
class vtkContextMapper2D;
class vtkPen;
class vtkBrush;
class vtkAxis;
class vtkVector2f;

class VTK_CHARTS_EXPORT vtkPlot : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPlot, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Paint legend event for the XY plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, float rect[4]);

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  virtual bool GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);

  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);
//ETX

  // Description:
  // Set the plot color
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);
  virtual void SetColor(double r,  double g, double b);
  virtual void GetColor(double rgb[3]);

  // Description:
  // Set the width of the line.
  virtual void SetWidth(float width);

  // Description:
  // Get the width of the line.
  virtual float GetWidth();

  // Description:
  // Get a pointer to the vtkPen object that controls the was this plot draws
  // lines.
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Get a pointer to the vtkBrush object that controls the was this plot fills
  // shapes.
  vtkGetObjectMacro(Brush, vtkBrush);

  // Description:
  // Set the plot label.
  vtkSetStringMacro(Label);

  // Description:
  // Get the plot label.
  const char* GetLabel();

  // Description:
  // Get the data object that the plot will draw.
  vtkGetObjectMacro(Data, vtkContextMapper2D);

  // Description:
  // Use the Y array index for the X value. If true any X column setting will be
  // ignored, and the X values will simply be the index of the Y column.
  vtkGetMacro(UseIndexForXSeries, bool);

  // Description:
  // Use the Y array index for the X value. If true any X column setting will be
  // ignored, and the X values will simply be the index of the Y column.
  vtkSetMacro(UseIndexForXSeries, bool);

  // Description:
  // This is a convenience function to set the input table and the x, y column
  // for the plot.
  virtual void SetInput(vtkTable *table);
  virtual void SetInput(vtkTable *table, const char *xColumn,
                        const char *yColumn);
  void SetInput(vtkTable *table, vtkIdType xColumn, vtkIdType yColumn);

  // Description:
  // Get the input table used by the plot.
  virtual vtkTable* GetInput();

  // Description:
  // Convenience function to set the input arrays. For most mappers index 0
  // is the x axis, and index 1 is the y axis. The name is the name of the
  // column in the vtkTable.
  virtual void SetInputArray(int index, const char *name);

  virtual void SetSelection(vtkIdTypeArray *id);
  vtkGetObjectMacro(Selection, vtkIdTypeArray);

  // Description:
  // Get/set the X axis associated with this plot.
  vtkGetObjectMacro(XAxis, vtkAxis);
  virtual void SetXAxis(vtkAxis* axis);

  // Description:
  // Get/set the Y axis associated with this plot.
  vtkGetObjectMacro(YAxis, vtkAxis);
  virtual void SetYAxis(vtkAxis* axis);

  virtual void GetBounds(double bounds[4])
  { bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0; }

//BTX
  // Description:
  // A General setter/getter that should be overridden. It can silently drop
  // options, case is important
  void SetProperty(const char *property, vtkVariant *var);
  vtkVariant GetProperty(const char *property);
//ETX

//BTX
protected:
  vtkPlot();
  ~vtkPlot();

  // Description:
  // This object stores the vtkPen that controls how the plot is drawn.
  vtkPen* Pen;

  // Description:
  // This object stores the vtkBrush that controls how the plot is drawn.
  vtkBrush* Brush;

  // Description:
  // Plot label, used by legend.
  char *Label;

  // Description:
  // Use the Y array index for the X value. If true any X column setting will be
  // ignored, and the X values will simply be the index of the Y column.
  bool UseIndexForXSeries;

  // Description:
  // This data member contains the data that will be plotted, it inherits
  // from vtkAlgorithm.
  vtkContextMapper2D *Data;

  // Description:
  // Selected indices for the table the plot is rendering
  vtkIdTypeArray *Selection;

  // Description:
  // The X axis associated with this plot.
  vtkAxis* XAxis;

  // Description:
  // The X axis associated with this plot.
  vtkAxis* YAxis;

private:
  vtkPlot(const vtkPlot &); // Not implemented.
  void operator=(const vtkPlot &); // Not implemented.

//ETX
};

#endif //__vtkPlot_h
