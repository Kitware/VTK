// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTextMapper.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGL2PSExporter.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"

#include <sstream>

namespace vtkTestGL2PSTextMapper
{
void setupTextMapper(vtkTextMapper* mapper, vtkActor2D* actor, vtkPolyData* anchor)
{
  vtkTextProperty* p = mapper->GetTextProperty();
  std::ostringstream label;
  label << "Angle: " << p->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  mapper->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetPosition();
  double* col = p->GetColor();
  vtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(
    col[0] * 255, col[1] * 255, col[2] * 255, 255);
}
} // end namespace vtkTestGL2PSTextMapper

//------------------------------------------------------------------------------
int TestGL2PSTextMapper(int, char*[])
{
  using namespace vtkTestGL2PSTextMapper;
  vtkNew<vtkRenderer> ren;

  int width = 600;
  int height = 600;
  int x[3] = { 100, 300, 500 };
  int y[4] = { 100, 233, 366, 500 };

  // Render the anchor points to check alignment:
  vtkNew<vtkPolyData> anchors;
  vtkNew<vtkPoints> points;
  anchors->SetPoints(points);
  vtkNew<vtkCellArray> verts;
  anchors->SetVerts(verts);
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  anchors->GetCellData()->SetScalars(colors);

  for (size_t row = 0; row < 4; ++row)
  {
    for (size_t col = 0; col < 3; ++col)
    {
      vtkNew<vtkTextMapper> mapper;
      vtkNew<vtkActor2D> actor;
      actor->SetMapper(mapper);

      switch (row)
      {
        case 0:
          mapper->GetTextProperty()->SetOrientation(45);
          break;
        case 1:
          mapper->GetTextProperty()->SetOrientation(-45);
          break;
        case 2:
          break;
        case 3:
          mapper->GetTextProperty()->SetOrientation(90);
          break;
      }
      switch (col)
      {
        case 0:
          mapper->GetTextProperty()->SetJustificationToRight();
          mapper->GetTextProperty()->SetVerticalJustificationToTop();
          break;
        case 1:
          mapper->GetTextProperty()->SetJustificationToCentered();
          mapper->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetJustificationToLeft();
          mapper->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
      }
      mapper->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      mapper->GetTextProperty()->SetBackgroundColor(0.0, 0.8 - col * .26, .8 - row * .2);
      mapper->GetTextProperty()->SetBackgroundOpacity(0.25);
      actor->SetPosition(x[col], y[row]);
      setupTextMapper(mapper, actor, anchors);
      ren->AddViewProp(actor);
    }
  }

  vtkNew<vtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors);
  vtkNew<vtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddViewProp(anchorActor);

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(0, 0, 400);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);
  win->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(win);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->TextAsPathOn();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    vtkTestingInteractor::TempDirectory + std::string("/TestGL2PSTextMapper");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
