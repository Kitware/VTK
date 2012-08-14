/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContextMathTextImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextUtilities.h"

#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
class ContextMathTextImageTest : public vtkContextItem
{
public:
  static ContextMathTextImageTest *New();
  vtkTypeMacro(ContextMathTextImageTest, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestContextMathTextImage(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 150);
  vtkNew<ContextMathTextImageTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(ContextMathTextImageTest)

// This function aims to test the primitives provided by the 2D API.
bool ContextMathTextImageTest::Paint(vtkContext2D *painter)
{
  painter->GetTextProp()->SetColor(0.4, 0.6, 0.7);
  painter->GetTextProp()->SetFontSize(60);
  painter->DrawMathTextString(20, 20, "$\\left{\\frac{-b\\pm\\sqrt{b^2-4ac}}"
                              "{2a}\\right}$");

  return true;
}
