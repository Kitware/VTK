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

#include "vtkSTLReader.h"
#include "vtkSTLWriter.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

#include "vtkPlaneSource.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"

#include <vtksys/SystemTools.hxx>

int UnitTestSTLWriter(int argc, char* argv[])
{
  int status = 0;

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  // Reader for verifying that header is written correctly
  vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();

  // Test header data
  std::string shortTextHeader = "This is a short text header.";
  std::string longTextHeader = "This is a long text header. It is longer "
                               "than the maximum 80 characters allowed for binary headers, "
                               "but should be no problem in text files.";
  vtkNew<vtkUnsignedCharArray> shortBinaryHeader;
  for (unsigned char c = 100; c < 135; c++)
  {
    shortBinaryHeader->InsertNextValue(c);
  }
  vtkNew<vtkUnsignedCharArray> longBinaryHeader;
  for (unsigned char c = 100; c < 195; c++)
  {
    longBinaryHeader->InsertNextValue(c);
  }

  vtkSmartPointer<vtkSTLWriter> writer1 = vtkSmartPointer<vtkSTLWriter>::New();
  writer1->Print(std::cout);

  writer1->SetFileTypeToASCII();
  std::string fileName;

  fileName = testDirectory + std::string("/") + std::string("ASCII.stl");
  writer1->SetFileName(fileName.c_str());

  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();

  writer1->SetInputConnection(sphere->GetOutputPort());
  writer1->SetHeader(shortTextHeader.c_str());
  writer1->Update();

  // Header: short text / ASCII => unchanged
  reader->SetFileName(fileName.c_str());
  reader->Update();
  if (shortTextHeader != reader->GetHeader())
  {
    cerr << "Unexpected short text header: " << reader->GetHeader();
    ++status;
  }

  writer1->SetFileTypeToBinary();
  fileName = testDirectory + std::string("/") + std::string("Binary.stl");
  writer1->SetHeader(longTextHeader.c_str());
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  // Header: long text / binary => truncated
  reader->SetFileName(fileName.c_str());
  reader->Update();
  std::string readHeader = reader->GetHeader();
  if (readHeader.size() != 80)
  {
    cerr << "Unexpected size of long text header: " << readHeader.size() << std::endl;
    ++status;
  }
  if (longTextHeader.compare(0, 80, readHeader) != 0)
  {
    cerr << "Unexpected content of long text header: " << readHeader << std::endl;
    ++status;
  }

  vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(sphere->GetOutputPort());

  writer1->SetInputConnection(stripper->GetOutputPort());
  fileName = testDirectory + std::string("/") + std::string("BinaryStrips.stl");
  writer1->SetBinaryHeader(shortBinaryHeader);
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  // Header: short binary / binary => unchanged
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkUnsignedCharArray* readBinaryHeader = reader->GetBinaryHeader();
  if (readBinaryHeader->GetNumberOfValues() != 80)
  {
    cerr << "Unexpected size of short binary header: " << readBinaryHeader->GetNumberOfValues()
         << std::endl;
    ++status;
  }
  for (vtkIdType i = 0; i < 80; i++)
  {
    if ((i < shortBinaryHeader->GetNumberOfValues() &&
          readBinaryHeader->GetValue(i) != shortBinaryHeader->GetValue(i)) ||
      (i >= shortBinaryHeader->GetNumberOfValues() && readBinaryHeader->GetValue(i) != 0))
    {
      cerr << "Unexpected content of binary header at position " << i << std::endl;
      ++status;
      break;
    }
  }

  // Make sure the reported number of written triangles is right in the binary file
  FILE* fp = fopen(fileName.c_str(), "rb");
  if (!fp)
  {
    cerr << "Could not open file '" << fileName << "'" << std::endl;
    ++status;
  }
  fseek(fp, 80, SEEK_SET);
  unsigned long int numTriangles = 0;
  size_t bytesRead = fread(&numTriangles, 1, 4, fp);
  if (bytesRead != 4)
  {
    cerr << "Could not read number of triangles." << std::endl;
    ++status;
  }
  if (numTriangles != 96)
  {
    cerr << "Wrong number of triangles saved to STL file from polygon strips" << std::endl;
    ++status;
  }
  fclose(fp);

  writer1->SetFileTypeToASCII();
  fileName = testDirectory + std::string("/") + std::string("ASCIIStrips.stl");
  writer1->SetFileName(fileName.c_str());
  writer1->Update();

  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  writer1->SetFileTypeToASCII();
  fileName = testDirectory + std::string("/") + std::string("ASCIIQuad.stl");
  writer1->SetFileName(fileName.c_str());
  writer1->SetInputConnection(plane->GetOutputPort());
  writer1->Update();
  writer1->SetFileTypeToBinary();
  fileName = testDirectory + std::string("/") + std::string("BinaryQuad.stl");
  writer1->SetBinaryHeader(longBinaryHeader);
  writer1->SetFileName(fileName.c_str());
  writer1->SetInputConnection(plane->GetOutputPort());
  writer1->Update();

  // Header: long binary / binary => truncated
  reader->SetFileName(fileName.c_str());
  reader->Update();
  readBinaryHeader = reader->GetBinaryHeader();
  if (readBinaryHeader->GetNumberOfValues() != 80)
  {
    cerr << "Unexpected size of long short binary header: " << readHeader.size();
    ++status;
  }
  for (vtkIdType i = 0; i < 80; i++)
  {
    if (readBinaryHeader->GetValue(i) != longBinaryHeader->GetValue(i))
    {
      cerr << "Unexpected content of long binary header at position " << i;
      ++status;
      break;
    }
  }

  fp = fopen(fileName.c_str(), "rb");
  if (!fp)
  {
    cerr << "Could not open file '" << fileName << "'" << std::endl;
    ++status;
  }
  fseek(fp, 80, SEEK_SET);
  bytesRead = fread(&numTriangles, 1, 4, fp);
  if (bytesRead != 4)
  {
    cerr << "Could not read number of triangles." << std::endl;
    ++status;
  }
  if (numTriangles != 2)
  {
    cerr << "Wrong number of triangles saved to STL file from polygon strips" << std::endl;
    ++status;
  }
  fclose(fp);

  // Check error conditions
  //
  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkSTLWriter> writer2 = vtkSmartPointer<vtkSTLWriter>::New();
  writer2->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  writer2->SetFileName("foo");
  vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();
  writer2->SetInputData(emptyPolyData);
  writer2->SetFileTypeToASCII();
  writer2->Update();
  int status1 = errorObserver->CheckErrorMessage("No data to write");
  if (status1)
  {
    ++status;
  }

  writer2->SetInputData(emptyPolyData);
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage("No data to write");
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName(nullptr);
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToASCII();
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage("Please specify FileName to write");
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName(nullptr);
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage("Please specify FileName to write");
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName("/");
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToASCII();
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage("Couldn't open file: /");
  if (status1)
  {
    ++status;
  }

  writer2->SetFileName("/");
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage("Couldn't open file: /");
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
    status1 = errorObserver->CheckErrorMessage("Ran out of disk space; deleting file: /dev/full");
    if (status1)
    {
      ++status;
    }

    writer2->SetInputConnection(stripper->GetOutputPort());
    writer2->Update();
    status1 = errorObserver->CheckErrorMessage("Ran out of disk space; deleting file: /dev/full");
    if (status1)
    {
      ++status;
    }

    writer2->SetFileName("/dev/full");
    writer2->SetInputConnection(sphere->GetOutputPort());
    writer2->SetFileTypeToBinary();
    writer2->Update();
    status1 = errorObserver->CheckErrorMessage("Ran out of disk space; deleting file: /dev/full");
    if (status1)
    {
      ++status;
    }
    writer2->SetInputConnection(stripper->GetOutputPort());
    writer2->Update();
    status1 = errorObserver->CheckErrorMessage("Ran out of disk space; deleting file: /dev/full");
    if (status1)
    {
      ++status;
    }
  }

  writer2->SetFileName("foo.stl");
  writer2->SetInputConnection(sphere->GetOutputPort());
  writer2->SetFileTypeToBinary();
  writer2->SetHeader("solid");
  writer2->Update();
  status1 = errorObserver->CheckErrorMessage(
    "Invalid header for Binary STL file. Cannot start with \"solid\"");
  if (status1)
  {
    ++status;
  }

  return status;
}
