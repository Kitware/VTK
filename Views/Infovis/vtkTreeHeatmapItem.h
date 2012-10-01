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
#include <vector>   // For lookup tables
#include <map>      // For string lookup tables

class vtkGraphLayout;
class vtkLookupTable;
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
  vtkGetObjectMacro(Tree, vtkTree);

  // Description:
  // Set the table that this item draws.  The first column of the table
  // must contain the names of the rows.  These names, in turn, must correspond
  // with the nodes names in the input tree.  See SetTree for more information.
  virtual void SetTable(vtkTable *table);

  // Description:
  // Get the table that this item draws.
  vtkGetObjectMacro(Table, vtkTable);

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
  // bounds determined by the table's cells.
  void ComputeMultiplier();

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
  // the specified cell size.
  void SetupTextProperty(vtkContext2D *painter, double cellHeight);

private:
  vtkTreeHeatmapItem(const vtkTreeHeatmapItem&); // Not implemented
  void operator=(const vtkTreeHeatmapItem&); // Not implemented

  vtkTree *Tree;
  vtkTree *LayoutTree;
  vtkTable *Table;
  unsigned long TreeHeatmapBuildTime;
  vtkNew<vtkGraphLayout> Layout;
  std::vector< vtkLookupTable * > LookupTables;
  double Multiplier;

  std::map< int, std::map< std::string, double> > StringToDoubleMaps;
};

#endif
