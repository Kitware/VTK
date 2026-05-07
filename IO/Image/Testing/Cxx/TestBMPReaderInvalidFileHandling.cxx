// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkBMPReader
// .SECTION Description
//

#include "vtkBMPReader.h"
#include "vtkExecutive.h"
#include "vtkMemoryResourceStream.h"
#include "vtkTestErrorObserver.h"

#include <iostream>

int TestBMPReaderInvalidFileHandling(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{

  auto testData = [](const std::vector<unsigned char>& vec, const std::string& expectedMsg)
  {
    // Convert vector to stream
    vtkNew<vtkMemoryResourceStream> stream;
    stream->SetBuffer(vec.data(), vec.size());

    vtkNew<vtkTest::ErrorObserver> errorObserver;
    vtkNew<vtkTest::ErrorObserver> errorObserverExec;

    // Initialize and update reader with errorObservers
    vtkNew<vtkBMPReader> reader;
    reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    reader->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserverExec);
    reader->SetStream(stream);
    reader->Update();

    auto result = errorObserver->CheckErrorMessage(expectedMsg);
    if (result != 0)
    {
      std::cout << "Error message : {\n"
                << errorObserver->GetErrorMessage() << "\n} does not match expected\n"
                << std::endl;
    }
    errorObserver->Clear();

    return result;
  };

  // Normal valid header (but no body)
  std::vector<unsigned char> headerValid = {
    // Header (14 bytes)
    0x42, 0x4D,             // Signature "BM"
    0xFF, 0x00, 0x00, 0x00, // File size (255 bytes, in future we can confirm this)
    0x00, 0x00,             // Reserved1
    0x00, 0x00,             // Reserved2
    0x36, 0x00, 0x00, 0x00, // Pixel data offset (54 bytes, header)

    // InfoHeader (40 bytes)
    0x28, 0x00, 0x00, 0x00, // Header size (40 bytes)
    0x01, 0x00, 0x00, 0x00, // Width (1 pixels)
    0x01, 0x00, 0x00, 0x00, // Height (1 pixels)
    0x01, 0x00,             // Planes (1)
    0x18, 0x00,             // Bits per pixel (24 : RGB)
    0x00, 0x00, 0x00, 0x00, // Compression (0 : BI_RGB : no compression)
    0x00, 0x00, 0x00, 0x00, // Image size (0 : ok for BI_RGB)
    0x13, 0x0B, 0x00, 0x00, // X pixels per meter (2835)
    0x13, 0x0B, 0x00, 0x00, // Y pixels per meter (2835)
    0x00, 0x00, 0x00, 0x00, // Total colors (0)
    0x00, 0x00, 0x00, 0x00  // Important colors (0)
  };

  // Invalid header with unsupported number of bits per pixel
  std::vector<unsigned char> headerInvalidBPP(headerValid);
  headerInvalidBPP[28] = 0xFF;

  // File containing no information
  std::vector<unsigned char> voidFile = { 0x00 };

  // Run tests
  if (testData(headerValid, "File operation failed.") != 0)
  {
    std::cerr << "Valid header test failed\n";
    return EXIT_FAILURE;
  }

  if (testData(headerInvalidBPP, "Only BMP depths of (8,24) are supported") != 0 ||
    testData(headerInvalidBPP, "Invalid information when parsing header") != 0)
  {
    std::cerr << "Invalid header BPP test failed\n";
    return EXIT_FAILURE;
  }

  if (testData(voidFile, "Error reading magic numbers") != 0 ||
    testData(voidFile, "Invalid information when parsing header") != 0)
  {
    std::cerr << "Voided file test failed\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
