/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

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
#include "vtkContextItem.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkOpenGLContextDevice2D.h"

#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
class APIDiagram : public vtkContextItem
{
public:
  static APIDiagram *New();
  vtkTypeMacro(APIDiagram, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);
};

//----------------------------------------------------------------------------
int TestDiagram( int argc, char * argv [] )
{
  // Set up a 2D chart actor, APIDiagram object andn add them to the renderer
  VTK_CREATE(vtkContextActor, actor);
  VTK_CREATE(APIDiagram, diagram);
  actor->GetScene()->AddItem(diagram);
  VTK_CREATE(vtkRenderer, renderer);
  renderer->SetBackground(1.0, 1.0, 1.0);
  VTK_CREATE(vtkRenderWindow, renderWindow);
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);

  // Force the use of the freetype based rendering strategy
  vtkOpenGLContextDevice2D::SafeDownCast(actor->GetContext()->GetDevice())
      ->SetStringRendererToFreeType();

  VTK_CREATE(vtkRenderWindowInteractor, interactor);
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
    }

  return !retVal;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(APIDiagram);
// This function draws our API diagram
bool APIDiagram::Paint(vtkContext2D *painter)
{
  // Drawing a hard wired diagram 800x600 as a demonstration of the 2D API
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetPen()->SetColor(0, 0, 0);

  painter->GetBrush()->SetColor(100, 255, 100);
  painter->DrawRect(100, 50, 200, 100);
  painter->DrawString(200, 100, "OpenGL");

  painter->GetBrush()->SetColor(255, 100, 0);
  painter->DrawRect(300, 50, 200, 100);
  painter->DrawString(400, 100, "Others?");

  painter->GetBrush()->SetColor(100, 0, 255);
  painter->DrawRect(500, 50, 200, 100);
  painter->DrawString(600, 100, "Others?");

  painter->GetBrush()->SetColor(180, 180, 255);
  painter->DrawRect(100, 150, 600, 100);
  painter->DrawString(400, 200, "2D API");

  painter->GetBrush()->SetColor(255, 255, 180);
  painter->DrawRect(100, 250, 600, 200);
  painter->DrawString(400, 400, "Canvas API");

  painter->GetBrush()->SetColor(180, 255, 180);
  painter->DrawRect(100, 250, 300, 100);
  painter->DrawString(250, 300, "Point Mark");

  painter->GetBrush()->SetColor(255, 255, 255);
  painter->DrawRect(100, 450, 600, 100);
  painter->DrawString(400, 500, "Canvas View");

  return true;
}
