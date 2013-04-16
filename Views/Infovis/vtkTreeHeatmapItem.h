/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

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
// vtkTree vtkTable vtkNewickTreeReader

#ifndef __vtkTreeHeatmapItem_h
#define __vtkTreeHeatmapItem_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkContextItem.h"

#include "vtkNew.h" // For vtkNew ivars
#include "vtkSmartPointer.h" // For vtkSmartPointer ivars
#include <vector>   // For lookup tables
#include <map>      // For string lookup tables

class vtkDoubleArray;
class vtkGraphLayout;
class vtkLookupTable;
class vtkTable;
class vtkTooltipItem;
class vtkTree;
class vtkPruneTreeFilter;

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
  // Set the table that this item draws.  The first column of the table
  // must contain the names of the rows.  These names, in turn, must correspond
  // with the nodes names in the input tree.  See SetTree for more information.
  virtual void SetTable(vtkTable *table);

  // Description:
  // Get the table that this item draws.
  vtkTable * GetTable();

  // Description:
  // Collapse subtrees until there are only n leaf nodes left in the tree.
  // The leaf nodes that remain are those that are closest to the root.
  // Any subtrees that were collapsed prior to this function being called
  // may be re-expanded.
  void CollapseToNumberOfLeafNodes(unsigned int n);

  // Get the collapsed tree
  vtkTree * GetPrunedTree();

  // Description:
  // Indicate which array within the Tree's VertexData should be used to
  // color the tree.  The specified array must be a vtkDoubleArray.
  // By default, the tree will be drawn in black.
  void SetTreeColorArray(const char *arrayName);

  //BTX
  // Description:
  // Returns true if the transform is interactive, false otherwise.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Display a tooltip when the user mouses over a cell in the heatmap.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &event);

  // Description:
  // Collapse or expand a subtree when the user double clicks on an
  // internal node.
  virtual bool MouseDoubleClickEvent( const vtkContextMouseEvent &event);
  //ETX

protected:
  vtkTreeHeatmapItem();
  ~vtkTreeHeatmapItem();

  // Description:
  // Generate some data needed for painting.  We cache this information as
  // it only needs to be generated when the input data changes.
  virtual void RebuildBuffers();

  // Description:
  // This function does the bulk of the actual work in rendering our tree &
  // heatmap data.
  virtual void PaintBuffers(vtkContext2D *painter);

  // Description:
  // This function returns a bool indicating whether or not we need to rebuild
  // our cached data before painting.
  virtual bool IsDirty();

  // Description:
  // Compute how to scale our data so that text labels will fit within the
  // bounds determined by the table's cells or the spacing between the leaf
  // nodes of the tree.
  void ComputeMultipliers();

  // Description:
  // Compute the bounds of our tree in pixel coordinates.
  void ComputeTreeBounds();

  // Description:
  // Generate a separate vtkLookupTable for each column in the table.
  void InitializeLookupTables();

  // Description:
  // Paints the tree & associated table as a heatmap.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Helper function.  Generates a vtkLookupTable for a Table column that
  // contains only strings.  Each string will be assigned a separate color.
  // This is useful for visualizing categorical data.
  void GenerateLookupTableForStringColumn(vtkIdType column);

  // Description:
  // Draw the heatmap when no corresponding tree is present.
  void PaintHeatmapWithoutTree(vtkContext2D *painter);

  // Description:
  // Initialize a vtkTextProperty for drawing labels.  This involves
  // calculating an appropriate font size so that labels will fit within
  // the specified cell size.  Returns FALSE if the text would be too
  // small to easily read; TRUE otherwise.
  bool SetupTextProperty(vtkContext2D *painter);

  // Description:
  // Get the value for the cell of the heatmap located at scene position (x, y)
  // This function assumes the caller has already determined that (x, y) falls
  // within the heatmap.
  std::string GetTooltipText(float x, float y);

  // Description:
  // Count the number of leaf nodes in the tree
  void CountLeafNodes();

  // Description:
  // Count the number of leaf nodes that descend from a given vertex.
  int CountLeafNodes(vtkIdType vertex);

  // Description:
  // Get the tree vertex closest to the specified coordinates.
  vtkIdType GetClosestVertex(double x, double y);

  // Description:
  // Collapse the subtree rooted at vertex.
  void CollapseSubTree(vtkIdType vertex);

  // Description:
  // Expand the previously collapsed subtree rooted at vertex.
  void ExpandSubTree(vtkIdType vertex);

  // Description:
  // Look up the original ID of a vertex in the pruned tree.
  vtkIdType GetOriginalId(vtkIdType vertex);

  // Description:
  // Look up the ID of a vertex in the pruned tree from a vertex ID
  // of the input tree.
  vtkIdType GetPrunedIdForOriginalId(vtkIdType originalId);

  // Description:
  // Check if the click at (x, y) should be considered as a click on
  // a collapsed subtree.  Returns the vtkIdType of the pruned subtree
  // if so, -1 otherwise.
  vtkIdType GetClickedCollapsedSubTree(double x, double y);

  // Description:
  // Calculate the extent of the data that is visible within the window.
  // This information is used to ensure that we only draw details that
  // will be seen by the user.  This improves rendering speed, particularly
  // for larger data.
  void UpdateVisibleSceneExtent(vtkContext2D *painter);

  // Description:
  // Returns true if any part of the line segment defined by endpoints
  // (x0, y0), (x1, y1) falls within the extent of the currently
  // visible scene.  Returns false otherwise.
  bool LineIsVisible(double x0, double y0, double x1, double y1);

private:
  vtkTreeHeatmapItem(const vtkTreeHeatmapItem&); // Not implemented
  void operator=(const vtkTreeHeatmapItem&); // Not implemented

  vtkSmartPointer<vtkTree> Tree;
  vtkSmartPointer<vtkTable> Table;
  vtkSmartPointer<vtkTree> PrunedTree;
  vtkSmartPointer<vtkTree> LayoutTree;
  unsigned long TreeHeatmapBuildTime;
  vtkNew<vtkGraphLayout> Layout;
  vtkNew<vtkTooltipItem> Tooltip;
  vtkNew<vtkPruneTreeFilter> PruneFilter;
  vtkNew<vtkLookupTable> TriangleLookupTable;
  vtkNew<vtkLookupTable> TreeLookupTable;
  vtkDoubleArray* TreeColorArray;
  std::vector< vtkLookupTable * > LookupTables;
  std::vector< vtkIdType > RowMap;
  double MultiplierX;
  double MultiplierY;
  int NumberOfLeafNodes;
  double CellWidth;
  double CellHeight;

  std::map< int, std::map< std::string, double> > StringToDoubleMaps;

  double HeatmapMinX;
  double HeatmapMinY;
  double HeatmapMaxX;
  double HeatmapMaxY;
  double TreeMinX;
  double TreeMinY;
  double TreeMaxX;
  double TreeMaxY;
  double SceneBottomLeft[3];
  double SceneTopRight[3];
  bool JustCollapsedOrExpanded;
  bool ColorTree;
};

#endif
