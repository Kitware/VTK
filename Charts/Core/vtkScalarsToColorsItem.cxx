/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkScalarsToColorsItem.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkScalarsToColorsItem::vtkScalarsToColorsItem()
{
  this->PolyLinePen = vtkPen::New();
  this->PolyLinePen->SetWidth(2.);
  this->PolyLinePen->SetColor(64, 64, 72); // Payne's grey, why not
  this->PolyLinePen->SetLineType(vtkPen::NO_PEN);

  this->Texture = nullptr;
  this->Interpolate = true;
  this->Shape = vtkPoints2D::New();
  this->Shape->SetDataTypeToFloat();
  this->Shape->SetNumberOfPoints(0);

  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkScalarsToColorsItem::OnScalarsToColorsModified);

  this->MaskAboveCurve = false;

  this->UserBounds[0] = this->UserBounds[2] = 0.0;
  this->UserBounds[1] = this->UserBounds[3] = -1.0;
}

//-----------------------------------------------------------------------------
vtkScalarsToColorsItem::~vtkScalarsToColorsItem()
{
  if (this->PolyLinePen)
  {
    this->PolyLinePen->Delete();
    this->PolyLinePen = nullptr;
  }
  if (this->Texture)
  {
    this->Texture->Delete();
    this->Texture = nullptr;
  }
  if (this->Shape)
  {
    this->Shape->Delete();
    this->Shape = nullptr;
  }
  if (this->Callback)
  {
    this->Callback->Delete();
    this->Callback = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::TransformDataToScreen(
    const double dataX, const double dataY, double &screenX, double &screenY)
{
  const bool logX = this->GetXAxis() && this->GetXAxis()->GetLogScaleActive();
  const bool logY = this->GetYAxis() && this->GetYAxis()->GetLogScaleActive();

  screenX = logX ? log10(dataX) : dataX;
  screenY = logY ? log10(dataY) : dataY;

  // now, shift/scale to screen space.
  const vtkRectd& ss = this->ShiftScale;
  screenX = (screenX + ss[0]) * ss[2];
  screenY = (screenY + ss[1]) * ss[3];
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::TransformScreenToData(
    const double screenX, const double screenY, double &dataX, double &dataY)
{
  // inverse shift/scale from screen space.
  const vtkRectd& ss = this->ShiftScale;
  dataX = (screenX / ss[2]) - ss[0];
  dataY = (screenY / ss[3]) - ss[1];

  const bool logX = this->GetXAxis() && this->GetXAxis()->GetLogScaleActive();
  const bool logY = this->GetYAxis() && this->GetYAxis()->GetLogScaleActive();

  if (logX)
  {
    dataX = pow(10., dataX);
  }
  if (logY)
  {
    dataY = pow(10., dataY);
  }
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interpolate: " << this->Interpolate << endl;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::GetBounds(double bounds[4])
{
  if (this->UserBounds[1] > this->UserBounds[0] &&
      this->UserBounds[3] > this->UserBounds[2])
  {
    bounds[0] = this->UserBounds[0];
    bounds[1] = this->UserBounds[1];
    bounds[2] = this->UserBounds[2];
    bounds[3] = this->UserBounds[3];
    return;
  }
  this->ComputeBounds(bounds);
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::ComputeBounds(double bounds[4])
{
  bounds[0] = 0.;
  bounds[1] = 1.;
  bounds[2] = 0.;
  bounds[3] = 1.;
}

//-----------------------------------------------------------------------------
bool vtkScalarsToColorsItem::Paint(vtkContext2D* painter)
{
  this->TextureWidth = this->GetScene()->GetViewWidth();
  if (this->Texture == nullptr ||
      this->Texture->GetMTime() < this->GetMTime())
  {
    this->ComputeTexture();
  }
  if (this->Texture == nullptr)
  {
    return false;
  }
  vtkSmartPointer<vtkPen> transparentPen = vtkSmartPointer<vtkPen>::New();
  transparentPen->SetLineType(vtkPen::NO_PEN);
  painter->ApplyPen(transparentPen);
  painter->GetBrush()->SetColorF(0., 0., 0., 1.);
  painter->GetBrush()->SetColorF(1., 1., 1., 1.);
  painter->GetBrush()->SetTexture(this->Texture);
  painter->GetBrush()->SetTextureProperties(
    (this->Interpolate ? vtkBrush::Nearest : vtkBrush::Linear) |
    vtkBrush::Stretch);
  const int size = this->Shape->GetNumberOfPoints();
  if (!this->MaskAboveCurve || size < 2)
  {
    double dbounds[4];
    this->GetBounds(dbounds);
    painter->DrawQuad(dbounds[0], dbounds[2],
                      dbounds[0], dbounds[3],
                      dbounds[1], dbounds[3],
                      dbounds[1], dbounds[2]);
  }
  else
  {
    const vtkRectd& ss = this->ShiftScale;

    vtkPoints2D* trapezoids = vtkPoints2D::New();
    trapezoids->SetNumberOfPoints(2*size);
    double point[2];
    vtkIdType j = -1;
    for (vtkIdType i = 0; i < size; ++i)
    {
      this->Shape->GetPoint(i, point);

      // shift/scale to scale from data space to rendering space.
      point[0] = (point[0] + ss[0]) * ss[2];
      point[1] = (point[1] + ss[1]) * ss[3];
      trapezoids->SetPoint(++j, point[0], 0.);
      trapezoids->SetPoint(++j, point);
    }
    painter->DrawQuadStrip(trapezoids);
    trapezoids->Delete();
  }

  if (this->PolyLinePen->GetLineType() != vtkPen::NO_PEN
    && size >= 2)
  {
    const vtkRectd& ss = this->ShiftScale;

    vtkNew<vtkPoints2D> transformedShape;
    transformedShape->SetNumberOfPoints(size);
    for (vtkIdType i = 0; i < size; ++i)
    {
      double point[2];
      this->Shape->GetPoint(i, point);
      // shift/scale to scale from data space to rendering space.
      point[0] = (point[0] + ss[0]) * ss[2];
      point[1] = (point[1] + ss[1]) * ss[3];
      transformedShape->SetPoint(i, point);
    }
    painter->ApplyPen(this->PolyLinePen);
    painter->DrawPoly(transformedShape);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::OnScalarsToColorsModified(vtkObject* caller,
                                                       unsigned long eid,
                                                       void *clientdata,
                                                       void* calldata)
{
  vtkScalarsToColorsItem* self =
    reinterpret_cast<vtkScalarsToColorsItem*>(clientdata);
  self->ScalarsToColorsModified(caller, eid, calldata);
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::ScalarsToColorsModified(vtkObject* vtkNotUsed(object),
                                                     unsigned long vtkNotUsed(eid),
                                                     void* vtkNotUsed(calldata))
{
  this->Modified();
}
