/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFreeTypeTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFreeTypeTools.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

#include <set>

namespace {

//----------------------------------------------------------------------------
int CheckIfIDExists(vtkTextProperty* property, std::set<size_t> & ids)
{
  size_t id;
  vtkFreeTypeTools::GetInstance()->MapTextPropertyToId(property, &id);
  if (ids.size() > 0 && ids.find(id) != ids.end())
  {
    std::cout << "ID " << id
              << " already exists for other vtkTextProperty settings\n";
    return EXIT_FAILURE;
  }
  ids.insert(id);

  return EXIT_SUCCESS;
}

}

//----------------------------------------------------------------------------
int TestFreeTypeTools(int, char *[])
{
  size_t result = EXIT_SUCCESS;
  std::set<size_t> ids;

  vtkSmartPointer<vtkTextProperty> property =
    vtkSmartPointer<vtkTextProperty>::New();

  // Initial settings
  property->BoldOff();
  property->ItalicOff();
  property->ShadowOff();
  property->SetFontSize(12);
  property->SetColor(1.0, 1.0, 1.0);
  property->SetOpacity(1.0);
  property->SetBackgroundColor(0.0, 0.0, 0.0);
  property->SetBackgroundOpacity(1.0);
  property->SetFontFamilyToArial();
  property->SetShadowOffset(2, 2);
  property->SetOrientation(0.0);
  property->SetLineSpacing(1.0);
  property->SetLineOffset(1.0);

  std::cout << "Bold\n";
  property->BoldOn();
  result += CheckIfIDExists(property, ids);
  property->BoldOff();

  std::cout << "Italic\n";
  property->ItalicOn();
  result += CheckIfIDExists(property, ids);
  property->ItalicOff();

  std::cout << "Shadow\n";
  property->ShadowOn();
  result += CheckIfIDExists(property, ids);
  property->ShadowOff();

  std::cout << "Font size\n";
  property->SetFontSize(14);
  result += CheckIfIDExists(property, ids);
  property->SetFontSize(12);

  std::cout << "Color\n";
  property->SetColor(0.0, 1.0, 1.0);
  result += CheckIfIDExists(property, ids);
  property->SetColor(1.0, 1.0, 1.0);

  std::cout << "Opacity\n";
  property->SetOpacity(0.9);
  result += CheckIfIDExists(property, ids);
  property->SetOpacity(1.0);

  std::cout << "BackgroundColor\n";
  property->SetBackgroundColor(1.0, 0.0, 0.0);
  result += CheckIfIDExists(property, ids);
  property->SetBackgroundColor(0.0, 0.0, 0.0);

  std::cout << "BackgroundOpacity\n";
  property->SetBackgroundOpacity(0.8);
  result += CheckIfIDExists(property, ids);
  property->SetBackgroundOpacity(1.0);

  std::cout << "FontFamily\n";
  property->SetFontFamilyToCourier();
  result += CheckIfIDExists(property, ids);
  property->SetFontFamilyToArial();

  std::cout << "ShadowOffset\n";
  property->SetShadowOffset(-2, -3);
  result += CheckIfIDExists(property, ids);
  property->SetShadowOffset(2, 2);

  std::cout << "Orientation\n";
  property->SetOrientation(90.0);
  result += CheckIfIDExists(property, ids);
  property->SetOrientation(0.0);

  std::cout << "LineSpacing\n";
  property->SetLineSpacing(2.0);
  result += CheckIfIDExists(property, ids);
  property->SetLineSpacing(1.0);

  std::cout << "LineOffset\n";
  property->SetLineOffset(2.0);
  result += CheckIfIDExists(property, ids);
  property->SetLineOffset(1.0);

  return result;
}
