/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartLegend.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkChart.h"
#include "vtkPlot.h"
#include "vtkTextProperty.h"
#include "vtkStdString.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"

#include "vtkObjectFactory.h"

#include <vector>

//-----------------------------------------------------------------------------
class vtkChartLegend::Private
{
public:
  Private() : Point(0, 0)
  {
  }
  ~Private()
  {
  }

  vtkVector2f Point;
  vtkWeakPointer<vtkChart> Chart;
  std::vector<vtkPlot*> ActivePlots;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartLegend);

//-----------------------------------------------------------------------------
vtkChartLegend::vtkChartLegend()
{
  this->Storage = new vtkChartLegend::Private;
  this->Point = this->Storage->Point.GetData();
  this->Rect.Set(0, 0, 0, 0);
  // Defaults to 12pt text, with top, right alignment to the specified point.
  this->LabelProperties->SetFontSize(12);
  this->LabelProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelProperties->SetJustificationToLeft();
  this->LabelProperties->SetVerticalJustificationToBottom();

  this->Pen->SetColor(0, 0, 0);
  this->Brush->SetColor(255, 255, 255, 255);
  this->HorizontalAlignment = vtkChartLegend::RIGHT;
  this->VerticalAlignment = vtkChartLegend::TOP;

  this->Padding = 5;
  this->SymbolWidth = 25;
  this->Inline = true;
  this->Button = -1;
  this->DragEnabled = true;
}

//-----------------------------------------------------------------------------
vtkChartLegend::~vtkChartLegend()
{
  delete this->Storage;
  this->Storage = NULL;
  this->Point = NULL;
}

//-----------------------------------------------------------------------------
void vtkChartLegend::Update()
{
  this->Storage->ActivePlots.clear();
  for (int i = 0; i < this->Storage->Chart->GetNumberOfPlots(); ++i)
    {
    if (this->Storage->Chart->GetPlot(i)->GetVisible()
        && this->Storage->Chart->GetPlot(i)->GetLabel().length() > 0)
      {
      this->Storage->ActivePlots.push_back(this->Storage->Chart->GetPlot(i));
      }
    // If we have a plot with multiple labels, we generally only want to show
    // the labels/legend symbols for the first one. So truncate at the first
    // one we encounter.
    if (this->Storage->Chart->GetPlot(i)->GetLabels() &&
        this->Storage->Chart->GetPlot(i)->GetLabels()->GetNumberOfTuples() > 1)
      {
      break;
      }
    }
  this->PlotTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkChartLegend.");

  if (!this->Visible || this->Storage->ActivePlots.size() == 0)
    {
    return true;
    }

  this->GetBoundingRect(painter);

  // Now draw a box for the legend.
  painter->ApplyPen(this->Pen.GetPointer());
  painter->ApplyBrush(this->Brush.GetPointer());
  painter->DrawRect(this->Rect.GetX(), this->Rect.GetY(),
                    this->Rect.GetWidth(), this->Rect.GetHeight());

  painter->ApplyTextProp(this->LabelProperties.GetPointer());

  vtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();
  painter->ComputeStringBounds("The", stringBounds->GetData());
  float baseHeight = stringBounds[1].GetY();

  vtkVector2f pos(this->Rect.GetX() + this->Padding + this->SymbolWidth,
                  this->Rect.GetY() + this->Rect.GetHeight() - this->Padding - floor(height));
  vtkRectf rect(this->Rect.GetX() + this->Padding, pos.GetY(),
               this->SymbolWidth-3, ceil(height));

  // Draw all of the legend labels and marks
  for(size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
    {
    vtkStringArray *labels = this->Storage->ActivePlots[i]->GetLabels();
    for (vtkIdType l = 0; labels && (l < labels->GetNumberOfValues()); ++l)
      {
      // This is fairly hackish, but gets the text looking reasonable...
      // Calculate a height for a "normal" string, then if this height is greater
      // that offset is used to move it down. Effectively hacking in a text
      // base line until better support is in the text rendering code...
      // There are still several one pixel glitches, but it looks better than
      // using the default vertical alignment. FIXME!
      vtkStdString testString = labels->GetValue(l);
      testString += "T";
      painter->ComputeStringBounds(testString, stringBounds->GetData());
      painter->DrawString(pos.GetX(), rect.GetY() + (baseHeight-stringBounds[1].GetY()),
                          labels->GetValue(l));

      // Paint the legend mark and increment out y value.
      this->Storage->ActivePlots[i]->PaintLegend(painter, rect, l);
      rect.SetY(rect.GetY() - height - this->Padding);
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
vtkRectf vtkChartLegend::GetBoundingRect(vtkContext2D *painter)
{
  if (this->RectTime > this->GetMTime() && this->RectTime > this->PlotTime)
    {
    return this->Rect;
    }

  painter->ApplyTextProp(this->LabelProperties.GetPointer());

  vtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();
  float maxWidth = 0.0f;

  // Calculate the widest legend label - needs the context to calculate font
  // metrics, but these could be cached.
  for(size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
    {
    vtkStringArray *labels = this->Storage->ActivePlots[i]->GetLabels();
    for (vtkIdType l = 0; labels && (l < labels->GetNumberOfTuples()); ++l)
      {
      painter->ComputeStringBounds(labels->GetValue(l),
                                   stringBounds->GetData());
      if (stringBounds[1].GetX() > maxWidth)
        {
        maxWidth = stringBounds[1].GetX();
        }
      }
    }

  // Figure out the size of the legend box and store locally.
  int numLabels = 0;
  for(size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
    {
    numLabels += this->Storage->ActivePlots[i]->GetNumberOfLabels();
    }

  // Default point placement is bottom left.
  this->Rect = vtkRectf(floor(this->Storage->Point.GetX()),
                        floor(this->Storage->Point.GetY()),
                        ceil(maxWidth + 2 * this->Padding + this->SymbolWidth),
                        ceil((numLabels * (height + this->Padding)) + this->Padding));

  this->RectTime.Modified();
  return this->Rect;
}

//-----------------------------------------------------------------------------
void vtkChartLegend::SetPoint(const vtkVector2f &point)
{
  this->Storage->Point = point;
  this->Modified();
}

//-----------------------------------------------------------------------------
const vtkVector2f& vtkChartLegend::GetPointVector()
{
  return this->Storage->Point;
}

//-----------------------------------------------------------------------------
void vtkChartLegend::SetLabelSize(int size)
{
  this->LabelProperties->SetFontSize(size);
}

//-----------------------------------------------------------------------------
int vtkChartLegend::GetLabelSize()
{
  return this->LabelProperties->GetFontSize();
}

//-----------------------------------------------------------------------------
vtkPen * vtkChartLegend::GetPen()
{
  return this->Pen.GetPointer();
}

//-----------------------------------------------------------------------------
vtkBrush * vtkChartLegend::GetBrush()
{
  return this->Brush.GetPointer();
}

//-----------------------------------------------------------------------------
vtkTextProperty * vtkChartLegend::GetLabelProperties()
{
  return this->LabelProperties.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkChartLegend::SetChart(vtkChart* chart)
{
  if (this->Storage->Chart == chart)
    {
    return;
    }
  else
    {
    this->Storage->Chart = chart;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkChart* vtkChartLegend::GetChart()
{
  return this->Storage->Chart;
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::Hit(const vtkContextMouseEvent &mouse)
{
  if (this->DragEnabled && mouse.GetScreenPos().GetX() > this->Rect.GetX() &&
      mouse.GetScreenPos().GetX() < this->Rect.GetX() + this->Rect.GetWidth() &&
      mouse.GetScreenPos().GetY() > this->Rect.GetY() &&
      mouse.GetScreenPos().GetY() < this->Rect.GetY() + this->Rect.GetHeight())
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (this->Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    vtkVector2f delta = mouse.GetScenePos() - mouse.GetLastScenePos();
    this->HorizontalAlignment = vtkChartLegend::CUSTOM;
    this->Storage->Point = this->Storage->Point + delta;
    this->GetScene()->SetDirty(true);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    this->Button = vtkContextMouseEvent::LEFT_BUTTON;
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  this->Button = -1;
  return true;
}

//-----------------------------------------------------------------------------
void vtkChartLegend::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
