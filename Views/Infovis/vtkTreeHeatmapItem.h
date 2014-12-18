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
// .NAME vtkTreeHeatmapItem - A 2D graphics item for rendering a tree and
// an associated heatmap.
//
// .SECTION Description
// This item draws a tree and a heatmap as a part of a vtkContextScene.
// The input tree's vertex data must contain at least two arrays.
// The first required array is a vtkStringArray called "node name".
// This array corresponds to the first column of the input table.
// The second required array is a scalar array called "node weight".
// This array is used by vtkTreeLayoutStrategy to set any particular
// node's distance from the root of the tree.
//
// The vtkNewickTreeReader automatically initializes both of these
// required arrays in its output tree.
//
// .SEE ALSO
// vtkDendrogramItem vtkHeatmapItem vtkTree vtkTable vtkNewickTreeReader

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

  // Description:
  // Set the tree that this item draws.  Note that this tree's vertex data
  // must contain a vtkStringArray called "node name".  Additionally, this
  // array must contain the same values as the first column of the input
  // table.  See SetTable for more information.  The vtkNewickTreeReader
  // automatically creates this required array for you.
  virtual void SetTree(vtkTree *tree);

  // Description:
  // Get the tree that this item draws.
  vtkTree * GetTree();

  // Description:
  // Set a tree to be drawn for the columns of the heatmap.  This tree's
  // vertex data must contain a vtkStringArray called "node name" that
  // corresponds to the names of the columns in the heatmap.
  virtual void SetColumnTree(vtkTree *tree);

  // Description:
  // Get the tree that represents the columns of the heatmap (if one has
  // been set).
  vtkTree * GetColumnTree();

  // Description:
  // Set the table that this item draws.  The first column of the table
  // must contain the names of the rows.  These names, in turn, must correspond
  // with the nodes names in the input tree.  See SetTree for more information.
  virtual void SetTable(vtkTable *table);

  // Description:
  // Get the table that this item draws.
  vtkTable * GetTable();

  // Description:
  // Get/Set the dendrogram contained by this item.
  vtkDendrogramItem * GetDendrogram();
  void SetDendrogram(vtkDendrogramItem *dendrogram);

  // Description:
  // Get/Set the heatmap contained by this item.
  vtkHeatmapItem * GetHeatmap();
  void SetHeatmap(vtkHeatmapItem *heatmap);

  // Description:
  // Reorder the rows in the table so they match the order of the leaf
  // nodes in our tree.
  void ReorderTable();

  // Description:
  // Reverse the order of the rows in our input table.  This is used
  // to simplify the table layout for DOWN_TO_UP and RIGHT_TO_LEFT
  // orientations.
  void ReverseTableRows();

  // Description:
  // Reverse the order of the rows in our input table.  This is used
  // to simplify the table layout for DOWN_TO_UP and UP_TO_DOWN
  // orientations.
  void ReverseTableColumns();

  // Description:
  // Set which way the tree / heatmap should face within the visualization.
  // The default is for both components to be drawn left to right.
  void SetOrientation(int orientation);

  // Description:
  // Get the current orientation.
  int GetOrientation();

  // Description:
  // Get the bounds of this item (xMin, xMax, yMin, Max) in pixel coordinates.
  void GetBounds(double bounds[4]);

  // Description:
  // Get the center point of this item in pixel coordinates.
  void GetCenter(double center[2]);

  // Description:
  // Get the size of this item in pixel coordinates.
  void GetSize(double size[2]);

  // Description:
  // Collapse subtrees until there are only n leaf nodes left in the tree.
  // The leaf nodes that remain are those that are closest to the root.
  // Any subtrees that were collapsed prior to this function being called
  // may be re-expanded.  Use this function instead of
  // this->GetDendrogram->CollapseToNumberOfLeafNodes(), as this function
  // also handles the hiding of heatmap rows that correspond to newly
  // collapsed subtrees.
  void CollapseToNumberOfLeafNodes(unsigned int n);

  // Description:
  // Get/Set how wide the edges of the trees should be.  Default is one pixel.
  float GetTreeLineWidth();
  void SetTreeLineWidth(float width);

  // Description:
  // Deprecated.  Use this->GetDendrogram()->GetPrunedTree() instead.
  vtkTree * GetPrunedTree();

  // Description:
  // Deprecated.  Use this->GetDendrogram()->SetColorArray(const char *arrayName)
  // instead.
  void SetTreeColorArray(const char *arrayName);

  //BTX

  // Description:
  // Returns true if the transform is interactive, false otherwise.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Propagate any double click onto the dendrogram to check if any
  // subtrees should be collapsed or expanded.
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &event);

  //ETX

protected:
  vtkTreeHeatmapItem();
  ~vtkTreeHeatmapItem();

  // Description:
  // Paints the tree & associated table as a heatmap.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Mark heatmap rows as hidden when a subtree is collapsed.
  void CollapseHeatmapRows();

  // Description:
  // Mark heatmap columns as hidden when a subtree is collapsed.
  void CollapseHeatmapColumns();

  vtkSmartPointer<vtkDendrogramItem> Dendrogram;
  vtkSmartPointer<vtkDendrogramItem> ColumnDendrogram;
  vtkSmartPointer<vtkHeatmapItem> Heatmap;
  int Orientation;

private:
  vtkTreeHeatmapItem(const vtkTreeHeatmapItem&); // Not implemented
  void operator=(const vtkTreeHeatmapItem&); // Not implemented

  unsigned long TreeHeatmapBuildTime;
};

#endif
