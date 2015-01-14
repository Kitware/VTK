/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSMathTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextActor.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGL2PSExporter.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

#include <sstream>

namespace vtkGL2PSTestMathTextActor {
void setupTextActor(vtkTextActor *actor, vtkPolyData *anchor)
{
  vtkTextProperty *p = actor->GetTextProperty();
  std::ostringstream label;
  label << p->GetVerticalJustificationAsString()[0]
        << p->GetJustificationAsString()[0] << " "
        << "$\\theta = " << p->GetOrientation() << "$";
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double *pos = actor->GetPosition();
  double *col = p->GetColor();
  vtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(col[0] * 255,
                                                        col[1] * 255,
                                                        col[2] * 255, 255);
}
} // end namespace vtkGL2PSTestMathTextActor3D

//----------------------------------------------------------------------------
int TestGL2PSMathTextActor(int, char *[])
{
  using namespace vtkGL2PSTestMathTextActor;
  vtkNew<vtkRenderer> ren;

  int width = 600;
  int height = 600;
  int x[3] = {100, 300, 500};
  int y[3] = {100, 300, 500};

  // Render the anchor points to check alignment:
  vtkNew<vtkPolyData> anchors;
  vtkNew<vtkPoints> points;
  anchors->SetPoints(points.GetPointer());
  vtkNew<vtkCellArray> verts;
  anchors->SetVerts(verts.GetPointer());
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  anchors->GetCellData()->SetScalars(colors.GetPointer());

  for (size_t row = 0; row < 3; ++row)
    {
    for (size_t col = 0; col < 3; ++col)
      {
      vtkNew<vtkTextActor> actor;
      switch (row)
        {
        case 0:
          actor->GetTextProperty()->SetJustificationToRight();
          break;
        case 1:
          actor->GetTextProperty()->SetJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetJustificationToLeft();
          break;
        }
      switch (col)
        {
        case 0:
          actor->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
        case 1:
          actor->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetVerticalJustificationToTop();
          break;
        }
      actor->GetTextProperty()->SetFontSize(22);
      actor->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .26);
      actor->GetTextProperty()->SetBackgroundColor(0.0,
                                                   1. - col * .26,
                                                   1. - row * .26);
      actor->GetTextProperty()->SetBackgroundOpacity(0.25);
      actor->SetPosition(x[col], y[row]);
      setupTextActor(actor.GetPointer(), anchors.GetPointer());
      ren->AddActor(actor.GetPointer());
      }
    }

  vtkNew<vtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors.GetPointer());
  vtkNew<vtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper.GetPointer());
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor.GetPointer());

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(width, height);

  win->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(win.GetPointer());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSMathTextActor");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
