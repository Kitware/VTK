// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that reading pixels from the framebuffer of the renderwindow works as expected
 */

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkFloatArray.h"
#include "vtkLogger.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"

#define VTK_TEST_READ_PIXELS(condition)                                                            \
  if (!(condition))                                                                                \
  {                                                                                                \
    vtkLog(ERROR, "Unsatisfied pixel value condition at line " << __LINE__ << ": " << #condition); \
    return EXIT_FAILURE;                                                                           \
  }

int TestReadPixels(int, char*[])
{
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;

  mapper->SetInputConnection(cone->GetOutputPort());
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(100.0 / 255.0, 110.0 / 255.0, 120.0 / 255.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // Verify background color as unsigned char RGBA values
  vtkNew<vtkUnsignedCharArray> ucharRGBA;
  renWin->GetRGBACharPixelData(0, 0, 299, 299, 0, ucharRGBA);

  unsigned char ucharTuple[4];
  ucharRGBA->GetTypedTuple(0, ucharTuple);
  VTK_TEST_READ_PIXELS(ucharTuple[0] == 100);
  VTK_TEST_READ_PIXELS(ucharTuple[1] == 110);
  VTK_TEST_READ_PIXELS(ucharTuple[2] == 120);
  VTK_TEST_READ_PIXELS(ucharTuple[3] == 0);

  ucharRGBA->GetTypedTuple(299 * 299, ucharTuple);
  VTK_TEST_READ_PIXELS(ucharTuple[0] == 100);
  VTK_TEST_READ_PIXELS(ucharTuple[1] == 110);
  VTK_TEST_READ_PIXELS(ucharTuple[2] == 120);
  VTK_TEST_READ_PIXELS(ucharTuple[3] == 0);

  // Verify background color as normalized float32 RGBA values
  vtkNew<vtkFloatArray> f32RGBA;
  renWin->GetRGBAPixelData(0, 0, 299, 299, 0, f32RGBA);

  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(0)[0] * 255) == 100);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(0)[1] * 255) == 110);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(0)[2] * 255) == 120);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(0)[3] * 255) == 0);

  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(299)[0] * 255) == 100);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(299)[1] * 255) == 110);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(299)[2] * 255) == 120);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGBA->GetTuple(299)[3] * 255) == 0);

  // Verify background color as unsigned char RGB values
  vtkNew<vtkUnsignedCharArray> ucharRGB;
  renWin->GetPixelData(0, 0, 299, 299, 0, ucharRGB, 0);

  ucharRGB->GetTypedTuple(0, ucharTuple);
  VTK_TEST_READ_PIXELS(ucharTuple[0] == 100);
  VTK_TEST_READ_PIXELS(ucharTuple[1] == 110);
  VTK_TEST_READ_PIXELS(ucharTuple[2] == 120);

  ucharRGB->GetTypedTuple(299 * 299, ucharTuple);
  VTK_TEST_READ_PIXELS(ucharTuple[0] == 100);
  VTK_TEST_READ_PIXELS(ucharTuple[1] == 110);
  VTK_TEST_READ_PIXELS(ucharTuple[2] == 120);

  // Verify background color as normalized float32 RGB values
  vtkNew<vtkFloatArray> f32RGB;
  renWin->GetRGBAPixelData(0, 0, 299, 299, 0, f32RGB, 0);

  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(0)[0] * 255) == 100);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(0)[1] * 255) == 110);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(0)[2] * 255) == 120);

  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(299)[0] * 255) == 100);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(299)[1] * 255) == 110);
  VTK_TEST_READ_PIXELS(static_cast<int>(f32RGB->GetTuple(299)[2] * 255) == 120);

  return EXIT_SUCCESS;
}
