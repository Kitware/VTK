/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFreeTypeLabelRenderStrategy.h"

#include "vtkActor2D.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkFreeTypeLabelRenderStrategy, "1.1");
vtkStandardNewMacro(vtkFreeTypeLabelRenderStrategy);

//----------------------------------------------------------------------------
vtkFreeTypeLabelRenderStrategy::vtkFreeTypeLabelRenderStrategy()
{
  this->FreeTypeUtilities = vtkFreeTypeUtilities::New();
  this->Mapper = vtkTextMapper::New();
  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
vtkFreeTypeLabelRenderStrategy::~vtkFreeTypeLabelRenderStrategy()
{
  this->FreeTypeUtilities->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::ComputeLabelBounds(
  vtkTextProperty* tprop, vtkUnicodeString label, double bds[4])
{
  // Check for empty string.
  vtkStdString str;
  label.utf8_str(str);
  if (str.length() == 0)
    {
    bds[0] = 0;
    bds[1] = 0;
    bds[2] = 0;
    bds[3] = 0;
    return;
    }

  if (!tprop)
    {
    tprop = this->DefaultTextProperty;
    }
  vtkSmartPointer<vtkTextProperty> copy = tprop;
  if (tprop->GetOrientation() != 0.0)
    {
    copy = vtkSmartPointer<vtkTextProperty>::New();
    copy->ShallowCopy(tprop);
    copy->SetOrientation(0.0);
    }
  int bbox[4];
  this->FreeTypeUtilities->GetBoundingBox(copy, label.utf8_str(), bbox);

  // Take line offset into account
  bds[0] = bbox[0];
  bds[1] = bbox[1];
  bds[2] = bbox[2] - tprop->GetLineOffset();
  bds[3] = bbox[3] - tprop->GetLineOffset();

  // Take justification into account
  double sz[2] = {bds[1] - bds[0], bds[3] - bds[2]};
  switch (tprop->GetJustification())
  {
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      bds[0] -= sz[0]/2;
      bds[1] -= sz[0]/2;
      break;
    case VTK_TEXT_RIGHT:
      bds[0] -= sz[0];
      bds[1] -= sz[0];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    case VTK_TEXT_BOTTOM:
      break;
    case VTK_TEXT_CENTERED:
      bds[2] -= sz[1]/2;
      bds[3] -= sz[1]/2;
      break;
    case VTK_TEXT_TOP:
      bds[2] -= sz[1];
      bds[3] -= sz[1];
      break;
  }
}

//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::RenderLabel(
  double x[3], vtkTextProperty* tprop, vtkUnicodeString label)
{
  if (!this->Renderer)
    {
    vtkErrorMacro("Renderer must be set before rendering labels.");
    return;
    }
  if (!tprop)
    {
    tprop = this->DefaultTextProperty;
    }
  this->Mapper->SetTextProperty(tprop);
  this->Mapper->SetInput(label.utf8_str());
  this->Actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Actor->GetPositionCoordinate()->SetValue(x);
  this->Mapper->RenderOverlay(this->Renderer, this->Actor);
}

//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
