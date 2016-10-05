/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBillboardTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBillboardTextActor3D.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"

#include <sstream>
#include <vector>

namespace vtkTestBillboardTextActor3D {
void setupBillboardTextActor3D(vtkBillboardTextActor3D *actor,
                               vtkPolyData *anchor)
{
  vtkTextProperty *p = actor->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double *pos = actor->GetPosition();
  double *col = p->GetColor();
  vtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], pos[2]);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(col[0] * 255,
                                                        col[1] * 255,
                                                        col[2] * 255, 255);
}

void setupGrid(vtkPolyData *grid)
{
  double marks[4] = {0., 200., 400., 600.};
  double thickness = 200.;

  vtkNew<vtkPoints> points;
  grid->SetPoints(points.Get());
  for (int x_i = 0; x_i < 4; ++x_i)
  {
    for (int y_i = 0; y_i < 4; ++y_i)
    {
      points->InsertNextPoint(marks[x_i], marks[y_i], -thickness / 2.);
      points->InsertNextPoint(marks[x_i], marks[y_i], +thickness / 2.);
    }
  }

  std::vector<vtkIdType> quads;

  for (vtkIdType col = 0; col < 4; ++col)
  {
    for (int row = 0; row < 3; ++row)
    {
      // Along y:
      vtkIdType base = 8 * col + 2 * row;
      quads.push_back(base + 0);
      quads.push_back(base + 1);
      quads.push_back(base + 3);
      quads.push_back(base + 2);
    }
  }

  vtkNew<vtkCellArray> cellArray;
  grid->SetPolys(cellArray.Get());
  for (size_t i = 0; i < quads.size(); i += 4)
  {
    grid->InsertNextCell(VTK_QUAD, 4, &quads[i]);
  }
}
} // end namespace vtkTestBillboardTextActor3D

//----------------------------------------------------------------------------
int TestBillboardTextActor3D(int, char *[])
{
  using namespace vtkTestBillboardTextActor3D;
  vtkNew<vtkRenderer> ren;
  ren->UseDepthPeelingOn();

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
      vtkNew<vtkBillboardTextActor3D> actor;
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
      actor->GetTextProperty()->SetFontSize(20);
      actor->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .26);
      actor->GetTextProperty()->SetBackgroundColor(0.,
                                                   1. - col * .26,
                                                   1. - row * .26);
      actor->GetTextProperty()->SetBackgroundOpacity(0.85);
      actor->SetPosition(x[col], y[row], 0.);
      setupBillboardTextActor3D(actor.GetPointer(), anchors.GetPointer());
      ren->AddActor(actor.GetPointer());
    }
  }

  vtkNew<vtkPolyDataMapper> anchorMapper;
  anchorMapper->SetInputData(anchors.GetPointer());
  vtkNew<vtkActor> anchorActor;
  anchorActor->SetMapper(anchorMapper.GetPointer());
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor.GetPointer());

  // Add some various 'empty' actors to make sure there are no surprises:
  vtkNew<vtkBillboardTextActor3D> nullInputActor;
  nullInputActor->SetInput(NULL);
  ren->AddActor(nullInputActor.GetPointer());

  vtkNew<vtkBillboardTextActor3D> emptyInputActor;
  emptyInputActor->SetInput("");
  ren->AddActor(emptyInputActor.GetPointer());

  vtkNew<vtkBillboardTextActor3D> spaceActor;
  spaceActor->SetInput(" ");
  ren->AddActor(spaceActor.GetPointer());

  vtkNew<vtkBillboardTextActor3D> tabActor;
  tabActor->SetInput("\t");
  ren->AddActor(tabActor.GetPointer());

  vtkNew<vtkBillboardTextActor3D> newlineActor;
  newlineActor->SetInput("\n");
  ren->AddActor(newlineActor.GetPointer());

  vtkNew<vtkPolyData> grid;
  setupGrid(grid.Get());
  vtkNew<vtkPolyDataMapper> gridMapper;
  gridMapper->SetInputData(grid.Get());
  vtkNew<vtkActor> gridActor;
  gridActor->GetProperty()->SetRepresentationToSurface();
  gridActor->GetProperty()->SetColor(0.6, 0.6, 0.6);
  gridActor->SetMapper(gridMapper.Get());
  ren->AddActor(gridActor.Get());

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(width/2, height/2, 1400);
  ren->GetActiveCamera()->SetFocalPoint(width/2, height/2, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->GetActiveCamera()->Roll(45.);
  ren->GetActiveCamera()->Elevation(45.);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
