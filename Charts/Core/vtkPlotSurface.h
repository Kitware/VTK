/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotSurface - 3D surface plot.
//
// .SECTION Description
// 3D surface plot.
//

#ifndef __vtkPlotSurface_h
#define __vtkPlotSurface_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              //  For vtkNew ivar
#include "vtkPlot3D.h"

class vtkContext2D;
class vtkLookupTable;
class vtkTable;

class VTKCHARTSCORE_EXPORT vtkPlotSurface : public vtkPlot3D
{
public:
  vtkTypeMacro(vtkPlotSurface, vtkPlot3D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPlotSurface * New();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the input to the surface plot.
  virtual void SetInputData(vtkTable *input);

  // Description:
  // Set the input to the surface plot.
  // Do not use these versions of SetInputData, as all the parameters
  // beyond the vtkTable are ignored.
  virtual void SetInputData(vtkTable *input, const vtkStdString &xName,
                            const vtkStdString &yName,
                            const vtkStdString &zName);
  virtual void SetInputData(vtkTable *input, const vtkStdString &xName,
                            const vtkStdString &yName,
                            const vtkStdString &zName,
                            const vtkStdString &colorName);
  virtual void SetInputData(vtkTable *input, vtkIdType xColumn,
                            vtkIdType yColumn, vtkIdType zColumn);

  // Description:
  // Set the range of the input data for the X dimension.  By default it is
  // (1, NumberOfColumns).  Calling this method after SetInputData() results
  // in recomputation of the plot's data.  Therefore, it is more efficient
  // to call it before SetInputData() when possible.
  void SetXRange(float min, float max);

  // Description:
  // Set the range of the input data for the Y dimension.  By default it is
  // (1, NumberOfRows).  Calling this method after SetInputData() results
  // in recomputation of the plot's data.  Therefore, it is more efficient
  // to call it before SetInputData() when possible.
  void SetYRange(float min, float max);

//BTX
protected:
  vtkPlotSurface();
  ~vtkPlotSurface();

  // Description:
  // Generate a surface (for OpenGL) from our list of points.
  void GenerateSurface();

  // Description:
  // Helper function used to setup a colored surface.
  void InsertSurfaceVertex(float *data, float value, int i, int j, int &pos);

  // Description:
  // Change data values if SetXRange() or SetYRange() were called.
  void RescaleData();

  // Description:
  // Map a column index to the user-specified range for the X-axis.
  float ColumnToX(int columnIndex);

  // Description:
  // Map a row index to the user-specified range for the Y-axis.
  float RowToY(int rowIndex);

  // Description:
  // Surface to render.
  std::vector<vtkVector3f> Surface;

  // Description:
  // The number of rows in the input table.
  vtkIdType NumberOfRows;

  // Description:
  // The number of columns in the input table.
  vtkIdType NumberOfColumns;

  // Description:
  // The number of vertices in the surface.
  vtkIdType NumberOfVertices;

  // Description:
  // This array indicates how the surface should be colored.
  vtkNew<vtkUnsignedCharArray> Colors;

  // Description:
  // The number of components used to color the surface.
  int ColorComponents;

  // Description:
  // The input table used to generate the surface.
  vtkTable *InputTable;

  // Description:
  // The lookup table used to color the surface by height (Z dimension).
  vtkNew<vtkLookupTable> LookupTable;

  // Description:
  // user-defined data ranges
  float XMinimum;
  float XMaximum;
  float YMinimum;
  float YMaximum;

  // Description:
  // true if user-defined data scaling has already been applied,
  // false otherwise.
  bool DataHasBeenRescaled;

private:
  vtkPlotSurface(const vtkPlotSurface &); // Not implemented.
  void operator=(const vtkPlotSurface &); // Not implemented.

//ETX
};

#endif //__vtkPlotSurface_h
