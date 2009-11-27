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
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkBlockItem, "1.1");
vtkStandardNewMacro(vtkBlockItem);

//-----------------------------------------------------------------------------
vtkBlockItem::vtkBlockItem()
{
  this->Label = NULL;
  this->MouseOver = false;
  this->MouseButtonPressed = -1;
  this->scalarFunction = NULL;
}

//-----------------------------------------------------------------------------
vtkBlockItem::~vtkBlockItem()
{
  this->SetLabel(NULL);
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

  int x = this->Dimensions[0] + 0.5 * this->Dimensions[2];
  int y = this->Dimensions[1] + 0.5 * this->Dimensions[3];
  painter->DrawText(x, y, this->Label);

  if (this->scalarFunction)
    {
    // We have a function pointer - do something...
    ;
    }
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
    return false;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseEnterEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseOver = true;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  int deltaX = mouse.Pos[0] - this->LastPosition[0];
  int deltaY = mouse.Pos[1] - this->LastPosition[1];
  this->LastPosition[0] = mouse.Pos[0];
  this->LastPosition[1] = mouse.Pos[1];

  if (this->MouseButtonPressed == 0)
    {
    // Move the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;

    return true;
    }
  else if (this->MouseButtonPressed == 1)
    {
    // Resize the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;
    this->Dimensions[2] -= deltaX;
    this->Dimensions[3] -= deltaY;

    return true;
    }
  else if (this->MouseButtonPressed == 2)
    {
    // Resize the block by this amount
    this->Dimensions[2] += deltaX;
    this->Dimensions[3] += deltaY;

    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkBlockItem::MouseLeaveEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseOver = false;
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
bool vtkBlockItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseButtonPressed = -1;
  return true;
}

void vtkBlockItem::SetScalarFunctor(double (*scalarFunction)(double, double))
{
  this->scalarFunction = scalarFunction;
}

//-----------------------------------------------------------------------------
void vtkBlockItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
