/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartParallelCoordinates.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotParallelCoordinates.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"

#include <algorithm>
#include <array>
#include <vector>

// Minimal storage class for STL containers etc.
struct vtkChartParallelCoordinates::Private final
{
  Private() { this->Plot = vtkSmartPointer<vtkPlotParallelCoordinates>::New(); }

  ~Private()
  {
    for (std::vector<vtkAxis*>::iterator it = this->Axes.begin(); it != this->Axes.end(); ++it)
    {
      (*it)->Delete();
    }
  }

  vtkSmartPointer<vtkPlotParallelCoordinates> Plot;
  vtkNew<vtkTransform2D> Transform;
  std::vector<vtkAxis*> Axes;

  /**
   * Store for each axe, an sorted array like { minRange1, maxRange1, minRange2, maxRange2, ...} to
   * simplify the treatment of selection for addition, subtraction, toggle
   */
  std::vector<std::vector<float>> AxesSelections;
  std::array<float, 2> CurrentSelection = { { -1, -1 } };

  int CurrentAxis = -1;
  int AxisResize = -1;

  bool InteractiveSelection = false;
};

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartParallelCoordinates);

//------------------------------------------------------------------------------
vtkChartParallelCoordinates::vtkChartParallelCoordinates()
{
  this->Storage = new vtkChartParallelCoordinates::Private;
  this->Storage->Plot->SetParent(this);
  this->GeometryValid = false;
  this->Selection = vtkIdTypeArray::New();
  this->Storage->Plot->SetSelection(this->Selection);
  this->Storage->InteractiveSelection = false;

  // Set up default mouse button assignments for parallel coordinates.
  this->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::RIGHT_BUTTON);
  this->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
}

//------------------------------------------------------------------------------
vtkChartParallelCoordinates::~vtkChartParallelCoordinates()
{
  this->Storage->Plot->SetSelection(nullptr);
  delete this->Storage;
  this->Selection->Delete();
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::Update()
{
  vtkTable* table = this->Storage->Plot->GetData()->GetInput();
  if (!table)
  {
    return;
  }

  if (table->GetMTime() < this->BuildTime && this->MTime < this->BuildTime &&
    this->GetScene()->GetMTime() < this->BuildTime)
  {
    return;
  }

  // Now we have a table, set up the axes accordingly, clear and build.
  if (static_cast<int>(this->Storage->Axes.size()) != this->VisibleColumns->GetNumberOfTuples())
  {
    for (std::vector<vtkAxis*>::iterator it = this->Storage->Axes.begin();
         it != this->Storage->Axes.end(); ++it)
    {
      this->RemoveItem(*it);
      (*it)->Delete();
    }
    this->Storage->Axes.clear();
    this->Storage->AxesSelections.clear();

    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      vtkAxis* axis = vtkAxis::New();
      axis->SetPosition(vtkAxis::PARALLEL);
      this->AddItem(axis);
      this->Storage->Axes.push_back(axis);
    }

    this->Storage->AxesSelections.resize(this->Storage->Axes.size());
  }

  vtkDataSetAttributes* rowData = table->GetRowData();

  // Now set up their ranges and locations
  for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    double range[2];
    rowData->GetRange(this->VisibleColumns->GetValue(i).c_str(), range);
    vtkAxis* axis = this->Storage->Axes[i];
    if (axis->GetBehavior() == 0)
    {
      axis->SetMinimum(range[0]);
      axis->SetMaximum(range[1]);
    }
    axis->SetTitle(this->VisibleColumns->GetValue(i));
  }

  this->GeometryValid = false;
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::Paint(vtkContext2D* painter)
{
  if (this->GetScene()->GetViewWidth() == 0 || this->GetScene()->GetViewHeight() == 0 ||
    !this->Visible || !this->Storage->Plot->GetVisible() ||
    this->VisibleColumns->GetNumberOfTuples() < 2)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  this->Update();
  this->UpdateGeometry();

  // Handle selections
  vtkIdTypeArray* idArray = nullptr;
  if (this->AnnotationLink)
  {
    vtkSelection* selection = this->AnnotationLink->GetCurrentSelection();
    if (this->AnnotationLink->GetMTime() > this->Storage->Plot->GetMTime())
    {
      vtkSelectionNode* node = selection->GetNumberOfNodes() > 0 ? selection->GetNode(0) : nullptr;
      idArray = node ? vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList()) : nullptr;
      this->Storage->Plot->SetSelection(idArray);
      this->Storage->Plot->Modified();

      // InteractiveSelection is true only when the selection has been changed by the
      // user interactively (see MouseButtonReleaseEvent). Otherwise, it means that
      // the selection has been changed indirectly. In this case, we need to reset the
      // axes selection so they will not appear out of sync with the current selection.
      if (!this->Storage->InteractiveSelection)
      {
        this->ResetAxesSelection();
      }
    }
  }
  else
  {
    vtkDebugMacro("No annotation link set.");
  }

  painter->PushMatrix();
  painter->AppendTransform(this->Storage->Transform);
  this->Storage->Plot->Paint(painter);
  painter->PopMatrix();

  // Now we have a table, set up the axes accordingly, clear and build.
  for (std::vector<vtkAxis*>::iterator it = this->Storage->Axes.begin();
       it != this->Storage->Axes.end(); ++it)
  {
    (*it)->Paint(painter);
  }

  // If there is a selected axis, draw the highlight
  if (this->Storage->CurrentAxis >= 0)
  {
    painter->GetBrush()->SetColor(200, 200, 200, 200);
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    painter->DrawRect(
      axis->GetPoint1()[0] - 10, this->Point1[1], 20, this->Point2[1] - this->Point1[1]);
  }

  // Draw the active selection
  std::array<float, 2> range = this->Storage->CurrentSelection;
  if (range[0] != range[1])
  {
    this->PaintRect(painter, this->Storage->CurrentAxis, range[0], range[1]);
  }

  // Draw all stored range
  for (size_t axis = 0; axis < this->Storage->AxesSelections.size(); ++axis)
  {
    size_t size = this->Storage->AxesSelections[axis].size();
    size = size - size % 2;
    for (size_t j = 0; j < size; j += 2)
    {
      float min = this->Storage->AxesSelections[axis][j];
      float max = this->Storage->AxesSelections[axis][j + 1];
      if (min != max)
      {
        this->PaintRect(painter, static_cast<int>(axis), min, max);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::PaintRect(vtkContext2D* painter, int axis, float min, float max)
{
  painter->GetBrush()->SetColor(200, 20, 20, 220);
  float x = this->Storage->Axes[axis]->GetPoint1()[0] - 5;
  float y = min;
  y *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
  y += this->Storage->Transform->GetMatrix()->GetElement(1, 2);
  float height = max - min;
  height *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

  painter->DrawRect(x, y, 10, height);
  return true;
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::SetColumnVisibility(const vtkStdString& name, bool visible)
{
  if (visible)
  {
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Already there, nothing more needs to be done
        return;
      }
    }
    // Add the column to the end of the list
    this->VisibleColumns->InsertNextValue(name);
    this->Modified();
    this->Update();
  }
  else
  {
    // Remove the value if present
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples() - 1)
        {
          this->VisibleColumns->SetValue(i, this->VisibleColumns->GetValue(i + 1));
          ++i;
        }
        this->VisibleColumns->SetNumberOfTuples(this->VisibleColumns->GetNumberOfTuples() - 1);
        if (this->Storage->CurrentAxis >= this->VisibleColumns->GetNumberOfTuples())
        {
          this->Storage->CurrentAxis = -1;
        }
        this->Modified();
        this->Update();
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::SetColumnVisibilityAll(bool visible)
{
  // We always need to clear the current visible columns.
  this->VisibleColumns->SetNumberOfTuples(0);
  this->Storage->CurrentAxis = -1;
  if (visible)
  {
    vtkTable* table = this->GetPlot(0)->GetInput();
    for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      this->SetColumnVisibility(table->GetColumnName(i), visible);
    }
  }
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::GetColumnVisibility(const vtkStdString& name)
{
  for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkChartParallelCoordinates::GetVisibleColumns()
{
  return this->VisibleColumns;
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::SetVisibleColumns(vtkStringArray* visColumns)
{
  if (!visColumns || visColumns->GetNumberOfTuples() == 0)
  {
    this->VisibleColumns->SetNumberOfTuples(0);
  }
  else
  {
    this->VisibleColumns->SetNumberOfTuples(visColumns->GetNumberOfTuples());
    this->VisibleColumns->DeepCopy(visColumns);
  }
  if (this->Storage->CurrentAxis >= this->VisibleColumns->GetNumberOfTuples())
  {
    this->Storage->CurrentAxis = -1;
  }
  this->Modified();
  this->Update();
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::SetPlot(vtkPlotParallelCoordinates* plot)
{
  this->Storage->Plot = plot;
  this->Storage->Plot->SetParent(this);
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartParallelCoordinates::GetPlot(vtkIdType)
{
  return this->Storage->Plot;
}

//------------------------------------------------------------------------------
vtkIdType vtkChartParallelCoordinates::GetNumberOfPlots()
{
  return 1;
}

//------------------------------------------------------------------------------
vtkAxis* vtkChartParallelCoordinates::GetAxis(int index)
{
  if (index < this->GetNumberOfAxes())
  {
    return this->Storage->Axes[index];
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkChartParallelCoordinates::GetNumberOfAxes()
{
  return static_cast<vtkIdType>(this->Storage->Axes.size());
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::UpdateGeometry()
{
  vtkVector2i geometry(this->GetScene()->GetViewWidth(), this->GetScene()->GetViewHeight());

  if (geometry.GetX() != this->Geometry[0] || geometry.GetY() != this->Geometry[1] ||
    !this->GeometryValid)
  {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry.GetData());

    vtkVector2i tileScale = this->Scene->GetLogicalTileScale();
    this->SetBorders(
      60 * tileScale.GetX(), 50 * tileScale.GetY(), 60 * tileScale.GetX(), 20 * tileScale.GetY());

    // Iterate through the axes and set them up to span the chart area.
    int xStep =
      (this->Point2[0] - this->Point1[0]) / (static_cast<int>(this->Storage->Axes.size()) - 1);
    int x = this->Point1[0];

    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
    {
      vtkAxis* axis = this->Storage->Axes[i];
      axis->SetPoint1(x, this->Point1[1]);
      axis->SetPoint2(x, this->Point2[1]);
      if (axis->GetBehavior() == 0)
      {
        axis->AutoScale();
      }
      axis->Update();
      x += xStep;
    }

    this->GeometryValid = true;
    // Cause the plot transform to be recalculated if necessary
    this->CalculatePlotTransform();
    this->Storage->Plot->Update();
  }
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::CalculatePlotTransform()
{
  // In the case of parallel coordinates everything is plotted in a normalized
  // system, where the range is from 0.0 to 1.0 in the y axis, and in screen
  // coordinates along the x axis.
  if (this->Storage->Axes.empty())
  {
    return;
  }

  vtkAxis* axis = this->Storage->Axes[0];
  float* min = axis->GetPoint1();
  float* max = axis->GetPoint2();
  float yScale = 1.0f / (max[1] - min[1]);

  this->Storage->Transform->Identity();
  this->Storage->Transform->Translate(0, axis->GetPoint1()[1]);
  // Get the scale for the plot area from the x and y axes
  this->Storage->Transform->Scale(1.0, 1.0 / yScale);
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::RecalculateBounds() {}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::Hit(const vtkContextMouseEvent& mouse)
{
  vtkVector2i pos(mouse.GetScreenPos());
  return pos[0] > this->Point1[0] - 10 && pos[0] < this->Point2[0] + 10 &&
    pos[1] > this->Point1[1] && pos[1] < this->Point2[1];
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseEnterEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    // If an axis is selected, then lets try to narrow down a selection...
    if (this->Storage->CurrentAxis >= 0)
    {
      std::array<float, 2>& range = this->Storage->CurrentSelection;

      // Normalize the coordinates
      float current = mouse.GetScenePos().GetY();
      current -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      current /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      if (current > 1.0f)
      {
        range[1] = 1.0f;
      }
      else if (current < 0.0f)
      {
        range[1] = 0.0f;
      }
      else
      {
        range[1] = current;
      }
    }
    this->Scene->SetDirty(true);
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    if (this->Storage->AxisResize == 0)
    {
      // Move the axis in x
      float deltaX = mouse.GetScenePos().GetX() - mouse.GetLastScenePos().GetX();

      axis->SetPoint1(axis->GetPoint1()[0] + deltaX, axis->GetPoint1()[1]);
      axis->SetPoint2(axis->GetPoint2()[0] + deltaX, axis->GetPoint2()[1]);

      vtkAxis* leftAxis = this->Storage->CurrentAxis > 0
        ? this->Storage->Axes[this->Storage->CurrentAxis - 1]
        : nullptr;

      vtkAxis* rightAxis =
        this->Storage->CurrentAxis < static_cast<int>(this->Storage->Axes.size()) - 1
        ? this->Storage->Axes[this->Storage->CurrentAxis + 1]
        : nullptr;

      if (leftAxis && axis->GetPoint1()[0] < leftAxis->GetPoint1()[0])
      {
        this->SwapAxes(this->Storage->CurrentAxis, this->Storage->CurrentAxis - 1);
        this->Storage->CurrentAxis--;
      }
      else if (rightAxis && axis->GetPoint1()[0] > rightAxis->GetPoint1()[0])
      {
        this->SwapAxes(this->Storage->CurrentAxis, this->Storage->CurrentAxis + 1);
        this->Storage->CurrentAxis++;
      }
    }
    else if (this->Storage->AxisResize == 1)
    {
      // Modify the bottom axis range...
      float deltaY = mouse.GetScenePos().GetY() - mouse.GetLastScenePos().GetY();
      float scale =
        (axis->GetPoint2()[1] - axis->GetPoint1()[1]) / (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMinimum(axis->GetMinimum() - deltaY / scale);

      // Now update everything that needs to be
      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
    }
    else if (this->Storage->AxisResize == 2)
    {
      // Modify the bottom axis range...
      float deltaY = mouse.GetScenePos().GetY() - mouse.GetLastScenePos().GetY();
      float scale =
        (axis->GetPoint2()[1] - axis->GetPoint1()[1]) / (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMaximum(axis->GetMaximum() - deltaY / scale);

      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
    }
    this->Scene->SetDirty(true);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    // Select an axis if we are within range
    if (mouse.GetScenePos()[1] > this->Point1[1] && mouse.GetScenePos()[1] < this->Point2[1])
    {
      // Iterate over the axes, see if we are within 10 pixels of an axis
      for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
      {
        vtkAxis* axis = this->Storage->Axes[i];
        if (axis->GetPoint1()[0] - 10 < mouse.GetScenePos()[0] &&
          axis->GetPoint1()[0] + 10 > mouse.GetScenePos()[0])
        {
          this->Storage->CurrentAxis = static_cast<int>(i);
          // This is a manual interactive selection
          this->Storage->InteractiveSelection = true;

          // Transform into normalized coordinates
          float low = mouse.GetScenePos()[1];
          low -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
          low /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
          std::array<float, 2>& range = this->Storage->CurrentSelection;
          range[0] = range[1] = low;

          this->Scene->SetDirty(true);
          return true;
        }
      }
    }
    this->Storage->CurrentAxis = -1;
    this->Scene->SetDirty(true);
    return true;
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    // Middle mouse button - move and zoom the axes
    // Iterate over the axes, see if we are within 10 pixels of an axis
    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
    {
      vtkAxis* axis = this->Storage->Axes[i];
      if (axis->GetPoint1()[0] - 10 < mouse.GetScenePos()[0] &&
        axis->GetPoint1()[0] + 10 > mouse.GetScenePos()[0])
      {
        this->Storage->CurrentAxis = static_cast<int>(i);
        if (mouse.GetScenePos().GetY() > axis->GetPoint1()[1] &&
          mouse.GetScenePos().GetY() < axis->GetPoint1()[1] + 20)
        {
          // Resize the bottom of the axis
          this->Storage->AxisResize = 1;
        }
        else if (mouse.GetScenePos().GetY() < axis->GetPoint2()[1] &&
          mouse.GetScenePos().GetY() > axis->GetPoint2()[1] - 20)
        {
          // Resize the top of the axis
          this->Storage->AxisResize = 2;
        }
        else
        {
          // Move the axis
          this->Storage->AxisResize = 0;
        }
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    if (this->Storage->CurrentAxis >= 0)
    {
      std::array<float, 2>& range = this->Storage->CurrentSelection;

      float final = mouse.GetScenePos()[1];
      final -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      final /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      // Set the final mouse position
      if (final > 1.0)
      {
        range[1] = 1.0;
      }
      else if (final < 0.0)
      {
        range[1] = 0.0;
      }
      else
      {
        range[1] = final;
      }

      // Update all range stored based on the new selection
      UpdateCurrentAxisSelection(this->Storage->CurrentAxis);
      this->Storage->CurrentSelection[0] = 0;
      this->Storage->CurrentSelection[1] = 0;

      // This is a manual interactive selection
      this->Storage->InteractiveSelection = true;

      if (this->AnnotationLink)
      {
        vtkSelection* selection = vtkSelection::New();
        vtkSelectionNode* node = vtkSelectionNode::New();
        selection->AddNode(node);
        node->SetContentType(vtkSelectionNode::INDICES);
        node->SetFieldType(vtkSelectionNode::POINT);

        node->SetSelectionList(this->Storage->Plot->GetSelection());
        this->AnnotationLink->SetCurrentSelection(selection);
        selection->Delete();
        node->Delete();
      }
      this->InvokeEvent(vtkCommand::SelectionChangedEvent);
      this->Scene->SetDirty(true);
    }
    return true;
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    this->Storage->CurrentAxis = -1;
    this->Storage->AxisResize = -1;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseWheelEvent(const vtkContextMouseEvent&, int)
{
  return true;
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::ResetSelection()
{
  // This function takes care of resetting the selection of the chart
  this->Storage->Plot->ResetSelectionRange();

  for (size_t axe = 0; axe < this->Storage->AxesSelections.size(); axe++)
  {
    if (!this->Storage->AxesSelections[axe].empty())
    {
      this->Storage->Plot->SetSelectionRange(
        static_cast<int>(axe), this->Storage->AxesSelections[axe]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::ResetAxeSelection(int axe)
{
  for (size_t i = 0; i < this->Storage->AxesSelections[axe].size(); i += 2)
  {
    float& min = this->Storage->AxesSelections[axe][i];
    float& max = this->Storage->AxesSelections[axe][i + 1];
    min = max = 0.0f;
  }
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::ResetAxesSelection()
{
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
  {
    this->ResetAxeSelection(static_cast<int>(i));
  }
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::SwapAxes(int a1, int a2)
{
  // only neighboring axes
  if (abs(a1 - a2) != 1)
    return;

  vtkAxis* axisTmp = this->Storage->Axes[a1];
  this->Storage->Axes[a1] = this->Storage->Axes[a2];
  this->Storage->Axes[a2] = axisTmp;

  std::vector<float> selTmp = this->Storage->AxesSelections[a1];
  this->Storage->AxesSelections[a1] = this->Storage->AxesSelections[a2];
  this->Storage->AxesSelections[a2] = selTmp;

  std::string colTmp = this->VisibleColumns->GetValue(a1);
  this->VisibleColumns->SetValue(a1, this->VisibleColumns->GetValue(a2));
  this->VisibleColumns->SetValue(a2, colTmp);

  this->Storage->Plot->Update();
}

//------------------------------------------------------------------------------
void vtkChartParallelCoordinates::UpdateCurrentAxisSelection(int axisId)
{
  // min/max in the selection can be not ordered
  std::array<float, 2> currentSelection = this->Storage->CurrentSelection;
  float minCurrentSelection = std::min(currentSelection[0], currentSelection[1]);
  float maxCurrentSelection = std::max(currentSelection[0], currentSelection[1]);

  bool isANewRange = true;
  bool startAMerge = false;

  // Invalid range will be set to -1
  size_t size = this->Storage->AxesSelections[axisId].size();
  size =
    this->Storage->AxesSelections[axisId].size() - this->Storage->AxesSelections[axisId].size() % 2;
  for (size_t i = 0; i < size; i += 2)
  {
    if (this->GetSelectionMode() == vtkContextScene::SELECTION_TOGGLE)
    {
      break;
    }

    float& minRange = this->Storage->AxesSelections[axisId][i];
    float& maxRange = this->Storage->AxesSelections[axisId][i + 1];

    if (this->GetSelectionMode() == vtkContextScene::SELECTION_ADDITION)
    {
      // Delete range inside the currentSeleciton
      if (minCurrentSelection < minRange && maxCurrentSelection > maxRange)
      {
        minRange = -1;
        if (startAMerge && i >= this->Storage->AxesSelections[axisId].size())
        {
          maxRange = maxCurrentSelection;
        }
        else
        {
          maxRange = -1;
          continue;
        }
      }

      // Increase the range in min
      if (minCurrentSelection < minRange && maxCurrentSelection > minRange &&
        maxCurrentSelection < maxRange)
      {
        if (startAMerge)
        {
          minRange = -1;
        }
        else
        {
          minRange = minCurrentSelection;
          isANewRange = false;
          break;
        }
      }

      // Merge range by addition
      if (minCurrentSelection > minRange && minCurrentSelection < maxRange)
      {
        if (maxCurrentSelection > maxRange)
        {
          bool isLastRange = (i + 2) >= this->Storage->AxesSelections[axisId].size();
          if (isLastRange)
          {
            maxRange = maxCurrentSelection;
            isANewRange = false;
            break;
          }
          else
          {
            if (maxCurrentSelection < this->Storage->AxesSelections[axisId][i + 2])
            {
              maxRange = maxCurrentSelection;
            }
            else
            {
              maxRange = -1;
              startAMerge = true;
            }
          }
          isANewRange = false;
        }
      }
    }

    if (this->GetSelectionMode() == vtkContextScene::SELECTION_SUBTRACTION)
    {
      isANewRange = false;

      // Delete a range
      if (minCurrentSelection <= minRange && maxCurrentSelection >= maxRange)
      {
        minRange = -1;
        maxRange = -1;
      }

      // Shrink a range
      if (minCurrentSelection < minRange &&
        (maxCurrentSelection > minRange && maxCurrentSelection <= maxRange))
      {
        minRange = maxCurrentSelection;
      }
      if (maxCurrentSelection > maxRange &&
        (minCurrentSelection < maxRange && minCurrentSelection >= minRange))
      {
        maxRange = minCurrentSelection;
      }

      // Split a range in 2 part
      if (minCurrentSelection > minRange && maxCurrentSelection < maxRange)
      {
        isANewRange = true;
        break;
      }
    }
  }

  // Remove invalidated ranges
  this->Storage->AxesSelections[axisId].erase(
    std::remove(this->Storage->AxesSelections[axisId].begin(),
      this->Storage->AxesSelections[axisId].end(), -1),
    this->Storage->AxesSelections[axisId].end());

  if (isANewRange)
  {
    this->Storage->AxesSelections[axisId].push_back(minCurrentSelection);
    this->Storage->AxesSelections[axisId].push_back(maxCurrentSelection);
  }

  std::sort(
    this->Storage->AxesSelections[axisId].begin(), this->Storage->AxesSelections[axisId].end());

  // To support multiple selection, we need to recalculate all the selection
  this->ResetSelection();
}
