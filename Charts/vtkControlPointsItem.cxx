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
#include "vtkContextScene.h"
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
  this->Pen->SetLineType(vtkPen::SOLID_LINE);
  this->Pen->SetWidth(1.);
  this->Pen->SetColorF(1., 1., 1.);
  this->Brush->SetColorF(0.85, 0.85, 1., 0.75);

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
void vtkControlPointsItem::GetBounds(double bounds[4])
{
  this->Points->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Paint(vtkContext2D* painter)
{
  if (this->Points->GetNumberOfPoints())
    {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    this->DrawPoints(painter, this->Points);
    }
  if (this->HighlightPoints->GetNumberOfPoints())
    {
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetColorF(0.87, 0.87, 1.);
    painter->GetBrush()->SetColorF(0.65, 0.65, 0.95, 0.55);
    this->DrawPoints(painter, this->HighlightPoints);
    }
  return true;
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
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawPoints(vtkContext2D* painter, vtkPoints2D* points)
{
  vtkTransform2D* sceneTransform = painter->GetTransform();
  vtkSmartPointer<vtkTransform2D> translation =
    vtkSmartPointer<vtkTransform2D>::New();

  double point[2];
  double transformedPoint[2];

  const int count = points->GetNumberOfPoints();
  for (int i = 0; i < count; ++i)
    {
    points->GetPoint(i, point);
    sceneTransform->TransformPoints(point, transformedPoint, 1);

    painter->PushMatrix();
    translation->Identity();
    translation->Translate(transformedPoint[0], transformedPoint[1]);
    painter->SetTransform(translation);
    painter->DrawWedge(0.f, 0.f, 6.f, 0.f, 0.f, 360.f);
    painter->DrawArc(0.f, 0.f, 6.f, 0.f, 360.f);
    painter->PopMatrix();
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::HighlightCurrentPoint(double* currentPoint)
{
  // initialize local variables...
  double* point = NULL;
  int numberOfPoints = this->HighlightPoints->GetNumberOfPoints();
  int i = 0;
  // Is the current point highlighted?
  // ->invert state
  while(i<numberOfPoints)
    {
    point = this->HighlightPoints->GetPoint(i);
    if(point[0] == currentPoint[0])
      {
      this->HighlightPoints->GetData()->RemoveTuple(i);
      return;
      }
    ++i;
    }
  // If we are here: point not found : add it
  this->HighlightPoints->InsertNextPoint(currentPoint);
}
