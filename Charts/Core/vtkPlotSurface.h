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

/**
 * @class   vtkPlotSurface
 * @brief   3D surface plot.
 *
 *
 * 3D surface plot.
 *
*/

#ifndef vtkPlotSurface_h
#define vtkPlotSurface_h

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
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;
  static vtkPlotSurface * New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  /**
   * Set the input to the surface plot.
   */
  void SetInputData(vtkTable *input) VTK_OVERRIDE;

  //@{
  /**
   * Set the input to the surface plot.
   * Do not use these versions of SetInputData, as all the parameters
   * beyond the vtkTable are ignored.
   */
  void SetInputData(vtkTable *input, const vtkStdString &xName,
                            const vtkStdString &yName,
                            const vtkStdString &zName) VTK_OVERRIDE;
  void SetInputData(vtkTable *input, const vtkStdString &xName,
                            const vtkStdString &yName,
                            const vtkStdString &zName,
                            const vtkStdString &colorName) VTK_OVERRIDE;
  void SetInputData(vtkTable *input, vtkIdType xColumn,
                            vtkIdType yColumn, vtkIdType zColumn) VTK_OVERRIDE;
  //@}

  /**
   * Set the range of the input data for the X dimension.  By default it is
   * (1, NumberOfColumns).  Calling this method after SetInputData() results
   * in recomputation of the plot's data.  Therefore, it is more efficient
   * to call it before SetInputData() when possible.
   */
  void SetXRange(float min, float max);

  /**
   * Set the range of the input data for the Y dimension.  By default it is
   * (1, NumberOfRows).  Calling this method after SetInputData() results
   * in recomputation of the plot's data.  Therefore, it is more efficient
   * to call it before SetInputData() when possible.
   */
  void SetYRange(float min, float max);

protected:
  vtkPlotSurface();
  ~vtkPlotSurface() VTK_OVERRIDE;

  /**
   * Generate a surface (for OpenGL) from our list of points.
   */
  void GenerateSurface();

  /**
   * Helper function used to setup a colored surface.
   */
  void InsertSurfaceVertex(float *data, float value, int i, int j, int &pos);

  /**
   * Change data values if SetXRange() or SetYRange() were called.
   */
  void RescaleData();

  /**
   * Map a column index to the user-specified range for the X-axis.
   */
  float ColumnToX(int columnIndex);

  /**
   * Map a row index to the user-specified range for the Y-axis.
   */
  float RowToY(int rowIndex);

  /**
   * Surface to render.
   */
  std::vector<vtkVector3f> Surface;

  /**
   * The number of rows in the input table.
   */
  vtkIdType NumberOfRows;

  /**
   * The number of columns in the input table.
   */
  vtkIdType NumberOfColumns;

  /**
   * The number of vertices in the surface.
   */
  vtkIdType NumberOfVertices;

  /**
   * The number of components used to color the surface.
   */
  int ColorComponents;

  /**
   * The input table used to generate the surface.
   */
  vtkTable *InputTable;

  /**
   * The lookup table used to color the surface by height (Z dimension).
   */
  vtkNew<vtkLookupTable> LookupTable;

  //@{
  /**
   * user-defined data ranges
   */
  float XMinimum;
  float XMaximum;
  float YMinimum;
  float YMaximum;
  //@}

  /**
   * true if user-defined data scaling has already been applied,
   * false otherwise.
   */
  bool DataHasBeenRescaled;

private:
  vtkPlotSurface(const vtkPlotSurface &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotSurface &) VTK_DELETE_FUNCTION;

};

#endif //vtkPlotSurface_h
