/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMetaIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of WriteToMemory flag for PNG/JPEG Writers
// .SECTION Description
//

#include <vtkSmartPointer.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkImageCast.h>

#include <vtksys/SystemTools.hxx>

int TestWriteToMemory(int argc, char *argv[])
{
  if ( argc <= 1 )
  {
    cout << "Usage: " << argv[0] << " <output file name>" << endl;
    return EXIT_FAILURE;
  }

  int extent[6] = {0, 99, 0, 99, 0, 0};
  vtkSmartPointer<vtkImageCanvasSource2D> imageSource =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  imageSource->SetExtent(extent);
  imageSource->SetScalarTypeToUnsignedChar();
  imageSource->SetNumberOfScalarComponents(3);
  imageSource->SetDrawColor(127, 45, 255);
  imageSource->FillBox(0, 99, 0, 99);
  imageSource->SetDrawColor(255,255,255);
  imageSource->FillBox(40, 70, 20, 50);
  imageSource->Update();

  vtkSmartPointer<vtkImageCast> castFilter =
    vtkSmartPointer<vtkImageCast>::New();
  castFilter->SetOutputScalarTypeToUnsignedChar ();
  castFilter->SetInputConnection(imageSource->GetOutputPort());
  castFilter->Update();

  vtkSmartPointer<vtkImageWriter> writer;

  std::string filename = argv[1];
  std::string fileext = filename.substr(filename.find_last_of(".") + 1);

  // Delete any existing files to prevent false failures
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  if (fileext == "png")
  {
    vtkSmartPointer<vtkPNGWriter> pngWriter =
      vtkSmartPointer<vtkPNGWriter>::New();
    pngWriter->WriteToMemoryOn();
    writer = pngWriter;
  }
  else if (fileext == "jpeg" || fileext == "jpg")
  {
    vtkSmartPointer<vtkJPEGWriter> jpgWriter =
      vtkSmartPointer<vtkJPEGWriter>::New();
    jpgWriter->WriteToMemoryOn();
    writer = jpgWriter;
  }

  writer->SetFileName(filename.c_str());
  writer->SetInputConnection(castFilter->GetOutputPort());
  writer->Update();
  writer->Write();

  // With WriteToMemory true no file should be written
  if (!vtksys::SystemTools::FileExists(filename))
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
