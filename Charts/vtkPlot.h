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

class VTK_CHARTS_EXPORT vtkPlot : public vtkContextItem
{
public:
  vtkTypeRevisionMacro(vtkPlot, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set the plot color
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a);

  // Description:
  // Set the width of the line.
  vtkSetMacro(Width, float);

  // Description:
  // Get the width of the line.
  vtkGetMacro(Width, float);

  // Description:
  // Set the plot label.
  vtkSetStringMacro(Label);

  // Description:
  // Get the plot label.
  vtkGetStringMacro(Label);

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
  // Convenience function to set the input arrays. For most mappers index 0
  // is the x axis, and index 1 is the y axis. The name is the name of the
  // column in the vtkTable.
  virtual void SetInputArray(int index, const char *name);

  virtual void SetSelection(vtkIdTypeArray *id);

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

  unsigned char Color[4];

  // Description:
  // Width in pixels of the plotted line.
  float Width;

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

private:
  vtkPlot(const vtkPlot &); // Not implemented.
  void operator=(const vtkPlot &); // Not implemented.

//ETX
};

#endif //__vtkPlot_h
