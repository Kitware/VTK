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
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraphLayout.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMarkerUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeLayoutStrategy.h"

#include <float.h>
#include <sstream>

vtkStandardNewMacro(vtkTreeHeatmapItem);
vtkCxxSetObjectMacro(vtkTreeHeatmapItem, Tree, vtkTree);
vtkCxxSetObjectMacro(vtkTreeHeatmapItem, Table, vtkTable);

vtkTreeHeatmapItem::vtkTreeHeatmapItem()
{
  this->Tree = 0;
  this->Table = 0;
  this->Multiplier = 100.0;
}

vtkTreeHeatmapItem::~vtkTreeHeatmapItem()
{
  if (this->Tree)
    {
    this->Tree->Delete();
    }

  if (this->Table)
    {
    this->Table->Delete();
    }
}

bool vtkTreeHeatmapItem::Paint(vtkContext2D *painter)
{
  if (!this->Tree || !this->Table)
    {
    return true;
    }

  if (this->IsDirty())
    {
    this->RebuildBuffers();
    }

  this->PaintBuffers(painter);
  return true;
}

bool vtkTreeHeatmapItem::IsDirty()
{
  if (!this->Tree || !this->Table)
    {
    return false;
    }
  if (this->Tree->GetMTime() > this->TreeHeatmapBuildTime ||
      this->Table->GetMTime() > this->GetMTime())
  
    {
    return true;
    }
  return false;
}

void vtkTreeHeatmapItem::RebuildBuffers()
{
  vtkNew<vtkTreeLayoutStrategy> strategy;
  strategy->SetDistanceArrayName("node weight");
  strategy->SetLeafSpacing(1.0);
  strategy->SetRotation(90.0);

  this->Layout->SetLayoutStrategy(strategy.GetPointer());
  this->Layout->SetInputData(this->Tree);
  this->Layout->Update();
  this->LayoutTree = vtkTree::SafeDownCast(this->Layout->GetOutput());

  this->ComputeMultiplier();
  this->InitializeLookupTable();

  if(this->Tree->GetMTime() > this->Table->GetMTime())
    {
    this->TreeHeatmapBuildTime = this->Tree->GetMTime();
    }
  else
    {
    this->TreeHeatmapBuildTime = this->Table->GetMTime();
    }
}
  
// Figure out the multiplier we need to use so that the table cells are large
// enough to be labeled.  Currently, we require the boxes to be big enough so
// an 18 pt font can be used.
void vtkTreeHeatmapItem::ComputeMultiplier()
{
  double targetFontSize = 18;
  double yMax = DBL_MIN; 
  double targetPoint[3];
  for (vtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
    {
    vtkIdType target = this->LayoutTree->GetTargetVertex(edge);
    this->LayoutTree->GetPoint(target, targetPoint);
    if (targetPoint[1] > yMax)
      {
      yMax = targetPoint[1];
      }
    }
  double currentFontSize =
    (yMax * this->Multiplier) / this->Table->GetNumberOfRows();
  if (currentFontSize < targetFontSize)
    {
    this->Multiplier = (this->Table->GetNumberOfRows() * targetFontSize) / yMax;
    }
}

void vtkTreeHeatmapItem::InitializeLookupTable()
{
  double min = DBL_MAX;
  double max = DBL_MIN;
  int numberOfValues = (this->Table->GetNumberOfColumns() -1) *
                       this->Table->GetNumberOfRows();
      
  for (vtkIdType row = 0; row < this->Table->GetNumberOfRows(); ++row)
    {
    for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
         ++column)
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
    }

  this->LookupTable->SetNumberOfTableValues(256);
  this->LookupTable->SetRange(min, max);
  this->LookupTable->Build();
  this->LookupTable->SetTableValue(0, 1, 0, 0);
  this->LookupTable->SetTableValue(128, 0.5, 0.5, 0.5);
  this->LookupTable->SetTableValue(255, 0, 1, 0);
}

void vtkTreeHeatmapItem::PaintBuffers(vtkContext2D *painter)
{
  double yMax = DBL_MIN; 
  double xMax = DBL_MIN; 
  double sourcePoint[3];
  double targetPoint[3];

  for (vtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
    {
    vtkIdType source = this->LayoutTree->GetSourceVertex(edge);
    vtkIdType target = this->LayoutTree->GetTargetVertex(edge);

    this->LayoutTree->GetPoint(source, sourcePoint);
    this->LayoutTree->GetPoint(target, targetPoint);

    double x0 = sourcePoint[0] * this->Multiplier;
    double y0 = sourcePoint[1] * this->Multiplier;
    double x1 = targetPoint[0] * this->Multiplier; 
    double y1 = targetPoint[1] * this->Multiplier;

    painter->DrawLine (x0, y0, x0, y1);
    painter->DrawLine (x0, y1, x1, y1);

    if (x1 > xMax)
      {
      xMax = x1;
      }
    if (y1 > yMax)
      {
      yMax = y1;
      }
    }

  // calculate how large our table cells will be when they are drawn
  double cellHeight = yMax / this->Table->GetNumberOfRows();
  double cellWidth; 
  if (cellHeight * 2 > 100)
    {
    cellWidth = 100;
    }
  else
    {
    cellWidth = cellHeight * 2;
    }
  
  // leave a small amount of space between the tree, the table,
  // and the row/column labels
  double spacing = cellWidth * 0.25;

  vtkStringArray *nodeNames = vtkStringArray::SafeDownCast(
    this->LayoutTree->GetVertexData()->GetAbstractArray("node name"));
  vtkStringArray *tableNames = vtkStringArray::SafeDownCast(
    this->Table->GetColumnByName("name"));

  // set up our text property to draw row names
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(floor(cellHeight));
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetOrientation(0);
  
  double xStart, yStart;
  double yMaxTable = DBL_MIN;
  
  for (vtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices();
       ++vertex)
    {
    if (!this->LayoutTree->IsLeaf(vertex))
      {
      continue;
      }

    // find the row in the table that corresponds to this vertex
    double point[3];
    this->LayoutTree->GetPoint(vertex, point);
    std::string nodeName = nodeNames->GetValue(vertex);
    vtkIdType tableRow = tableNames->LookupValue(nodeName);

    for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
         ++column)
      {
      // get the color for this cell from the lookup table
      vtkVariant value = this->Table->GetValue(tableRow, column);
      double color[3];
      this->LookupTable->GetColor(value.ToDouble(), color);
      painter->GetBrush()->SetColorF(color[0], color[1], color[2]);

      // draw this cell of the table
      xStart = xMax + spacing + cellWidth * (column - 1);
      yStart = point[1] * this->Multiplier - (cellHeight / 2);
      painter->DrawRect(xStart, yStart, cellWidth, cellHeight);

      // keep track of where the top of the table is, so we know where to 
      // draw the column labels later.
      if (yStart + cellHeight > yMaxTable)
        {
        yMaxTable = yStart + cellHeight;
        }
      }

    // draw the label for this row
    xStart = xMax + spacing * 2 +
      cellWidth * (this->Table->GetNumberOfColumns() - 1);
    yStart = point[1] * this->Multiplier;
    painter->DrawString(xStart, yStart, nodeName);
    }
  
  // draw column labels
  painter->GetTextProp()->SetOrientation(90);
  for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
       ++column)
    {
      std::string columnName = this->Table->GetColumn(column)->GetName();
      xStart = xMax + spacing + cellWidth * column - cellWidth / 2;
      yStart = yMaxTable + spacing;
      painter->DrawString(xStart, yStart, columnName);
    }
}

void vtkTreeHeatmapItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Tree: " << (this->Tree ? "" : "(null)") << std::endl;
  if (this->Tree)
    {
    this->Tree->PrintSelf(os, indent.GetNextIndent());
    }
  os << "Table: " << (this->Table ? "" : "(null)") << std::endl;
  if (this->Table)
    {
    this->Table->PrintSelf(os, indent.GetNextIndent());
    }
}
