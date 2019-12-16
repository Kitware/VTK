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
// .NAME Test of WriteToMemory flag for PNG/JPEG/BMP Writers
// .SECTION Description
//
#include "vtkTestUtilities.h"

#include <vtkBMPReader.h>
#include <vtkBMPWriter.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkImageCast.h>
#include <vtkImageViewer.h>
#include <vtkJPEGReader.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGReader.h>
#include <vtkPNGWriter.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <vtksys/SystemTools.hxx>

int TestWriteToUnicodeFile(int argc, char* argv[])
{
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <output file name>" << endl;
    return EXIT_FAILURE;
  }

  const char* tdir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir = tdir;
  delete[] tdir;
  if (tempDir.empty())
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }

  tempDir = tempDir + "/" +
    "\xc3\xba\xce\xae\xd1\x97\xc3\xa7\xe1\xbb\x99\xe2\x82\xab\xe2\x84\xae"; // u8"úήїçộ₫℮"
  if (!vtksys::SystemTools::FileExists(tempDir))
  {
    vtksys::SystemTools::MakeDirectory(tempDir);
  }

  std::string filename = tempDir + "/" + "\xef\xbd\xb7\xef\xbe\x80"; // u8"ｷﾀ"
  filename.append(argv[1]);
  size_t dotpos = filename.find_last_of('.');
  if (dotpos == std::string::npos)
  {
    std::cout << "Could not determine file extension.\n";
    return EXIT_FAILURE;
  }
  std::string fileext = filename.substr(dotpos + 1);
  filename.insert(dotpos, "\xea\x92\x84"); // u8"ꒄ"

  int extent[6] = { 0, 99, 0, 99, 0, 0 };
  vtkSmartPointer<vtkImageCanvasSource2D> imageSource =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  imageSource->SetExtent(extent);
  imageSource->SetScalarTypeToUnsignedChar();
  imageSource->SetNumberOfScalarComponents(3);
  imageSource->SetDrawColor(127, 45, 255);
  imageSource->FillBox(0, 99, 0, 99);
  imageSource->SetDrawColor(255, 255, 255);
  imageSource->FillBox(40, 70, 20, 50);
  imageSource->Update();

  vtkSmartPointer<vtkImageCast> filter = vtkSmartPointer<vtkImageCast>::New();
  filter->SetOutputScalarTypeToUnsignedChar();
  filter->SetInputConnection(imageSource->GetOutputPort());
  filter->Update();

  vtkSmartPointer<vtkImageWriter> writer;
  vtkSmartPointer<vtkImageReader2> reader;

  // Delete any existing files to prevent false failures
  if (!vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  if (fileext == "png")
  {
    writer = vtkSmartPointer<vtkPNGWriter>::New();
    reader = vtkSmartPointer<vtkPNGReader>::New();
  }
  else if (fileext == "jpeg" || fileext == "jpg")
  {
    writer = vtkSmartPointer<vtkJPEGWriter>::New();
    reader = vtkSmartPointer<vtkJPEGReader>::New();
  }
  else if (fileext == "bmp")
  {
    writer = vtkSmartPointer<vtkBMPWriter>::New();
    reader = vtkSmartPointer<vtkBMPReader>::New();
  }

  writer->SetInputConnection(filter->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Update();
  writer->Write();

  if (vtksys::SystemTools::FileExists(filename))
  {
    if (!reader->CanReadFile(filename.c_str()))
    {
      cerr << "CanReadFile failed for " << filename.c_str() << "\n";
      return EXIT_FAILURE;
    }

    // Read the input image
    reader->SetFileName(filename.c_str());
    reader->Update();

    const char* fileExtensions = reader->GetFileExtensions();
    cout << "File extensions: " << fileExtensions << endl;

    const char* descriptiveName = reader->GetDescriptiveName();
    cout << "Descriptive name: " << descriptiveName << endl;

    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
