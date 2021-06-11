/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRangeHandlesItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRangeHandlesItem.h"

#include "vtkBrush.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform2D.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkRangeHandlesItem);
vtkSetObjectImplementationMacro(
  vtkRangeHandlesItem, ColorTransferFunction, vtkColorTransferFunction);

//------------------------------------------------------------------------------
vtkRangeHandlesItem::vtkRangeHandlesItem()
{
  this->Brush->SetColor(125, 135, 144, 200);
  this->HighlightBrush->SetColor(255, 0, 255, 200);
  this->RangeLabelBrush->SetColor(255, 255, 255, 200);
  this->ExtentToAxisRangeOff();
  this->SetHandleOrientation(vtkPlotRangeHandlesItem::Orientation::VERTICAL);
  this->LockTooltipToMouseOff();
}

//------------------------------------------------------------------------------
vtkRangeHandlesItem::~vtkRangeHandlesItem()
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkRangeHandlesItem::ComputeHandlesDrawRange()
{
  double screenBounds[4];
  this->GetBounds(screenBounds);
  this->ComputeHandleDelta(screenBounds);
  this->ComputeRange(screenBounds);
}

//------------------------------------------------------------------------------
bool vtkRangeHandlesItem::Paint(vtkContext2D* painter)
{
  if (!this->ColorTransferFunction)
  {
    return false;
  }
  return this->Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
void vtkRangeHandlesItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
  {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
void vtkRangeHandlesItem::GetBounds(double* bounds)
{
  if (!this->ColorTransferFunction)
  {
    vtkErrorMacro("vtkRangeHandlesItem should always be used with a ColorTransferFunction");
    return;
  }

  double tfRange[2];
  this->ColorTransferFunction->GetRange(tfRange);
  double unused;
  this->TransformDataToScreen(tfRange[0], 1, bounds[0], unused);
  this->TransformDataToScreen(tfRange[1], 1, bounds[1], unused);
  bounds[2] = 0;
  bounds[3] = 1;
}

//------------------------------------------------------------------------------
void vtkRangeHandlesItem::GetHandlesRange(double range[2])
{
  this->ColorTransferFunction->GetRange(range);
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    range[this->ActiveHandle] = this->ActiveHandleRangeValue;
  }
}

//------------------------------------------------------------------------------
void vtkRangeHandlesItem::SetActiveHandlePosition(double position)
{
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    // Clamp the position and set the handle position
    double bounds[4];
    double clampedPos[2] = { position, 1 };
    this->GetBounds(bounds);

    double minRange = bounds[0];
    double maxRange = bounds[1];
    bounds[0] += this->HandleDelta;
    bounds[1] -= this->HandleDelta;

    vtkPlot::ClampPos(clampedPos, bounds);

    this->ActiveHandlePosition = clampedPos[0];

    // Correct the position for range set
    if (this->ActiveHandle == vtkPlotRangeHandlesItem::LEFT_HANDLE)
    {
      position -= this->HandleDelta;
    }
    else // if (this->ActiveHandle == vtkRangeHandlesItem::RIGHT_HANDLE)
    {
      position += this->HandleDelta;
    }

    // Make the range value stick to the range for easier use
    if (minRange - this->HandleDelta <= position && position <= minRange + this->HandleDelta)
    {
      position = minRange;
    }
    if (maxRange - this->HandleDelta <= position && position <= maxRange + this->HandleDelta)
    {
      position = maxRange;
    }

    // Transform it to data and set it
    double unused;
    this->TransformScreenToData(position, 1, this->ActiveHandleRangeValue, unused);
  }
}
