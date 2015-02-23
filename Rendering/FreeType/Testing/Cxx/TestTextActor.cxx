/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor.cxx

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

namespace vtkTestTextActor {
void setupTextActor(vtkTextActor *actor, vtkPolyData *anchor)
{
  vtkTextProperty *p = actor->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
        << "Actor Angle: " << actor->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
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
} // end namespace vtkTestTextActor

//----------------------------------------------------------------------------
int TestTextActor(int, char *[])
{
  using namespace vtkTestTextActor;
  vtkNew<vtkRenderer> ren;

  int width = 600;
  int height = 600;
  int x[3] = {100, 300, 500};
  int y[4] = {100, 233, 366, 500};

  // Render the anchor points to check alignment:
  vtkNew<vtkPolyData> anchors;
  vtkNew<vtkPoints> points;
  anchors->SetPoints(points.GetPointer());
  vtkNew<vtkCellArray> verts;
  anchors->SetVerts(verts.GetPointer());
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  anchors->GetCellData()->SetScalars(colors.GetPointer());

  for (size_t row = 0; row < 4; ++row)
    {
    for (size_t col = 0; col < 3; ++col)
      {
      vtkNew<vtkTextActor> actor;
      switch (row)
        {
        case 0:
          actor->GetTextProperty()->SetOrientation(45);
          break;
        case 1:
          actor->SetOrientation(-45);
          break;
        case 2:
          break;
        case 3:
          actor->GetTextProperty()->SetOrientation(45);
          actor->SetOrientation(45);
          break;
        }
      switch (col)
        {
        case 0:
          actor->GetTextProperty()->SetJustificationToRight();
          actor->GetTextProperty()->SetVerticalJustificationToTop();
          break;
        case 1:
          actor->GetTextProperty()->SetJustificationToCentered();
          actor->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetJustificationToLeft();
          actor->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
        }
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      actor->GetTextProperty()->SetBackgroundColor(0.25, 0.4 - col * .13,
                                                   .5 - row * .1);
      actor->GetTextProperty()->SetBackgroundOpacity(1.0);

      actor->SetPosition(x[col], y[row]);
      setupTextActor(actor.GetPointer(), anchors.GetPointer());
      ren->AddActor2D(actor.GetPointer());
      }
    }

  vtkNew<vtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors.GetPointer());
  vtkNew<vtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper.GetPointer());
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor2D(anchorActor.GetPointer());

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(0, 0, 400);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
