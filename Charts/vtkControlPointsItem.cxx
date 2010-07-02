/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkControlPointsItem::vtkControlPointsItem()
{
  this->Points = vtkPoints2D::New();
  this->HighlightPoints = vtkPoints2D::New();
  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkControlPointsItem::CallComputePoints);
}

//-----------------------------------------------------------------------------
vtkControlPointsItem::~vtkControlPointsItem()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = 0;
    }
  if (this->HighlightPoints)
    {
    this->HighlightPoints->Delete();
    this->HighlightPoints = 0;
    }
  if (this->Callback)
    {
    this->Callback->Delete();
    this->Callback = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Paint(vtkContext2D* painter)
{
  if (this->Points->GetNumberOfPoints())
    {
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetColorF(1., 1., 1.);
    painter->GetBrush()->SetColorF(0.85, 0.85, 0.85, 0.85);
    this->DrawPoints(painter, this->Points);
    }
  if (this->HighlightPoints->GetNumberOfPoints())
    {
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetColorF(0.9, 0.9, 1.);
    painter->GetBrush()->SetColorF(0.65, 0.65, 0.95, 0.95);
    this->DrawPoints(painter, this->HighlightPoints);
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::CallComputePoints(
  vtkObject* vtkNotUsed(sender), unsigned long vtkNotUsed(event),
  void* receiver, void* vtkNotUsed(params))
{
  vtkControlPointsItem* item =
    reinterpret_cast<vtkControlPointsItem*>(receiver);
  item->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ComputePoints()
{
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawPoints(vtkContext2D* painter, vtkPoints2D* points)
{
  const int count = points->GetNumberOfPoints();
  double point[2];
  vtkSmartPointer<vtkTransform2D> translation =
    vtkSmartPointer<vtkTransform2D>::New();
  for (int i = 0; i < count; ++i)
    {
    points->GetPoint(i,point);
    painter->PushMatrix();
    translation->Identity();
    // TODO, use world coordinates, not local coordinates
    translation->Translate(point[0]*100.f, point[1]*100.f);
    painter->SetTransform(translation);
    painter->DrawWedge(0.f, 0.f, 5.f, 0.f, 0.f, 360.f);
    painter->DrawArc(0.f, 0.f, 5.f, 0.f, 360.f);
    painter->PopMatrix();
    }
}
