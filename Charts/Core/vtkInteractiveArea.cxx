// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <algorithm>

#include "vtkCommand.h"
#include "vtkContextClip.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkInteractiveArea.h"
#include "vtkObjectFactory.h"
#include "vtkPlotGrid.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"

//@{
/**
 * Hold mouse action key-mappings and other action related resources.
 */
VTK_ABI_NAMESPACE_BEGIN
class vtkInteractiveArea::MouseActions
{
public:
  enum
  {
    MaxAction = 1
  };

  MouseActions()
  {
    this->Pan() = vtkContextMouseEvent::LEFT_BUTTON;
    // this->Zoom() = vtkContextMouseEvent::MIDDLE_BUTTON;
  }

  short& Pan() { return Data[0]; }
  // short& Zoom() { return Data[1]; }

  /**
   *  The box created as the mouse is dragged around the screen.
   */
  vtkRectf MouseBox;

private:
  MouseActions(MouseActions const&) = delete;
  void operator=(MouseActions const*) = delete;

  short Data[MaxAction];
};
//@}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkInteractiveArea);

//------------------------------------------------------------------------------
vtkInteractiveArea::vtkInteractiveArea()
  : Actions(new MouseActions)
{
  Superclass::Interactive = true;
}

//------------------------------------------------------------------------------
vtkInteractiveArea::~vtkInteractiveArea()
{
  delete Actions;
}

//------------------------------------------------------------------------------
void vtkInteractiveArea::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkInteractiveArea::SetAxisRange(vtkRectd const& data)
{
  /// TODO This might be a hack. The intention is to only reset the axis range in
  // Superclass::LayoutAxes at initialization and not during interaction.
  if (!this->Scene->GetDirty())
  {
    Superclass::SetAxisRange(data);
  }
}

//------------------------------------------------------------------------------
bool vtkInteractiveArea::Paint(vtkContext2D* painter)
{
  return Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
bool vtkInteractiveArea::Hit(const vtkContextMouseEvent& mouse)
{
  if (!this->Interactive)
  {
    return false;
  }

  vtkVector2f const pos(mouse.GetScenePos());
  vtkVector2i const bottomLeft = this->DrawAreaGeometry.GetBottomLeft();
  vtkVector2i const topRight = this->DrawAreaGeometry.GetTopRight();

  return pos[0] > bottomLeft[0] && pos[0] < topRight[0] && pos[1] > bottomLeft[1] &&
    pos[1] < topRight[1];
}

//------------------------------------------------------------------------------
bool vtkInteractiveArea::MouseWheelEvent(const vtkContextMouseEvent& vtkNotUsed(mouse), int delta)
{
  // Adjust the grid (delta stands for the number of wheel clicks)
  this->RecalculateTickSpacing(this->TopAxis, delta);
  this->RecalculateTickSpacing(this->BottomAxis, delta);
  this->RecalculateTickSpacing(this->LeftAxis, delta);
  this->RecalculateTickSpacing(this->RightAxis, delta);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  // ComputeViewTransform is called through Superclass::Paint
  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//------------------------------------------------------------------------------
bool vtkInteractiveArea::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions->Pan())
  {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    vtkVector2d scenePos(mouse.GetScenePos().Cast<double>().GetData());
    vtkVector2d lastScenePos(mouse.GetLastScenePos().Cast<double>().GetData());
    vtkVector2d pos(0.0, 0.0);
    vtkVector2d last(0.0, 0.0);

    // Go from screen to scene coordinates to work out the delta
    vtkAxis* xAxis = this->BottomAxis;
    vtkAxis* yAxis = this->LeftAxis;
    vtkTransform2D* transform = this->Transform->GetTransform();
    transform->InverseTransformPoints(scenePos.GetData(), pos.GetData(), 1);
    transform->InverseTransformPoints(lastScenePos.GetData(), last.GetData(), 1);
    vtkVector2d delta = last - pos;
    delta[0] /= xAxis->GetScalingFactor();
    delta[1] /= yAxis->GetScalingFactor();

    // Now move the axis and recalculate the transform
    delta[0] = delta[0] > 0 ? std::min(delta[0], xAxis->GetMaximumLimit() - xAxis->GetMaximum())
                            : std::max(delta[0], xAxis->GetMinimumLimit() - xAxis->GetMinimum());

    delta[1] = delta[1] > 0 ? std::min(delta[1], yAxis->GetMaximumLimit() - yAxis->GetMaximum())
                            : std::max(delta[1], yAxis->GetMinimumLimit() - yAxis->GetMinimum());

    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    // ComputeViewTransform is called through Superclass::Paint
    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
bool vtkInteractiveArea::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions->Pan())
  {
    this->Actions->MouseBox.Set(mouse.GetPos().GetX(), mouse.GetPos().GetY(), 0.0, 0.0);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkInteractiveArea::RecalculateTickSpacing(vtkAxis* axis, int numClicks)
{
  double min = axis->GetMinimum();
  double max = axis->GetMaximum();
  double const increment = (max - min) * 0.1;

  if (increment > 0.0)
  {
    min += numClicks * increment;
    max -= numClicks * increment;
  }
  else
  {
    min -= numClicks * increment;
    max += numClicks * increment;
  }

  axis->SetMinimum(min);
  axis->SetMaximum(max);
  axis->RecalculateTickSpacing();
}

//------------------------------------------------------------------------------
void vtkInteractiveArea::ComputeViewTransform()
{
  double const minX = this->BottomAxis->GetMinimum();
  double const minY = this->LeftAxis->GetMinimum();

  vtkVector2d origin(minX, minY);
  vtkVector2d scale(this->BottomAxis->GetMaximum() - minX, this->LeftAxis->GetMaximum() - minY);

  vtkVector2d shift(0.0, 0.0);
  vtkVector2d factor(1.0, 1.0);
  /// TODO Cache and only compute if zoom changed
  this->ComputeZoom(origin, scale, shift, factor);

  this->BottomAxis->SetScalingFactor(factor[0]);
  this->BottomAxis->SetShift(shift[0]);
  this->LeftAxis->SetScalingFactor(factor[1]);
  this->LeftAxis->SetShift(shift[1]);

  // Update transform
  this->Transform->Identity();

  vtkRecti& boundsPixel = this->DrawAreaGeometry;
  float const xOrigin = static_cast<float>(boundsPixel.GetLeft());
  float const yOrigin = static_cast<float>(boundsPixel.GetBottom());
  this->Transform->Translate(xOrigin, yOrigin);

  float const xScalePixels = this->DrawAreaGeometry.GetWidth() / scale[0];
  float const yScalePixels = this->DrawAreaGeometry.GetHeight() / scale[1];
  this->Transform->Scale(xScalePixels, yScalePixels);

  float const xTrans = -(this->BottomAxis->GetMinimum() + shift[0]) * factor[0];
  float const yTrans = -(this->LeftAxis->GetMinimum() + shift[1]) * factor[1];
  this->Transform->Translate(xTrans, yTrans);
}

//------------------------------------------------------------------------------
void vtkInteractiveArea::ComputeZoom(
  vtkVector2d const& origin, vtkVector2d& scale, vtkVector2d& shift, vtkVector2d& factor)
{
  for (int i = 0; i < 2; ++i)
  {
    if (log10(fabs(origin[i]) / scale[i]) > 2)
    {
      shift[i] = floor(log10(fabs(origin[i]) / scale[i]) / 3.0) * 3.0;
      shift[i] = -origin[i];
    }
    if (fabs(log10(scale[i])) > 10)
    {
      // We need to scale the transform to show all data, do this in blocks.
      factor[i] = pow(10.0, floor(log10(scale[i]) / 10.0) * -10.0);
      scale[i] = scale[i] * factor[i];
    }
  }
}
VTK_ABI_NAMESPACE_END
