/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartBox.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkContextMapper2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotBox.h"
#include "vtkPlotGrid.h"
#include "vtkPoints2D.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTooltipItem.h"
#include "vtkTransform2D.h"

#include <algorithm>
#include <vector>


// Minimal storage class for STL containers etc.
class vtkChartBox::Private
{
public:
  Private()
  {
    this->Plot = vtkSmartPointer<vtkPlotBox>::New();
    this->YAxis->SetPosition(vtkAxis::LEFT);
    this->YAxis->SetPoint1(0, 0);
    this->YAxis->SetTitle("Y");
  }
  ~Private()
  {
  }
  vtkSmartPointer<vtkPlotBox> Plot;
  std::vector<float> XPosition;
  vtkNew<vtkTransform2D> Transform;
  vtkNew<vtkAxis> YAxis;
  vtkNew<vtkPlotGrid> Grid;
  float SelectedColumnDelta;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartBox);

//-----------------------------------------------------------------------------
vtkChartBox::vtkChartBox()
{
  this->Storage = new vtkChartBox::Private;
  this->Storage->Plot->SetParent(this);
  this->AddItem(this->Storage->YAxis.GetPointer());
  this->GeometryValid = false;
  this->Selection = vtkIdTypeArray::New();
  this->SelectedColumn = -1;
  this->Storage->Plot->SetSelection(this->Selection);
  this->VisibleColumns = vtkStringArray::New();

  this->Tooltip = vtkSmartPointer<vtkTooltipItem>::New();
  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip);

  // Set up default mouse button assignments for parallel coordinates.
  this->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::RIGHT_BUTTON);
  this->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
}

//-----------------------------------------------------------------------------
vtkChartBox::~vtkChartBox()
{
  this->Storage->Plot->SetSelection(NULL);
  delete this->Storage;
  this->Selection->Delete();
  this->VisibleColumns->Delete();
}

//-----------------------------------------------------------------------------
void vtkChartBox::Update()
{
  vtkTable* table = this->Storage->Plot->GetData()->GetInput();
  if (!table)
  {
    return;
  }

  if (table->GetMTime() < this->BuildTime && this->MTime < this->BuildTime)
  {
    return;
  }

  int nbCols = this->VisibleColumns->GetNumberOfTuples();

  this->Storage->XPosition.resize(nbCols);

  double grange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  // Now set up their ranges and locations
  for (int i = 0; i < nbCols; ++i)
  {
    vtkDataArray* array =
      vtkArrayDownCast<vtkDataArray>(table->GetColumnByName(
    this->VisibleColumns->GetValue(i)));
    if (array)
    {
      double range[2];
      array->GetRange(range);
      if (range[0] < grange[0])
      {
        grange[0] = range[0];
      }
      if (range[1] > grange[1])
      {
        grange[1] = range[1];
      }
    }
  }

  this->Storage->YAxis->SetMinimum(grange[0]);
  this->Storage->YAxis->SetMaximum(grange[1]);

  this->GeometryValid = false;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkChartBox::Paint(vtkContext2D *painter)
{
  if (this->GetScene()->GetViewWidth() == 0 ||
      this->GetScene()->GetViewHeight() == 0 ||
      !this->Visible || !this->Storage->Plot->GetVisible() ||
      this->VisibleColumns->GetNumberOfTuples() < 1)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  //this->UpdateGeometry(painter);
  this->Update();
  this->UpdateGeometry(painter);

  // Handle selections
  vtkIdTypeArray *idArray = 0;
  if (this->AnnotationLink)
  {
    vtkSelection *selection = this->AnnotationLink->GetCurrentSelection();
    if (selection->GetNumberOfNodes() &&
        this->AnnotationLink->GetMTime() > this->Storage->Plot->GetMTime())
    {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
      this->Storage->Plot->SetSelection(idArray);
    }
  }
  else
  {
    vtkDebugMacro("No annotation link set.");
  }

  painter->PushMatrix();
  painter->SetTransform(this->Storage->Transform.GetPointer());
  this->Storage->Plot->Paint(painter);
  painter->PopMatrix();

  this->Storage->YAxis->Paint(painter);

  if (this->Title)
  {
    painter->ApplyTextProp(this->TitleProperties);
    vtkVector2f stringBounds[2];
    painter->ComputeStringBounds(this->Title, stringBounds->GetData());
    float height = 1.1 * stringBounds[1].GetY();

    // Shift the position of the title down if it would be outside the window
    float shift;
    if (this->Point2[1] + height > this->Geometry[1])
    {
      shift = this->Point2[1] + height - this->Geometry[1];
    }
    else
    {
      shift = 0.0f;
    }
    vtkPoints2D *rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0],
                          this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0]-this->Point1[0],
                          height - shift);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  if (this->GetShowLegend())
  {
    vtkRectf rect;
    rect.Set(0, 2, 10, 20);
    this->Storage->Plot->PaintLegend(painter, rect, 0);
  }

  if (this->Tooltip && this->Tooltip->GetVisible())
  {
    this->Tooltip->Paint(painter);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetColumnVisibility(const vtkStdString& name,
                                      bool visible)
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
        while (i < this->VisibleColumns->GetNumberOfTuples()-1)
        {
          this->VisibleColumns->SetValue(i, this->VisibleColumns->GetValue(i+1));
          ++i;
        }
        this->VisibleColumns->SetNumberOfTuples(
            this->VisibleColumns->GetNumberOfTuples()-1);
        if (this->SelectedColumn >= this->VisibleColumns->GetNumberOfTuples())
        {
          this->SelectedColumn = -1;
        }
        this->Modified();
        this->Update();
        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetColumnVisibility(vtkIdType column, bool visible)
{
  vtkPlot *plot = this->GetPlot(0);
  if (!plot || !plot->GetInput())
  {
    return;
  }
  vtkTable *table = plot->GetInput();
  if (table)
  {
    this->SetColumnVisibility(table->GetColumnName(column), visible);
  }
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetColumnVisibilityAll(bool visible)
{
  // We always need to clear the current visible columns.
  this->VisibleColumns->SetNumberOfTuples(0);
  this->SelectedColumn = -1;
  if (visible)
  {
    vtkPlot *plot = this->GetPlot(0);
    if (!plot || !plot->GetInput())
    {
      return;
    }
    vtkTable *table = plot->GetInput();
    for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      this->SetColumnVisibility(table->GetColumnName(i), visible);
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkChartBox::GetColumnVisibility(const vtkStdString& name)
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

//-----------------------------------------------------------------------------
bool vtkChartBox::GetColumnVisibility(vtkIdType column)
{
  vtkPlot *plot = this->GetPlot(0);
  if (!plot || !plot->GetInput())
  {
    return false;
  }
  vtkTable *table = plot->GetInput();
  return this->GetColumnVisibility(table->GetColumnName(column));
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartBox::GetNumberOfVisibleColumns()
{
  return this->VisibleColumns->GetNumberOfTuples();
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartBox::GetColumnId(const vtkStdString& name)
{
  vtkPlot *plot = this->GetPlot(0);
  if (!plot || !plot->GetInput())
  {
    return -1;
  }
  vtkTable *table = plot->GetInput();
  vtkIdType nbColumn = table->GetNumberOfColumns();
  for (vtkIdType i = 0; i < nbColumn; i++)
  {
    if (!strcmp(table->GetColumnName(i), name.c_str()))
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChartBox::GetYAxis()
{
  return this->Storage->YAxis.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetPlot(vtkPlotBox *plot)
{
  this->Storage->Plot = plot;
  this->Storage->Plot->SetParent(this);
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChartBox::GetPlot(vtkIdType)
{
  return this->Storage->Plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartBox::GetNumberOfPlots()
{
  return 1;
}

//-----------------------------------------------------------------------------
float vtkChartBox::GetXPosition(int index)
{
  return (index < static_cast<int>(this->Storage->XPosition.size())) ?
    this->Storage->XPosition[index] : 0;
}

//-----------------------------------------------------------------------------
void vtkChartBox::UpdateGeometry(vtkContext2D* painter)
{
  vtkVector2i geometry(this->GetScene()->GetViewWidth(),
                       this->GetScene()->GetViewHeight());

  if (geometry.GetX() != this->Geometry[0] ||
    geometry.GetY() != this->Geometry[1] || !this->GeometryValid)
  {
    vtkAxis* axis = this->Storage->YAxis.GetPointer();

    axis->SetPoint1(0, this->Point1[1]);
    axis->SetPoint2(0, this->Point2[1]);
    if (axis->GetBehavior() == 0)
    {
      axis->AutoScale();
    }
    axis->Update();

    int leftBorder = 0;
    if (axis->GetVisible())
    {
      vtkRectf bounds = axis->GetBoundingRect(painter);
      leftBorder = int(bounds.GetWidth());
    }
    axis->SetPoint1(leftBorder, this->Point1[1]);
    axis->SetPoint2(leftBorder, this->Point2[1]);

    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry.GetData());

    vtkVector2i tileScale = this->Scene->GetLogicalTileScale();
    this->SetBorders(leftBorder, 30 * tileScale.GetY(),
                     0, 20 * tileScale.GetY());

    int nbPlots = static_cast<int>(this->Storage->XPosition.size());
    // Iterate through the axes and set them up to span the chart area.
    int xStep = (this->Point2[0] - this->Point1[0]) / nbPlots;
    int x = this->Point1[0] + (xStep / 2);

    for (int i = 0; i < nbPlots; ++i)
    {
      this->Storage->XPosition[i] = x;

      x += xStep;
    }
    this->GeometryValid = true;

    // Cause the plot transform to be recalculated if necessary
    this->CalculatePlotTransform();

    if (this->VisibleColumns->GetNumberOfValues() > 1)
    {
      this->Storage->Plot->SetBoxWidth(0.5f *
        (this->GetXPosition(1) - this->GetXPosition(0)));
    }

    this->Storage->Plot->Update();
  }
}

//-----------------------------------------------------------------------------
void vtkChartBox::CalculatePlotTransform()
{
  // In the case of box plots everything is plotted in a normalized
  // system, where the range is from 0.0 to 1.0 in the y axis, and in screen
  // coordinates along the x axis.
  vtkAxis* axis = this->Storage->YAxis.GetPointer();
  float *min = axis->GetPoint1();
  float *max = axis->GetPoint2();
  float yScale = 1.0f / (max[1] - min[1]);

  this->Storage->Transform->Identity();
  this->Storage->Transform->Translate(0, axis->GetPoint1()[1]);
  // Get the scale for the plot area from the x and y axes
  this->Storage->Transform->Scale(1.0, 1.0 / yScale);
}

//-----------------------------------------------------------------------------
bool vtkChartBox::Hit(const vtkContextMouseEvent &mouse)
{
  vtkVector2i pos(mouse.GetScreenPos());
  float width = this->Storage->Plot->GetBoxWidth() / 2.f;
  return
    pos[0] > this->Point1[0] - width &&
    pos[0] < this->Point2[0] + width &&
    pos[1] > this->Point1[1] &&
    pos[1] < this->Point2[1];
}

//-----------------------------------------------------------------------------
bool vtkChartBox::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == this->Actions.Pan() && this->SelectedColumn >= 0)
  {
    if (this->Tooltip)
    {
      this->Tooltip->SetVisible(false);
    }

    // Move the plot in x
    float posX = mouse.GetScenePos().GetX() + this->SelectedColumnDelta;
    this->Storage->XPosition[this->SelectedColumn] = posX;

    int nbCols = static_cast<int>(this->Storage->XPosition.size());
    int left = this->SelectedColumn - 1;
    int right = this->SelectedColumn + 1;

    float width = this->Storage->Plot->GetBoxWidth() * 0.5f;

    if (left >= 0 && (posX - width) < this->Storage->XPosition[left])
    {
      this->SwapAxes(this->SelectedColumn, this->SelectedColumn - 1);
      this->SelectedColumn--;
    }
    else if (right < nbCols && (posX + width) > this->Storage->XPosition[right])
    {
      this->SwapAxes(this->SelectedColumn, this->SelectedColumn + 1);
      this->SelectedColumn++;
    }
    this->Scene->SetDirty(true);
    this->Storage->XPosition[this->SelectedColumn] = posX;
  }

  if (mouse.GetButton() == vtkContextMouseEvent::NO_BUTTON)
  {
    this->Scene->SetDirty(true);

    if (this->Tooltip)
    {
      this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartBox::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Pan())
  {
    // Select a plot if we are within range
    if (mouse.GetScenePos()[1] > this->Point1[1] &&
        mouse.GetScenePos()[1] < this->Point2[1])
    {
      // Iterate over the axes, see if we are within 10 pixels of an axis
      for (size_t i = 0; i < this->Storage->XPosition.size(); ++i)
      {
        float selX = this->Storage->XPosition[i];
        float width = this->Storage->Plot->GetBoxWidth() / 2.f;
        if (selX - width < mouse.GetScenePos()[0] &&
            selX + width > mouse.GetScenePos()[0])
        {
          this->SelectedColumn = static_cast<int>(i);
          this->SelectedColumnDelta =
            this->GetXPosition(this->SelectedColumn) - mouse.GetScenePos().GetX();
          this->Scene->SetDirty(true);
          return true;
        }
      }
    }
    this->SelectedColumn = -1;
    this->Scene->SetDirty(true);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartBox::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  this->SelectedColumn = -1;
  if (mouse.GetButton() == this->Actions.Select())
  {
    if (this->SelectedColumn >= 0)
    {
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
    this->GeometryValid = false;
    this->SelectedColumn = -1;
    return true;
  }
  this->Scene->SetDirty(true);
  return true;
}

//-----------------------------------------------------------------------------
int vtkChartBox::LocatePointInPlot(const vtkVector2f &position,
                                   const vtkVector2f &tolerance,
                                   vtkVector2f &plotPos,
                                   vtkPlot *plot,
                                   vtkIdType &)
{
  if (plot && plot->GetVisible())
  {
    vtkPlotBox* plotBar = vtkPlotBox::SafeDownCast(plot);
    if (plotBar)
    {
      // If the plot is a vtkPlotBar, get the segment index too
      return plotBar->GetNearestPoint(position, tolerance,
                                      &plotPos);
    }
    else
    {
      return plot->GetNearestPoint(position, tolerance, &plotPos);
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool vtkChartBox::LocatePointInPlots(const vtkContextMouseEvent &mouse,
                                     int invokeEvent)
{
  vtkVector2i pos(mouse.GetScreenPos());
  if (pos[0] > this->Point1[0] &&
      pos[0] < this->Point2[0] &&
      pos[1] > this->Point1[1] &&
      pos[1] < this->Point2[1])
  {
    vtkVector2f plotPos, position;
    vtkTransform2D* transform =
      this->Storage->Transform.GetPointer();
    transform->InverseTransformPoints(mouse.GetPos().GetData(),
      position.GetData(), 1);
    // Use a tolerance of +/- 5 pixels
    vtkVector2f tolerance(5*(1.0/transform->GetMatrix()->GetElement(0, 0)),
      5*(1.0/transform->GetMatrix()->GetElement(1, 1)));

    vtkPlot* plot = this->Storage->Plot.GetPointer();
    vtkIdType segmentIndex = -1;
    int seriesIndex =
      LocatePointInPlot(position, tolerance, plotPos, plot, segmentIndex);

    if (seriesIndex >= 0)
    {
      // We found a point, set up the tooltip and return
      vtkRectd ss(plot->GetShiftScale());
      vtkVector2d plotPosd(plotPos[0] / ss[2] - ss[0],
        plotPos[1] / ss[3] - ss[1]);
      this->SetTooltipInfo(mouse, plotPosd, seriesIndex, plot,
        segmentIndex);
      if (invokeEvent >= 0)
      {
        vtkChartBoxData plotIndex;
        plotIndex.SeriesName =
          this->GetVisibleColumns()->GetValue(seriesIndex);
        plotIndex.Position = plotPos;
        plotIndex.ScreenPosition = mouse.GetScreenPos();
        plotIndex.Index = segmentIndex;
        // Invoke an event, with the client data supplied
        this->InvokeEvent(invokeEvent, static_cast<void*>(&plotIndex));
      }
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetTooltip(vtkTooltipItem *tooltip)
{
  if (tooltip == this->Tooltip)
  {
    // nothing to change
    return;
  }

  if (this->Tooltip)
  {
    // remove current tooltip from scene
    this->RemoveItem(this->Tooltip);
  }

  this->Tooltip = tooltip;

  if (this->Tooltip)
  {
    // add new tooltip to scene
    this->AddItem(this->Tooltip);
  }
}

//-----------------------------------------------------------------------------
vtkTooltipItem* vtkChartBox::GetTooltip()
{
  return this->Tooltip;
}

//-----------------------------------------------------------------------------
void vtkChartBox::SetTooltipInfo(const vtkContextMouseEvent& mouse,
                                 const vtkVector2d &plotPos,
                                 vtkIdType seriesIndex, vtkPlot* plot,
                                 vtkIdType segmentIndex)
{
  if (!this->Tooltip)
  {
    return;
  }

  // Have the plot generate its tooltip label
  vtkStdString tooltipLabel = plot->GetTooltipLabel(plotPos, seriesIndex,
                                                    segmentIndex);

  // Set the tooltip
  this->Tooltip->SetText(tooltipLabel);
  this->Tooltip->SetPosition(mouse.GetScreenPos()[0] + 2,
                             mouse.GetScreenPos()[1] + 2);
}

//-----------------------------------------------------------------------------
void vtkChartBox::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkChartBox::SwapAxes(int a1, int a2)
{
  vtkStdString colTmp = this->VisibleColumns->GetValue(a1);
  this->VisibleColumns->SetValue(a1, this->VisibleColumns->GetValue(a2));
  this->VisibleColumns->SetValue(a2, colTmp);

  int xStep = (this->Point2[0] - this->Point1[0]) /
                (static_cast<int>(this->Storage->XPosition.size()));
  int xPos = (this->Point1[0] + (xStep / 2)) + xStep * a1;
  this->Storage->XPosition[a1] = xPos;

  this->GeometryValid = true;

  this->Storage->Plot->Update();
}
