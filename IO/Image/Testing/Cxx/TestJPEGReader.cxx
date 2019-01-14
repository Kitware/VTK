/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestJPEGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkJPEGReader
// .SECTION Description
//


#include "vtkSmartPointer.h"

#include "vtkJPEGReader.h"

#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"


int TestJPEGReader(int argc, char *argv[])
{

  if ( argc <= 1 )
  {
    cout << "Usage: " << argv[0] << " <jpeg file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkSmartPointer<vtkJPEGReader> JPEGReader =
    vtkSmartPointer<vtkJPEGReader>::New();

  // Check the image can be read
  if (!JPEGReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  JPEGReader->SetFileName(filename.c_str());
  JPEGReader->Update();

  // Read and display the image properties
  const char* fileExtensions = JPEGReader->GetFileExtensions();
  cout << "File xtensions: " << fileExtensions << endl;

  const char* descriptiveName = JPEGReader->GetDescriptiveName();
  cout << "Descriptive name: " << descriptiveName << endl;


  // Visualize
  vtkSmartPointer<vtkImageViewer> imageViewer =
    vtkSmartPointer<vtkImageViewer>::New();
  imageViewer->SetInputConnection(JPEGReader->GetOutputPort());
  imageViewer->SetColorWindow(256);
  imageViewer->SetColorLevel(127.5);
  imageViewer->Render();

  return EXIT_SUCCESS;
}
