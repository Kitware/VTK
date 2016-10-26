/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextOpaquePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBillboardTextActor3D.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"
#include "vtkTextMapper.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

#include <string>

namespace {
template <typename T>
void configureTextArray(vtkNew<T> objs[2][2][3],
                        const std::string &prefix)
{
  // Text options: half / full opacity
  double tOpacity[2] = { 0.5, 1.0 };
  std::string tLabel[2] = { "TH", "TF" };

  // Border ("E"dge) properties: On / Off
  int eFlag[2] = { 0, 1 };
  std::string eLabel[2] = { "E0", "E1" };

  // Background properties: Off, half, full opacity
  double bOpacity[3] = { 0.0, 0.5, 1.0 };
  std::string bLabel[3] = { "B0", "BH", "BF" };

  for (size_t t = 0; t < 2; ++t)
  {
    for (size_t e = 0; e < 2; ++e)
    {
      for (size_t b = 0; b < 3; ++b)
      {
        T *obj = objs[t][e][b].Get();
        vtkTextProperty *tprop = obj->GetTextProperty();

        tprop->SetJustificationToCentered();
        tprop->SetVerticalJustificationToCentered();

        std::string input = prefix + ": " + tLabel[t] + eLabel[e] + bLabel[b];

        obj->SetInput(input.c_str());
        tprop->SetColor(0., 0., 1.);
        tprop->SetOpacity(tOpacity[t]);

        tprop->SetFrameColor(0., 1., 0.);
        tprop->SetFrameWidth(2);
        tprop->SetFrame(eFlag[e]);

        tprop->SetBackgroundColor(1., 0., 0.);
        tprop->SetBackgroundOpacity(bOpacity[b]);
      }
    }
  }
}
}

//------------------------------------------------------------------------------
// This test ensures that text rendered with
// vtkTextProperty::ForceOpaqueTextures is handled by the opaque render pass.
int TestTextOpaquePass(int, char *[])
{
  // Create combinations of opacities/features [text][border][background]
  // Text has two values, half or full opacity.
  // Border has two states: off or full opacity.
  // Background has three states: off, half, or full opacity
  vtkNew<vtkTextActor> textActor[2][2][3];

  vtkNew<vtkTextActor3D> textActor3D[2][2][3];

  vtkNew<vtkTextMapper> textMapper[2][2][3];
  vtkNew<vtkActor2D> textMapperActor[2][2][3];

  vtkNew<vtkBillboardTextActor3D> billboardActor[2][2][3];

  configureTextArray(textActor, "vtkTextActor");
  configureTextArray(textActor3D, "vtkTextActor3D");
  configureTextArray(textMapper, "vtkTextMapper");
  configureTextArray(billboardActor, "vtkBillboardTextActor3D");

  int width = 600;
  int height = 600;

  // Disable everything but opaque and overlay:
  vtkNew<vtkRenderStepsPass> pass;
  pass->SetTranslucentPass(NULL);
  pass->SetVolumetricPass(NULL);

  vtkNew<vtkRenderer> ren;
  ren->SetPass(pass.Get());
  ren->GradientBackgroundOn();
  ren->SetBackground(0., 0., 0.);
  ren->SetBackground2(1., 1., 1.);

  // To make things easier, setup the camera so that WC@Z=0 roughly match DC.
  ren->GetActiveCamera()->ParallelProjectionOn();
  ren->GetActiveCamera()->SetPosition(width/2, height/2, 1.);
  ren->GetActiveCamera()->SetFocalPoint(width/2, height/2, 0.);
  ren->GetActiveCamera()->SetViewUp(0., 1., 0.);
  ren->GetActiveCamera()->SetParallelScale(height/2);

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  win->SetSize(width, height);
  win->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  // Used for computing coordinates:
  double dx = width / 5.;
  double dy = height / 13.;

  for (size_t t = 0; t < 2; ++t)
  {
    for (size_t e = 0; e < 2; ++e)
    {
      for (size_t b = 0; b < 3; ++b)
      {
        textMapperActor[t][e][b]->SetMapper(textMapper[t][e][b].Get());

        ren->AddViewProp(textActor[t][e][b].Get());
        ren->AddViewProp(textActor3D[t][e][b].Get());
        ren->AddViewProp(textMapperActor[t][e][b].Get());
        ren->AddViewProp(billboardActor[t][e][b].Get());

        // Convert TEB coordinates into a flat index:
        size_t idx = t * 6 + e * 3 + b;

        // Set positions:
        textActor[t][e][b]->SetPosition(dx * 1, dy * (idx + 1));
        textActor3D[t][e][b]->SetPosition(dx * 2, dy * (idx + 1.5), 0.);
        textMapperActor[t][e][b]->SetPosition(dx * 3, dy * (idx + 1));
        billboardActor[t][e][b]->SetPosition(dx * 4, dy * (idx + 1.5), 0.);

        // Force opaque for 3D actors:
        textActor3D[t][e][b]->SetForceOpaque(true);
        billboardActor[t][e][b]->SetForceOpaque(true);
      }
    }
  }

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
