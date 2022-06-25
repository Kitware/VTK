/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFreeTypeTextMapperWithColumns.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextMapper.h"

#include "vtkActor2D.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

#include <string>

struct ActorInfo
{
  const char* Text;
  int FontSize;
  double Color[3];
  double BackgroundColor[3];
  double BackgroundOpacity;
  int Justification;
  int VerticalJustification;
  double CellOffset;
  double LineOffset;
  double LineSpacing;
  double Orientation;
  bool Frame;
  int FontFamily;
  double Position[2];
} actor_info[] = {
  { "Sample multiline\ntext rendered\nwith mathText $\\sum_{i=0}^\\infty x_i$", 16,
    { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, 0.5, VTK_TEXT_LEFT, VTK_TEXT_TOP, 0., 0., 1., 0., false,
    VTK_TIMES, { 10., 100. } },
  { "Sample multiline\ntext rendered\nwith mathText $\\sum_{i=0}^\\infty x_i$", 16,
    { 0.0, 1.0, 0.0 }, { 0.4, 0.5, 0.0 }, 0.5, VTK_TEXT_RIGHT, VTK_TEXT_CENTERED, 0., 5., 1., 0.,
    true, VTK_TIMES, { 350., 60. } },
  { "Cell1 | Cell2 | $\\sum_{i=0}^\\infty x_i$\n12 | $\\sum_{i=0}^\\infty x_i$ | 2345", 16,
    { 0.0, 0.0, 1.0 }, { 0.2, 0.0, 0.5 }, 0.5, VTK_TEXT_CENTERED, VTK_TEXT_CENTERED, 0., 5., 1., 0.,
    true, VTK_COURIER, { 500., 60. } },
  { "Cell1 | Cell2 | $\\sum_{i=0}^\\infty x_i$\n12 | $\\sum_{i=0}^\\infty x_i$ | 2345", 16,
    { 1.0, 0.2, 0.0 }, { 0.3, 0.1, 0.7 }, 0.5, VTK_TEXT_CENTERED, VTK_TEXT_BOTTOM, 30., 10., 1., 0.,
    true, VTK_ARIAL, { 150., 150. } },
  { "Cell1 | Cell2 | $\\sum_{i=0}^\\infty x_i$\n12 | $\\sum_{i=0}^\\infty x_i$ | 2345", 16,
    { 1.0, 0.2, 0.0 }, { 0.3, 0.1, 0.7 }, 0.5, VTK_TEXT_CENTERED, VTK_TEXT_TOP, 30., 10., 1.5, 50.,
    false, VTK_ARIAL, { 400., 320. } },
  { "1|2|3|4\n1|2|3", 16, { 0.3, 0.9, 0.5 }, { 0.0, 1.0, 0.0 }, 0.5, VTK_TEXT_CENTERED,
    VTK_TEXT_CENTERED, 41., 0., 1., 0., true, VTK_ARIAL, { 120., 350. } },
  { "|||\n", 16, { 0.6, 0.8, 0.2 }, { 0.0, 1.0, 0.0 }, 0.5, VTK_TEXT_CENTERED, VTK_TEXT_CENTERED,
    41., 0., 1., 0., false, VTK_ARIAL, { 120., 400. } },
  { "TEST|||\n\n\n", 20, { 1.0, 0.6, 0.6 }, { 0.0, 0.5, 0.0 }, 0.5, VTK_TEXT_CENTERED,
    VTK_TEXT_CENTERED, 20., 0., 1.5, 0., true, VTK_ARIAL, { 120., 500. } },
  { "TEST|||\n\n\n|Test", 20, { 0.2, 0.7, 0.4 }, { 0.0, 1.0, 0.0 }, 0.5, VTK_TEXT_CENTERED,
    VTK_TEXT_CENTERED, 41., 0., 1., 0., true, VTK_ARIAL, { 150., 550. } },
  { "1|2|3|4\n1|2|3", 16, { 0.3, 0.9, 0.5 }, { 0.0, 1.0, 0.0 }, 0.5, VTK_TEXT_CENTERED,
    VTK_TEXT_CENTERED, 41., 0., 1., 90., true, VTK_ARIAL, { 260., 390. } },
  { "\\| Test FreeType escaped pipe \\pi \\|", 16, { 0.0, 0.0, 1.0 }, { 0.2, 1.0, 0.0 }, 0.5,
    VTK_TEXT_CENTERED, VTK_TEXT_CENTERED, 0., 0., 1., 0., false, VTK_ARIAL, { 450., 430. } },
  { "\\| Test MathText escaped pipe $\\pi$ \\| | column", 16, { 1.0, 0.0, 0.0 }, { 0.0, 0.3, 0.2 },
    0.5, VTK_TEXT_CENTERED, VTK_TEXT_CENTERED, 0., 0., 1., 0., false, VTK_ARIAL, { 400., 510. } },
};

//------------------------------------------------------------------------------
int TestFreeTypeTextMapperWithColumns(int argc, char* /*argv*/[])
{
  if (argc < 2)
  {
    cerr << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.1, 0.1);
  vtkNew<vtkRenderWindow> win;
  win->SetSize(600, 600);
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  for (auto& info : actor_info)
  {
    vtkNew<vtkTextMapper> mapper;

    mapper->SetInput(info.Text);

    vtkTextProperty* prop = mapper->GetTextProperty();
    prop->SetFontSize(info.FontSize);
    prop->SetColor(info.Color);
    prop->SetBackgroundColor(info.BackgroundColor);
    prop->SetBackgroundOpacity(info.BackgroundOpacity);
    prop->SetJustification(info.Justification);
    prop->SetVerticalJustification(info.VerticalJustification);
    prop->SetCellOffset(info.CellOffset);
    prop->SetLineOffset(info.LineOffset);
    prop->SetLineSpacing(info.LineSpacing);
    prop->SetOrientation(info.Orientation);
    prop->SetFrame(info.Frame);
    prop->SetFontFamily(info.FontFamily);

    vtkNew<vtkActor2D> actor;
    actor->SetMapper(mapper);
    actor->SetPosition(info.Position);

    ren->AddActor(actor);
  }

  win->SetMultiSamples(0);
  win->Render();
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
