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
int TestOpenSlideReader(int argc, char** argv)
{
  // This test is known to fail with openslide library libopenslide-dev shipped
  // with ubuntu 14.04 as of March 31'2016. It does pass on fedora23, or if the
  // openslide library is built from source
  const char* rasterFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                 "Data/Microscopy/small2.ndpi");

  //std::cout << "Got Filename: " << rasterFileName << std::endl;

  // Create reader to read shape file.
  vtkNew<vtkOpenSlideReader> reader;
  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  delete [] rasterFileName;

  // For debug
  // reader->SetUpdateExtent(extent);
  // vtkNew<vtkPNGWriter> writer;
  // writer->SetInputConnection(reader->GetOutputPort());
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
  imageViewer->SetInputConnection(reader->GetOutputPort());
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
