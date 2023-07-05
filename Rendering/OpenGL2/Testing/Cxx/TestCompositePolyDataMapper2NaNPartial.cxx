// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCompositePolyDataMapper2.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTrivialProducer.h"

int TestCompositePolyDataMapper2NaNPartial(int, char*[])
{
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->Update();
  vtkPolyData* sphere = vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkSmartPointer<vtkPolyData> sphere1 = vtkSmartPointer<vtkPolyData>::Take(sphere->NewInstance());
  sphere1->DeepCopy(sphere);

  sphereSource->SetCenter(1., 0., 0.);
  sphereSource->Update();
  sphere = vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkSmartPointer<vtkPolyData> sphere2 = vtkSmartPointer<vtkPolyData>::Take(sphere->NewInstance());
  sphere2->DeepCopy(sphere);

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("Scalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(sphere1->GetNumberOfPoints());
  for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
  {
    scalars->SetTypedComponent(i, 0, static_cast<float>(i));
  }

  // Only add scalars to sphere 1.
  sphere1->GetPointData()->SetScalars(scalars);

  vtkNew<vtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(2);
  mbds->SetBlock(0, sphere1);
  mbds->SetBlock(1, sphere2);

  vtkNew<vtkTrivialProducer> source;
  source->SetOutput(mbds);

  vtkNew<vtkLookupTable> lut;
  lut->SetValueRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetScalarVisibility(1);
  mapper->SetScalarRange(scalars->GetRange());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0., 0., 1.);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
