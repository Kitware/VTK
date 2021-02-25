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

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

namespace
{
float vtkComputePosition(int alignment, float pos, float size, int vp_size, int margin)
{
  switch (alignment)
  {
    case vtkBlockItem::LEFT:
    case vtkBlockItem::BOTTOM:
      return margin;

    case vtkBlockItem::RIGHT:
    case vtkBlockItem::TOP:
      return (vp_size - size - margin);

    case vtkBlockItem::CENTER:
      return (0.5 * (vp_size - size - margin));

    case vtkBlockItem::CUSTOM:
    default:
      return pos;
  }
}
}
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkBlockItem);
vtkCxxSetObjectMacro(vtkBlockItem, LabelProperties, vtkTextProperty);
//------------------------------------------------------------------------------
vtkBlockItem::vtkBlockItem()
  : Dimensions{ 0, 0, 0, 0 }
  , MouseOver(false)
  , scalarFunction(nullptr)
  , LabelProperties(vtkTextProperty::New())
  , HorizontalAlignment(vtkBlockItem::CUSTOM)
  , VerticalAlignment(vtkBlockItem::CUSTOM)
  , AutoComputeDimensions(false)
  , Padding{ 5, 5 }
  , Margins{ 10, 10 }
{
  this->LabelProperties->SetVerticalJustificationToCentered();
  this->LabelProperties->SetJustificationToCentered();
  this->LabelProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelProperties->SetFontSize(24);

  this->Brush->SetColor(255, 0, 0);
  this->MouseOverBrush->SetColor(0, 255, 0);
  this->Pen->SetColor(0, 0, 0);
}

//------------------------------------------------------------------------------
vtkBlockItem::~vtkBlockItem()
{
  this->SetLabelProperties(nullptr);
}

//------------------------------------------------------------------------------
bool vtkBlockItem::Paint(vtkContext2D* painter)
{
  this->CachedTextProp->ShallowCopy(painter->GetTextProp());
  this->CachedPen->DeepCopy(painter->GetPen());
  this->CachedBrush->DeepCopy(painter->GetBrush());

  painter->ApplyTextProp(this->LabelProperties);

  vtkVector4<float> dims(this->Dimensions);
  const vtkVector2i tileScale = this->Scene->GetLogicalTileScale();

  // if requested, resize the dims to fit the label.
  if (this->Label && this->AutoComputeDimensions)
  {
    float bounds[4];
    painter->ComputeStringBounds(this->Label, bounds);
    vtkLogF(TRACE, "label bds: x=%f, y=%f, w=%f, h=%f", bounds[0], bounds[1], bounds[2], bounds[3]);

    dims[2] = bounds[2] + 2 * this->Padding[0] * tileScale[0];
    dims[3] = bounds[3] + 2 * this->Padding[1] * tileScale[1];
  }

  // if requested, update the position for the box.
  if (this->AutoComputeDimensions)
  {
    const vtkVector2i geometry(this->GetScene()->GetViewWidth(), this->GetScene()->GetViewHeight());
    vtkLogF(TRACE, "size %d, %d", geometry[0], geometry[1]);

    dims[0] = vtkComputePosition(
      this->HorizontalAlignment, dims[0], dims[2], geometry[0], this->Margins[0]);
    dims[1] =
      vtkComputePosition(this->VerticalAlignment, dims[1], dims[3], geometry[1], this->Margins[1]);
  }

  this->Dimensions[0] = dims[0];
  this->Dimensions[1] = dims[1];
  this->Dimensions[2] = dims[2];
  this->Dimensions[3] = dims[3];

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->MouseOver ? this->MouseOverBrush : this->Brush);
  painter->DrawRect(
    this->Dimensions[0], this->Dimensions[1], this->Dimensions[2], this->Dimensions[3]);

  if (this->Label)
  {
    if (this->AutoComputeDimensions)
    {
      // put the label in the box (minus the Padding).
      float rect[4];
      rect[0] = this->Dimensions[0] + this->Padding[0] * tileScale[0];
      rect[1] = this->Dimensions[1] + this->Padding[1] * tileScale[1];
      rect[2] = this->Dimensions[2] - 2 * this->Padding[0] * tileScale[0];
      rect[3] = this->Dimensions[3] - 2 * this->Padding[1] * tileScale[1];
      painter->DrawStringRect(rect, this->Label);
    }
    else
    {
      // anchor label at center of the box (this what was done traditionally)
      float x = this->Dimensions[0] + 0.5 * this->Dimensions[2];
      float y = this->Dimensions[1] + 0.5 * this->Dimensions[3];
      painter->DrawString(x, y, this->Label);
    }
  }

  if (this->scalarFunction)
  {
    // We have a function pointer - do something...
    ;
  }
  this->PaintChildren(painter);

  painter->ApplyTextProp(this->CachedTextProp);
  painter->ApplyPen(this->CachedPen);
  painter->ApplyBrush(this->CachedBrush);
  return true;
}

//------------------------------------------------------------------------------
bool vtkBlockItem::Hit(const vtkContextMouseEvent& mouse)
{
  if (!this->GetVisible() || !this->GetInteractive())
  {
    return false;
  }

  vtkVector2f pos = mouse.GetPos();
  if (pos[0] > this->Dimensions[0] && pos[0] < this->Dimensions[0] + this->Dimensions[2] &&
    pos[1] > this->Dimensions[1] && pos[1] < this->Dimensions[1] + this->Dimensions[3])
  {
    return true;
  }
  else
  {
    return this->vtkAbstractContextItem::Hit(mouse);
  }
}

//------------------------------------------------------------------------------
bool vtkBlockItem::MouseEnterEvent(const vtkContextMouseEvent&)
{
  this->MouseOver = true;
  this->GetScene()->SetDirty(true);
  return true;
}

//------------------------------------------------------------------------------
bool vtkBlockItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  vtkVector2f delta = mouse.GetPos() - mouse.GetLastPos();

  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    // Move the block by this amount
    this->Dimensions[0] += delta.GetX();
    this->Dimensions[1] += delta.GetY();

    this->GetScene()->SetDirty(true);
    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
  }
  else if (mouse.GetButton() == mouse.MIDDLE_BUTTON)
  {
    // Resize the block by this amount
    this->Dimensions[0] += delta.GetX();
    this->Dimensions[1] += delta.GetY();
    this->Dimensions[2] -= delta.GetX();
    this->Dimensions[3] -= delta.GetY();

    this->GetScene()->SetDirty(true);
    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
  }
  else if (mouse.GetButton() == mouse.RIGHT_BUTTON)
  {
    // Resize the block by this amount
    this->Dimensions[2] += delta.GetX();
    this->Dimensions[3] += delta.GetY();

    this->GetScene()->SetDirty(true);
    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkBlockItem::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  this->MouseOver = false;
  this->GetScene()->SetDirty(true);
  return true;
}

//------------------------------------------------------------------------------
bool vtkBlockItem::MouseButtonPressEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkBlockItem::MouseButtonReleaseEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
void vtkBlockItem::SetLabel(const vtkStdString& label)
{
  if (this->Label != label)
  {
    this->Label = label;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkStdString vtkBlockItem::GetLabel()
{
  return this->Label;
}

//------------------------------------------------------------------------------
void vtkBlockItem::SetScalarFunctor(double (*ScalarFunction)(double, double))
{
  this->scalarFunction = ScalarFunction;
}

//------------------------------------------------------------------------------
void vtkBlockItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
