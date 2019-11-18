/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextWidget.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkTextRepresentation.h"

vtkStandardNewMacro(vtkTextWidget);

//-------------------------------------------------------------------------
vtkTextWidget::vtkTextWidget() = default;

//-------------------------------------------------------------------------
vtkTextWidget::~vtkTextWidget() = default;

//-------------------------------------------------------------------------
void vtkTextWidget::SetTextActor(vtkTextActor* textActor)
{
  vtkTextRepresentation* textRep = reinterpret_cast<vtkTextRepresentation*>(this->WidgetRep);
  if (!textRep)
  {
    this->CreateDefaultRepresentation();
    textRep = reinterpret_cast<vtkTextRepresentation*>(this->WidgetRep);
  }

  if (textRep->GetTextActor() != textActor)
  {
    textRep->SetTextActor(textActor);
    this->Modified();
  }
}

//-------------------------------------------------------------------------
vtkTextActor* vtkTextWidget::GetTextActor()
{
  vtkTextRepresentation* textRep = reinterpret_cast<vtkTextRepresentation*>(this->WidgetRep);
  if (!textRep)
  {
    return nullptr;
  }
  else
  {
    return textRep->GetTextActor();
  }
}

//----------------------------------------------------------------------
void vtkTextWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkTextRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void vtkTextWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
