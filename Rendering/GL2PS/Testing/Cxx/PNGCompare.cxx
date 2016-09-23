/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PNGCompare.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkTesting.h"
#include "vtkTestingInteractor.h"

#include <string>
#include <cstdio> // For remove

int PNGCompare(int argc, char *argv[])
{
  // Grab the test image
  std::string testImageFileName;
  for (int i = 0; i < argc; ++i)
  {
    if ( strcmp(argv[i],"--test-file") == 0 && i < argc-1)
    {
      testImageFileName = std::string(argv[i+1]);
      break;
    }
  }

  if (testImageFileName.empty())
  {
    cout << "Error: No reference image specified (use --test-file <png file>)"
         << endl;
    return EXIT_FAILURE;
  }

  // Set up testing object (modeled after vtkTestingInteractor::Start())
  vtkNew<vtkTesting> testing;

  // Location of the temp directory for testing
  testing->AddArgument("-T");
  testing->AddArgument(vtkTestingInteractor::TempDirectory.c_str());

  // Location of the Data directory
  testing->AddArgument("-D");
  testing->AddArgument(vtkTestingInteractor::DataDirectory.c_str());

  // The name of the valid baseline image
  testing->AddArgument("-V");
  testing->AddArgument(vtkTestingInteractor::ValidBaseline.c_str());

  // Regression test the image
  int result = testing->RegressionTest(testImageFileName,
                                       vtkTestingInteractor::ErrorThreshold);

  vtkTestingInteractor::TestReturnStatus = result;

  if (result == vtkTesting::PASSED)
  {
    remove(testImageFileName.c_str());
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
