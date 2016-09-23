/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitSTLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdlib>

#include "vtkSmartPointer.h"
#include "vtkSTLWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTestErrorObserver.h"

#include "vtkPlaneSource.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"

#include <vtksys/SystemTools.hxx>

#define CHECK_ERROR_MSG(msg, status)                   \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
  { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  errorObserver->Clear()

int UnitTestSTLWriter(int argc,char *argv[])
{
  int status = 0;

  char *tempDir = vtkTestUtilities::GetArgOrEnvOrDefault(
    "-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete [] tempDir;

  vtkSmartPointer<vtkSTLWriter> writer1 =
    vtkSmartPointer<vtkSTLWriter>::New();
  writer1->Print(std::cout);

  writer1->SetFileTypeToASCII();
  std::string fileName;

  fileName = testDirectory + std::string("/") + std::string("ASCII.stl");
  writer1->SetFileName(fileName.c_str());

  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();

  writer1->SetInputConnection(sphere->GetOutputPort());
  writer1->Update();

  writer1->SetFileTypeToBinary();
  fileName = testDirectory + std::string("/") + std::string("Binary.stl");
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  vtkSmartPointer<vtkStripper> stripper =
    vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(sphere->GetOutputPort());

  writer1->SetInputConnection(stripper->GetOutputPort());
  fileName = testDirectory + std::string("/") + std::string("BinaryStrips.stl");
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  writer1->SetFileTypeToASCII();
  fileName = testDirectory + std::string("/") + std::string("ASCIIStrips.stl");
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  // Check error conditions
  //
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkSTLWriter> writer2 =
    vtkSmartPointer<vtkSTLWriter>::New();
  writer2->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  writer2->SetFileName("foo");
  vtkSmartPointer<vtkPolyData> emptyPolyData =
    vtkSmartPointer<vtkPolyData>::New();
  writer2->SetInputData(emptyPolyData);
  writer2->SetFileTypeToASCII();
  writer2->Update();
  int status1 = 0;
  CHECK_ERROR_MSG("No data to write", status1);
  if (status1)
  {
    ++status;
  }

  writer2->SetInputData(emptyPolyData);
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("No data to write", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName(NULL);
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToASCII();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("Please specify FileName to write", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName(NULL);
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("Please specify FileName to write", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName("/");
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToASCII();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("Couldn't open file: /", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName("/");
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("Couldn't open file: /", status);
  if (status1)
  {
    ++status;
  }


  if (vtksys::SystemTools::FileExists("/dev/full"))
  {
    writer2->SetFileName("/dev/full");
    writer2->SetInputConnection(sphere->GetOutputPort());
    writer2->SetFileTypeToASCII();
    writer2->Update();
    status1 = 0;
    CHECK_ERROR_MSG("Ran out of disk space; deleting file: /dev/full", status);
    if (status1)
    {
      ++status;
    }

    writer2->SetInputConnection(stripper->GetOutputPort());
    writer2->Update();
    status1 = 0;
    CHECK_ERROR_MSG("Ran out of disk space; deleting file: /dev/full", status);
    if (status1)
    {
      ++status;
    }

    writer2->SetFileName("/dev/full");
    writer2->SetInputConnection(sphere->GetOutputPort());
    writer2->SetFileTypeToBinary();
    writer2->Update();
    status1 = 0;
    CHECK_ERROR_MSG("Ran out of disk space; deleting file: /dev/full", status);    if (status1)
    {
      ++status;
    }
    writer2->SetInputConnection(stripper->GetOutputPort());
    writer2->Update();
    status1 = 0;
    CHECK_ERROR_MSG("Ran out of disk space; deleting file: /dev/full", status);    if (status1)
    {
      ++status;
    }
  }
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  writer2->SetFileName("foo.stl");
  writer2->SetInputConnection(plane->GetOutputPort());

  writer2->SetFileTypeToASCII();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("STL file only supports triangles", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("STL file only supports triangles", status);
  if (status1)
  {
    ++status;
  }

  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->SetHeader("solid");
  writer2->Update();
  status1 = 0;
  CHECK_ERROR_MSG("Invalid header for Binary STL file. Cannot start with \"solid\"", status1);
  if (status1)
  {
    ++status;
  }

  return status;
}
