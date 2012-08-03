/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimpleFontRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkFreeTypeTools.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
class SystemFontRenderTest : public vtkContextItem
{
public:
  static SystemFontRenderTest *New();
  vtkTypeMacro(SystemFontRenderTest, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestSystemFontRendering( int, char * [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(520, 360);
  vtkNew<SystemFontRenderTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  // Force the use of the freetype based rendering strategy
  vtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
      ->SetStringRendererToFreeType();

  // Use the FontConfig font lookup
  vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(SystemFontRenderTest);
bool SystemFontRenderTest::Paint(vtkContext2D *painter)
{
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);

  int y = 360;

  painter->GetTextProp()->SetFontFamilyToArial();

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  painter->GetTextProp()->SetFontFamilyToTimes();

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  painter->GetTextProp()->SetFontFamilyToCourier();

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  return true;
}
