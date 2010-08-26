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
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkContext2D.h"
#include "vtkTransform2D.h"
#include "vtkContextItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkPoints2D.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
class ContextTest : public vtkContextItem
{
public:
  static ContextTest *New();
  vtkTypeMacro(ContextTest, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestContext( int, char * [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  vtkSmartPointer<ContextTest> test = vtkSmartPointer<ContextTest>::New();
  view->GetScene()->AddItem(test);

  // Force the use of the freetype based rendering strategy
  vtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
      ->SetStringRendererToFreeType();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(ContextTest);
// This function aims to test the primitives provided by the 2D API.
bool ContextTest::Paint(vtkContext2D *painter)
{
  // Test the string drawing functionality of the context
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetTextProp()->SetFontFamilyToArial();
  painter->GetPen()->SetColor(0, 0, 0, 255);
  painter->GetBrush()->SetColor(0, 0, 0, 255);
  painter->DrawString(400, 25, "OpenGL is used as a backend to the context.");

  // Draw some individual lines of different thicknesses.
  for (int i = 0; i < 10; ++i)
    {
    painter->GetPen()->SetColor(255,
                                static_cast<unsigned char>(float(i)*25.0),
                                0);
    painter->GetPen()->SetWidth(1.0 + float(i));
    painter->DrawLine(10, 50 + float(i)*10, 60, 50 + float(i)*10);
    }

  // Use the draw lines function now to draw a shape.
  vtkSmartPointer<vtkPoints2D> points = vtkSmartPointer<vtkPoints2D>::New();
  points->SetNumberOfPoints(30);
  for (int i = 0; i < 30; ++i)
    {
    double point[2] = { float(i) * 25.0 + 10.0,
                        sin(float(i) / 5.0) * 100.0 + 200.0 };
    points->SetPoint(i, point);
    }
  painter->GetPen()->SetColor(0, 255, 0);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoly(points);

  // Now to draw some points
  painter->GetPen()->SetColor(0, 0, 255);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoint(10, 10);
  painter->DrawPoint(790, 10);
  painter->DrawPoint(10, 590);
  painter->DrawPoint(790, 590);

  // Draw some individual lines of different thicknesses.
  for (int i = 0; i < 10; ++i)
    {
    painter->GetPen()->SetColor(0,
                                static_cast<unsigned char>(float(i)*25.0),
                                255);
    painter->GetPen()->SetWidth(1.0 + float(i));
    painter->DrawPoint(75, 50 + float(i)*10);
    }

  painter->GetPen()->SetColor(0, 0, 255);
  painter->GetPen()->SetWidth(3.0);
  painter->DrawPoints(points);

  // Now draw a rectangle
  painter->GetPen()->SetColor(100, 200, 255);
  painter->GetPen()->SetWidth(3.0);
  painter->GetBrush()->SetColor(100, 255, 100);
  painter->DrawRect(100, 50, 200, 100);

  // Add in an arbitrary quad
  painter->GetPen()->SetColor(159, 0, 255);
  painter->GetPen()->SetWidth(1.0);
  painter->GetBrush()->SetColor(100, 55, 0, 200);
  painter->DrawQuad(350, 50, 375, 150,
                    525, 199, 666, 45);

  // Now to test out the transform...
  vtkSmartPointer<vtkTransform2D> transform = vtkSmartPointer<vtkTransform2D>::New();
  transform->Translate(20, 200);
  painter->GetDevice()->SetMatrix(transform->GetMatrix());
  painter->GetPen()->SetColor(255, 0, 0);
  painter->GetPen()->SetWidth(6.0);
  painter->DrawPoly(points);

  transform->Translate(0, 10);
  painter->GetDevice()->SetMatrix(transform->GetMatrix());
  painter->GetPen()->SetColor(0, 0, 200);
  painter->GetPen()->SetWidth(2.0);
  painter->DrawPoints(points);

  transform->Translate(0, -20);
  painter->GetDevice()->SetMatrix(transform->GetMatrix());
  painter->GetPen()->SetColor(100, 0, 200);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoints(points);

  // Now for an ellipse...
  painter->GetPen()->SetColor(0, 0, 0);
  painter->GetPen()->SetWidth(1.0);
  painter->GetBrush()->SetColor(0, 0, 100, 69);
  painter->DrawEllipse(110.0, 89.0, 20, 100);

  return true;
}
