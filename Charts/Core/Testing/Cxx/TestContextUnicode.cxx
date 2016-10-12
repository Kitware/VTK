/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

#include "vtkUnicodeString.h"

#include "vtkRegressionTestImage.h"

#include <string>

//----------------------------------------------------------------------------
class ContextUnicode : public vtkContextItem
{
public:
  static ContextUnicode *New();
  vtkTypeMacro(ContextUnicode, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;
  std::string FontFile;
};

//----------------------------------------------------------------------------
int TestContextUnicode(int argc, char * argv [])
{
  if (argc < 2)
  {
    cout << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  std::string fontFile(argv[1]);

  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(200, 100);
  vtkSmartPointer<ContextUnicode> test = vtkSmartPointer<ContextUnicode>::New();
  test->FontFile = fontFile;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return !retVal;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(ContextUnicode);
// This function aims to test the primitives provided by the 2D API.
bool ContextUnicode::Paint(vtkContext2D *painter)
{
  // Test the string drawing functionality of the context
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetTextProp()->SetFontFamily(VTK_FONT_FILE);
  painter->GetTextProp()->SetFontFile(this->FontFile.c_str());
  painter->DrawString(70, 20, "Angstrom");
  painter->DrawString(150, 20, vtkUnicodeString::from_utf8("\xe2\x84\xab"));
  painter->DrawString(100, 80,
                      vtkUnicodeString::from_utf8("a\xce\xb1"));
  painter->DrawString(100, 50,
                      vtkUnicodeString::from_utf8("\xce\xb1\xce\xb2\xce\xb3"));
  return true;
}
