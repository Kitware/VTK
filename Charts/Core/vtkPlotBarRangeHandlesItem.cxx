// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlotBarRangeHandlesItem.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlotBarRangeHandlesItem);

//------------------------------------------------------------------------------
void vtkPlotBarRangeHandlesItem::GetBounds(double bounds[4])
{
  if (!this->PlotBar)
  {
    vtkErrorMacro("vtkPlotBarRangeHandlesItem should always be used with a PlotBar");
    return;
  }

  if (this->HandleOrientation != this->PlotBar->GetOrientation())
  {
    vtkErrorMacro("Handles orientation must be the same orientation as vtkPlotBar.");
    return;
  }

  this->Superclass::GetBounds(bounds);

  double plotBounds[4];
  this->PlotBar->GetBounds(plotBounds);
  if (this->PlotBar->GetOrientation() == vtkPlotBar::VERTICAL)
  {
    bounds[0] = plotBounds[0];
    bounds[1] = plotBounds[1];
  }
  else // HORIZONTAL
  {
    bounds[0] = plotBounds[2];
    bounds[1] = plotBounds[3];
  }
}

//------------------------------------------------------------------------------
void vtkPlotBarRangeHandlesItem::SetActiveHandlePosition(double position)
{
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    // Clamp the position and set the handle position
    double bounds[4];
    this->GetBounds(bounds);

    double clampedPos[2] = { position, 0.0 };

    vtkPlot::ClampPos(clampedPos, bounds);

    // Pick the nearest point in the bar plot
    vtkVector2f point(clampedPos[this->HandleOrientation], clampedPos[1 - this->HandleOrientation]);
    vtkVector2f tolerance(0.0, 0.0);
    vtkVector2f output;
    vtkIdType segmentId;
    if (this->PlotBar->GetNearestPoint(point, tolerance, &output, &segmentId) != -1)
    {
      // Place handles on their respective side of the bar
      float plotChartWidth = this->PlotBar->GetWidth();
      if (this->ActiveHandle == vtkPlotRangeHandlesItem::LEFT_HANDLE)
      {
        this->ActiveHandlePosition = output[0] - 0.5 * plotChartWidth;
      }
      else
      {
        this->ActiveHandlePosition = output[0] + 0.5 * plotChartWidth;
      }
    }
    else
    {
      // Could not pick data at this position, use clamped position instead.
      this->ActiveHandlePosition = clampedPos[0];
    }

    // Using the ActiveHandlePosition to compute the ActiveHandleRangeValue
    // ensures that the handle sticks to the picked bar.
    position = this->ActiveHandlePosition;

    // Transform it to data and set it
    double unused;
    this->TransformScreenToData(position, 1, this->ActiveHandleRangeValue, unused);
  }
}

//------------------------------------------------------------------------------
void vtkPlotBarRangeHandlesItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PlotBar: ";
  if (this->PlotBar)
  {
    os << endl;
    this->PlotBar->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
VTK_ABI_NAMESPACE_END
