// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkFeatures.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkXMLMultiBlockDataReader.h>

#include <vtkLogger.h>
#include <vtkRegressionTestImage.h>
#include <vtkTesting.h>

/**
 * The purpose of this test is to make sure that we always have the same image produced by the
 * composite polydata mapper if we do multiple render call, it was previously not the case when
 * a multiblock have some overlap between these blocks.
 */
int TestCompositeDataOverlappingCells(int argc, char* argv[])
{
  vtkDebugWithObjectMacro(nullptr, "Load the multiblock.");

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename += "/Data/overlap_faces.vtm";

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkDebugWithObjectMacro(nullptr, "Setup everything to be able to render with VTK this data");

  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->SetSize(400, 400);

  vtkSmartPointer<vtkCompositePolyDataMapper> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SelectColorArray("SpatioTemporalHarmonics");

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  vtkDebugWithObjectMacro(nullptr,
    "Everything should be setup now, do multiple render call and check that we always "
    "have the same result.");

  int status = EXIT_SUCCESS;
  int numberOfRenderCalls = 10;

  for (int i = 0; i < numberOfRenderCalls; i++)
  {
    reader->GetOutputInformation(0)->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), i);
    reader->Update();
    int retVal = vtkRegressionTestImageThreshold(win, 0.05);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
      // if we test this executable we don't want to do multiple render call, skip.
      break;
    }

    if (retVal == vtkRegressionTester::FAILED)
    {
      vtkErrorWithObjectMacro(
        nullptr, << "The " << i
                 << "th image produced is wrong, we should always have the same image produced.");
      status = EXIT_FAILURE;
      break;
    }
  }

  return status;
}
