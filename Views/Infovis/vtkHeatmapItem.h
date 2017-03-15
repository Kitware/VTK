/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeatmapItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHeatmapItem
 * @brief   A 2D graphics item for rendering a heatmap
 *
 *
 * This item draws a heatmap as a part of a vtkContextScene.
 *
 * .SEE ALSO
 * vtkTable
*/

#ifndef vtkHeatmapItem_h
#define vtkHeatmapItem_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkContextItem.h"

#include "vtkNew.h"                // For vtkNew ivars
#include "vtkSmartPointer.h"       // For vtkSmartPointer ivars
#include "vtkStdString.h"          // For get/set ivars
#include "vtkVector.h"             // For vtkVector2f ivar
#include <map>                     // For column ranges
#include <set>                     // For blank row support
#include <vector>                  // For row mapping

class vtkBitArray;
class vtkCategoryLegend;
class vtkColorLegend;
class vtkLookupTable;
class vtkStringArray;
class vtkTable;
class vtkTooltipItem;
class vtkVariantArray;

class VTKVIEWSINFOVIS_EXPORT vtkHeatmapItem : public vtkContextItem
{
public:
  static vtkHeatmapItem *New();
  vtkTypeMacro(vtkHeatmapItem, vtkContextItem);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the table that this item draws.  The first column of the table
   * must contain the names of the rows.
   */
  virtual void SetTable(vtkTable *table);

  /**
   * Get the table that this item draws.
   */
  vtkTable * GetTable();

  /**
   * Get the table that this item draws.
   */
  vtkStringArray * GetRowNames();

  //@{
  /**
   * Get/Set the name of the column that specifies the name
   * of this table's rows.  By default, we assume this
   * column will be named "name".  If no such column can be
   * found, we then assume that the 1st column in the table
   * names the rows.
   */
  vtkGetMacro(NameColumn, vtkStdString);
  vtkSetMacro(NameColumn, vtkStdString);
  //@}

  /**
   * Set which way the table should face within the visualization.
   */
  void SetOrientation(int orientation);

  /**
   * Get the current heatmap orientation.
   */
  int GetOrientation();

  /**
   * Get the angle that row labels should be rotated for the corresponding
   * heatmap orientation.  For the default orientation (LEFT_TO_RIGHT), this
   * is 0 degrees.
   */
  double GetTextAngleForOrientation(int orientation);

  //@{
  /**
   * Set the position of the heatmap.
   */
  vtkSetVector2Macro(Position, float);
  void SetPosition(const vtkVector2f &pos);
  //@}

  //@{
  /**
   * Get position of the heatmap.
   */
  vtkGetVector2Macro(Position, float);
  vtkVector2f GetPositionVector();
  //@}

  //@{
  /**
   * Get/Set the height of the cells in our heatmap.
   * Default is 18 pixels.
   */
  vtkGetMacro(CellHeight, double);
  vtkSetMacro(CellHeight, double);
  //@}

  //@{
  /**
   * Get/Set the width of the cells in our heatmap.
   * Default is 36 pixels.
   */
  vtkGetMacro(CellWidth, double);
  vtkSetMacro(CellWidth, double);
  //@}

  /**
   * Get the bounds for this item as (Xmin,Xmax,Ymin,Ymax).
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Mark a row as blank, meaning that no cells will be drawn for it.
   * Used by vtkTreeHeatmapItem to represent missing data.
   */
  void MarkRowAsBlank(std::string rowName);

  /**
   * Paints the table as a heatmap.
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  //@{
  /**
   * Get the width of the largest row or column label drawn by this
   * heatmap.
   */
  vtkGetMacro(RowLabelWidth, float);
  vtkGetMacro(ColumnLabelWidth, float);
  //@}

  /**
   * Enum for Orientation.
   */
  enum
  {
    LEFT_TO_RIGHT,
    UP_TO_DOWN,
    RIGHT_TO_LEFT,
    DOWN_TO_UP
  };

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Display a tooltip when the user mouses over a cell in the heatmap.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent &event) VTK_OVERRIDE;

  /**
   * Display a legend for a column of data.
   */
  bool MouseDoubleClickEvent(const vtkContextMouseEvent &event) VTK_OVERRIDE;

protected:
  vtkHeatmapItem();
  ~vtkHeatmapItem() VTK_OVERRIDE;

  vtkVector2f PositionVector;
  float* Position;

  /**
   * Generate some data needed for painting.  We cache this information as
   * it only needs to be generated when the input data changes.
   */
  virtual void RebuildBuffers();

  /**
   * This function does the bulk of the actual work in rendering our heatmap.
   */
  virtual void PaintBuffers(vtkContext2D *painter);

  /**
   * This function returns a bool indicating whether or not we need to rebuild
   * our cached data before painting.
   */
  virtual bool IsDirty();

  /**
   * Generate a separate vtkLookupTable for each column in the table.
   */
  void InitializeLookupTables();

  /**
   * Helper function.  Find the prominent, distinct values in the specified
   * column of strings and add it to our "master list" of categorical values.
   * This list is then used to generate a vtkLookupTable for all categorical
   * data within the heatmap.
   */
  void AccumulateProminentCategoricalDataValues(vtkIdType column);

  /**
   * Setup the default lookup table to use for continuous (not categorical)
   * data.
   */
  void GenerateContinuousDataLookupTable();

  /**
   * Setup the default lookup table to use for categorical (not continuous)
   * data.
   */
  void GenerateCategoricalDataLookupTable();

  /**
   * Get the value for the cell of the heatmap located at scene position (x, y)
   * This function assumes the caller has already determined that (x, y) falls
   * within the heatmap.
   */
  std::string GetTooltipText(float x, float y);

  /**
   * Calculate the extent of the data that is visible within the window.
   * This information is used to ensure that we only draw details that
   * will be seen by the user.  This improves rendering speed, particularly
   * for larger data.
   */
  void UpdateVisibleSceneExtent(vtkContext2D *painter);

  /**
   * Returns true if any part of the line segment defined by endpoints
   * (x0, y0), (x1, y1) falls within the extent of the currently
   * visible scene.  Returns false otherwise.
   */
  bool LineIsVisible(double x0, double y0, double x1, double y1);

  /**
   * Compute the extent of the heatmap.  This does not include
   * the text labels.
   */
  void ComputeBounds();

  /**
   * Compute the width of our longest row label and the width of our
   * longest column label.  These values are used by GetBounds().
   */
  void ComputeLabelWidth(vtkContext2D *painter);

  // Setup the position, size, and orientation of this heatmap's color
  // legend based on the heatmap's current orientation.
  void PositionColorLegend(int orientation);

  // Setup the position, size, and orientation of this heatmap's
  // legends based on the heatmap's current orientation.
  void PositionLegends(int orientation);

  vtkSmartPointer<vtkTable> Table;
  vtkStringArray * RowNames;
  vtkStdString NameColumn;

private:
  vtkHeatmapItem(const vtkHeatmapItem&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHeatmapItem&) VTK_DELETE_FUNCTION;

  unsigned long HeatmapBuildTime;
  vtkNew<vtkCategoryLegend> CategoryLegend;
  vtkNew<vtkColorLegend> ColorLegend;
  vtkNew<vtkTooltipItem> Tooltip;
  vtkNew<vtkLookupTable> ContinuousDataLookupTable;
  vtkNew<vtkLookupTable> CategoricalDataLookupTable;
  vtkNew<vtkLookupTable> ColorLegendLookupTable;
  vtkNew<vtkStringArray> CategoricalDataValues;
  vtkNew<vtkVariantArray> CategoryLegendValues;
  double CellWidth;
  double CellHeight;

  std::map< vtkIdType, std::pair< double, double > > ColumnRanges;
  std::vector< vtkIdType > SceneRowToTableRowMap;
  std::vector< vtkIdType > SceneColumnToTableColumnMap;
  std::set<std::string> BlankRows;

  double MinX;
  double MinY;
  double MaxX;
  double MaxY;
  double SceneBottomLeft[3];
  double SceneTopRight[3];
  float RowLabelWidth;
  float ColumnLabelWidth;

  vtkBitArray* CollapsedRowsArray;
  vtkBitArray* CollapsedColumnsArray;
  bool LegendPositionSet;
};

#endif
