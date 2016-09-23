/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextMapper.h"

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

namespace vtkTestTextMapper {
void setupTextMapper(vtkTextMapper *mapper,
                     vtkActor2D *actor,
                     vtkPolyData *anchor)
{
  vtkTextProperty *p = mapper->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  mapper->SetInput(label.str().c_str());

  // Add the anchor point:
  double *pos = actor->GetPosition();
  double *col = p->GetColor();
  vtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(col[0] * 255,
                                                        col[1] * 255,
                                                        col[2] * 255, 255);
}
} // end namespace vtkTestTextMapper

//----------------------------------------------------------------------------
int TestTextMapper(int, char *[])
{
  using namespace vtkTestTextMapper;
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
      vtkNew<vtkTextMapper> mapper;
      switch (row)
      {
        case 0:
          mapper->GetTextProperty()->SetJustificationToRight();
          break;
        case 1:
          mapper->GetTextProperty()->SetJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetJustificationToLeft();
          break;
      }
      switch (col)
      {
        case 0:
          mapper->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
        case 1:
          mapper->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetVerticalJustificationToTop();
          break;
      }
      mapper->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      mapper->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      vtkNew<vtkActor2D> actor;
      actor->SetPosition(x[col], y[row]);
      actor->SetMapper(mapper.GetPointer());
      setupTextMapper(mapper.GetPointer(), actor.GetPointer(),
                      anchors.GetPointer());
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
