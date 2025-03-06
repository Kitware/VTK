// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description:
// Tests vtkValuePass in FLOATING_POINT mode and ensures the depth test is enabled.
// Renders a scalar from a polydata into a float buffer.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkFloatArray.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageSliceMapper.h"
#include "vtkLookupTable.h"
#include "vtkNamedColors.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSuperquadricSource.h"
#include "vtkValuePass.h"

#include <iostream>
#include <string>

int TestValuePassFloatingPoint2(int, char*[])
{
  vtkNew<vtkSuperquadricSource> torusSource;
  torusSource->SetToroidal(1);

  // Render the scalar into an image
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(torusSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iRen;
  iRen->SetRenderWindow(renWin);

  renderer->ResetCamera();

  vtkNew<vtkValuePass> valuePass;
  valuePass->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, "TextureCoords");
  valuePass->SetInputComponentToProcess(0);

  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(valuePass);

  vtkNew<vtkSequencePass> sequence;
  sequence->SetPasses(passes);

  vtkNew<vtkCameraPass> cameraPass;
  cameraPass->SetDelegatePass(sequence);
  renderer->SetPass(cameraPass);
  renWin->Render();

  // Get the rendered image
  vtkFloatArray* renderedArray = valuePass->GetFloatImageDataArray(renderer);
  int* ext = valuePass->GetFloatImageExtents();
  renderedArray->SetName("FloatArray");
  vtkNew<vtkImageData> image;
  image->SetExtent(ext);
  image->GetPointData()->SetScalars(renderedArray);

  // Now that the floating point array has been rendered, remove the renderer.
  renWin->RemoveRenderer(renderer);

  // Sow the rendered image on the screen
  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);
  lut->SetTableRange(image->GetScalarRange());

  vtkNew<vtkImageMapToColors> mapColors;
  mapColors->SetLookupTable(lut);
  mapColors->SetInputDataObject(image);

  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputConnection(mapColors->GetOutputPort());

  vtkNew<vtkRenderer> newRenderer;
  newRenderer->AddActor(imageActor);

  vtkNew<vtkNamedColors> colors;
  newRenderer->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());
  renWin->AddRenderer(newRenderer);
  renWin->SetWindowName("RenderScalarToFloatBuffer");
  renWin->Render();
  iRen->Start();

  return EXIT_SUCCESS;
}
