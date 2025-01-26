// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotRangeHandlesItem.h"

#include "vtkAxis.h"
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
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"

#include <sstream>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlotRangeHandlesItem);

//------------------------------------------------------------------------------
vtkPlotRangeHandlesItem::vtkPlotRangeHandlesItem()
{
  this->Brush->SetColor(125, 135, 144, 200);
  this->HighlightBrush->SetColor(255, 0, 255, 200);
  this->RangeLabelBrush->SetColor(255, 255, 255, 200);
}

//------------------------------------------------------------------------------
vtkPlotRangeHandlesItem::~vtkPlotRangeHandlesItem() = default;

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::ComputeHandlesDrawRange()
{
  double screenBounds[4];
  this->GetBounds(screenBounds);
  this->ComputeHandleDelta(screenBounds);
  double range[2];
  this->GetHandlesRange(range);

  double unused;
  this->TransformDataToScreen(range[0], 1, range[0], unused);
  this->TransformDataToScreen(range[1], 1, range[1], unused);

  this->ComputeRange(range);
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::ComputeRange(double* range)
{
  if (this->ActiveHandle == vtkPlotRangeHandlesItem::LEFT_HANDLE)
  {
    double previousLeftValue = this->LeftHandleDrawRange[0];
    this->LeftHandleDrawRange[0] = this->ActiveHandlePosition - this->HandleDelta;
    this->LeftHandleDrawRange[1] = this->ActiveHandlePosition + this->HandleDelta;
    if (this->SynchronizeRangeHandles)
    {
      double leftShift = this->LeftHandleDrawRange[0] - previousLeftValue;
      this->RightHandleDrawRange[0] += leftShift;
      this->RightHandleDrawRange[1] += leftShift;
      return;
    }
  }
  else
  {
    this->LeftHandleDrawRange[0] = range[0];
    this->LeftHandleDrawRange[1] = range[0] + 2.0 * this->HandleDelta;
  }

  if (this->ActiveHandle == vtkPlotRangeHandlesItem::RIGHT_HANDLE)
  {
    this->RightHandleDrawRange[0] = this->ActiveHandlePosition - this->HandleDelta;
    this->RightHandleDrawRange[1] = this->ActiveHandlePosition + this->HandleDelta;
  }
  else
  {
    this->RightHandleDrawRange[0] = range[1];
    this->RightHandleDrawRange[1] = range[1] - 2.0 * this->HandleDelta;
  }
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::ComputeHandleDelta(double screenBounds[4])
{
  // Try to use the scene to produce correctly size handles
  double width = 400.0;
  vtkContextScene* scene = this->GetScene();
  if (scene && scene->GetSceneWidth() > 0 && scene->GetSceneHeight() > 0)
  {
    if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
    {
      width = static_cast<double>(scene->GetSceneWidth());
    }
    else // HORIZONTAL
    {
      width = static_cast<double>(scene->GetSceneHeight());
    }
  }

  this->HandleDelta =
    this->HandleWidth * static_cast<float>((screenBounds[1] - screenBounds[0]) / width);
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::Paint(vtkContext2D* painter)
{
  if (!this->Visible)
  {
    return false;
  }

  vtkNew<vtkPen> transparentPen;
  transparentPen->SetLineType(vtkPen::NO_PEN);
  painter->ApplyPen(transparentPen);

  // Compute handles draw range
  this->ComputeHandlesDrawRange();

  int highlightedHandle = this->ActiveHandle;
  if (highlightedHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    highlightedHandle = this->HoveredHandle;
  }

  // Draw Left Handle
  if (highlightedHandle == vtkPlotRangeHandlesItem::LEFT_HANDLE)
  {
    painter->ApplyBrush(this->HighlightBrush);
  }
  else
  {
    painter->ApplyBrush(this->Brush);
  }

  double length[2] = { this->Extent[2], this->Extent[3] };
  if (this->ExtentToAxisRange)
  {
    double screenBounds[4];
    this->GetBounds(screenBounds);
    length[0] = screenBounds[2];
    length[1] = screenBounds[3];
  }

  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    painter->DrawQuad(this->LeftHandleDrawRange[0], length[0], this->LeftHandleDrawRange[0],
      length[1], this->LeftHandleDrawRange[1], length[1], this->LeftHandleDrawRange[1], length[0]);
  }
  else // HORIZONTAL
  {
    painter->DrawQuad(length[0], this->LeftHandleDrawRange[0], length[1],
      this->LeftHandleDrawRange[0], length[1], this->LeftHandleDrawRange[1], length[0],
      this->LeftHandleDrawRange[1]);
  }

  // Draw Right Handle
  if (highlightedHandle == vtkPlotRangeHandlesItem::RIGHT_HANDLE)
  {
    painter->ApplyBrush(this->HighlightBrush);
  }
  else
  {
    painter->ApplyBrush(this->Brush);
  }

  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    painter->DrawQuad(this->RightHandleDrawRange[0], length[0], this->RightHandleDrawRange[0],
      length[1], this->RightHandleDrawRange[1], length[1], this->RightHandleDrawRange[1],
      length[0]);
  }
  else // HORIZONTAL
  {
    painter->DrawQuad(length[0], this->RightHandleDrawRange[0], length[1],
      this->RightHandleDrawRange[0], length[1], this->RightHandleDrawRange[1], length[0],
      this->RightHandleDrawRange[1]);
  }

  // Draw range info
  if (highlightedHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    this->InvokeEvent(vtkCommand::HighlightEvent);
    double range[2];
    this->GetHandlesRange(range);
    std::stringstream label;
    label << "Range : [" << this->GetNumber(range[0], nullptr) << ", "
          << this->GetNumber(range[1], nullptr) << "]";

    vtkVector2f labelBounds[2];
    painter->ComputeStringBounds(label.str(), labelBounds[0].GetData());

    float labelStartX = this->HoveredPosition[0] - labelBounds[1].GetX() / 2.0f;
    float labelStartY = this->HoveredPosition[1] - labelBounds[1].GetY() * 2.0f;

    // When the tooltip is not locked to the mouse position, place it at the
    // middle of the X axis.
    if (!this->LockTooltipToMouse)
    {
      double screenBounds[4];
      this->GetBounds(screenBounds);

      labelStartX =
        static_cast<float>(screenBounds[1] + screenBounds[0]) / 2.0f - labelBounds[1].GetX() / 2.0f;
      labelStartY = 0;
    }

    float scale[2];
    painter->GetTransform()->GetScale(scale);

    // Make sure justification is set to left as this is not guaranteed by all
    // types of vtkChart.
    vtkTextProperty* currentTextProp = painter->GetTextProp();
    int currentJustification = currentTextProp->GetJustification();
    currentTextProp->SetJustificationToLeft();
    painter->ApplyTextProp(currentTextProp);

    painter->ApplyBrush(this->RangeLabelBrush);
    painter->DrawRect(labelStartX - 5.0f / scale[0], labelStartY,
      labelBounds[1].GetX() + 8.0f / scale[0], labelBounds[1].GetY() + 10.0f / scale[1]);
    painter->DrawString(labelStartX, labelStartY + 3.0f / scale[1], label.str());

    // Reset justification
    currentTextProp->SetJustification(currentJustification);
    painter->ApplyTextProp(currentTextProp);
  }

  this->PaintChildren(painter);
  return true;
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "HandleWidth: " << this->HandleWidth << endl;
  os << indent << "HoveredHandle: " << this->HoveredHandle << endl;
  os << indent << "ActiveHandle: " << this->ActiveHandle << endl;
  os << indent << "ActiveHandlePosition: " << this->ActiveHandlePosition << endl;
  os << indent << "ActiveHandleRangeValue: " << this->ActiveHandleRangeValue << endl;
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::GetBounds(double* bounds)
{
  double range[2] = { this->Extent[0], this->Extent[1] };
  double length[2] = { this->Extent[2], this->Extent[3] };

  this->GetAxesUnscaledRange(range, length);

  this->TransformDataToScreen(range[0], length[0], bounds[0], bounds[2]);
  this->TransformDataToScreen(range[1], length[1], bounds[1], bounds[3]);
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::GetAxesRange(double* abcissaRange, double* ordinateRange)
{
  // Set default values in case axes are not set
  if (abcissaRange)
  {
    abcissaRange[0] = 0;
    abcissaRange[1] = 0;
  }

  if (ordinateRange)
  {
    ordinateRange[0] = 0;
    ordinateRange[1] = 0;
  }

  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    if (this->GetXAxis())
    {
      this->GetXAxis()->GetRange(abcissaRange);
    }

    if (this->GetYAxis())
    {
      this->GetYAxis()->GetRange(ordinateRange);
    }
  }
  else // HORIZONTAL
  {
    if (this->GetYAxis())
    {
      this->GetYAxis()->GetRange(abcissaRange);
    }

    if (this->GetXAxis())
    {
      this->GetXAxis()->GetRange(ordinateRange);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::GetAxesUnscaledRange(double* abcissaRange, double* ordinateRange)
{
  // Set default values in case axes are not set
  if (abcissaRange)
  {
    abcissaRange[0] = 0;
    abcissaRange[1] = 0;
  }

  if (ordinateRange)
  {
    ordinateRange[0] = 0;
    ordinateRange[1] = 0;
  }

  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    if (this->GetXAxis())
    {
      this->GetXAxis()->GetUnscaledRange(abcissaRange);
    }

    if (this->GetYAxis())
    {
      this->GetYAxis()->GetUnscaledRange(ordinateRange);
    }
  }
  else // HORIZONTAL
  {
    if (this->GetYAxis())
    {
      this->GetYAxis()->GetUnscaledRange(abcissaRange);
    }

    if (this->GetXAxis())
    {
      this->GetXAxis()->GetUnscaledRange(ordinateRange);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::TransformScreenToData(
  double inX, double inY, double& outX, double& outY)
{
  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    this->Superclass::TransformScreenToData(inX, inY, outX, outY);
  }
  else
  {
    this->Superclass::TransformScreenToData(inY, inX, outY, outX);
  }
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::TransformDataToScreen(
  double inX, double inY, double& outX, double& outY)
{
  if (this->HandleOrientation == vtkPlotRangeHandlesItem::VERTICAL)
  {
    this->Superclass::TransformDataToScreen(inX, inY, outX, outY);
  }
  else
  {
    this->Superclass::TransformDataToScreen(inY, inX, outY, outX);
  }
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::Hit(const vtkContextMouseEvent& mouse)
{
  if (!this->Interactive || !this->Visible)
  {
    return false;
  }

  // Add more tolerance than the mouse interaction to make sure handles do
  // not stay highlighted when moving the mouse
  vtkVector2f vpos = mouse.GetPos();
  vtkVector2f tolerance = { 2.0f * this->HandleDelta, 0 };
  return this->FindRangeHandle(vpos, tolerance) != vtkPlotRangeHandlesItem::NO_HANDLE;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  vtkVector2f vpos = mouse.GetPos();
  vtkVector2f tolerance = { 2.0f * this->HandleDelta, 0 };
  this->ActiveHandle = this->FindRangeHandle(vpos, tolerance);
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    this->HoveredHandle = this->ActiveHandle;
    this->SetActiveHandlePosition(vpos[this->HandleOrientation]);
    this->SetCursor(VTK_CURSOR_SIZEWE - this->HandleOrientation);
    this->GetScene()->SetDirty(true);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    vtkVector2f vpos = mouse.GetPos();
    this->SetActiveHandlePosition(vpos[this->HandleOrientation]);

    if (this->IsActiveHandleMoved(3.0 * this->HandleDelta))
    {
      this->HoveredHandle = vtkPlotRangeHandlesItem::NO_HANDLE;
    }
    if (this->HoveredHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
    {
      this->SetCursor(VTK_CURSOR_SIZEWE - this->HandleOrientation);
    }
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->ActiveHandle = vtkPlotRangeHandlesItem::NO_HANDLE;
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    vtkVector2f vpos = mouse.GetPos();
    this->SetActiveHandlePosition(vpos[this->HandleOrientation]);
    this->HoveredPosition[this->HandleOrientation] = this->ActiveHandlePosition;
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseEnterEvent(const vtkContextMouseEvent& mouse)
{
  vtkVector2f vpos = mouse.GetPos();
  vtkVector2f tolerance = { 2.0f * this->HandleDelta, 0 };
  this->HoveredHandle = this->FindRangeHandle(vpos, tolerance);
  if (this->HoveredHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    return false;
  }
  this->SetCursor(VTK_CURSOR_SIZEWE - this->HandleOrientation);
  this->GetScene()->SetDirty(true);

  if (this->ActiveHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    this->HoveredPosition[this->HandleOrientation] = vpos[this->HandleOrientation];
    this->HoveredPosition[1 - this->HandleOrientation] = vpos[1 - this->HandleOrientation];
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseLeaveEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  if (this->HoveredHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    return false;
  }

  this->HoveredHandle = vtkPlotRangeHandlesItem::NO_HANDLE;
  this->GetScene()->SetDirty(true);

  if (this->ActiveHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    this->SetCursor(VTK_CURSOR_DEFAULT);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    this->HoveredHandle = vtkPlotRangeHandlesItem::NO_HANDLE;
    this->InvokeEvent(vtkCommand::LeftButtonDoubleClickEvent);
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkPlotRangeHandlesItem::FindRangeHandle(const vtkVector2f& point, const vtkVector2f& tolerance)
{
  double pos[2];
  pos[0] = point.GetX();
  pos[1] = point.GetY();

  double length[2] = { this->Extent[2], this->Extent[3] };
  if (this->ExtentToAxisRange)
  {
    double screenBounds[4];
    this->GetBounds(screenBounds);
    length[0] = screenBounds[2];
    length[1] = screenBounds[3];
  }

  if (length[0] - tolerance.GetY() <= pos[1 - this->HandleOrientation] &&
    pos[1 - this->HandleOrientation] <= length[1] + tolerance.GetY())
  {
    if (this->LeftHandleDrawRange[0] - tolerance.GetX() <= pos[this->HandleOrientation] &&
      pos[this->HandleOrientation] <= this->LeftHandleDrawRange[1] + tolerance.GetX())
    {
      return vtkPlotRangeHandlesItem::LEFT_HANDLE;
    }
    else if (this->RightHandleDrawRange[0] - tolerance.GetX() <= pos[this->HandleOrientation] &&
      pos[this->HandleOrientation] <= this->RightHandleDrawRange[1] + tolerance.GetX())
    {
      return vtkPlotRangeHandlesItem::RIGHT_HANDLE;
    }
  }
  return vtkPlotRangeHandlesItem::NO_HANDLE;
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::GetHandlesRange(double range[2])
{
  if (this->ActiveHandle != vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    double previousExtent = this->Extent[this->ActiveHandle];
    this->Extent[this->ActiveHandle] = this->ActiveHandleRangeValue;
    if (this->SynchronizeRangeHandles && this->ActiveHandle == vtkPlotRangeHandlesItem::LEFT_HANDLE)
    {
      double shift = this->ActiveHandleRangeValue - previousExtent;
      this->Extent[1] += shift;
    }
  }
  range[0] = this->Extent[0];
  range[1] = this->Extent[1];
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::SetActiveHandlePosition(double position)
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
    else // if (this->ActiveHandle == vtkPlotRangeHandlesItem::RIGHT_HANDLE)
    {
      position += this->HandleDelta;
    }

    // Make the range value stick to the range for easier use
    if (minRange - this->HandleDelta <= clampedPos[0] &&
      clampedPos[0] <= minRange + this->HandleDelta)
    {
      position = minRange;
    }
    if (maxRange - this->HandleDelta <= clampedPos[0] &&
      clampedPos[0] <= maxRange + this->HandleDelta)
    {
      position = maxRange;
    }

    // Transform it to data and set it
    double unused;
    this->TransformScreenToData(position, 1, this->ActiveHandleRangeValue, unused);
  }
}

//------------------------------------------------------------------------------
bool vtkPlotRangeHandlesItem::IsActiveHandleMoved(double tolerance)
{
  if (this->ActiveHandle == vtkPlotRangeHandlesItem::NO_HANDLE)
  {
    return false;
  }

  double unused, position;
  this->TransformDataToScreen(this->ActiveHandleRangeValue, 1, position, unused);

  double bounds[4];
  this->GetBounds(bounds);

  return (bounds[this->ActiveHandle] - tolerance <= position &&
    position <= bounds[this->ActiveHandle] + tolerance);
}

//------------------------------------------------------------------------------
void vtkPlotRangeHandlesItem::SetCursor(int cursor)
{
  vtkRenderer* renderer = this->GetScene()->GetRenderer();
  if (renderer)
  {
    vtkRenderWindow* window = renderer->GetRenderWindow();
    if (window)
    {
      window->SetCurrentCursor(cursor);
    }
  }
}
VTK_ABI_NAMESPACE_END
