/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHeatmapItem.h"

#include "vtkBitArray.h"
#include "vtkBrush.h"
#include "vtkColorSeries.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTooltipItem.h"
#include "vtkTransform2D.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkHeatmapItem);

//-----------------------------------------------------------------------------
vtkHeatmapItem::vtkHeatmapItem() : PositionVector(0, 0)
{
  this->Position = this->PositionVector.GetData();
  this->Interactive = true;
  this->HeatmapBuildTime = 0;
  this->Table = vtkSmartPointer<vtkTable>::New();

  /* initialize bounds so that the mouse cursor is never considered
   * "inside" the heatmap */
  this->MinX = 1.0;
  this->MinY = 1.0;
  this->MaxX = 0.0;
  this->MaxY = 0.0;

  this->CellHeight = 18.0;
  this->CellWidth = this->CellHeight * 2.0;

  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip.GetPointer());
}

//-----------------------------------------------------------------------------
vtkHeatmapItem::~vtkHeatmapItem()
{
}


//-----------------------------------------------------------------------------
void vtkHeatmapItem::SetPosition(const vtkVector2f &pos)
{
  this->PositionVector = pos;
}

//-----------------------------------------------------------------------------
vtkVector2f vtkHeatmapItem::GetPositionVector()
{
  return this->PositionVector;
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::SetTable(vtkTable *table)
{
  if (table == NULL || table->GetNumberOfRows() == 0)
    {
    this->Table = vtkSmartPointer<vtkTable>::New();
    return;
    }
  this->Table = table;
}

//-----------------------------------------------------------------------------
vtkTable * vtkHeatmapItem::GetTable()
{
  return this->Table;
}

//-----------------------------------------------------------------------------
bool vtkHeatmapItem::Paint(vtkContext2D *painter)
{
  if (this->Table->GetNumberOfRows() == 0)
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
bool vtkHeatmapItem::IsDirty()
{
  if (this->Table->GetNumberOfRows() == 0)
    {
    return false;
    }
  if (this->Table->GetMTime() > this->HeatmapBuildTime)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::RebuildBuffers()
{
  if (this->Table->GetNumberOfRows() == 0)
    {
    return;
    }

  this->InitializeLookupTables();
  this->HeatmapBuildTime = this->Table->GetMTime();
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::InitializeLookupTables()
{
  this->ColumnRanges.clear();
  this->CategoricalDataValues->Reset();

  for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
       ++column)
    {
    if (this->Table->GetValue(0, column).IsString())
      {
      this->AccumulateProminentCategoricalDataValues(column);
      continue;
      }
    double min = VTK_DOUBLE_MAX;
    double max = VTK_DOUBLE_MIN;
    for (vtkIdType row = 0; row < this->Table->GetNumberOfRows(); ++row)
      {
      double value = this->Table->GetValue(row, column).ToDouble();
      if (value > max)
        {
        max = value;
        }
      if (value < min)
        {
        min = value;
        }
      }
    this->ColumnRanges[column] = std::pair<double, double>(min, max);
    }

  this->GenerateCategoricalDataLookupTable();
  this->GenerateContinuousDataLookupTable();
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::GenerateContinuousDataLookupTable()
{
  this->ContinuousDataLookupTable->SetNumberOfTableValues(255);
  this->ContinuousDataLookupTable->Build();
  this->ContinuousDataLookupTable->SetRange(0, 255);
  this->ContinuousDataLookupTable->SetNanColor(0.75, 0.75, 0.75, 1.0);

  // black to red
  for (int i = 0; i < 85; ++i)
    {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(i, f, 0, 0);
    }

 // red to yellow
  for (int i = 0; i < 85; ++i)
    {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(85 + i, 1.0, f, 0);
    }

 // yellow to white
  for (int i = 0; i < 85; ++i)
    {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(170 + i, 1.0, 1.0, f);
    }
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::AccumulateProminentCategoricalDataValues(vtkIdType column)
{
  vtkStringArray *stringColumn = vtkStringArray::SafeDownCast(
    this->Table->GetColumn(column));

  // add each distinct value from this column to our master list
  vtkNew<vtkVariantArray> distinctValues;
  stringColumn->SetMaxDiscreteValues(stringColumn->GetNumberOfTuples() - 1);
  stringColumn->GetProminentComponentValues(0, distinctValues.GetPointer());

  for (int i = 0; i < distinctValues->GetNumberOfTuples(); ++i)
    {
    vtkVariant v = distinctValues->GetValue(i);
    if (this->CategoricalDataValues->LookupValue(v) == -1)
      {
      this->CategoricalDataValues->InsertNextValue(v.ToString());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::GenerateCategoricalDataLookupTable()
{
  this->CategoricalDataLookupTable->ResetAnnotations();
  this->CategoricalDataLookupTable->SetNanColor(0.75, 0.75, 0.75, 1.0);

  // make each distinct categorical value an index into our lookup table
  for (int i = 0; i < this->CategoricalDataValues->GetNumberOfTuples(); ++i)
    {
    this->CategoricalDataLookupTable->SetAnnotation(
      this->CategoricalDataValues->GetValue(i),
      this->CategoricalDataValues->GetValue(i));
    }

  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(vtkColorSeries::BREWER_QUALITATIVE_SET3);
  colorSeries->BuildLookupTable(this->CategoricalDataLookupTable.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::PaintBuffers(vtkContext2D *painter)
{
  // Calculate the extent of the data that is visible within the window.
  this->UpdateVisibleSceneExtent(painter);

  // leave a small amount of space between the heatmap and the row/column
  // labels
  double spacing = this->CellWidth * 0.25;

  // variables used to calculate the positions of elements drawn on screen.
  double cellStartX = 0.0;
  double cellStartY = 0.0;
  double labelStartX = 0.0;
  double labelStartY = 0.0;

  // the name of each row.
  vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
    this->Table->GetColumn(0));

  // whether or not each row has been collapsed.
  vtkBitArray *collapsedRowsArray = vtkBitArray::SafeDownCast(
    this->Table->GetFieldData()->GetArray("collapsed rows"));
  bool currentlyCollapsing = false;

  // this map helps us display information about the correct row
  // in our tooltips
  this->SceneRowToTableRowMap.clear();
  this->SceneRowToTableRowMap.assign(this->Table->GetNumberOfRows(), -1);

  // Setup text property & calculate an appropriate font size for this zoom
  // level.  "Igq" was selected for the range of height of its characters.
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetOrientation(0.0);
  int fontSize = painter->ComputeFontSizeForBoundedString("Igq", VTK_FLOAT_MAX,
                                                          this->CellHeight);
  //canDrawText is set to false if we're too zoomed out to draw legible text.
  bool canDrawText = true;
  if (fontSize < 8)
    {
    canDrawText = false;
    }
  bool drawRowLabels = canDrawText;

  // Handle as many orientation issues as possible outside of the nested
  // for loops below.
  //
  // 1) Determine the fixed bounds of the heatmap here.  The "row" dimension
  //    is fixed, but the "column" dimension can vary due to collapsed rows.
  //
  // 2) Once we know the fixed bounds of our heatmap, we can detect whether
  //    or not our row labels would be currently visible on screen.  If
  //    they're not on screen, then we shouldn't bother drawing them later.
  bool variableY = true;
  int orientation = this->GetOrientation();
  switch (orientation)
    {
    case vtkHeatmapItem::DOWN_TO_UP:
      // 1)
      this->MinX = VTK_DOUBLE_MAX;
      this->MaxX = VTK_DOUBLE_MIN;
      this->MinY = this->Position[1];
      this->MaxY = this->Position[1] +
        this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
      variableY = false;

      // 2)
      if (drawRowLabels &&
          (this->SceneBottomLeft[1] > this->MaxY + spacing ||
           this->SceneTopRight[1] < this->MaxY + spacing))
        {
        drawRowLabels = false;
        }
      break;

    case vtkHeatmapItem::RIGHT_TO_LEFT:
      // 1)
      this->MinX = this->Position[0];
      this->MaxX = this->Position[0] +
        this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
      this->MinY = VTK_DOUBLE_MAX;
      this->MaxY = VTK_DOUBLE_MIN;

      // 2)
      if (drawRowLabels &&
          (this->SceneBottomLeft[0] > this->MinX - spacing ||
           this->SceneTopRight[0] < this->MinX - spacing))
        {
        drawRowLabels = false;
        }
      else
        {
        painter->GetTextProp()->SetJustificationToRight();
        }
      break;

    case vtkHeatmapItem::UP_TO_DOWN:
      // 1)
      this->MinX = VTK_DOUBLE_MAX;
      this->MaxX = VTK_DOUBLE_MIN;
      this->MinY = this->Position[1];
      this->MaxY = this->Position[1] +
        this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
      variableY = false;

      // 2)
      if (drawRowLabels &&
          (this->SceneBottomLeft[1] > this->MinY - spacing ||
           this->SceneTopRight[1] < this->MinY - spacing))
        {
        drawRowLabels = false;
        }
      else
        {
        painter->GetTextProp()->SetJustificationToRight();
        }
      break;

    case vtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      // 1)
      this->MinX = this->Position[0];
      this->MaxX = this->Position[0] +
        this->CellWidth * (this->Table->GetNumberOfColumns() - 1);
      this->MinY = VTK_DOUBLE_MAX;
      this->MaxY = VTK_DOUBLE_MIN;

      // 2)
      if (drawRowLabels &&
          (this->SceneBottomLeft[0] > this->MaxX + spacing ||
           this->SceneTopRight[0] < this->MaxX + spacing))
        {
        drawRowLabels = false;
        }
      break;
    }

  // set the orientation of our text property to draw row names
  if (drawRowLabels)
    {
    painter->GetTextProp()->SetOrientation(
      this->GetTextAngleForOrientation(orientation));
    }

  // keep track of what row we're drawing next
  vtkIdType rowToDraw = 0;

  for (vtkIdType row = 0; row != this->Table->GetNumberOfRows(); ++row)
    {
    // check if this row has been collapsed or not
    if (collapsedRowsArray && collapsedRowsArray->GetValue(row) == 1)
      {
      // a contiguous block of collapsed rows is represented as a single blank
      // row by this item.
      if (!currentlyCollapsing)
        {
        this->SceneRowToTableRowMap[rowToDraw] = -1;
        ++rowToDraw;
        currentlyCollapsing = true;
        }
      continue;
      }
    currentlyCollapsing = false;

    // get the name of this row
    std::string name = rowNames->GetValue(row);

    // only draw the cells of this row if it isn't explicitly marked as blank
    if (this->BlankRows.find(name) == this->BlankRows.end())
      {
      for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
           ++column)
        {
        // get the color for this cell from the lookup table
        double color[4];
        vtkVariant value = this->Table->GetValue(row, column);
        if (value.IsString())
          {
          this->CategoricalDataLookupTable->GetAnnotationColor(value, color);
          }
        else
          {
          // set the range on our continuous lookup table for this column
          this->ContinuousDataLookupTable->SetRange(
            this->ColumnRanges[column].first,
            this->ColumnRanges[column].second);

          // get the color for this value
          this->ContinuousDataLookupTable->GetColor(value.ToDouble(), color);
          }
        painter->GetBrush()->SetColorF(color[0], color[1], color[2]);

        // draw this cell of the table
        double w = 0.0;
        double h = 0.0;
        switch(orientation)
          {
          case vtkHeatmapItem::DOWN_TO_UP:
            cellStartX = this->Position[0] + this->CellHeight * rowToDraw;
            cellStartY = this->MinY + this->CellWidth * (column - 1);
            h = this->CellWidth;
            w = this->CellHeight;
            break;

          case vtkHeatmapItem::RIGHT_TO_LEFT:
            cellStartX = this->MinX + this->CellWidth * (column - 1);
            cellStartY = this->Position[1] + this->CellHeight * rowToDraw;
            w = this->CellWidth;
            h = this->CellHeight;
            break;

          case vtkHeatmapItem::UP_TO_DOWN:
            cellStartX = this->Position[0] + this->CellHeight * rowToDraw;
            cellStartY = this->MinY + this->CellWidth * (column - 1);
            h = this->CellWidth;
            w = this->CellHeight;
            break;

          case vtkHeatmapItem::LEFT_TO_RIGHT:
          default:
            cellStartX = this->MinX + this->CellWidth * (column - 1);
            cellStartY = this->Position[1] + this->CellHeight * rowToDraw;
            w = this->CellWidth;
            h = this->CellHeight;
            break;
          }

        if (this->LineIsVisible(cellStartX, cellStartY, cellStartX + this->CellWidth,
                                cellStartY + this->CellHeight) ||
            this->LineIsVisible(cellStartX, cellStartY + this->CellHeight,
                                cellStartX + this->CellWidth, cellStartY))
          {
          painter->DrawRect(cellStartX, cellStartY, w, h);
          }

        }
      }
    else
      {
      switch(orientation)
        {
        case vtkHeatmapItem::DOWN_TO_UP:
        case vtkHeatmapItem::UP_TO_DOWN:
          cellStartX = this->Position[0] + this->CellHeight * rowToDraw;
          break;

        case vtkHeatmapItem::RIGHT_TO_LEFT:
        case vtkHeatmapItem::LEFT_TO_RIGHT:
        default:
          cellStartY = this->Position[1] + this->CellHeight * rowToDraw;
          break;
        }
      }
    // keep track of where the edges of the table are.
    // this is used to position column labels and tool tips.
    if (variableY)
      {
      if (cellStartY + this->CellHeight > this->MaxY)
        {
        this->MaxY = cellStartY + this->CellHeight;
        }
      if (cellStartY < this->MinY)
        {
        this->MinY = cellStartY;
        }
      }
    else
      {
      if (cellStartX + this->CellHeight > this->MaxX)
        {
        this->MaxX = cellStartX + this->CellHeight;
        }
      if (cellStartX < this->MinX)
        {
        this->MinX = cellStartX;
        }
      }

    this->SceneRowToTableRowMap[rowToDraw] = row;
    ++rowToDraw;

    // draw this row's label if it would be visible
    if (!drawRowLabels)
      {
      continue;
      }

    switch (orientation)
      {
      case vtkHeatmapItem::DOWN_TO_UP:
        labelStartX = cellStartX + this->CellHeight / 2.0;
        labelStartY = this->MaxY + spacing;
        break;
      case vtkHeatmapItem::RIGHT_TO_LEFT:
        labelStartX = this->MinX - spacing;
        labelStartY = cellStartY + this->CellHeight / 2.0;
        break;
      case vtkHeatmapItem::UP_TO_DOWN:
        labelStartX = cellStartX + this->CellHeight / 2.0;
        labelStartY = this->MinY - spacing;
        break;
      case vtkHeatmapItem::LEFT_TO_RIGHT:
      default:
        labelStartX = this->MaxX + spacing;
        labelStartY = cellStartY + this->CellHeight / 2.0;
        break;
      }

    if (this->SceneBottomLeft[0] < labelStartX &&
        this->SceneTopRight[0] > labelStartX   &&
        this->SceneBottomLeft[1] < labelStartY &&
        this->SceneTopRight[1] > labelStartY)
      {
      painter->DrawString(labelStartX, labelStartY, name);
      }
    }

  // draw column labels
  if (!canDrawText)
    {
    return;
    }

  // ensure the possibility that some of our column labels would be visible
  // on screen.
  switch (orientation)
    {
    case vtkHeatmapItem::DOWN_TO_UP:
    case vtkHeatmapItem::UP_TO_DOWN:
      if (this->SceneBottomLeft[0] > this->MaxX + spacing ||
          this->SceneTopRight[0] < this->MaxX + spacing)
        {
        return;
        }
      painter->GetTextProp()->SetOrientation(0);
      break;

    case vtkHeatmapItem::RIGHT_TO_LEFT:
    case vtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      if (this->SceneBottomLeft[1] > this->MaxY + spacing &&
          this->SceneTopRight[1] < this->MaxY + spacing)
        {
        return;
        }
      painter->GetTextProp()->SetOrientation(90);
      break;
    }

  painter->GetTextProp()->SetJustificationToLeft();

  for (vtkIdType column = 1; column < this->Table->GetNumberOfColumns();
       ++column)
    {
    switch(orientation)
      {
      case vtkHeatmapItem::DOWN_TO_UP:
      case vtkHeatmapItem::UP_TO_DOWN:
        labelStartX = this->MaxX + spacing;
        labelStartY =
          this->MinY + this->CellWidth * column - this->CellWidth / 2;
        break;

      case vtkHeatmapItem::RIGHT_TO_LEFT:
      case vtkHeatmapItem::LEFT_TO_RIGHT:
      default:
        labelStartX =
          this->MinX + this->CellWidth * column - this->CellWidth / 2;
        labelStartY = this->MaxY + spacing;
        break;
      }

    std::string columnName = this->Table->GetColumn(column)->GetName();
    if (this->SceneBottomLeft[0] < labelStartX &&
        this->SceneTopRight[0] > labelStartX &&
        this->SceneBottomLeft[1] < labelStartY &&
        this->SceneTopRight[1] > labelStartY)
      {
      painter->DrawString(labelStartX, labelStartY, columnName);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::UpdateVisibleSceneExtent(vtkContext2D *painter)
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
bool vtkHeatmapItem::LineIsVisible(double x0, double y0,
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
bool vtkHeatmapItem::MouseMoveEvent(const vtkContextMouseEvent &event)
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
    if (pos[0] <= this->MaxX && pos[0] >= this->MinX &&
        pos[1] <= this->MaxY && pos[1] >= this->MinY)
      {
      this->Tooltip->SetPosition(pos[0], pos[1]);

      std::string tooltipText = this->GetTooltipText(pos[0], pos[1]);
      if (tooltipText.compare("") != 0)
        {
        this->Tooltip->SetText(tooltipText);
        this->Tooltip->SetVisible(true);
        this->Scene->SetDirty(true);
        return true;
        }
      }
    bool shouldRepaint = this->Tooltip->GetVisible();
    this->Tooltip->SetVisible(false);
    if (shouldRepaint)
      {
      this->Scene->SetDirty(true);
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
std::string vtkHeatmapItem::GetTooltipText(float x, float y)
{
  vtkIdType column = 0;
  int sceneRow = 0;
  int orientation = this->GetOrientation();
  if (orientation == vtkHeatmapItem::UP_TO_DOWN ||
      orientation == vtkHeatmapItem::DOWN_TO_UP)
    {
    column = floor((y - this->MinY) / this->CellWidth);
    sceneRow = floor(abs(x - this->Position[0]) / this->CellHeight);
    }
  else
    {
    column = floor((x - this->MinX) / this->CellWidth);
    sceneRow = floor(abs(y - this->Position[1]) / this->CellHeight);
    }

  vtkIdType row = -1;
  if (static_cast<unsigned int>(sceneRow) < this->SceneRowToTableRowMap.size())
    {
    row = this->SceneRowToTableRowMap[sceneRow];
    }

  if (row > -1 && column + 1 < this->Table->GetNumberOfColumns())
    {
    vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
      this->Table->GetColumn(0));
    std::string rowName = rowNames->GetValue(row);
    if (this->BlankRows.find(rowName) != this->BlankRows.end())
      {
      return "";
      }

    std::string columnName = this->Table->GetColumn(column + 1)->GetName();

    std::string tooltipText = "(";
    tooltipText += rowName;
    tooltipText += ", ";
    tooltipText += columnName;
    tooltipText += ")\n";
    tooltipText += this->Table->GetValue(row, column + 1).ToString();

    return tooltipText;
    }
  return "";
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::SetOrientation(int orientation)
{
  vtkIntArray *existingArray = vtkIntArray::SafeDownCast(
    this->Table->GetFieldData()->GetArray("orientation"));
  if (existingArray)
    {
    existingArray->SetValue(0, orientation);
    }
  else
    {
    vtkSmartPointer<vtkIntArray> orientationArray =
      vtkSmartPointer<vtkIntArray>::New();
    orientationArray->SetNumberOfComponents(1);
    orientationArray->SetName("orientation");
    orientationArray->InsertNextValue(orientation);
    this->Table->GetFieldData()->AddArray(orientationArray);
    }
}

//-----------------------------------------------------------------------------
int vtkHeatmapItem::GetOrientation()
{
  vtkIntArray *orientationArray = vtkIntArray::SafeDownCast(
    this->Table->GetFieldData()->GetArray("orientation"));
  if (orientationArray)
    {
    return orientationArray->GetValue(0);
    }
  return vtkHeatmapItem::LEFT_TO_RIGHT;
}

//-----------------------------------------------------------------------------
double vtkHeatmapItem::GetTextAngleForOrientation(int orientation)
{
  switch(orientation)
    {
    case vtkHeatmapItem::DOWN_TO_UP:
      return 90.0;
      break;

    case vtkHeatmapItem::RIGHT_TO_LEFT:
      return 0.0;
      break;

    case vtkHeatmapItem::UP_TO_DOWN:
      return 270.0;
      break;

    case vtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      return 0.0;
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::GetBounds(double bounds[4])
{
  bounds[0] = this->MinX;
  bounds[1] = this->MaxX;
  bounds[2] = this->MinY;
  bounds[3] = this->MaxY;
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::MarkRowAsBlank(std::string rowName)
{
  this->BlankRows.insert(rowName);
}

//-----------------------------------------------------------------------------
bool vtkHeatmapItem::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void vtkHeatmapItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Table: " << (this->Table ? "" : "(null)") << std::endl;
  if (this->Table->GetNumberOfRows() > 0)
    {
    this->Table->PrintSelf(os, indent.GetNextIndent());
    }
}
