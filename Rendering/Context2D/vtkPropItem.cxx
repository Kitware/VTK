/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPropItem.h"

#include "vtkProp.h"
#include "vtkProp3D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

vtkObjectFactoryNewMacro(vtkPropItem)
vtkCxxSetObjectMacro(vtkPropItem, PropObject, vtkProp)

//------------------------------------------------------------------------------
vtkPropItem::vtkPropItem()
  : PropObject(NULL)
{
}

//------------------------------------------------------------------------------
vtkPropItem::~vtkPropItem()
{
  this->SetPropObject(NULL);
}

//------------------------------------------------------------------------------
void vtkPropItem::UpdateTransforms()
{
  vtkErrorMacro(<<"Missing override in the rendering backend. Some items "
                "may be rendered incorrectly.");
}

//------------------------------------------------------------------------------
void vtkPropItem::ResetTransforms()
{
  vtkErrorMacro(<<"Missing override in the rendering backend. Some items "
                  "may be rendered incorrectly.");
}

//------------------------------------------------------------------------------
bool vtkPropItem::Paint(vtkContext2D *)
{
  if (!this->PropObject)
  {
    return false;
  }

  this->UpdateTransforms();

  int result = this->PropObject->RenderOpaqueGeometry(this->Scene->GetRenderer());
  if (this->PropObject->HasTranslucentPolygonalGeometry())
  {
    result += this->PropObject->RenderTranslucentPolygonalGeometry(
          this->Scene->GetRenderer());
  }
  result += this->PropObject->RenderOverlay(this->Scene->GetRenderer());

  this->ResetTransforms();

  return result != 0;
}

//------------------------------------------------------------------------------
void vtkPropItem::ReleaseGraphicsResources()
{
  if (this->PropObject && this->Scene && this->Scene->GetRenderer())
  {
    this->PropObject->ReleaseGraphicsResources(
          this->Scene->GetRenderer()->GetVTKWindow());
  }
}

//------------------------------------------------------------------------------
void vtkPropItem::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop:";
  if (this->PropObject)
  {
    os << "\n";
    this->PropObject->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(NULL)\n";
  }
}
