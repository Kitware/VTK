/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPDFContextShading.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDFExporter.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"
#include "vtkUnsignedCharArray.h"

#include "vtkRenderingOpenGLConfigure.h"

#include <string>

namespace {
class ContextPDFTest : public vtkContextItem
{
public:
  static ContextPDFTest *New();
  vtkTypeMacro(ContextPDFTest, vtkContextItem)
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;
};
vtkStandardNewMacro(ContextPDFTest)
} // end anon namespace

int TestPDFContextShading(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<ContextPDFTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  std::string filename =
      vtkTestingInteractor::TempDirectory +
      std::string("/TestPDFContextShading.pdf");

  vtkNew<vtkPDFExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileName(filename.c_str());
  exp->Write();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
// This function aims to test the primitives provided by the 2D API.
namespace {
bool ContextPDFTest::Paint(vtkContext2D *painter)
{
  float poly[] = {
     50.f,  50.f,
     25.f, 150.f,
     50.f, 250.f,
    150.f, 275.f,
    250.f, 250.f,
    275.f, 150.f,
    250.f,  50.f,
    150.f,  25.f
  };
  unsigned char polyColor[] = {
     32, 192,  64,
    128,  32,  64,
    192,  16, 128,
    255,  16,  92,
    128, 128,  16,
     64, 255,  32,
     32, 192, 128,
     32, 128, 255
  };
  painter->DrawPolygon(poly, 8, polyColor, 3);

  float triangle[] = {
    100.f, 100.f,
    150.f, 200.f,
    200.f, 100.f
  };
  unsigned char triangleColor[] = {
    255, 0, 0,
    0, 255, 0,
    0, 0, 255
  };
  painter->DrawPolygon(triangle, 3, triangleColor, 3);

  float line[] = {
    290, 290,
    290, 150,
    290,  10,
    150,  10,
     10,  10,
     10, 150,
     10, 290,
    150, 290,
    290, 290
  };
  unsigned char lineColor[] = {
    255,  32,  16,
    128, 128,  32,
    255, 255,  64,
    128, 192, 128,
     64, 128, 192,
    255,   0,   0,
      0, 255,   0,
      0,   0, 255,
    255,  32,  16
  };
  painter->DrawPoly(line, 9, lineColor, 3);

  return true;
}
} // end anon namespace
