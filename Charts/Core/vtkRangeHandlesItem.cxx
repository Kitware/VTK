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
#include "vtkTransform2D.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRangeHandlesItem);
vtkSetObjectImplementationMacro(
  vtkRangeHandlesItem, ColorTransferFunction, vtkColorTransferFunction);

//-----------------------------------------------------------------------------
vtkRangeHandlesItem::vtkRangeHandlesItem()
{
  this->Brush->SetColor(125, 135, 144, 200);
  this->ActiveHandleBrush->SetColor(255, 0, 255, 200);
  this->RangeLabelBrush->SetColor(255, 255, 255, 200);
}

//-----------------------------------------------------------------------------
vtkRangeHandlesItem::~vtkRangeHandlesItem()
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkRangeHandlesItem::ComputeHandlesDrawRange()
{
  double screenBounds[4];
  this->GetBounds(screenBounds);
  this->HandleDelta = static_cast<float>((screenBounds[1] - screenBounds[0]) / 200.0);
  if (this->ActiveHandle == vtkRangeHandlesItem::LEFT_HANDLE)
  {
    this->LeftHandleDrawRange[0] = this->ActiveHandlePosition - this->HandleDelta;
    this->LeftHandleDrawRange[1] = this->ActiveHandlePosition + this->HandleDelta;
  }
  else
  {
    this->LeftHandleDrawRange[0] = screenBounds[0];
    this->LeftHandleDrawRange[1] = screenBounds[0] + 2.0f * this->HandleDelta;
  }
  if (this->ActiveHandle == vtkRangeHandlesItem::RIGHT_HANDLE)
  {
    this->RightHandleDrawRange[0] = this->ActiveHandlePosition - this->HandleDelta;
    this->RightHandleDrawRange[1] = this->ActiveHandlePosition + this->HandleDelta;
  }
  else
  {
    this->RightHandleDrawRange[0] = screenBounds[1];
    this->RightHandleDrawRange[1] = screenBounds[1] - 2.0f * this->HandleDelta;
  }
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::Paint(vtkContext2D* painter)
{
  if (!this->Visible || !this->ColorTransferFunction)
  {
    return false;
  }

  vtkNew<vtkPen> transparentPen;
  transparentPen->SetLineType(vtkPen::NO_PEN);
  painter->ApplyPen(transparentPen);

  // Compute handles draw range
  this->ComputeHandlesDrawRange();

  // Draw Left Handle
  if (this->ActiveHandle == vtkRangeHandlesItem::LEFT_HANDLE ||
    this->HoveredHandle == vtkRangeHandlesItem::LEFT_HANDLE)
  {
    painter->ApplyBrush(this->ActiveHandleBrush);
  }
  else
  {
    painter->ApplyBrush(this->Brush);
  }
  painter->DrawQuad(this->LeftHandleDrawRange[0], 0, this->LeftHandleDrawRange[0], 1,
    this->LeftHandleDrawRange[1], 1, this->LeftHandleDrawRange[1], 0);

  // Draw Right Handle
  if (this->ActiveHandle == vtkRangeHandlesItem::RIGHT_HANDLE ||
    this->HoveredHandle == vtkRangeHandlesItem::RIGHT_HANDLE)
  {
    painter->ApplyBrush(this->ActiveHandleBrush);
  }
  else
  {
    painter->ApplyBrush(this->Brush);
  }
  painter->DrawQuad(this->RightHandleDrawRange[0], 0, this->RightHandleDrawRange[0], 1,
    this->RightHandleDrawRange[1], 1, this->RightHandleDrawRange[1], 0);

  // Draw range info
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE ||
    this->HoveredHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    double range[2];
    this->GetHandlesRange(range);
    std::string label =
      "Range : [" + std::to_string(range[0]) + ", " + std::to_string(range[1]) + "]";

    vtkVector2f labelBounds[2];
    painter->ComputeStringBounds(label, labelBounds[0].GetData());

    float scale[2];
    painter->GetTransform()->GetScale(scale);
    double screenBounds[4];
    this->GetBounds(screenBounds);

    float labelStart =
      static_cast<float>(screenBounds[1] + screenBounds[0]) / 2.0f - labelBounds[1].GetX() / 2.0f;
    painter->ApplyBrush(this->RangeLabelBrush);
    painter->DrawRect(labelStart - 5.0f / scale[0], 0, labelBounds[1].GetX() + 8.0f / scale[0],
      labelBounds[1].GetY() + 10.0f / scale[1]);
    painter->DrawString(labelStart, 3.0f / scale[1], label);
  }

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
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
  os << indent << "HoveredHandle: " << this->HoveredHandle << endl;
  os << indent << "ActiveHandle: " << this->ActiveHandle << endl;
  os << indent << "ActiveHandlePosition: " << this->ActiveHandlePosition << endl;
  os << indent << "ActiveHandleRangeValue: " << this->ActiveHandleRangeValue << endl;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::Hit(const vtkContextMouseEvent& mouse)
{
  // Add more tolerance than the mouse interaction to make sure handles do
  // not stay highlighted when moving the mouse
  vtkVector2f vpos = mouse.GetPos();
  vtkVector2f tolerance = { 2.0f * static_cast<float>(this->HandleDelta), 0 };
  return this->FindRangeHandle(vpos, tolerance) != vtkRangeHandlesItem::NO_HANDLE;
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  vtkVector2f vpos = mouse.GetPos();
  vtkVector2f tolerance = { 2.0f * static_cast<float>(this->HandleDelta), 0 };
  this->HoveredHandle = vtkRangeHandlesItem::NO_HANDLE;
  this->ActiveHandle = this->FindRangeHandle(vpos, tolerance);
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    this->SetActiveHandlePosition(vpos.GetX());
    this->GetScene()->SetDirty(true);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    vtkVector2f vpos = mouse.GetPos();
    this->SetActiveHandlePosition(vpos.GetX());
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->ActiveHandle = vtkRangeHandlesItem::NO_HANDLE;
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    vtkVector2f vpos = mouse.GetPos();
    this->SetActiveHandlePosition(vpos.GetX());
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::MouseEnterEvent(const vtkContextMouseEvent& mouse)
{
  if (this->ActiveHandle == vtkRangeHandlesItem::NO_HANDLE)
  {
    vtkVector2f vpos = mouse.GetPos();
    vtkVector2f tolerance = { 2.0f * static_cast<float>(this->HandleDelta), 0 };
    this->HoveredHandle = this->FindRangeHandle(vpos, tolerance);
    if (this->HoveredHandle != this->PreviousHoveredHandle)
    {
      this->GetScene()->SetDirty(true);
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkRangeHandlesItem::MouseLeaveEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  if (this->ActiveHandle == vtkRangeHandlesItem::NO_HANDLE &&
    this->HoveredHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    this->HoveredHandle = vtkRangeHandlesItem::NO_HANDLE;
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
int vtkRangeHandlesItem::FindRangeHandle(const vtkVector2f& point, const vtkVector2f& tolerance)
{
  double pos[2];
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  if (0 - tolerance.GetY() <= pos[1] && pos[1] <= 1 + tolerance.GetY())
  {
    if (this->LeftHandleDrawRange[0] - tolerance.GetX() <= pos[0] &&
      pos[0] <= this->LeftHandleDrawRange[1] + tolerance.GetX())
    {
      return vtkRangeHandlesItem::LEFT_HANDLE;
    }
    else if (this->RightHandleDrawRange[0] - tolerance.GetX() <= pos[0] &&
      pos[0] <= this->RightHandleDrawRange[1] + tolerance.GetX())
    {
      return vtkRangeHandlesItem::RIGHT_HANDLE;
    }
  }
  return vtkRangeHandlesItem::NO_HANDLE;
}

//-----------------------------------------------------------------------------
void vtkRangeHandlesItem::GetHandlesRange(double range[2])
{
  this->ColorTransferFunction->GetRange(range);
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE)
  {
    range[this->ActiveHandle] = this->ActiveHandleRangeValue;
  }
}

//-----------------------------------------------------------------------------
void vtkRangeHandlesItem::SetActiveHandlePosition(double position)
{
  if (this->ActiveHandle != vtkRangeHandlesItem::NO_HANDLE)
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
    if (this->ActiveHandle == vtkRangeHandlesItem::LEFT_HANDLE)
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
