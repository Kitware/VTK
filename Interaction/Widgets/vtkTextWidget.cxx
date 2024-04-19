// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTextWidget.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkTextRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTextWidget);

//------------------------------------------------------------------------------
vtkTextWidget::vtkTextWidget() = default;

//------------------------------------------------------------------------------
vtkTextWidget::~vtkTextWidget() = default;

//------------------------------------------------------------------------------
void vtkTextWidget::SetRepresentation(vtkTextRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkTextWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkTextRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkTextWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
