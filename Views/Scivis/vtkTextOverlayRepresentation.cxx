// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTextOverlayRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTextOverlayRepresentation);

//------------------------------------------------------------------------------
vtkTextOverlayRepresentation::vtkTextOverlayRepresentation()
{
  // There is no data behind a text overlay, so there is nothing to connect.
  this->SetNumberOfInputPorts(0);
  this->TextActor->GetTextProperty()->SetFontSize(18);
}

//------------------------------------------------------------------------------
vtkTextOverlayRepresentation::~vtkTextOverlayRepresentation() = default;

//------------------------------------------------------------------------------
bool vtkTextOverlayRepresentation::AddToView(vtkScivisView* view)
{
  view->GetRenderer()->AddViewProp(this->TextActor);
  return true;
}

//------------------------------------------------------------------------------
bool vtkTextOverlayRepresentation::RemoveFromView(vtkScivisView* view)
{
  view->GetRenderer()->RemoveViewProp(this->TextActor);
  return true;
}

//------------------------------------------------------------------------------
void vtkTextOverlayRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->TextActor->SetVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkTextOverlayRepresentation::GetVisibility()
{
  return this->TextActor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkTextOverlayRepresentation::SetText(const char* text)
{
  const char* current = this->GetText();
  if (current == text || (current && text && strcmp(current, text) == 0))
  {
    return;
  }
  this->TextActor->SetInput(text);
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkTextOverlayRepresentation::GetText()
{
  return this->TextActor->GetInput();
}

//------------------------------------------------------------------------------
void vtkTextOverlayRepresentation::SetPosition(int x, int y)
{
  const double* current = this->TextActor->GetPosition();
  if (current[0] == x && current[1] == y)
  {
    return;
  }
  this->TextActor->SetDisplayPosition(x, y);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkTextOverlayRepresentation::GetPosition()
{
  return this->TextActor->GetPosition();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkTextOverlayRepresentation::GetTextProperty()
{
  return this->TextActor->GetTextProperty();
}

//------------------------------------------------------------------------------
vtkTextActor* vtkTextOverlayRepresentation::GetTextActor()
{
  return this->TextActor;
}

//------------------------------------------------------------------------------
void vtkTextOverlayRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  const char* text = this->GetText();
  os << indent << "Text: " << (text ? text : "(none)") << "\n";
  os << indent << "Visibility: " << this->GetVisibility() << "\n";
  os << indent << "TextActor:\n";
  this->TextActor->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
