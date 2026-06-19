// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkFileResourceStream.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

#include "vtkOCCTPDCReader.h"

namespace
{
int TestReader(int argc, char* argv[], const std::string& filePath, unsigned int format)
{
  // Compute the full path to the file
  char* path = vtkTestUtilities::ExpandDataFileName(argc, argv, filePath.c_str(), 0);
  vtkNew<vtkFileResourceStream> stream;
  stream->Open(path);

  vtkNew<vtkOCCTPDCReader> reader;
  reader->RelativeDeflectionOn();
  reader->SetLinearDeflection(0.125);
  reader->SetAngularDeflection(0.25);
  reader->ReadWireOn();
  reader->SetFileFormat(format);
  reader->SetStream(stream);
  reader->Update();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(reader->GetOutput());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->RotateY(90);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->SetSize(400, 400);
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal;
}
} // End Anonymous namespace

int TestOCCTPDCStreamReader(int argc, char* argv[])
{
  if (argc < 3)
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(argc, argv, "/Data/nist_ctc_01_asme1_ap203.stp", vtkOCCTPDCReader::Format::AUTO))
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(argc, argv, "/Data/cheese.brep", vtkOCCTPDCReader::Format::AUTO))
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(argc, argv, "/Data/cheese.xbf", vtkOCCTPDCReader::Format::AUTO))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
