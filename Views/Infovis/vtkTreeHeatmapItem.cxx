/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTreeHeatmapItem.h"
#include "vtkDendrogramItem.h"
#include "vtkHeatmapItem.h"

#include "vtkDataSetAttributes.h"
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"

#include <algorithm>

vtkStandardNewMacro(vtkTreeHeatmapItem);

//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::vtkTreeHeatmapItem()
{
  this->Interactive = true;
  this->Orientation = vtkDendrogramItem::LEFT_TO_RIGHT;
  this->TreeHeatmapBuildTime = 0;

  this->Dendrogram = vtkSmartPointer<vtkDendrogramItem>::New();
  this->Dendrogram->ExtendLeafNodesOn();
  this->Dendrogram->SetVisible(false);
  this->AddItem(this->Dendrogram);

  this->ColumnDendrogram = vtkSmartPointer<vtkDendrogramItem>::New();
  this->ColumnDendrogram->ExtendLeafNodesOn();
  this->ColumnDendrogram->SetVisible(false);
  this->ColumnDendrogram->SetDrawLabels(false);
  this->AddItem(this->ColumnDendrogram);

  this->Heatmap = vtkSmartPointer<vtkHeatmapItem>::New();
  this->Heatmap->SetVisible(false);
  this->AddItem(this->Heatmap);

  this->ColumnDendrogram->SetLeafSpacing(this->Heatmap->GetCellWidth());
}

//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::~vtkTreeHeatmapItem()
{
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTree(vtkTree *tree)
{
  this->Dendrogram->SetTree(tree);
  if (tree == NULL)
    {
    return;
    }

  if (this->GetTable() != NULL &&
      this->GetTable()->GetNumberOfRows() != 0)
    {
    this->Dendrogram->SetDrawLabels(false);
    }
  this->Dendrogram->SetVisible(true);

  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTable() != NULL && this->GetTable()->GetNumberOfRows() != 0)
    {
    this->ReorderTable();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTable(vtkTable *table)
{
  this->Heatmap->SetTable(table);
  if (table == NULL)
    {
    return;
    }

  if (this->Dendrogram->GetTree() != NULL &&
      this->Dendrogram->GetTree()->GetNumberOfVertices() != 0)
    {
    this->Dendrogram->SetDrawLabels(false);
    }
  this->Heatmap->SetVisible(true);


  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTree() != NULL && this->GetTree()->GetNumberOfVertices() != 0)
    {
    this->ReorderTable();
    }

  // add an array to this table's field data to keep track of collapsed rows
  // (unless it already has the array)
  vtkBitArray *existingRowsArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed rows"));
  if (existingRowsArray)
    {
    for(vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
      {
      existingRowsArray->SetValue(row, 0);
      }
    }
  else
    {
    vtkSmartPointer<vtkBitArray> collapsedRowsArray =
      vtkSmartPointer<vtkBitArray>::New();
    collapsedRowsArray->SetNumberOfComponents(1);
    collapsedRowsArray->SetName("collapsed rows");
    for(vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
      {
      collapsedRowsArray->InsertNextValue(0);
      }
    this->GetTable()->GetFieldData()->AddArray(collapsedRowsArray);
    }

  // add an array to this table's field data to keep track of collapsed columns
  // (unless it already has the array)
  vtkBitArray *existingColumnsArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed columns"));
  if (existingColumnsArray)
    {
    for(vtkIdType col = 0; col < this->GetTable()->GetNumberOfColumns(); ++col)
      {
      existingColumnsArray->SetValue(col, 0);
      }
    }
  else
    {
    vtkSmartPointer<vtkBitArray> collapsedColumnsArray =
      vtkSmartPointer<vtkBitArray>::New();
    collapsedColumnsArray->SetNumberOfComponents(1);
    collapsedColumnsArray->SetName("collapsed columns");
    for(vtkIdType col = 0; col < this->GetTable()->GetNumberOfColumns(); ++col)
      {
      collapsedColumnsArray->InsertNextValue(0);
      }
    this->GetTable()->GetFieldData()->AddArray(collapsedColumnsArray);
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetColumnTree(vtkTree *tree)
{
  this->ColumnDendrogram->SetTree(tree);
  if (tree == NULL)
    {
    return;
    }

  if (this->Orientation == vtkDendrogramItem::LEFT_TO_RIGHT ||
      this->Orientation == vtkDendrogramItem::LEFT_TO_RIGHT)
    {
    this->ColumnDendrogram->SetOrientation(vtkDendrogramItem::UP_TO_DOWN);
    }
  else
    {
    this->ColumnDendrogram->SetOrientation(vtkDendrogramItem::RIGHT_TO_LEFT);
    }

  this->ColumnDendrogram->SetVisible(true);
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetColumnTree()
{
  return this->ColumnDendrogram->GetTree();
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDendrogramItem> vtkTreeHeatmapItem::GetDendrogram()
{
  return this->Dendrogram;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetDendrogram(vtkSmartPointer<vtkDendrogramItem> item)
{
  this->Dendrogram = item;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkHeatmapItem> vtkTreeHeatmapItem::GetHeatmap()
{
  return this->Heatmap;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetHeatmap(vtkSmartPointer<vtkHeatmapItem> item)
{
  this->Heatmap = item;
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetTree()
{
  return this->Dendrogram->GetTree();
}

//-----------------------------------------------------------------------------
vtkTable * vtkTreeHeatmapItem::GetTable()
{
  return this->Heatmap->GetTable();
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ReorderTable()
{
  // make a copy of our table and then empty out the original.
  vtkNew<vtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (vtkIdType row = this->GetTable()->GetNumberOfRows() - 1; row > -1; --row)
    {
    this->GetTable()->RemoveRow(row);
    }

  // get the names of the vertices in our tree.
  vtkStringArray *vertexNames = vtkStringArray::SafeDownCast(
    this->GetTree()->GetVertexData()->GetAbstractArray("node name"));

  // get array of row names from the table.  We assume this is the first row.
  vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
    tableCopy->GetColumn(0));

  for (vtkIdType vertex = 0; vertex < this->GetTree()->GetNumberOfVertices();
       ++vertex)
    {
    if (!this->GetTree()->IsLeaf(vertex))
      {
      continue;
      }

    // find the row in the table that corresponds to this vertex
    std::string vertexName = vertexNames->GetValue(vertex);
    vtkIdType tableRow = rowNames->LookupValue(vertexName);
    if (tableRow < 0)
      {
      vtkIdType newRowNum = this->GetTable()->InsertNextBlankRow();
      this->GetTable()->SetValue(newRowNum, 0, vtkVariant(vertexName));
      this->Heatmap->MarkRowAsBlank(vertexName);
      continue;
      }

    // copy it back into our original table
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
    }

  if (this->Orientation == vtkDendrogramItem::DOWN_TO_UP ||
      this->Orientation == vtkDendrogramItem::UP_TO_DOWN)
    {
    this->ReverseTableColumns();
    }
  if (this->Orientation == vtkDendrogramItem::RIGHT_TO_LEFT ||
      this->Orientation == vtkDendrogramItem::DOWN_TO_UP)
    {
    this->ReverseTableRows();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ReverseTableRows()
{
  // make a copy of our table and then empty out the original.
  vtkNew<vtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (vtkIdType row = 0; row < tableCopy->GetNumberOfRows(); ++row)
    {
    this->GetTable()->RemoveRow(row);
    }

  // re-insert the rows back into our original table in reverse order
  for (vtkIdType tableRow = tableCopy->GetNumberOfRows() - 1; tableRow >= 0;
       --tableRow)
    {
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ReverseTableColumns()
{
  // make a copy of our table and then empty out the original.
  vtkNew<vtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (vtkIdType col = tableCopy->GetNumberOfColumns() - 1; col > 0; --col)
    {
    this->GetTable()->RemoveColumn(col);
    }

  // re-insert the columns back into our original table in reverse order
  for (vtkIdType col = tableCopy->GetNumberOfColumns() - 1; col >= 1; --col)
    {
    this->GetTable()->AddColumn(tableCopy->GetColumn(col));
    }
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::Paint(vtkContext2D *painter)
{
  this->Dendrogram->Paint(painter);

  double treeBounds[4];
  this->Dendrogram->GetBounds(treeBounds);
  double spacing = this->Dendrogram->GetLeafSpacing() / 2.0;

  double heatmapStartX, heatmapStartY;

  switch (this->Orientation)
    {
    case vtkDendrogramItem::UP_TO_DOWN:
      heatmapStartX = treeBounds[0] - spacing;
      heatmapStartY = treeBounds[2] - (this->GetTable()->GetNumberOfColumns() - 1) *
                      this->Heatmap->GetCellWidth() - spacing;
      break;
    case vtkDendrogramItem::DOWN_TO_UP:
      heatmapStartX = treeBounds[0] - spacing;
      heatmapStartY = treeBounds[3] + spacing;
      break;
    case vtkDendrogramItem::RIGHT_TO_LEFT:
      heatmapStartX = treeBounds[0] - (this->GetTable()->GetNumberOfColumns() - 1) *
                      this->Heatmap->GetCellWidth() - spacing;
      heatmapStartY = treeBounds[2] - spacing;
      break;
    case vtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      heatmapStartX = treeBounds[1] + spacing;
      heatmapStartY = treeBounds[2] - spacing;
      break;
    }
  this->Heatmap->SetPosition(heatmapStartX, heatmapStartY);
  this->Heatmap->Paint(painter);

  if (this->ColumnDendrogram->GetVisible())
    {
    double columnTreeStartX, columnTreeStartY;

    double heatmapBounds[4];
    this->Heatmap->GetBounds(heatmapBounds);

    this->ColumnDendrogram->PrepareToPaint(painter);
    this->ColumnDendrogram->GetBounds(treeBounds);

    float offset = 0.0;
    if (this->Heatmap->GetRowLabelWidth() > 0.0)
      {
      offset = this->Heatmap->GetRowLabelWidth() + spacing;
      }
    switch (this->Orientation)
      {
      case vtkDendrogramItem::UP_TO_DOWN:
        columnTreeStartX = heatmapBounds[1] + (treeBounds[1] - treeBounds[0]) +
          spacing;
        columnTreeStartY = heatmapBounds[3] -
          this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        break;
      case vtkDendrogramItem::DOWN_TO_UP:
        columnTreeStartX = heatmapBounds[1] + (treeBounds[1] - treeBounds[0]) +
          spacing;
        columnTreeStartY = heatmapBounds[3] - offset -
          this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        break;
      case vtkDendrogramItem::RIGHT_TO_LEFT:
        columnTreeStartX = heatmapBounds[0] + offset +
          this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        columnTreeStartY = heatmapBounds[3] + spacing +
          (treeBounds[3] - treeBounds[2]);
        break;
      case vtkDendrogramItem::LEFT_TO_RIGHT:
      default:
        columnTreeStartX = heatmapBounds[0] +
          this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        columnTreeStartY = heatmapBounds[3] + spacing +
          (treeBounds[3] - treeBounds[2]);
        break;
      }

    this->ColumnDendrogram->SetPosition(columnTreeStartX, columnTreeStartY);
    this->ColumnDendrogram->Paint(painter);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::MouseDoubleClickEvent(
  const vtkContextMouseEvent &event)
{
  bool treeChanged = this->Dendrogram->MouseDoubleClickEvent(event);

  // update the heatmap if a subtree just collapsed or expanded.
  if (treeChanged)
    {
    this->CollapseHeatmapRows();
    }
  else
    {
    treeChanged = this->ColumnDendrogram->MouseDoubleClickEvent(event);
    if (treeChanged)
      {
      this->CollapseHeatmapColumns();
      }
    }
  return treeChanged;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseHeatmapRows()
{
  vtkBitArray *collapsedRowsArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed rows"));

  vtkStringArray *vertexNames = vtkStringArray::SafeDownCast(
    this->Dendrogram->GetPrunedTree()->GetVertexData()
    ->GetAbstractArray("node name"));

  vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
    this->GetTable()->GetColumn(0));

  for (vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
    {
    std::string name = rowNames->GetValue(row);
    // if we can't find this name in the layout tree, then the corresponding
    // row in the heatmap should be marked as collapsed.
    if (vertexNames->LookupValue(name) == -1)
      {
      collapsedRowsArray->SetValue(row, 1);
      }
    else
      {
      collapsedRowsArray->SetValue(row, 0);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseHeatmapColumns()
{
  vtkBitArray *collapsedColumnsArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed columns"));

  vtkStringArray *vertexNames = vtkStringArray::SafeDownCast(
    this->ColumnDendrogram->GetPrunedTree()->GetVertexData()
    ->GetAbstractArray("node name"));

  for (vtkIdType col = 1; col < this->GetTable()->GetNumberOfColumns(); ++col)
    {
    std::string name = this->GetTable()->GetColumn(col)->GetName();

    // if we can't find this name in the layout tree, then the corresponding
    // column in the heatmap should be marked as collapsed.
    if (vertexNames->LookupValue(name) == -1)
      {
      collapsedColumnsArray->SetValue(col, 1);
      }
    else
      {
      collapsedColumnsArray->SetValue(col, 0);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetOrientation(int orientation)
{
  int previousOrientation = this->Orientation;
  this->Orientation = orientation;
  this->Dendrogram->SetOrientation(this->Orientation);
  this->Heatmap->SetOrientation(this->Orientation);

  if (this->Orientation == vtkDendrogramItem::LEFT_TO_RIGHT ||
      this->Orientation == vtkDendrogramItem::RIGHT_TO_LEFT)
    {
    this->ColumnDendrogram->SetOrientation(vtkDendrogramItem::UP_TO_DOWN);
    }
  else
    {
    this->ColumnDendrogram->SetOrientation(vtkDendrogramItem::RIGHT_TO_LEFT);
    }

  // reverse our table if we're changing from a "not backwards" orientation
  // to one that it backwards.
  if ( (this->Orientation == vtkDendrogramItem::UP_TO_DOWN ||
        this->Orientation == vtkDendrogramItem::DOWN_TO_UP) &&
       (previousOrientation != vtkDendrogramItem::UP_TO_DOWN &&
        previousOrientation != vtkDendrogramItem::DOWN_TO_UP) )
    {
    this->ReverseTableColumns();
    }
  if ( (this->Orientation == vtkDendrogramItem::RIGHT_TO_LEFT ||
        this->Orientation == vtkDendrogramItem::DOWN_TO_UP) &&
       (previousOrientation != vtkDendrogramItem::RIGHT_TO_LEFT &&
        previousOrientation != vtkDendrogramItem::DOWN_TO_UP) )
    {
    this->ReverseTableRows();
    }
}

//-----------------------------------------------------------------------------
int vtkTreeHeatmapItem::GetOrientation()
{
  return this->Orientation;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetBounds(double bounds[4])
{
  double treeBounds[4] =
    {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  if (this->GetTree()->GetNumberOfVertices() > 0)
    {
    this->Dendrogram->GetBounds(treeBounds);
    }

  double tableBounds[4] =
    {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  if (this->GetTable()->GetNumberOfRows() > 0)
    {
    this->Heatmap->GetBounds(tableBounds);
    }

  double columnTreeBounds[4] =
    {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  if (this->ColumnDendrogram->GetTree() != NULL)
    {
    this->ColumnDendrogram->GetBounds(columnTreeBounds);
    }

  double xMin, xMax, yMin, yMax;

  xMin = std::min(std::min(treeBounds[0], tableBounds[0]), columnTreeBounds[0]);
  xMax = std::max(std::max(treeBounds[1], tableBounds[1]), columnTreeBounds[1]);
  yMin = std::min(std::min(treeBounds[2], tableBounds[2]), columnTreeBounds[2]);
  yMax = std::max(std::max(treeBounds[3], tableBounds[3]), columnTreeBounds[3]);

  bounds[0] = xMin;
  bounds[1] = xMax;
  bounds[2] = yMin;
  bounds[3] = yMax;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetCenter(double center[2])
{
  double bounds[4];
  this->GetBounds(bounds);

  center[0] = bounds[0] + (bounds[1] - bounds[0]) / 2.0;
  center[1] = bounds[2] + (bounds[3] - bounds[2]) / 2.0;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetSize(double size[2])
{
  double bounds[4];
  this->GetBounds(bounds);

  size[0] = abs(bounds[1] - bounds[0]);
  size[1] = abs(bounds[3] - bounds[2]);
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTreeColorArray(const char *arrayName)
{
  this->Dendrogram->SetColorArray(arrayName);
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseToNumberOfLeafNodes(unsigned int n)
{
  this->Dendrogram->CollapseToNumberOfLeafNodes(n);
  this->CollapseHeatmapRows();
}

//-----------------------------------------------------------------------------
float vtkTreeHeatmapItem::GetTreeLineWidth()
{
  return this->Dendrogram->GetLineWidth();
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTreeLineWidth(float width)
{
  this->Dendrogram->SetLineWidth(width);
  this->ColumnDendrogram->SetLineWidth(width);
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetPrunedTree()
{
  return this->Dendrogram->GetPrunedTree();
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
  this->Dendrogram->PrintSelf(os, indent);
  this->Heatmap->PrintSelf(os, indent);
}
