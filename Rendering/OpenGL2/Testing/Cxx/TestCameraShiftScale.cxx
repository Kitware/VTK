/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

namespace
{

void createData(vtkPolyData* poly)
{
  // create data
  vtkNew<vtkPoints> pts;
  vtkNew<vtkCellArray> cells;

  int xres = 200;
  int yres = 20;
  pts->SetDataType(VTK_DOUBLE);
  for (int y = 0; y < yres; y++)
  {
    double angle = 2.0 * y / yres;
    for (int x = 0; x < xres; x++)
    {
      // every ten steps in x double in size
      int step = x / 10;
      double size = pow(2.0, step);
      double radius = 0.001 * (1.0 + 10.0 * (size - 1.0) + (x % 10) * size);
      pts->InsertNextPoint(40000.0 + radius * cos(angle), radius * sin(angle), 0.0);
    }
  }
  poly->SetPoints(pts);

  for (int y = 0; y < yres - 1; y++)
  {
    for (int x = 0; x < xres - 1; x++)
    {
      vtkIdType cellids[3];
      cellids[0] = y * xres + x;
      cellids[1] = y * xres + x + 1;
      cellids[2] = (y + 1) * xres + x + 1;
      cells->InsertNextCell(3, cellids);
      cellids[0] = y * xres + x;
      cellids[1] = (y + 1) * xres + x + 1;
      cellids[2] = (y + 1) * xres + x;
      cells->InsertNextCell(3, cellids);
    }
  }
  poly->SetPolys(cells);
}

// Press 'm' to change mapper shift scale method between none, auto, and focal point
void keypressFunc(vtkObject* caller, unsigned long vtkNotUsed(eventId), void* clientData,
  void* vtkNotUsed(callData))
{
  const auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
  auto mapper = static_cast<vtkOpenGLPolyDataMapper*>(clientData);
  if (iren->GetKeyCode() == ' ')
  {
    auto currMethod = mapper->GetVBOShiftScaleMethod();
    if (currMethod == vtkOpenGLVertexBufferObject::DISABLE_SHIFT_SCALE)
    {
      mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::ALWAYS_AUTO_SHIFT_SCALE);
    }
    else if (currMethod == vtkOpenGLVertexBufferObject::ALWAYS_AUTO_SHIFT_SCALE)
    {
      mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::AUTO_SHIFT);
    }
    else if (currMethod == vtkOpenGLVertexBufferObject::AUTO_SHIFT)
    {
      mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::FOCAL_POINT_SHIFT_SCALE);
    }
    else if (currMethod == vtkOpenGLVertexBufferObject::FOCAL_POINT_SHIFT_SCALE)
    {
      mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::DISABLE_SHIFT_SCALE);
    }

    createData(mapper->GetInput());
    static_cast<vtkRenderWindowInteractor*>(caller)->Render();
  }
}

} // end anon

//------------------------------------------------------------------------------
int TestCameraShiftScale(int argc, char* argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkPolyData> poly;
  createData(poly);

  mapper->SetInputData(poly);

  mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::FOCAL_POINT_SHIFT_SCALE);

  actor->SetMapper(mapper);
  actor->GetProperty()->SetDiffuse(0.0);
  actor->GetProperty()->SetAmbient(1.0);
  actor->GetProperty()->SetRepresentationToWireframe();
  actor->SetPosition(-40000, 0, 0);

  renderer->SetBackground(0.1, 0.2, 0.4);

  renderer->GetActiveCamera()->SetPosition(0.001, 0.0015, 0.01);
  renderer->GetActiveCamera()->SetFocalPoint(0.001, 0.0015, 0);

  renderer->ResetCameraClippingRange();
  renderWindow->Render();
  renderWindow->Render();

  vtkNew<vtkCallbackCommand> keypressCallback;
  keypressCallback->SetCallback(keypressFunc);
  keypressCallback->SetClientData(mapper);

  iren->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
