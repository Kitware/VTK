/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This tests reading a LAS file.
 */

#include <vtkCamera.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkXMLPolyDataWriter.h"

#include "vtkLASReader.h"

int TestLASReader(int argc, char **argv)
{
  //const char* fileName = "Data/tp_manual_20160907131754_flt.las";
  const char* fileName = "Data/test_buildings.las";
  const char* path = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName);
  vtkNew<vtkLASReader> reader;

  //Select source file
  reader->SetFileName(path);
  reader->SetVisualizationType(vtkLASReader::Classification);

  //Read the output
  reader->Update();

  vtkSmartPointer<vtkPolyData> outputData = reader->GetOutput();

  vtkNew<vtkXMLPolyDataWriter> writer;
  writer->SetFileName("/home/danlipsa/tmp/test.vtp");
  writer->SetInputData(outputData);
  writer->Write();

  //Visualise in a render window
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(outputData);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Elevation(-90.0);

  int retVal = vtkRegressionTestImage (renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start ();
    retVal = vtkRegressionTester::PASSED;
  }
  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
