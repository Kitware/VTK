/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColorLegend.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"
#include "vtkScalarsToColors.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorLegend);

//-----------------------------------------------------------------------------
vtkColorLegend::vtkColorLegend()
{
  this->Interpolate = true;
  this->Axis = vtkSmartPointer<vtkAxis>::New();
  this->Axis->SetPosition(vtkAxis::RIGHT);
  this->AddItem(this->Axis);
  this->SetInline(false);
  this->SetHorizontalAlignment(vtkChartLegend::RIGHT);
  this->SetVerticalAlignment(vtkChartLegend::BOTTOM);

  this->Callback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(vtkColorLegend::OnScalarsToColorsModified);

  this->TransferFunction = NULL;

  this->Orientation = vtkColorLegend::VERTICAL;

  this->Position.Set(0.0, 0.0, 0.0, 0.0);
  this->CustomPositionSet = false;
  this->DrawBorder = false;
}

//-----------------------------------------------------------------------------
vtkColorLegend::~vtkColorLegend()
{
}

//-----------------------------------------------------------------------------
void vtkColorLegend::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interpolate: " << this->Interpolate << endl;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::GetBounds(double bounds[4])
{
  if (this->TransferFunction)
  {
    bounds[0] = this->TransferFunction->GetRange()[0];
    bounds[1] = this->TransferFunction->GetRange()[1];
  }
  else
  {
    bounds[0] = 0.0;
    bounds[1] = 1.0;
  }
  bounds[2] = 0.0;
  bounds[3] = 1.0;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::Update()
{
  if (this->ImageData == 0 ||
      this->ImageData->GetMTime() < this->GetMTime())
  {
    this->ComputeTexture();
  }

  // check if the range of our TransferFunction changed
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1])
  {
    vtkWarningMacro(<< "The color transfer function seems to be empty.");
    this->Axis->Update();
    return;
  }

  double axisBounds[2];
  this->Axis->GetUnscaledRange(axisBounds);
  if (bounds[0] != axisBounds[0] || bounds[1] != axisBounds[1])
  {
    this->Axis->SetUnscaledRange(bounds[0], bounds[1]);
  }

  this->Axis->Update();
}

//-----------------------------------------------------------------------------
bool vtkColorLegend::Paint(vtkContext2D* painter)
{
  if (this->TransferFunction == NULL)
  {
    return true;
  }

  this->GetBoundingRect(painter);

  if (this->DrawBorder)
  {
    // Draw a box around the legend.
    painter->ApplyPen(this->Pen.GetPointer());
    painter->ApplyBrush(this->Brush.GetPointer());
    painter->DrawRect(this->Rect.GetX(), this->Rect.GetY(),
                      this->Rect.GetWidth(), this->Rect.GetHeight());
  }

  painter->DrawImage(this->Position, this->ImageData);

  this->Axis->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetTransferFunction(vtkScalarsToColors* transfer)
{
  this->TransferFunction = transfer;
}

//-----------------------------------------------------------------------------
vtkScalarsToColors * vtkColorLegend::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetPoint(float x, float y)
{
  this->Superclass::SetPoint(x, y);
  this->CustomPositionSet = false;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetTextureSize(float w, float h)
{
  this->Position.SetWidth(w);
  this->Position.SetHeight(h);
  this->CustomPositionSet = false;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetPosition(const vtkRectf& pos)
{
  this->Position = pos;
  this->SetPoint(pos[0], pos[1]);
  this->UpdateAxisPosition();
  this->CustomPositionSet = true;
}

//-----------------------------------------------------------------------------
vtkRectf vtkColorLegend::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
vtkRectf vtkColorLegend::GetBoundingRect(vtkContext2D *painter)
{
  if (this->CacheBounds && this->RectTime > this->GetMTime() &&
      this->RectTime > this->PlotTime &&
      this->RectTime > this->Axis->GetMTime())
  {
    return this->Rect;
  }

  if (!this->CustomPositionSet)
  {
    // if the Position ivar was not explicitly set, we compute the
    // location of the lower left point of the legend here.
    float posX = floor(this->Point[0]);
    float posY = floor(this->Point[1]);
    float posW = this->Position.GetWidth();
    float posH = this->Position.GetHeight();

    if (this->Orientation == vtkColorLegend::VERTICAL)
    {
      // For vertical orientation, we need to move our anchor point
      // further to the left to accommodate the width of the axis.
      // To do this, we query our axis to get its preliminary bounds.
      // Even though its position has not yet been set, its width &
      // height should still be accurate.
      this->UpdateAxisPosition();
      this->Axis->Update();
      vtkRectf axisRect = this->Axis->GetBoundingRect(painter);
      posX -= axisRect.GetWidth();
    }

    // Compute bottom left point based on current alignment.
    if (this->HorizontalAlignment == vtkChartLegend::CENTER)
    {
      posX -= posW / 2.0;
    }
    else if (this->HorizontalAlignment == vtkChartLegend::RIGHT)
    {
      posX -= posW;
    }
    if (this->VerticalAlignment == vtkChartLegend::CENTER)
    {
      posY -= posH / 2.0;
    }
    else if (this->VerticalAlignment == vtkChartLegend::TOP)
    {
      posY -= posH;
    }

    this->Position.SetX(posX);
    this->Position.SetY(posY);
    this->UpdateAxisPosition();
  }

  this->Axis->Update();
  vtkRectf axisRect = this->Axis->GetBoundingRect(painter);

  if (this->Orientation == vtkColorLegend::HORIZONTAL)
  {
    // "+ 1" so the texture doesn't obscure the border
    this->Rect = vtkRectf(this->Position.GetX(),
                          this->Position.GetY() - axisRect.GetHeight() + 1,
                          this->Position.GetWidth() + 1,
                          this->Position.GetHeight() + axisRect.GetHeight());
  }
  else
  {
    this->Rect = vtkRectf(this->Position.GetX(),
                          this->Position.GetY(),
                          this->Position.GetWidth() + axisRect.GetWidth(),
                          this->Position.GetHeight());
  }

  this->RectTime.Modified();
  return this->Rect;
}


//-----------------------------------------------------------------------------
void vtkColorLegend::ComputeTexture()
{
  if (this->TransferFunction == NULL)
  {
    return;
  }

  if (!this->ImageData)
  {
    this->ImageData = vtkSmartPointer<vtkImageData>::New();
  }
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1])
  {
    vtkWarningMacro(<< "The color transfer function seems to be empty.");
    return;
  }

  // Set the axis up
  this->Axis->SetUnscaledRange(bounds[0], bounds[1]);
  //this->Axis->AutoScale();

  // Could depend on the screen resolution
  const int dimension = 256;
  double* values = new double[dimension];
  // Texture 1D
  if (this->Orientation == vtkColorLegend::VERTICAL)
  {
    this->ImageData->SetExtent(0, 0,
                               0, dimension-1,
                               0, 0);
  }
  else
  {
    this->ImageData->SetExtent(0, dimension-1,
                               0, 0,
                               0, 0);
  }
  this->ImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  for (int i = 0; i < dimension; ++i)
  {
    values[i] = bounds[0] + i * (bounds[1] - bounds[0]) / (dimension - 1);
  }
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->ImageData->GetScalarPointer());
  this->TransferFunction->MapScalarsThroughTable2(
    values, ptr, VTK_DOUBLE, dimension, 1, 3);
  delete [] values;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::OnScalarsToColorsModified(vtkObject* caller,
                                               unsigned long eid,
                                               void *clientdata,
                                               void* calldata)
{
  vtkColorLegend* self =
    reinterpret_cast<vtkColorLegend*>(clientdata);
  self->ScalarsToColorsModified(caller, eid, calldata);
}

//-----------------------------------------------------------------------------
void vtkColorLegend::ScalarsToColorsModified(vtkObject* vtkNotUsed(object),
                                             unsigned long vtkNotUsed(eid),
                                             void* vtkNotUsed(calldata))
{
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetOrientation(int orientation)
{
  if (orientation < 0 || orientation > 1)
  {
    vtkErrorMacro("Error, invalid orientation value supplied: " << orientation)
    return;
  }
  this->Orientation = orientation;
  if (this->Orientation == vtkColorLegend::HORIZONTAL)
  {
    this->Axis->SetPosition(vtkAxis::BOTTOM);
  }
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetTitle(const vtkStdString &title)
{
  this->Axis->SetTitle(title);
}

//-----------------------------------------------------------------------------
vtkStdString vtkColorLegend::GetTitle()
{
  return this->Axis->GetTitle();
}

//-----------------------------------------------------------------------------
void vtkColorLegend::UpdateAxisPosition()
{
  if (this->Orientation == vtkColorLegend::VERTICAL)
  {
    this->Axis->SetPoint1(
      vtkVector2f(this->Position.GetX() + this->Position.GetWidth(),
                  this->Position.GetY()));
    this->Axis->SetPoint2(
      vtkVector2f(this->Position.GetX() + this->Position.GetWidth(),
                  this->Position.GetY() + this->Position.GetHeight()));
  }
  else
  {
    this->Axis->SetPoint1(
      vtkVector2f(this->Position.GetX(), this->Position.GetY()));
    this->Axis->SetPoint2(
      vtkVector2f(this->Position.GetX() + this->Position.GetWidth(),
                  this->Position.GetY()));
  }
}

//-----------------------------------------------------------------------------
bool vtkColorLegend::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  bool retval = this->Superclass::MouseMoveEvent(mouse);
  this->Position[0] = this->Point[0];
  this->Position[1] = this->Point[1];
  this->UpdateAxisPosition();
  return retval;
}
