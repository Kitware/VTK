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
  this->SetInline(false);
  this->SetHorizontalAlignment(vtkChartLegend::RIGHT);
  this->SetVerticalAlignment(vtkChartLegend::BOTTOM);

  this->Callback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(vtkColorLegend::OnScalarsToColorsModified);

  this->TransferFunction = 0;
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
  this->Axis->Update();
}

//-----------------------------------------------------------------------------
bool vtkColorLegend::Paint(vtkContext2D* painter)
{
  painter->DrawImage(this->Position, this->ImageData);

  this->Axis->Paint(painter);

  return true;
}

void vtkColorLegend::SetTransferFunction(vtkScalarsToColors* transfer)
{
  this->TransferFunction = transfer;
}

vtkScalarsToColors * vtkColorLegend::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void vtkColorLegend::SetPosition(const vtkRectf& pos)
{
  this->Position = pos;
  this->Axis->SetPoint1(vtkVector2f(pos.GetX() + pos.GetWidth(), pos.GetY()));
  this->Axis->SetPoint2(vtkVector2f(pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight()));
}

//-----------------------------------------------------------------------------
vtkRectf vtkColorLegend::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
vtkRectf vtkColorLegend::GetBoundingRect(vtkContext2D *painter)
{
  if (this->RectTime > this->GetMTime() && this->RectTime > this->PlotTime &&
      this->RectTime > this->Axis->GetMTime())
    {
    return this->Rect;
    }

  this->Axis->Update();
  vtkRectf axisRect = this->Axis->GetBoundingRect(painter);

  // Default point placement is bottom left.
  this->Rect = vtkRectf(0.0, 0.0,
                        this->SymbolWidth + axisRect.GetWidth(),
                        this->Position.GetHeight() + axisRect.GetHeight());

  this->RectTime.Modified();
  return this->Rect;
}


//-----------------------------------------------------------------------------
void vtkColorLegend::ComputeTexture()
{
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

  // Could depend of the screen resolution
  const int dimension = 256;
  double* values = new double[dimension];
  // Texture 1D
  this->ImageData->SetExtent(0, 0,
                             0, dimension-1,
                             0, 0);
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
