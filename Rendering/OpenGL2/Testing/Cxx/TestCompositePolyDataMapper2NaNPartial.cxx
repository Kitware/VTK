/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2NaNPartial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTrivialProducer.h"

int TestCompositePolyDataMapper2NaNPartial(int, char*[])
{
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->Update();
  vtkPolyData *sphere =
      vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkSmartPointer<vtkPolyData> sphere1 =
      vtkSmartPointer<vtkPolyData>::Take(sphere->NewInstance());
  sphere1->DeepCopy(sphere);

  sphereSource->SetCenter(1., 0., 0.);
  sphereSource->Update();
  sphere = vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkSmartPointer<vtkPolyData> sphere2 =
      vtkSmartPointer<vtkPolyData>::Take(sphere->NewInstance());
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
  sphere1->GetPointData()->SetScalars(scalars.Get());

  vtkNew<vtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(2);
  mbds->SetBlock(0, sphere1);
  mbds->SetBlock(1, sphere2.Get());

  vtkNew<vtkTrivialProducer> source;
  source->SetOutput(mbds.Get());

  vtkNew<vtkLookupTable> lut;
  lut->SetValueRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetLookupTable(lut.Get());
  mapper->SetScalarVisibility(1);
  mapper->SetScalarRange(scalars->GetRange());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetInputArrayToProcess(0, 0, 0,
                                 vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                 vtkDataSetAttributes::SCALARS);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetColor(0., 0., 1.);
  renderer->AddActor(actor.Get());

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin.Get());
  renWin->AddRenderer(renderer.Get());

  renWin->SetSize(500,500);
  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
