/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextProperty.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkTextProperty, "1.4");
vtkStandardNewMacro(vtkTextProperty);

//----------------------------------------------------------------------------
// Control wheter to globally force text antialiasing (ALL), 
// disable antialiasing (NONE), allow antialising (SOME) depending on
// the per-object AntiAliasing attribute.
 
static int vtkTextPropertyGlobalAntiAliasing = VTK_TEXT_GLOBAL_ANTIALIASING_SOME;

void vtkTextProperty::SetGlobalAntiAliasing(int val)
{
  if (val == vtkTextPropertyGlobalAntiAliasing)
    {
    return;
    }
  if (val < VTK_TEXT_GLOBAL_ANTIALIASING_SOME)
    {
    val = VTK_TEXT_GLOBAL_ANTIALIASING_SOME;
    }
  else if (val > VTK_TEXT_GLOBAL_ANTIALIASING_ALL)
    {
    val = VTK_TEXT_GLOBAL_ANTIALIASING_ALL;
    }

  vtkTextPropertyGlobalAntiAliasing = val;
}

int vtkTextProperty::GetGlobalAntiAliasing()
{
  return vtkTextPropertyGlobalAntiAliasing;
}

//----------------------------------------------------------------------------
// Creates a new text property with Font size 12, bold off, italic off,
// and Arial font

vtkTextProperty::vtkTextProperty()
{
  // TOFIX: the default text prop color is set to a special (-1, -1, -1) value
  // to maintain backward compatibility for a while. Text mapper classes will
  // use the Actor2D color instead of the text prop color if this value is 
  // found (i.e. if the text prop color has not been set).

  this->Color[0] = -1.0;
  this->Color[1] = -1.0;
  this->Color[2] = -1.0;

  // TOFIX: same goes for opacity

  this->Opacity  = -1.0;

  this->FontFamily = VTK_ARIAL;
  this->FontSize = 12;

  this->Bold = 0;
  this->Italic = 0;
  this->Shadow = 0;
  this->AntiAliasing = 1;

  this->Justification = VTK_TEXT_LEFT;
  this->VerticalJustification = VTK_TEXT_BOTTOM;

  this->LineOffset = 0.0;
  this->LineSpacing = 1.0;
}

//----------------------------------------------------------------------------
// Shallow copy of a text property.

void vtkTextProperty::ShallowCopy(vtkTextProperty *tprop)
{
  this->SetColor(tprop->GetColor());
  this->SetOpacity(tprop->GetOpacity());

  this->SetFontFamily(tprop->GetFontFamily());
  this->SetFontSize(tprop->GetFontSize());

  this->SetBold(tprop->GetBold());
  this->SetItalic(tprop->GetItalic());
  this->SetShadow(tprop->GetShadow());
  this->SetAntiAliasing(tprop->GetAntiAliasing());

  this->SetJustification(tprop->GetJustification());
  this->SetVerticalJustification(tprop->GetVerticalJustification());

  this->SetLineOffset(tprop->GetLineOffset());
  this->SetLineSpacing(tprop->GetLineSpacing());
}

//----------------------------------------------------------------------------
vtkTextProperty::~vtkTextProperty()
{
}

//----------------------------------------------------------------------------
void vtkTextProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Color: (" << this->Color[0] << ", " 
     << this->Color[1] << ", " << this->Color[2] << ")\n";

  os << indent << "Opacity: " << this->Opacity << "\n";

  os << indent << "FontFamily: " << this->GetFontFamilyAsString() << "\n";
  os << indent << "FontSize: " << this->FontSize << "\n";

  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");

  os << indent << "Justification: " 
     << this->GetJustificationAsString() << "\n";

  os << indent << "Vertical justification: " 
     << this->GetVerticalJustificationAsString() << "\n";

  os << indent << "Line Offset: " << this->LineOffset << "\n";
  os << indent << "Line Spacing: " << this->LineSpacing << "\n";
  os << indent << "AntiAliasing: " << this->AntiAliasing << "\n";
}
