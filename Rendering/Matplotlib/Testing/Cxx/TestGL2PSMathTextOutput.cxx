/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSMathTextOutput.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkGL2PSExporter.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
class GL2PSMathTextOutputTest : public vtkContextItem
{
public:
  static GL2PSMathTextOutputTest *New();
  vtkTypeMacro(GL2PSMathTextOutputTest, vtkContextItem)
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestGL2PSMathTextOutput(int, char *[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 160);
  vtkNew<GL2PSMathTextOutputTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToBSP();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOff();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSMathTextOutput");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(GL2PSMathTextOutputTest)

// This function aims to test the primitives provided by the 2D API.
bool GL2PSMathTextOutputTest::Paint(vtkContext2D *painter)
{
  painter->GetTextProp()->SetColor(.7, .4, .5);
  painter->GetTextProp()->SetFontSize(30);
  painter->DrawString(20, 100, "Bezier curve");
  painter->DrawMathTextString(20, 120, "$\\mathfrak{B\\'ezier\\/curve}:$");
  painter->GetTextProp()->SetFontSize(25);
  painter->DrawMathTextString(40, 20, "$B_{[0,n]}(t) = \\sum_{j=0}^{n}\\/t^j"
                              "\\left["
                              "\\frac{n!}{(n-j)!}\\sum_{i=0}^{j}\\/"
                              "\\frac{(-1)^{i+j}P_i}{i!(j-i)!}"
                              "\\right] = "
                              "(1-t)B_{[0,n-1]}(t) + tB_{[1,n]}(t)$");
  return true;
}
