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
/**
 * @class   vtkTanglegramItem
 * @brief   Display two related trees
 *
 *
 * This item draws two trees with connections between their leaf nodes.
 * Use SetTable() to specify what leaf nodes correspond to one another
 * between the two trees.  See the documentation for this function for
 * more details on how this table should be formatted.
 *
 * .SEE ALSO
 * vtkTree vtkTable vtkDendrogramItem vtkNewickTreeReader
*/

#ifndef vtkTanglegramItem_h
#define vtkTanglegramItem_h

#include "vtkViewsInfovisModule.h" // For export macro

#include "vtkContextItem.h"
#include "vtkSmartPointer.h"       // For SmartPointer ivars
#include "vtkTable.h"  // For get/set

class vtkDendrogramItem;
class vtkLookupTable;
class vtkStringArray;
class vtkTree;

class VTKVIEWSINFOVIS_EXPORT vtkTanglegramItem : public vtkContextItem
{
public:
  static vtkTanglegramItem *New();
  vtkTypeMacro(vtkTanglegramItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Set the first tree
   */
  virtual void SetTree1(vtkTree *tree);

  /**
   * Set the second tree
   */
  virtual void SetTree2(vtkTree *tree);

  //@{
  /**
   * Get/Set the table that describes the correspondences between the
   * two trees.  The first column should contain the names of the leaf
   * nodes from tree #1.  The columns of this table should be named
   * after the leaf nodes of tree #2.  A non-zero cell should be used
   * to create a connection between the two trees.  Different numbers
   * in the table will result in connections being drawn in different
   * colors.
   */
  vtkTable * GetTable();
  void SetTable(vtkTable *table);
  //@}

  //@{
  /**
   * Get/Set the label for tree #1.
   */
  vtkGetStringMacro(Tree1Label);
  vtkSetStringMacro(Tree1Label);
  //@}

  //@{
  /**
   * Get/Set the label for tree #2.
   */
  vtkGetStringMacro(Tree2Label);
  vtkSetStringMacro(Tree2Label);
  //@}

  /**
   * Set which way the tanglegram should face within the visualization.
   * The default is for tree #1 to be drawn left to right.
   */
  void SetOrientation(int orientation);

  /**
   * Get the current orientation.
   */
  int GetOrientation();

  //@{
  /**
   * Get/Set the smallest font size that is still considered legible.
   * If the current zoom level requires our vertex labels to be smaller
   * than this size the labels will not be drawn at all.  Default value
   * is 8 pt.
   */
  vtkGetMacro(MinimumVisibleFontSize, int);
  vtkSetMacro(MinimumVisibleFontSize, int);
  //@}

  //@{
  /**
   * Get/Set how much larger the dendrogram labels should be compared to the
   * vertex labels.  Because the vertex labels automatically resize based
   * on zoom levels, this is a relative (not absolute) size.  Default value
   * is 4 pts larger than the vertex labels.
   */
  vtkGetMacro(LabelSizeDifference, int);
  vtkSetMacro(LabelSizeDifference, int);
  //@}

  //@{
  /**
   * Get/Set how wide the correspondence lines should be.  Default is two pixels.
   */
  vtkGetMacro(CorrespondenceLineWidth, float);
  vtkSetMacro(CorrespondenceLineWidth, float);
  //@}

  //@{
  /**
   * Get/Set how wide the edges of the trees should be.  Default is one pixel.
   */
  float GetTreeLineWidth();
  void SetTreeLineWidth(float width);
  //@}

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Propagate any double click onto the dendrograms to check if any
   * subtrees should be collapsed or expanded.
   */
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &event);

protected:
  vtkTanglegramItem();
  ~vtkTanglegramItem();

  /**
   * Update the bounds of our two dendrograms.
   */
  void RefreshBuffers(vtkContext2D *painter);

  /**
   * Calculate and set an appropriate position for our second dendrogram.
   */
  void PositionTree2();

  /**
   * Draw the lines between the corresponding vertices of our two dendrograms.
   */
  void PaintCorrespondenceLines(vtkContext2D *painter);

  /**
   * Draw the labels of our two dendrograms.
   */
  void PaintTreeLabels(vtkContext2D *painter);

  /**
   * Reorder the children of tree #2 to minimize the amount of crossings
   * in our tanglegram.
   */
  void ReorderTree();

  /**
   * Helper function used by ReorderTree.
   * Rearrange the children of the specified parent vertex in order to minimize
   * tanglegram crossings.
   */
  void ReorderTreeAtVertex(vtkIdType parent, vtkTree *tree);

  /**
   * Helper function used by ReorderTreeAtVertex.  Get the average height of
   * the vertices that correspond to the vertex parameter.  This information
   * is used to determine what order sibling vertices should have within the
   * tree.
   */
  double GetPositionScoreForVertex(vtkIdType vertex, vtkTree *tree);

  /**
   * Initialize the lookup table used to color the lines between the two
   * dendrograms.
   */
  void GenerateLookupTable();

  /**
   * Paints the tree & associated table as a heatmap.
   */
  virtual bool Paint(vtkContext2D *painter);

private:
  vtkSmartPointer<vtkDendrogramItem> Dendrogram1;
  vtkSmartPointer<vtkDendrogramItem> Dendrogram2;
  vtkSmartPointer<vtkLookupTable> LookupTable;
  vtkSmartPointer<vtkTable> Table;
  vtkStringArray *Tree1Names;
  vtkStringArray *Tree2Names;
  vtkStringArray *SourceNames;
  double Tree1Bounds[4];
  double Tree2Bounds[4];
  double Spacing;
  double LabelWidth1;
  double LabelWidth2;
  bool PositionSet;
  bool TreeReordered;
  char* Tree1Label;
  char* Tree2Label;
  int Orientation;
  int MinimumVisibleFontSize;
  int LabelSizeDifference;
  float CorrespondenceLineWidth;

  vtkTanglegramItem(const vtkTanglegramItem&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTanglegramItem&) VTK_DELETE_FUNCTION;

};

#endif
