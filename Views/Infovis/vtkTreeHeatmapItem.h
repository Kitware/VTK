/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeHeatmapItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTreeHeatmapItem
 * @brief   A 2D graphics item for rendering a tree and
 * an associated heatmap.
 *
 *
 * This item draws a tree and a heatmap as a part of a vtkContextScene.
 * The input tree's vertex data must contain at least two arrays.
 * The first required array is a vtkStringArray called "node name".
 * This array corresponds to the first column of the input table.
 * The second required array is a scalar array called "node weight".
 * This array is used by vtkTreeLayoutStrategy to set any particular
 * node's distance from the root of the tree.
 *
 * The vtkNewickTreeReader automatically initializes both of these
 * required arrays in its output tree.
 *
 * .SEE ALSO
 * vtkDendrogramItem vtkHeatmapItem vtkTree vtkTable vtkNewickTreeReader
*/

#ifndef vtkTreeHeatmapItem_h
#define vtkTreeHeatmapItem_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkContextItem.h"

#include "vtkNew.h" // For vtkNew ivars
#include "vtkSmartPointer.h" // For vtkSmartPointer ivars
#include <vector>   // For lookup tables
#include <map>      // For string lookup tables

class vtkDendrogramItem;
class vtkHeatmapItem;
class vtkTable;
class vtkTree;

class VTKVIEWSINFOVIS_EXPORT vtkTreeHeatmapItem : public vtkContextItem
{
public:
  static vtkTreeHeatmapItem *New();
  vtkTypeMacro(vtkTreeHeatmapItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Set the tree that this item draws.  Note that this tree's vertex data
   * must contain a vtkStringArray called "node name".  Additionally, this
   * array must contain the same values as the first column of the input
   * table.  See SetTable for more information.  The vtkNewickTreeReader
   * automatically creates this required array for you.
   */
  virtual void SetTree(vtkTree *tree);

  /**
   * Get the tree that this item draws.
   */
  vtkTree * GetTree();

  /**
   * Set a tree to be drawn for the columns of the heatmap.  This tree's
   * vertex data must contain a vtkStringArray called "node name" that
   * corresponds to the names of the columns in the heatmap.
   */
  virtual void SetColumnTree(vtkTree *tree);

  /**
   * Get the tree that represents the columns of the heatmap (if one has
   * been set).
   */
  vtkTree * GetColumnTree();

  /**
   * Set the table that this item draws.  The first column of the table
   * must contain the names of the rows.  These names, in turn, must correspond
   * with the nodes names in the input tree.  See SetTree for more information.
   */
  virtual void SetTable(vtkTable *table);

  /**
   * Get the table that this item draws.
   */
  vtkTable * GetTable();

  //@{
  /**
   * Get/Set the dendrogram contained by this item.
   */
  vtkDendrogramItem * GetDendrogram();
  void SetDendrogram(vtkDendrogramItem *dendrogram);
  //@}

  //@{
  /**
   * Get/Set the heatmap contained by this item.
   */
  vtkHeatmapItem * GetHeatmap();
  void SetHeatmap(vtkHeatmapItem *heatmap);
  //@}

  /**
   * Reorder the rows in the table so they match the order of the leaf
   * nodes in our tree.
   */
  void ReorderTable();

  /**
   * Reverse the order of the rows in our input table.  This is used
   * to simplify the table layout for DOWN_TO_UP and RIGHT_TO_LEFT
   * orientations.
   */
  void ReverseTableRows();

  /**
   * Reverse the order of the rows in our input table.  This is used
   * to simplify the table layout for DOWN_TO_UP and UP_TO_DOWN
   * orientations.
   */
  void ReverseTableColumns();

  /**
   * Set which way the tree / heatmap should face within the visualization.
   * The default is for both components to be drawn left to right.
   */
  void SetOrientation(int orientation);

  /**
   * Get the current orientation.
   */
  int GetOrientation();

  /**
   * Get the bounds of this item (xMin, xMax, yMin, Max) in pixel coordinates.
   */
  void GetBounds(double bounds[4]);

  /**
   * Get the center point of this item in pixel coordinates.
   */
  void GetCenter(double center[2]);

  /**
   * Get the size of this item in pixel coordinates.
   */
  void GetSize(double size[2]);

  /**
   * Collapse subtrees until there are only n leaf nodes left in the tree.
   * The leaf nodes that remain are those that are closest to the root.
   * Any subtrees that were collapsed prior to this function being called
   * may be re-expanded.  Use this function instead of
   * this->GetDendrogram->CollapseToNumberOfLeafNodes(), as this function
   * also handles the hiding of heatmap rows that correspond to newly
   * collapsed subtrees.
   */
  void CollapseToNumberOfLeafNodes(unsigned int n);

  //@{
  /**
   * Get/Set how wide the edges of the trees should be.  Default is one pixel.
   */
  float GetTreeLineWidth();
  void SetTreeLineWidth(float width);
  //@}

  /**
   * Deprecated.  Use this->GetDendrogram()->GetPrunedTree() instead.
   */
  vtkTree * GetPrunedTree();

  /**
   * Deprecated.  Use this->GetDendrogram()->SetColorArray(const char *arrayName)
   * instead.
   */
  void SetTreeColorArray(const char *arrayName);

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Propagate any double click onto the dendrogram to check if any
   * subtrees should be collapsed or expanded.
   */
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &event);

protected:
  vtkTreeHeatmapItem();
  ~vtkTreeHeatmapItem();

  /**
   * Paints the tree & associated table as a heatmap.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Mark heatmap rows as hidden when a subtree is collapsed.
   */
  void CollapseHeatmapRows();

  /**
   * Mark heatmap columns as hidden when a subtree is collapsed.
   */
  void CollapseHeatmapColumns();

  vtkSmartPointer<vtkDendrogramItem> Dendrogram;
  vtkSmartPointer<vtkDendrogramItem> ColumnDendrogram;
  vtkSmartPointer<vtkHeatmapItem> Heatmap;
  int Orientation;

private:
  vtkTreeHeatmapItem(const vtkTreeHeatmapItem&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeHeatmapItem&) VTK_DELETE_FUNCTION;

  vtkMTimeType TreeHeatmapBuildTime;
};

#endif
