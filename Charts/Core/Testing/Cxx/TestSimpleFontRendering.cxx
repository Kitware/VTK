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

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
class SimpleFontRenderTest : public vtkContextItem
{
public:
  static SimpleFontRenderTest *New();
  vtkTypeMacro(SimpleFontRenderTest, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestSimpleFontRendering( int, char * [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(150, 30);
  vtkNew<SimpleFontRenderTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  // Force the use of the freetype based rendering strategy
  vtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
      ->SetStringRendererToFreeType();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(SimpleFontRenderTest);
bool SimpleFontRenderTest::Paint(vtkContext2D *painter)
{
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontFamilyToArial();
  painter->GetTextProp()->SetFontSize(24);
  painter->DrawString(5,  5, "1");
  painter->DrawString(50, 5, "1.5");
  painter->DrawString(100, 5, "10");
  return true;
}
