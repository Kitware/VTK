/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTooltipItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTooltipItem.h"

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"

#include "vtkNew.h"
#include "vtkStdString.h"
#include <sstream>

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTooltipItem);

//-----------------------------------------------------------------------------
vtkTooltipItem::vtkTooltipItem() : PositionVector(0, 0)
{
  this->Position = this->PositionVector.GetData();
  this->TextProperties = vtkTextProperty::New();
  this->TextProperties->SetVerticalJustificationToBottom();
  this->TextProperties->SetJustificationToLeft();
  this->TextProperties->SetColor(0.0, 0.0, 0.0);
  this->Pen = vtkPen::New();
  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->Brush = vtkBrush::New();
  this->Brush->SetColor(242, 242, 242);
}

//-----------------------------------------------------------------------------
vtkTooltipItem::~vtkTooltipItem()
{
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProperties->Delete();
}

//-----------------------------------------------------------------------------
void vtkTooltipItem::SetPosition(const vtkVector2f &pos)
{
  this->PositionVector = pos;
}

//-----------------------------------------------------------------------------
vtkVector2f vtkTooltipItem::GetPositionVector()
{
  return this->PositionVector;
}

//-----------------------------------------------------------------------------
void vtkTooltipItem::SetText(const vtkStdString &text)
{
  if (this->Text != text)
  {
    this->Text = text;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkTooltipItem::GetText()
{
  return this->Text;
}

//-----------------------------------------------------------------------------
void vtkTooltipItem::Update()
{

}

//-----------------------------------------------------------------------------
bool vtkTooltipItem::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkTooltipItem.");

  if (!this->Visible || !this->Text)
  {
    return false;
  }

  // save painter settings
  vtkNew<vtkPen> previousPen;
  previousPen->DeepCopy(painter->GetPen());
  vtkNew<vtkBrush> previousBrush;
  previousBrush->DeepCopy(painter->GetBrush());
  vtkNew<vtkTextProperty> previousTextProp;
  previousTextProp->ShallowCopy(painter->GetTextProp());

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->ApplyTextProp(this->TextProperties);

  // Compute the bounds, then make a few adjustments to the size we will use
  vtkVector2f bounds[2];
  painter->ComputeStringBounds(this->Text, bounds[0].GetData());
  if (bounds[1].GetX() == 0.0f && bounds[1].GetY() == 0.0f)
  {
    // This signals only non-renderable characters, so return
    return false;
  }
  float scale[2];
  float position[2];
  painter->GetTransform()->GetScale(scale);
  painter->GetTransform()->GetPosition(position);
  bounds[0] = vtkVector2f(this->PositionVector.GetX()-5/scale[0],
                          this->PositionVector.GetY()-3/scale[1]);
  bounds[1].Set(bounds[1].GetX()+10/scale[0], bounds[1].GetY()+10/scale[1]);
  // Pull the tooltip back in if it will go off the edge of the screen.
  float maxX = (this->Scene->GetViewWidth() - position[0])/scale[0];
  if (bounds[0].GetX() >= maxX - bounds[1].GetX())
  {
    bounds[0].SetX(maxX - bounds[1].GetX());
  }
  // Draw a rectangle as background, and then center our text in there
  painter->DrawRect(bounds[0].GetX(), bounds[0].GetY(), bounds[1].GetX(), bounds[1].GetY());
  painter->DrawString(bounds[0].GetX()+5/scale[0], bounds[0].GetY()+3/scale[1], this->Text);

  // restore painter settings
  painter->ApplyPen(previousPen.GetPointer());
  painter->ApplyBrush(previousBrush.GetPointer());
  painter->ApplyTextProp(previousTextProp.GetPointer());

  return true;
}

//-----------------------------------------------------------------------------
void vtkTooltipItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
