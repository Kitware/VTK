/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSubsetWithSeed.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkActor.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkExtractSubsetWithSeed.h"
#include "vtkGeometryFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLStructuredGridReader.h"

namespace
{
vtkSmartPointer<vtkDataObject> GetDataSet(int argc, char* argv[])
{
  char* fname[3];
  fname[0] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_0.vts");
  fname[1] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_1.vts");
  fname[2] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_2.vts");

  vtkNew<vtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < 3; ++cc)
  {
    vtkNew<vtkXMLStructuredGridReader> reader;
    reader->SetFileName(fname[cc]);
    reader->Update();

    mb->SetBlock(cc, reader->GetOutputDataObject(0));

    delete[] fname[cc];
  }
  return mb;
}
}

int TestExtractSubsetWithSeed(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  auto data = GetDataSet(argc, argv);

  vtkNew<vtkExtractSubsetWithSeed> extract1;
  extract1->SetInputDataObject(data);
  extract1->SetSeed(1.74, 0.65, 26.6);
  extract1->SetDirectionToLineI();
  extract1->Update();

  vtkNew<vtkGeometryFilter> geom1;
  geom1->SetInputConnection(extract1->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper2> mapper1;
  mapper1->SetInputConnection(geom1->GetOutputPort());

  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);

  vtkNew<vtkExtractSubsetWithSeed> extract2;
  extract2->SetInputDataObject(data);
  extract2->SetSeed(1.74, 0.65, 26.6);
  extract2->SetDirectionToPlaneJK();
  extract2->Update();

  vtkNew<vtkGeometryFilter> geom2;
  geom2->SetInputConnection(extract2->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper2> mapper2;
  mapper2->SetInputConnection(geom2->GetOutputPort());

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  vtkNew<vtkStructuredGridOutlineFilter> outline;
  outline->SetInputDataObject(data);

  vtkNew<vtkCompositePolyDataMapper2> mapperOutline;
  mapperOutline->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> actorOutline;
  actorOutline->SetMapper(mapperOutline);
  renderer->AddActor(actorOutline);

  renWin->Render();
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTester::Test(argc, argv, renWin, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return !retVal;
}
