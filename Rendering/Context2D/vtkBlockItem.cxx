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
#include "vtkVectorOperators.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBlockItem)

//-----------------------------------------------------------------------------
vtkBlockItem::vtkBlockItem()
{
  this->MouseOver = false;
  this->scalarFunction = NULL;
  this->Dimensions[0] = 0;
  this->Dimensions[1] = 0;
  this->Dimensions[2] = 0;
  this->Dimensions[3] = 0;
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
  painter->DrawRect(this->Dimensions[0], this->Dimensions[1],
                    this->Dimensions[2], this->Dimensions[3]);

  float x = this->Dimensions[0] + 0.5 * this->Dimensions[2];
  float y = this->Dimensions[1] + 0.5 * this->Dimensions[3];
  if (this->Label)
    {
    painter->DrawString(x, y, this->Label);
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
  vtkVector2f pos = mouse.GetPos();
  if (pos[0] > this->Dimensions[0] &&
      pos[0] < this->Dimensions[0] + this->Dimensions[2] &&
      pos[1] > this->Dimensions[1] &&
      pos[1] < this->Dimensions[1] + this->Dimensions[3])
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
  vtkVector2f delta = mouse.GetPos() - mouse.GetLastPos();

  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Move the block by this amount
    this->Dimensions[0] += delta.X();
    this->Dimensions[1] += delta.Y();

    this->GetScene()->SetDirty(true);
    return true;
    }
  else if (mouse.GetButton() == mouse.MIDDLE_BUTTON)
    {
    // Resize the block by this amount
    this->Dimensions[0] += delta.X();
    this->Dimensions[1] += delta.Y();
    this->Dimensions[2] -= delta.X();
    this->Dimensions[3] -= delta.Y();

    this->GetScene()->SetDirty(true);
    return true;
    }
  else if (mouse.GetButton() == mouse.RIGHT_BUTTON)
    {
    // Resize the block by this amount
    this->Dimensions[2] += delta.X();
    this->Dimensions[3] += delta.Y();

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
bool vtkBlockItem::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
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
