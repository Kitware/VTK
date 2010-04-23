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
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"

#include "vtkObjectFactory.h"

#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkChartLegend::Private
{
public:
  Private()
  {
  }
  ~Private()
  {
  }

  vtkVector2f Point;
  vtkWeakPointer<vtkChart> Chart;
  vtkstd::vector<vtkPlot*> ActivePlots;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartLegend);

//-----------------------------------------------------------------------------
vtkChartLegend::vtkChartLegend()
{
  this->Storage = new vtkChartLegend::Private;
  this->Point = this->Storage->Point.GetData();
  // Defaults to 12pt text, with top, right alignment to the specified point.
  this->LabelSize = 12;
  this->HorizontalAlignment = this->RIGHT;
  this->VerticalAlignment = this->TOP;
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
    if (this->Storage->Chart->GetPlot(i)->GetVisible())
      {
      this->Storage->ActivePlots.push_back(this->Storage->Chart->GetPlot(i));
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkChartLegend::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkChartLegend.");

  painter->GetPen()->SetWidth(1.0);
  vtkTextProperty *prop = painter->GetTextProp();
  prop->SetFontSize(this->LabelSize);
  prop->SetColor(0.0, 0.0, 0.0);
  prop->SetJustificationToLeft();
  prop->SetVerticalJustificationToBottom();

  vtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();
  painter->ComputeStringBounds("The", stringBounds->GetData());
  float baseHeight = stringBounds[1].GetY();
  float maxWidth = 0.0f;

  // Calculate the widest legend label - needs the context to calculate font
  // metrics, but these could be cached.
  for(size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
    {
    if (this->Storage->ActivePlots[i]->GetLabel())
      {
      painter->ComputeStringBounds(this->Storage->ActivePlots[i]->GetLabel(),
                                   stringBounds->GetData());
      if (stringBounds[1].X() > maxWidth)
        {
        maxWidth = stringBounds[1].X();
        }
      }
    }

  // Figure out the size of the legend box and store locally.
  int padding = 5;
  int symbolWidth = 25;
  vtkVector2f rectCorner(floor(this->Storage->Point.X()-maxWidth)-2*padding-symbolWidth,
                         floor(this->Storage->Point.Y() -
                               this->Storage->ActivePlots.size()*(height+padding)) -
                         padding);
  vtkVector2f rectDim(ceil(maxWidth)+2*padding+symbolWidth,
                      ceil((this->Storage->ActivePlots.size())*(height+padding)) +
                      padding);
  // Now draw a box for the legend.
  painter->GetBrush()->SetColor(255, 255, 255, 255);
  painter->DrawRect(rectCorner.X(), rectCorner.Y(), rectDim.X(), rectDim.Y());

  vtkVector2f pos(rectCorner.X() + padding + symbolWidth,
                  rectCorner.Y() + rectDim.Y() - padding - floor(height));
  float rect[] = { rectCorner.X() + padding, pos.Y(),
                   symbolWidth-3, ceil(height) };

  // Draw all of the legend labels and marks
  for(size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
    {
    if (this->Storage->ActivePlots[i]->GetLabel())
      {
      // This is fairly hackish, but gets the text looking reasonable...
      // Calculate a height for a "normal" string, then if this height is greater
      // that offset is used to move it down. Effectively hacking in a text
      // base line until better support is in the text rendering code...
      // There are still several one pixel glitches, but it looks better than
      // using the default vertical alignment. FIXME!
      vtkStdString testString = this->Storage->ActivePlots[i]->GetLabel();
      testString += "T";
      painter->ComputeStringBounds(testString, stringBounds->GetData());
      painter->DrawString(pos.X(), rect[1] + (baseHeight-stringBounds[1].Y()),
                          this->Storage->ActivePlots[i]->GetLabel());

      // Paint the legend mark and increment out y value.
      this->Storage->ActivePlots[i]->PaintLegend(painter, rect);
      rect[1] -= height+padding;
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartLegend::SetPoint(const vtkVector2f &point)
{
  this->Storage->Point = point;
}

//-----------------------------------------------------------------------------
const vtkVector2f& vtkChartLegend::GetPointVector()
{
  return this->Storage->Point;
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
void vtkChartLegend::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
