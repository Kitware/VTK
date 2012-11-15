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
#include "vtkTreeHeatmapItem.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraphLayout.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPruneTreeFilter.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTooltipItem.h"
#include "vtkTransform2D.h"
#include "vtkTree.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>
#include <queue>
#include <sstream>

vtkStandardNewMacro(vtkTreeHeatmapItem);
//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::vtkTreeHeatmapItem()
{
  this->Interactive = true;
  this->JustCollapsedOrExpanded = false;
  this->TreeHeatmapBuildTime = 0;
  this->Tree = vtkSmartPointer<vtkTree>::New();
  this->PrunedTree = vtkSmartPointer<vtkTree>::New();
  this->LayoutTree = vtkSmartPointer<vtkTree>::New();
  this->Table = vtkSmartPointer<vtkTable>::New();

  /* initialize bounds so that the mouse cursor is never considered
   * "inside" the heatmap or tree */
  this->HeatmapMinX = 1.0;
  this->HeatmapMinY = 1.0;
  this->HeatmapMaxX = 0.0;
  this->HeatmapMaxY = 0.0;
  this->TreeMinX = 1.0;
  this->TreeMinY = 1.0;
  this->TreeMaxX = 0.0;
  this->TreeMaxY = 0.0;

  this->NumberOfLeafNodes = 0;
  this->MultiplierX = 100.0;
  this->MultiplierY = 100.0;
  this->CellWidth = 100.0;
  this->CellHeight = 50.0;

  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip.GetPointer());
  this->PruneFilter->SetShouldPruneParentVertex(false);
}

//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::~vtkTreeHeatmapItem()
{
  while (!this->LookupTables.empty())
    {
    this->LookupTables.back()->Delete();
    this->LookupTables.pop_back();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTree(vtkTree *tree)
{
  if (tree == NULL || tree->GetNumberOfVertices() == 0)
    {
    vtkWarningMacro( << "Warning: empty input tree");
    return;
    }

  this->Tree = tree;

  // initialize some additional arrays for the tree's vertex data
  vtkNew<vtkUnsignedIntArray> vertexIsPruned;
  vertexIsPruned->SetNumberOfComponents(1);
  vertexIsPruned->SetName("VertexIsPruned");
  vertexIsPruned->SetNumberOfValues(
    this->Tree->GetNumberOfVertices());
  vertexIsPruned->FillComponent(0, 0.0);
  this->Tree->GetVertexData()->AddArray(vertexIsPruned.GetPointer());

  vtkNew<vtkIdTypeArray> originalId;
  originalId->SetNumberOfComponents(1);
  originalId->SetName("OriginalId");
  vtkIdType numVertices = this->Tree->GetNumberOfVertices();
  originalId->SetNumberOfValues(numVertices);
  for (vtkIdType i = 0; i < numVertices; ++i)
    {
    originalId->SetValue(i, i);
    }
  this->Tree->GetVertexData()->AddArray(originalId.GetPointer());

  // make a copy of the full tree for later pruning
  this->PrunedTree->DeepCopy(this->Tree);

  // setup the lookup table that's used to color the triangles representing
  // collapsed subtrees.  First we find maximum possible value.
  vtkIdType root = this->Tree->GetRoot();
  if (this->Tree->GetNumberOfChildren(root) == 1)
    {
    root = this->Tree->GetChild(root, 0);
    }
  int numLeavesInBiggestSubTree = 0;
  for (vtkIdType child = 0; child < this->Tree->GetNumberOfChildren(root);
       ++child)
    {
    vtkIdType childVertex = this->Tree->GetChild(root, child);
    int numLeaves = this->CountLeafNodes(childVertex);
    if (numLeaves > numLeavesInBiggestSubTree)
      {
      numLeavesInBiggestSubTree = numLeaves;
      }
    }

  this->TriangleLookupTable->SetNumberOfTableValues(256);
  this->TriangleLookupTable->SetHueRange(0.5, 0.045);
  this->TriangleLookupTable->SetRange(
    2.0, static_cast<double>(numLeavesInBiggestSubTree));
  this->TriangleLookupTable->Build();
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTable(vtkTable *table)
{
  this->Table = table;
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetTree()
{
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkTable * vtkTreeHeatmapItem::GetTable()
{
  return this->Table;
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::Paint(vtkContext2D *painter)
{
  if (this->Tree->GetNumberOfVertices() == 0 &&
      this->Table->GetNumberOfRows() == 0)
    {
    return true;
    }

  if (this->IsDirty())
    {
    this->RebuildBuffers();
    }

  this->PaintBuffers(painter);
  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::IsDirty()
{
  if (this->Tree->GetNumberOfVertices() == 0 &&
      this->Table->GetNumberOfRows() == 0)
    {
    return false;
    }
  if (this->PrunedTree->GetMTime() > this->TreeHeatmapBuildTime)
    {
    return true;
    }
  if (this->Tree->GetNumberOfVertices() == 0)
    {
    if (this->Table->GetMTime() > this->TreeHeatmapBuildTime)
      {
      return true;
      }
    }
  else if(this->Table->GetNumberOfRows() == 0)
    {
    if (this->Tree->GetMTime() > this->TreeHeatmapBuildTime)
      {
      return true;
      }
    }
  else
    {
    if (this->Tree->GetMTime() > this->TreeHeatmapBuildTime ||
        this->Table->GetMTime() > this->GetMTime())
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::RebuildBuffers()
{
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    vtkNew<vtkTreeLayoutStrategy> strategy;
    strategy->SetDistanceArrayName("node weight");
    strategy->SetLeafSpacing(1.0);
    strategy->SetRotation(90.0);

    this->Layout->SetLayoutStrategy(strategy.GetPointer());
    this->Layout->SetInputData(this->PrunedTree);
    this->Layout->Update();
    this->LayoutTree = vtkTree::SafeDownCast(this->Layout->GetOutput());
    this->CountLeafNodes();
    }

  this->ComputeMultipliers();

  if (this->Tree->GetNumberOfVertices() > 0)
    {
    this->ComputeTreeBounds();

    // calculate how large our table cells will be when they are drawn.
    // These values are also used when representing collapsed subtrees.
    double numLeaves = static_cast<double>(this->NumberOfLeafNodes);
    if (numLeaves == 1)
      {
      this->CellHeight = 50.0;
      }
    else
      {
      this->CellHeight = (this->TreeMaxY / (numLeaves - 1.0));
      }
    this->CellWidth = this->CellHeight * 2;
    }

  if (this->Table->GetNumberOfRows() > 0)
    {
    this->InitializeLookupTables();
    }

  if (this->Tree->GetNumberOfVertices() == 0)
    {
    this->TreeHeatmapBuildTime = this->Table->GetMTime();
    }
  else if (this->Table->GetNumberOfRows() == 0)
    {
    if( this->PrunedTree->GetMTime() > this->Tree->GetMTime())
      {
      this->TreeHeatmapBuildTime = this->PrunedTree->GetMTime();
      }
    else if(this->Tree->GetMTime() > this->Table->GetMTime())
      {
      this->TreeHeatmapBuildTime = this->Tree->GetMTime();
      }
    }
  else
    {
    if( this->PrunedTree->GetMTime() > this->Tree->GetMTime())
      {
      this->TreeHeatmapBuildTime = this->PrunedTree->GetMTime();
      }
    else if(this->Tree->GetMTime() > this->Table->GetMTime())
      {
      this->TreeHeatmapBuildTime = this->Tree->GetMTime();
      }
    else
      {
      this->TreeHeatmapBuildTime = this->Table->GetMTime();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ComputeMultipliers()
{
  double targetFontSize = 18;
  double yMax = 1;
  double targetPoint[3];
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    for (vtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
      {
      vtkIdType target = this->LayoutTree->GetTargetVertex(edge);
      this->LayoutTree->GetPoint(target, targetPoint);
      if (targetPoint[1] > yMax)
        {
        yMax = targetPoint[1];
        }
      }
    }

  double numRows;
  if (this->Tree->GetNumberOfVertices() == 0)
    {
    numRows = this->Table->GetNumberOfRows();
    yMax = this->CellHeight * numRows;
    }
  else
    {
    numRows = this->NumberOfLeafNodes;
    }

  double currentFontSize =
    (yMax * this->MultiplierX) / numRows;
  if (currentFontSize < targetFontSize)
    {
    this->MultiplierX = (numRows * targetFontSize) / yMax;
    this->MultiplierY = this->MultiplierX;
    }
  if (this->JustCollapsedOrExpanded)
    {
    this->MultiplierY = (this->CellHeight * (numRows - 1)) / yMax;
    this->JustCollapsedOrExpanded = false;
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ComputeTreeBounds()
{
  this->TreeMinX = VTK_DOUBLE_MAX;
  this->TreeMinY = VTK_DOUBLE_MAX;
  this->TreeMaxX = VTK_DOUBLE_MIN;
  this->TreeMaxY = VTK_DOUBLE_MIN;

  double sourcePoint[3];
  double targetPoint[3];

  for (vtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
    {
    vtkIdType source = this->LayoutTree->GetSourceVertex(edge);
    this->LayoutTree->GetPoint(source, sourcePoint);
    double x0 = sourcePoint[0] * this->MultiplierX;
    double y0 = sourcePoint[1] * this->MultiplierY;

    vtkIdType target = this->LayoutTree->GetTargetVertex(edge);
    this->LayoutTree->GetPoint(target, targetPoint);
    double x1 = targetPoint[0] * this->MultiplierX;
    double y1 = targetPoint[1] * this->MultiplierY;

    if (x0 < this->TreeMinX)
      {
      this->TreeMinX = x0;
      }
    if (y0 < this->TreeMinY)
      {
      this->TreeMinY = y0;
      }
    if (x0 > this->TreeMaxX)
      {
      this->TreeMaxX = x0;
      }
    if (y0 > this->TreeMaxY)
      {
      this->TreeMaxY = y0;
      }
    if (x1 < this->TreeMinX)
      {
      this->TreeMinX = x1;
      }
    if (y1 < this->TreeMinY)
      {
      this->TreeMinY = y1;
      }
    if (x1 > this->TreeMaxX)
      {
      this->TreeMaxX = x1;
      }
    if (y1 > this->TreeMaxY)
      {
      this->TreeMaxY = y1;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CountLeafNodes()
{
  // figure out how many leaf nodes we have.
  this->NumberOfLeafNodes = 0;
  for (vtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices();
       ++vertex)
    {
    if (!this->LayoutTree->IsLeaf(vertex))
      {
      continue;
      }
    ++this->NumberOfLeafNodes;
    }
}

//-----------------------------------------------------------------------------
int vtkTreeHeatmapItem::CountLeafNodes(vtkIdType vertex)
{
  // figure out how many leaf nodes descend from vertex.
  int numLeaves = 0;
  for (vtkIdType child = 0; child < this->Tree->GetNumberOfChildren(vertex);
       ++child)
    {
    vtkIdType childVertex = this->Tree->GetChild(vertex, child);
    if (this->Tree->IsLeaf(childVertex))
      {
      ++numLeaves;
      }
    else
      {
      numLeaves += this->CountLeafNodes(childVertex);
      }
    }
  return numLeaves;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::InitializeLookupTables()
{
  while (!this->LookupTables.empty())
    {
    this->LookupTables.back()->Delete();
    this->LookupTables.pop_back();
    }
  this->LookupTables.reserve( this->Table->GetNumberOfColumns() + 1 );

  for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
       ++column)
    {
    if (this->Table->GetValue(0, column).IsString())
      {
      this->GenerateLookupTableForStringColumn(column);
      continue;
      }
    double min = VTK_DOUBLE_MAX;
    double max = VTK_DOUBLE_MIN;
    vtkLookupTable *lookupTable = vtkLookupTable::New();
    this->LookupTables.push_back(lookupTable);
    for (vtkIdType row = 0; row < this->Table->GetNumberOfRows(); ++row)
      {
      double value = this->Table->GetValue(row, column).ToDouble();
      if (value > max)
        {
        max = value;
        }
      else if (value < min)
        {
        min = value;
        }
      }
    this->LookupTables[column-1]->SetNumberOfTableValues(256);
    this->LookupTables[column-1]->SetRange(min, max);
    this->LookupTables[column-1]->Build();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GenerateLookupTableForStringColumn(vtkIdType column)
{
  //generate a sorted vector of all the strings in this column
  std::vector< std::string > sortedStrings;
  for (vtkIdType row = 0; row < this->Table->GetNumberOfRows(); ++row)
    {
    std::string value = this->Table->GetValue(row, column).ToString();
    // no duplicates
    if (std::find(sortedStrings.begin(), sortedStrings.end(), value) ==
        sortedStrings.end())
      {
      sortedStrings.push_back(value);
      }
    }
  std::sort(sortedStrings.begin(), sortedStrings.end());

  // map each string to a double value based on its position
  // in alphabetical order
  std::map< std::string, double> stringToDouble;
  for (unsigned int i = 0; i < sortedStrings.size(); ++i)
    {
    stringToDouble[ sortedStrings[i] ] = (double)i;
    }

  // associate this mapping with the column number
  this->StringToDoubleMaps[column] = stringToDouble;

  // generate a lookup table for this column
  this->LookupTables.push_back(vtkLookupTable::New());
  this->LookupTables[column-1]->SetNumberOfTableValues(256);
  this->LookupTables[column-1]->SetRange(0, sortedStrings.size() - 1);
  this->LookupTables[column-1]->Build();
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::PaintBuffers(vtkContext2D *painter)
{
  if (this->Tree->GetNumberOfVertices() == 0)
    {
    this->PaintHeatmapWithoutTree(painter);
    return;
    }

  double xStart, yStart;
  double sourcePoint[3];
  double targetPoint[3];
  double spacing = 25;
  int numberOfCollapsedSubTrees = 0;

  vtkUnsignedIntArray *vertexIsPruned = vtkUnsignedIntArray::SafeDownCast(
    this->Tree->GetVertexData()->GetArray("VertexIsPruned"));

  // Calculate the extent of the data that is visible within the window.
  this->UpdateVisibleSceneExtent(painter);

  // draw the tree
  for (vtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
    {
    vtkIdType source = this->LayoutTree->GetSourceVertex(edge);
    vtkIdType target = this->LayoutTree->GetTargetVertex(edge);

    this->LayoutTree->GetPoint(source, sourcePoint);
    this->LayoutTree->GetPoint(target, targetPoint);

    double x0 = sourcePoint[0] * this->MultiplierX;
    double y0 = sourcePoint[1] * this->MultiplierY;
    double x1 = targetPoint[0] * this->MultiplierX;
    double y1 = targetPoint[1] * this->MultiplierY;

    // check if the target vertex is the root of a collapsed tree
    bool alreadyDrewCollapsedSubTree = false;
    vtkIdType originalId = this->GetOriginalId(target);

    if (vertexIsPruned->GetValue(originalId) > 0)
      {
      ++numberOfCollapsedSubTrees;
      float trianglePoints[6];
      trianglePoints[0] = sourcePoint[0] * this->MultiplierX;
      trianglePoints[1] = targetPoint[1] * this->MultiplierY;
      trianglePoints[2] = this->TreeMaxX;
      trianglePoints[3] = targetPoint[1]  * this->MultiplierY - this->CellHeight / 2;
      trianglePoints[4] = this->TreeMaxX;
      trianglePoints[5] = targetPoint[1] * this->MultiplierY + this->CellHeight / 2;
      if (this->LineIsVisible(trianglePoints[0], trianglePoints[1],
                              trianglePoints[2], trianglePoints[3]) ||
          this->LineIsVisible(trianglePoints[0], trianglePoints[1],
                              trianglePoints[4], trianglePoints[5]) ||
          this->LineIsVisible(trianglePoints[2], trianglePoints[3],
                              trianglePoints[4], trianglePoints[5]))
        {
        double color[3];
        double colorKey =
          static_cast<double>(vertexIsPruned->GetValue(originalId));
        this->TriangleLookupTable->GetColor(colorKey, color);
        painter->GetBrush()->SetColorF(color[0], color[1], color[2]);
        painter->DrawPolygon(trianglePoints, 3);
        }
      alreadyDrewCollapsedSubTree = true;
      }

    if (this->LineIsVisible(x0, y0, x0, y1))
      {
      painter->DrawLine (x0, y0, x0, y1);
      }
    if (!alreadyDrewCollapsedSubTree)
      {
      if (this->LineIsVisible(x0, y1, x1, y1))
        {
        painter->DrawLine (x0, y1, x1, y1);
        }
      }
    }

  // special case: all the true leaf nodes have been collapsed
  if (this->NumberOfLeafNodes <= numberOfCollapsedSubTrees)
    {
    return;
    }

  // get array of node names from the tree
  vtkStringArray *nodeNames = vtkStringArray::SafeDownCast(
    this->LayoutTree->GetVertexData()->GetAbstractArray("node name"));

  // leave a small amount of space between the tree, the table,
  // and the row/column labels
  spacing = this->CellWidth * 0.25;

  bool canDrawText = this->SetupTextProperty(painter);

  if (this->Table->GetNumberOfRows() == 0)
    {
    // special case for tree with no table
    // draw labels for the leaf nodes
    xStart = this->TreeMaxX + spacing;

    if (!canDrawText || this->SceneBottomLeft[0] > xStart ||
        this->SceneTopRight[0] < xStart)
      {
      return;
      }

    for (vtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices();
         ++vertex)
      {
      if (!this->LayoutTree->IsLeaf(vertex))
        {
        continue;
        }
      double point[3];
      this->LayoutTree->GetPoint(vertex, point);
      std::string nodeName = nodeNames->GetValue(vertex);
      yStart = point[1] * this->MultiplierY;
      if (this->SceneBottomLeft[1] < yStart && this->SceneTopRight[1] > yStart)
        {
        painter->DrawString(xStart, yStart, nodeName);
        }
      }
    return;
    }

  // get array of row names from the table.  We assume this is the first row.
  vtkStringArray *tableNames = vtkStringArray::SafeDownCast(
    this->Table->GetColumn(0));

  this->RowMap.clear();
  this->RowMap.assign(this->NumberOfLeafNodes, -1);
  this->HeatmapMinX = this->TreeMaxX + spacing;
  this->HeatmapMaxX = this->TreeMaxX + spacing +
    this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
  this->HeatmapMinY = VTK_DOUBLE_MAX;
  this->HeatmapMaxY = VTK_DOUBLE_MIN;
  xStart = this->TreeMaxX + spacing * 2 +
    this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
  bool drawRowLabels = canDrawText;
  if (this->SceneBottomLeft[0] > xStart || this->SceneTopRight[0] < xStart)
    {
    drawRowLabels = false;
    }

  for (vtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices();
       ++vertex)
    {
    if (!this->LayoutTree->IsLeaf(vertex))
      {
      continue;
      }

    // For now, we don't draw a heatmap row for a pruned branch.
    vtkIdType originalId = this->GetOriginalId(vertex);
    if (vertexIsPruned->GetValue(originalId) > 0)
      {
      continue;
      }

    // determine which row we're drawing
    double point[3];
    this->LayoutTree->GetPoint(vertex, point);
    int currentRow = floor(point[1] * this->MultiplierY / this->CellHeight + 0.5);

    // find the row in the table that corresponds to this vertex
    std::string nodeName = nodeNames->GetValue(vertex);
    vtkIdType tableRow = tableNames->LookupValue(nodeName);

    this->RowMap[currentRow] = tableRow;

    for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
         ++column)
      {
      // get the color for this cell from the lookup table
      double color[3];
      if (this->Table->GetValue(tableRow, column).IsString())
        {
        // get the string to int mapping for this column
        std::map< std::string, double> stringToDouble =
          this->StringToDoubleMaps[column];

        // get the integer value for the current string
        std::string cellStr =
          this->Table->GetValue(tableRow, column).ToString();
        double colorKey = stringToDouble[cellStr];

        // now we can lookup the appropriate color for this string
        this->LookupTables[column-1]->GetColor(colorKey, color);
        }
      else
        {
        vtkVariant value = this->Table->GetValue(tableRow, column);
        this->LookupTables[column-1]->GetColor(value.ToDouble(), color);
        }
      painter->GetBrush()->SetColorF(color[0], color[1], color[2]);

      // draw this cell of the table
      xStart = this->HeatmapMinX + this->CellWidth * (column - 1);
      yStart = point[1] * this->MultiplierY - (this->CellHeight / 2);
      if (this->LineIsVisible(xStart, yStart, xStart + this->CellWidth,
                              yStart + this->CellHeight) ||
          this->LineIsVisible(xStart, yStart + this->CellHeight,
                              xStart + this->CellWidth, yStart))
        {
        painter->DrawRect(xStart, yStart, this->CellWidth, this->CellHeight);
        }

      // keep track of where the top and bottom of the table is.
      // this is used to position column labels and tool tips.
      if (yStart + this->CellHeight > this->HeatmapMaxY)
        {
        this->HeatmapMaxY = yStart + this->CellHeight;
        }
      if (yStart < this->HeatmapMinY)
        {
        this->HeatmapMinY = yStart;
        }
      }

    // draw the label for this row
    if (drawRowLabels)
      {
      xStart = this->TreeMaxX + spacing * 2 +
        this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
      yStart = point[1] * this->MultiplierY;
      if (this->SceneBottomLeft[1] < yStart && this->SceneTopRight[1] > yStart)
        {
        painter->DrawString(xStart, yStart, nodeName);
        }
      }
    }

  // special case for when we've collapsed away the top row of the heatmap
  if (this->HeatmapMaxY < this->TreeMaxY + this->CellHeight / 2)
    {
    this->HeatmapMaxY = this->TreeMaxY + this->CellHeight / 2;
    }

  // draw column labels
  yStart = this->HeatmapMaxY + spacing;
  if (canDrawText && this->SceneBottomLeft[1] < yStart &&
      this->SceneTopRight[1] > yStart)
    {
    painter->GetTextProp()->SetOrientation(90);
    for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
         ++column)
      {
      std::string columnName = this->Table->GetColumn(column)->GetName();
      xStart =
        this->HeatmapMinX + this->CellWidth * column - this->CellWidth / 2;
      if (this->SceneBottomLeft[0] < xStart && this->SceneTopRight[0] > xStart)
        {
        painter->DrawString(xStart, yStart, columnName);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::PaintHeatmapWithoutTree(vtkContext2D *painter)
{
  // leave a small amount of space between the tree, the table,
  // and the row/column labels
  double spacing = this->CellWidth * 0.25;

  // get array of row names from the table.  We assume this is the first row.
  vtkStringArray *tableNames = vtkStringArray::SafeDownCast(
    this->Table->GetColumn(0));

  // calculate a font size that's appropriate for this zoom level
  this->SetupTextProperty(painter);

  double xStart, yStart;
  this->HeatmapMinX = 0;
  this->HeatmapMaxX = this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
  this->HeatmapMinY = VTK_DOUBLE_MAX;
  this->HeatmapMaxY = VTK_DOUBLE_MIN;

  for (vtkIdType row = 0; row < this->Table->GetNumberOfRows();
       ++row)
    {
    for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
         ++column)
      {
      // get the color for this cell from the lookup table
      double color[3];
      if (this->Table->GetValue(row, column).IsString())
        {
        // get the string to int mapping for this column
        std::map< std::string, double> stringToDouble =
          this->StringToDoubleMaps[column];

        // get the integer value for the current string
        std::string cellStr = this->Table->GetValue(row, column).ToString();
        double colorKey = stringToDouble[cellStr];

        // now we can lookup the appropriate color for this string
        this->LookupTables[column-1]->GetColor(colorKey, color);
        }
      else
        {
        vtkVariant value = this->Table->GetValue(row, column);
        this->LookupTables[column-1]->GetColor(value.ToDouble(), color);
        }
      painter->GetBrush()->SetColorF(color[0], color[1], color[2]);

      // draw this cell of the table
      xStart = this->CellWidth * (column - 1);
      yStart = this->CellHeight * row;
      painter->DrawRect(xStart, yStart, this->CellWidth, this->CellHeight);

      // keep track of where the top of the table is, so we know where to
      // draw the column labels later.
      if (yStart + this->CellHeight > this->HeatmapMaxY)
        {
        this->HeatmapMaxY = yStart + this->CellHeight;
        }
      if (yStart < this->HeatmapMinY)
        {
        this->HeatmapMinY = yStart;
        }
      }

    // draw the label for this row
    std::string rowLabel = tableNames->GetValue(row);
    xStart = spacing * 2 + this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
    yStart = this->CellHeight * row + this->CellHeight / 2;
    painter->DrawString(xStart, yStart, rowLabel);
    }

  // draw column labels
  painter->GetTextProp()->SetOrientation(90);
  for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
       ++column)
    {
      std::string columnName = this->Table->GetColumn(column)->GetName();
      xStart = this->CellWidth * column - this->CellWidth / 2;
      yStart = this->HeatmapMaxY + spacing;
      painter->DrawString(xStart, yStart, columnName);
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::UpdateVisibleSceneExtent(vtkContext2D *painter)
{
  float position[2];
  painter->GetTransform()->GetPosition(position);
  this->SceneBottomLeft[0] = -position[0];
  this->SceneBottomLeft[1] = -position[1];
  this->SceneBottomLeft[2] = 0.0;

  this->SceneTopRight[0] =
    static_cast<double>(this->GetScene()->GetSceneWidth() - position[0]);
  this->SceneTopRight[1] =
    static_cast<double>(this->GetScene()->GetSceneHeight() - position[1]);
  this->SceneTopRight[2] = 0.0;
  vtkNew<vtkMatrix3x3> inverse;
  painter->GetTransform()->GetInverse(inverse.GetPointer());
  inverse->MultiplyPoint(this->SceneBottomLeft, this->SceneBottomLeft);
  inverse->MultiplyPoint(this->SceneTopRight, this->SceneTopRight);
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::LineIsVisible(double x0, double y0,
                                        double x1, double y1)
{
  // use local variables to improve readibility
  double xMinScene = this->SceneBottomLeft[0];
  double yMinScene = this->SceneBottomLeft[1];
  double xMaxScene = this->SceneTopRight[0];
  double yMaxScene = this->SceneTopRight[1];

  // if either end point of the line segment falls within the screen,
  // then the line segment is visible.
  if ( (xMinScene <= x0 && xMaxScene >= x0 &&
        yMinScene <= y0 && yMaxScene >= y0) ||
       (xMinScene <= x1 && xMaxScene >= x1 &&
        yMinScene <= y1 && yMaxScene >= y1) )
    {
    return true;
    }

  // figure out which end point is "greater" than the other in both dimensions
  double xMinLine, xMaxLine, yMinLine, yMaxLine;
  if (x0 < x1)
    {
    xMinLine = x0;
    xMaxLine = x1;
    }
  else
    {
    xMinLine = x1;
    xMaxLine = x0;
    }
  if (y0 < y1)
    {
    yMinLine = y0;
    yMaxLine = y1;
    }
  else
    {
    yMinLine = y1;
    yMaxLine = y0;
    }

  // case where the Y range of the line falls within the visible scene
  // and the X range of the line contains the entire visible scene
  if (yMinScene <= yMinLine && yMaxScene >= yMinLine &&
      yMinScene <= yMaxLine && yMaxScene >= yMaxLine &&
      xMinLine <= xMinScene && xMaxLine >= xMaxScene)
    {
    return true;
    }

  // case where the X range of the line falls within the visible scene
  // and the Y range of the line contains the entire visible scene
  if (xMinScene <= xMinLine && xMaxScene >= xMinLine &&
      xMinScene <= xMaxLine && xMaxScene >= xMaxLine &&
      yMinLine <= yMinScene && yMaxLine >= yMaxScene)
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::SetupTextProperty(vtkContext2D *painter)
{
  // set up our text property to draw row names
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetOrientation(0);

  // calculate an appropriate font size
  float stringBounds[4];
  stringBounds[3] = VTK_FLOAT_MAX;
  std::string testString = "Igq"; //selected for range of height
  int currentFontSize = floor(this->CellHeight);
  if (currentFontSize > 500)
    {
    currentFontSize = 500;
    }
  painter->GetTextProp()->SetFontSize(currentFontSize);
  painter->ComputeStringBounds(testString, stringBounds);
  if (stringBounds[3] > this->CellWidth || stringBounds[3] > this->CellHeight)
    {
    while (currentFontSize > 0 &&
            (stringBounds[3] > this->CellWidth ||
             stringBounds[3] > this->CellHeight))
      {
      --currentFontSize;
      if (currentFontSize < 8)
      {
        return false;
      }
      painter->GetTextProp()->SetFontSize(currentFontSize);
      painter->ComputeStringBounds(testString, stringBounds);
      }
    }
  else
    {
      while (stringBounds[3] < this->CellWidth &&
             stringBounds[3] < this->CellHeight && currentFontSize < 500)
      {
      ++currentFontSize;
      painter->GetTextProp()->SetFontSize(currentFontSize);
      painter->ComputeStringBounds(testString, stringBounds);
      }
    --currentFontSize;
    painter->GetTextProp()->SetFontSize(currentFontSize);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::MouseMoveEvent(const vtkContextMouseEvent &event)
{
  if (event.GetButton() == vtkContextMouseEvent::NO_BUTTON)
    {
    float pos[3];
    vtkNew<vtkMatrix3x3> inverse;
    pos[0] = event.GetPos().GetX();
    pos[1] = event.GetPos().GetY();
    pos[2] = 0;
    this->GetScene()->GetTransform()->GetInverse(inverse.GetPointer());
    inverse->MultiplyPoint(pos, pos);
    if (pos[0] <= this->HeatmapMaxX && pos[0] >= this->HeatmapMinX &&
        pos[1] <= this->HeatmapMaxY && pos[1] >= this->HeatmapMinY)
      {
      this->Tooltip->SetPosition(pos[0], pos[1]);

      std::string tooltipText = this->GetTooltipText(pos[0], pos[1]);
      if (tooltipText.compare("") != 0)
        {
        this->Tooltip->SetText(tooltipText);
        this->Tooltip->SetVisible(true);
        this->Scene->SetDirty(true);
        }
      return true;
      }
    else
      {
      bool shouldRepaint = this->Tooltip->GetVisible();
      this->Tooltip->SetVisible(false);
      if (shouldRepaint)
        {
        this->Scene->SetDirty(true);
        }
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
std::string vtkTreeHeatmapItem::GetTooltipText(float x, float y)
{
  vtkIdType column = floor((x - this->HeatmapMinX) / this->CellWidth);
  int sceneRow = floor(y / this->CellHeight + 0.5);

  if (this->Tree->GetNumberOfVertices() > 0)
    {
    int dataRow = this->RowMap[sceneRow];
    if (dataRow != -1)
      {
      return this->Table->GetValue(dataRow, column + 1).ToString();
      }
    return "";
    }
  return this->Table->GetValue(sceneRow, column + 1).ToString();
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::MouseDoubleClickEvent(
  const vtkContextMouseEvent &event)
{
  // get the position of the double click and convert it to scene coordinates
  double pos[3];
  vtkNew<vtkMatrix3x3> inverse;
  pos[0] = event.GetPos().GetX();
  pos[1] = event.GetPos().GetY();
  pos[2] = 0;
  this->GetScene()->GetTransform()->GetInverse(inverse.GetPointer());
  inverse->MultiplyPoint(pos, pos);

  // this event is only captured within the tree (not the heatmap)
  if (pos[0] <= this->TreeMaxX && pos[0] >= this->TreeMinX)
    {
    vtkIdType collapsedSubTree =
      this->GetClickedCollapsedSubTree(pos[0], pos[1]);
    if (collapsedSubTree != -1)
      {
      // re-expand the subtree rooted at this vertex
      this->ExpandSubTree(collapsedSubTree);
      }
    else
      {
      // collapse the subtree rooted at this vertex
      vtkIdType closestVertex =
        this->GetClosestVertex(pos[0] / this->MultiplierX,
                               pos[1] / this->MultiplierY);
      this->CollapseSubTree(closestVertex);
      }

    this->Scene->SetDirty(true);
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
vtkIdType vtkTreeHeatmapItem::GetClickedCollapsedSubTree(double x, double y)
{
  // iterate over all the collapsed subtrees to see if this click refers
  // to one of them.
  vtkUnsignedIntArray *vertexIsPruned = vtkUnsignedIntArray::SafeDownCast(
    this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  vtkIdTypeArray *originalIdArray = vtkIdTypeArray::SafeDownCast(
    this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  for (vtkIdType originalId = 0;
       originalId < vertexIsPruned->GetNumberOfTuples(); ++originalId)
    {
    if (vertexIsPruned->GetValue(originalId) > 0)
      {
      // Find PrunedTree's vertex that corresponds to this originalId.
      for (vtkIdType prunedId = 0;
           prunedId < originalIdArray->GetNumberOfTuples(); ++prunedId)
        {
        if (originalIdArray->GetValue(prunedId) == originalId)
          {
          // determined where this collapsed subtree is rooted.
          double point[3];
          this->LayoutTree->GetPoint(prunedId, point);

          // we also need the location of this node's parent
          double parentPoint[3];
          this->LayoutTree->GetPoint(
            this->LayoutTree->GetParent(prunedId), parentPoint);

          // proper height (Y) range: within +/- CellHeight of the vertex's
          // Y value.
          float yMin = point[1] * this->MultiplierY - this->CellHeight / 2;
          float yMax = point[1] * this->MultiplierY + this->CellHeight / 2;
          if (y >= yMin && y <= yMax)
            {
            //proper width (X) range: >= parent's X value.
            if (x >= parentPoint[0])
              {
              return prunedId;
              }
            }

          break;
          }
        }
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
vtkIdType vtkTreeHeatmapItem::GetClosestVertex(double x, double y)
{
  double minDistance = VTK_DOUBLE_MAX;
  vtkIdType closestVertex = -1;
  for (vtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices();
       ++vertex)
    {
    double point[3];
    this->LayoutTree->GetPoint(vertex, point);
    double distance = sqrt( (x - point[0]) * (x - point[0]) +
                            (y - point[1]) * (y - point[1]) );

    if (distance < minDistance)
      {
      minDistance = distance;
      closestVertex = vertex;
      }
    }
  return closestVertex;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseSubTree(vtkIdType vertex)
{
  // no removing the root of the tree
  if (vertex == this->PrunedTree->GetRoot())
    {
    return;
    }

  // look up the original ID of the vertex that's being collapsed.
  vtkIdTypeArray *originalIdArray = vtkIdTypeArray::SafeDownCast(
    this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  vtkIdType originalId = originalIdArray->GetValue(vertex);

  // use this value as the index to the original (un-reindexed) tree's
  // "VertexIsPruned" array.  Mark that vertex as pruned by recording
  // how many collapsed leaf nodes exist beneath it.
  int numLeavesCollapsed = this->CountLeafNodes(originalId);
  // no collapsing of leaf nodes
  if (numLeavesCollapsed == 0)
    {
    return;
    }
  vtkUnsignedIntArray *vertexIsPruned = vtkUnsignedIntArray::SafeDownCast(
    this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  vertexIsPruned->SetValue(originalId, numLeavesCollapsed);

  vtkNew<vtkTree> prunedTreeCopy;
  prunedTreeCopy->ShallowCopy(this->PrunedTree);

  this->PruneFilter->SetInputData(prunedTreeCopy.GetPointer());
  this->PruneFilter->SetParentVertex(vertex);
  this->PruneFilter->Update();
  this->PrunedTree = this->PruneFilter->GetOutput();
  this->JustCollapsedOrExpanded = true;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ExpandSubTree(vtkIdType vertex)
{
  // mark this vertex as "not pruned"
  vtkUnsignedIntArray *vertexIsPruned = vtkUnsignedIntArray::SafeDownCast(
    this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  vtkIdType vertexOriginalId = this->GetOriginalId(vertex);
  vertexIsPruned->SetValue(vertexOriginalId, 0);

  // momentarily revert PrunedTree to the full (unpruned) Tree.
  this->PrunedTree->DeepCopy(this->Tree);

  // re-prune as necessary.  this->Tree has the list of originalIds that
  // need to be re-pruned.
  for (vtkIdType originalId = 0;
       originalId < vertexIsPruned->GetNumberOfTuples(); ++originalId)
    {
    if (vertexIsPruned->GetValue(originalId) > 0)
      {
      // Find PrunedTree's vertex that corresponds to this originalId.
      // Use this to re-collapse the subtrees that were not just expanded.
      vtkIdTypeArray *originalIdArray = vtkIdTypeArray::SafeDownCast(
        this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
      for (vtkIdType prunedId = 0;
           prunedId < originalIdArray->GetNumberOfTuples(); ++prunedId)
        {
        if (originalIdArray->GetValue(prunedId) == originalId)
          {
          this->CollapseSubTree(prunedId);
          break;
          }
        }
      }
    }
  this->JustCollapsedOrExpanded = true;
}

//-----------------------------------------------------------------------------
vtkIdType vtkTreeHeatmapItem::GetOriginalId(vtkIdType vertex)
{
  vtkIdTypeArray *originalIdArray = vtkIdTypeArray::SafeDownCast(
    this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  return originalIdArray->GetValue(vertex);
}

//-----------------------------------------------------------------------------
vtkIdType vtkTreeHeatmapItem::GetPrunedIdForOriginalId(vtkIdType originalId)
{
  vtkIdTypeArray *originalIdArray = vtkIdTypeArray::SafeDownCast(
    this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  for (vtkIdType i = 0; i < originalIdArray->GetNumberOfTuples(); ++i)
    {
    if (originalIdArray->GetValue(i) == originalId)
      {
      return i;
      }
    }
  return -1;
}

// this struct & class allow us to generate a priority queue of vertices.
struct WeightedVertex
{
  vtkIdType ID;
  double weight;
};
class CompareWeightedVertices
{
  public:
  // Returns true if v2 is higher priority than v1
  bool operator()(WeightedVertex& v1, WeightedVertex& v2)
  {
  if (v1.weight < v2.weight)
    {
    return false;
    }
   return true;
  }
};

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseToNumberOfLeafNodes(unsigned int n)
{
  // check that the number requested is actually smaller than the number of
  // leaf nodes in the tree.
  unsigned int numLeaves = this->CountLeafNodes(this->Tree->GetRoot());
  if (n >= numLeaves)
    {
    vtkWarningMacro( << "n >= total leaf nodes" );
    return;
    }

  // reset pruned tree to contain the entire input tree
  this->PrunedTree->DeepCopy(this->Tree);

  // Initialize a priority queue of vertices based on their weight.
  // Vertices with lower weight (closer to the root) have a higher priority.
  std::priority_queue<WeightedVertex, std::vector<WeightedVertex>,
                      CompareWeightedVertices> queue;
  std::vector<vtkIdType> verticesToCollapse;
  vtkDoubleArray *nodeWeights = vtkDoubleArray::SafeDownCast(
    this->Tree->GetVertexData()->GetAbstractArray("true node weight"));
  if (nodeWeights == NULL)
    {
    nodeWeights = vtkDoubleArray::SafeDownCast(
      this->Tree->GetVertexData()->GetAbstractArray("node weight"));
    }

  // initially, the priority queue contains the children of the root node.
  vtkIdType root = this->Tree->GetRoot();
  for (vtkIdType child = 0; child < this->Tree->GetNumberOfChildren(root);
       ++child)
    {
    vtkIdType childVertex = this->Tree->GetChild(root, child);
    WeightedVertex v = {childVertex, nodeWeights->GetValue(childVertex)};
    queue.push(v);
    }

  // use the priority queue to find the vertices that we should collapse.
  unsigned int numberOfLeafNodesFound = 0;
  while (queue.size() + numberOfLeafNodesFound < n)
    {
    WeightedVertex v = queue.top();
    queue.pop();
    if (this->Tree->GetNumberOfChildren(v.ID) == 0)
      {
      verticesToCollapse.push_back(v.ID);
      ++numberOfLeafNodesFound;
      continue;
      }

    for (vtkIdType child = 0; child < this->Tree->GetNumberOfChildren(v.ID);
         ++child)
      {
      vtkIdType childVertex = this->Tree->GetChild(v.ID, child);
      WeightedVertex v2 = {childVertex, nodeWeights->GetValue(childVertex)};
      queue.push(v2);
      }
    }

  // collapse the vertices that we found.
  for (unsigned int i = 0; i < verticesToCollapse.size(); ++i)
    {
    vtkIdType prunedId = this->GetPrunedIdForOriginalId(verticesToCollapse[i]);
    if (prunedId == -1)
      {
      vtkErrorMacro("prunedId is -1");
      continue;
      }
    this->CollapseSubTree(prunedId);
    }
  while (!queue.empty())
    {
    WeightedVertex v = queue.top();
    queue.pop();
    vtkIdType prunedId = this->GetPrunedIdForOriginalId(v.ID);
    if (prunedId == -1)
      {
      vtkErrorMacro("prunedId is -1");
      continue;
      }
    this->CollapseSubTree(prunedId);
    }
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Tree: " << (this->Tree ? "" : "(null)") << std::endl;
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    this->Tree->PrintSelf(os, indent.GetNextIndent());
    }
  os << "Table: " << (this->Table ? "" : "(null)") << std::endl;
  if (this->Table->GetNumberOfRows() > 0)
    {
    this->Table->PrintSelf(os, indent.GetNextIndent());
    }
}
