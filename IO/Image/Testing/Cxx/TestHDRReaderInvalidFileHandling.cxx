// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkBMPReader
// .SECTION Description
//

#include "vtkExecutive.h"
#include "vtkHDRReader.h"
#include "vtkMemoryResourceStream.h"
#include "vtkTestErrorObserver.h"

#include <iostream>

int TestHDRReaderInvalidFileHandling(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  auto testData = [](const std::vector<unsigned char>& vec, const std::string& expectedMsg)
  {
    // Convert vector to stream
    vtkNew<vtkMemoryResourceStream> stream;
    stream->SetBuffer(vec.data(), vec.size());

    vtkNew<vtkTest::ErrorObserver> errorObserver;
    vtkNew<vtkTest::ErrorObserver> errorObserverExec;

    // Initialize and update reader with errorObservers
    vtkNew<vtkHDRReader> reader;
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
    '#', '?', 'R', 'A', 'D', 'I', 'A', 'N', 'C', 'E', '\n', //
    'F', 'O', 'R', 'M', 'A', 'T', '=', '3', '2', '-', 'b', 'i', 't', '_', 'r', 'l', 'e', '_', 'r',
    'g', 'b', 'e', '\n', //
    '\n',                //
    '-', 'Y', ' ', '1', '2', '8', ' ', '+', 'X', ' ', '2', '5', '6',
    '\n' //
  };

  // Invalid header with no dimentions
  std::vector<unsigned char> headerInvalidDims(headerValid);
  headerInvalidDims.resize(34);
  std::string invalid = "some invalid text";
  headerInvalidDims.insert(headerInvalidDims.end(), invalid.begin(), invalid.end());

  // File containing no information
  std::vector<unsigned char> voidFile = { 0x00 };

  // Run tests
  if (testData(headerValid, "Unable to read as expected") != 0 ||
    testData(headerValid, "HDRReader : Bad line data") != 0)
  {
    std::cerr << "Valid header test failed\n";
    return EXIT_FAILURE;
  }

  if (testData(headerInvalidDims, "Error reading dimensions") != 0 ||
    testData(headerInvalidDims, "Invalid information when parsing header") != 0)
  {
    std::cerr << "Invalid header dimensions test failed\n";
    return EXIT_FAILURE;
  }

  if (testData(voidFile, "Error reading program type") != 0 ||
    testData(voidFile, "Invalid information when parsing header") != 0)
  {
    std::cerr << "Voided file test failed\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
