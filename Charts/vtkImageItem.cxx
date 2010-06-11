/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageItem.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkTransform2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkImageData.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageItem);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkImageItem, Image, vtkImageData);

//-----------------------------------------------------------------------------
vtkImageItem::vtkImageItem()
{
  this->Label = NULL;
  this->Image = NULL;
  this->MouseOver = false;
  this->MouseButtonPressed = -1;
  this->ScalarFunction = NULL;
}

//-----------------------------------------------------------------------------
vtkImageItem::~vtkImageItem()
{
  this->SetLabel(NULL);
}

//-----------------------------------------------------------------------------
bool vtkImageItem::Paint(vtkContext2D *painter)
{
  // Drawing a hard wired diagram 800x600 as a demonstration of the 2D API
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

  if (this->Image)
    {
    // Draw our image in the bottom left corner of the item
    painter->DrawImage(this->Dimensions[0]+10, this->Dimensions[1]+10, this->Image);
    }

  if (this->MouseOver && this->Label)
    {
    painter->GetBrush()->SetColor(255, 200, 0);
    painter->DrawRect(this->Dimensions[0]+10, this->Dimensions[1]+50,
                      100, 20);
    painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
    painter->GetTextProp()->SetFontSize(12);
    painter->DrawString(this->Dimensions[0]+60, this->Dimensions[1]+60,
                        this->Label);
    }

  if (this->ScalarFunction)
    {
    // We have a function pointer - do something...
    ;
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkImageItem::Hit(const vtkContextMouseEvent &mouse)
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
}

//-----------------------------------------------------------------------------
bool vtkImageItem::MouseEnterEvent(const vtkContextMouseEvent &)
{
  this->MouseOver = true;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkImageItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  // Work out our deltas...
  int deltaX = static_cast<int>(mouse.ScenePos[0] - mouse.LastScenePos[0]);
  int deltaY = static_cast<int>(mouse.ScenePos[1] - mouse.LastScenePos[1]);

  if (mouse.Button == 0) // Left mouse button - translate
    {
    // Move the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;
    return true;
    }
  else if (mouse.Button == 1)
    {
    // Resize the block by this amount
    this->Dimensions[0] += deltaX;
    this->Dimensions[1] += deltaY;
    this->Dimensions[2] -= deltaX;
    this->Dimensions[3] -= deltaY;

    return true;
    }
  else if (mouse.Button == 2)
    {
    // Resize the block by this amount
    this->Dimensions[2] += deltaX;
    this->Dimensions[3] += deltaY;

    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkImageItem::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  this->MouseOver = false;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkImageItem::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkImageItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  return true;
}

void vtkImageItem::SetScalarFunctor(double (*scalarFunction)(double, double))
{
  this->ScalarFunction = scalarFunction;
}

//-----------------------------------------------------------------------------
void vtkImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
