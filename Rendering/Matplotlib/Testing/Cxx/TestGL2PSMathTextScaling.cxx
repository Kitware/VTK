/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSMathTextScaling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkGL2PSExporter.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
class GL2PSMathTextScalingTest : public vtkContextItem
{
public:
  static GL2PSMathTextScalingTest *New();
  vtkTypeMacro(GL2PSMathTextScalingTest, vtkContextItem)
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestGL2PSMathTextScaling(int, char *[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 600);
  view->GetRenderWindow()->SetDPI(120);
  vtkNew<GL2PSMathTextScalingTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);

  vtkNew<vtkGL2PSExporter> exp;

  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOff();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSMathTextScaling");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(GL2PSMathTextScalingTest)

// This function aims to test the primitives provided by the 2D API.
bool GL2PSMathTextScalingTest::Paint(vtkContext2D *painter)
{
  painter->GetBrush()->SetColor(50, 50, 128);
  painter->DrawRect(0, 0, 400, 600);

  painter->GetTextProp()->SetColor(.7, .4, .5);
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetVerticalJustificationToBottom();

  for (int i = 0; i < 10; ++i)
  {
    int fontSize = 5 + i * 3;
    float y = 600 - ((pow(i, 1.2) + 1) * 40);
    painter->GetTextProp()->SetFontSize(fontSize);
    painter->DrawString(5, y, "Text");
    painter->DrawMathTextString(120, y, "MathText");
  }

  return true;
}
