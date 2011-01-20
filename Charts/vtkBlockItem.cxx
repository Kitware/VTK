/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBlockItem.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkStdString.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBlockItem);

//-----------------------------------------------------------------------------
vtkBlockItem::vtkBlockItem()
{
  this->MouseOver = false;
  this->MouseButtonPressed = vtkContextMouseEvent::NO_BUTTON;
  this->scalarFunction = NULL;
  this->Dimensions[0]=0;
  this->Dimensions[1]=0;
  this->Dimensions[2]=0;
  this->Dimensions[3]=0;
}

//-----------------------------------------------------------------------------
vtkBlockItem::~vtkBlockItem()
{
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::Paint(vtkContext2D *painter)
{
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetPen()->SetColor(0, 0, 0);

  if (this->MouseOver)
    {
    painter->GetBrush()->SetColor(255, 0, 0);
    }
  else
    {
    painter->GetBrush()->SetColor(0, 255, 0);
    }
  painter->DrawRect(static_cast<float>(this->Dimensions[0]),
                    static_cast<float>(this->Dimensions[1]),
                    static_cast<float>(this->Dimensions[2]),
                    static_cast<float>(this->Dimensions[3]));

  int x = vtkContext2D::FloatToInt(this->Dimensions[0] + 0.5 * this->Dimensions[2]);
  int y = vtkContext2D::FloatToInt(this->Dimensions[1] + 0.5 * this->Dimensions[3]);
  if (this->Label)
    {
    painter->DrawString(static_cast<float>(x),static_cast<float>(y),
                        this->Label);
    }

  if (this->scalarFunction)
    {
    // We have a function pointer - do something...
    ;
    }

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::Hit(const vtkContextMouseEvent &mouse)
{
    if (mouse.Pos[0] > this->Dimensions[0] &&
        mouse.Pos[0] < this->Dimensions[0]+this->Dimensions[2] &&
        mouse.Pos[1] > this->Dimensions[1] &&
        mouse.Pos[1] < this->Dimensions[1]+this->Dimensions[3])
    {
    return true;
    }
  else
    {
    return this->vtkAbstractContextItem::Hit(mouse);
    }
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseEnterEvent(const vtkContextMouseEvent &)
{
  this->MouseOver = true;
  this->GetScene()->SetDirty(true);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  int deltaX = static_cast<int>(mouse.Pos[0] - mouse.LastPos[0]);
  int deltaY = static_cast<int>(mouse.Pos[1] - mouse.LastPos[1]);

  if (mouse.Button == mouse.LEFT_BUTTON)
    {
    // Move the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;

    this->GetScene()->SetDirty(true);
    return true;
    }
  else if (mouse.Button == mouse.MIDDLE_BUTTON)
    {
    // Resize the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;
    this->Dimensions[2] -= deltaX;
    this->Dimensions[3] -= deltaY;

    this->GetScene()->SetDirty(true);
    return true;
    }
  else if (mouse.Button == mouse.RIGHT_BUTTON)
    {
    // Resize the block by this amount
    this->Dimensions[2] += deltaX;
    this->Dimensions[3] += deltaY;

    this->GetScene()->SetDirty(true);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  this->MouseOver = false;
  this->GetScene()->SetDirty(true);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseButtonPressed = mouse.Button;
  this->LastPosition[0] = mouse.Pos[0];
  this->LastPosition[1] = mouse.Pos[1];
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  this->MouseButtonPressed = vtkContextMouseEvent::NO_BUTTON;
  return true;
}

//-----------------------------------------------------------------------------
void vtkBlockItem::SetLabel(const vtkStdString &label)
{
  if (this->Label != label)
    {
    this->Label = label;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkBlockItem::GetLabel()
{
  return this->Label;
}

//-----------------------------------------------------------------------------
void vtkBlockItem::SetScalarFunctor(double (*ScalarFunction)(double, double))
{
  this->scalarFunction = ScalarFunction;
}

//-----------------------------------------------------------------------------
void vtkBlockItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
