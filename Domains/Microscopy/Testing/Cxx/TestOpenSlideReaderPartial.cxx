/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenSlideReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkNew.h>
#include <vtkOpenSlideReader.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkPNGWriter.h>

// VTK includes
#include <vtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestOpenSlideReaderPartial(int argc, char** argv)
{
  if ( argc <= 1 )
  {
    std::cout << "Usage: " << argv[0] << " <image file>" << endl;
    return EXIT_FAILURE;
  }

  std::cout << "Got Filename: " << argv[1] << std::endl;

  // Create reader to read shape file.
  vtkNew<vtkOpenSlideReader> reader;
  reader->SetFileName(argv[1]);
  reader->UpdateInformation();

  int extent[6] = {100,299,100,299,0,0};

  reader->UpdateExtent(extent);

  vtkNew<vtkImageData> data;
  data->ShallowCopy(reader->GetOutput());

  // // For debug
  // vtkNew<vtkPNGWriter> writer;
  // writer->SetInputData(data.GetPointer());
  // writer->SetFileName("this.png");
  // writer->SetUpdateExtent(extent);
  // writer->Update();
  // writer->Write();

  // Visualize
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(window.GetPointer());

  vtkNew<vtkImageViewer2> imageViewer;
  imageViewer->SetInputData(data.GetPointer());
  //imageViewer->SetExtent(1000,1500,1000,1500,0,0);
  imageViewer->SetupInteractor(renderWindowInteractor.GetPointer());
  //imageViewer->SetSlice(0);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
