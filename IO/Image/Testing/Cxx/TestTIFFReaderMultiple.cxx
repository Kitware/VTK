/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTIFFReaderMultiple.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkTIFFReader.h>
#include <vtkSmartPointer.h>
#include "vtkTestErrorObserver.h"

#ifndef _WIN32
#include <sys/resource.h>
#endif

int TestTIFFReaderMultiple(int argc, char *argv[])
{
  if ( argc <= 1 )
    {
    std::cout << "Usage: " << argv[0] << " <meta image file>" << endl;
    return EXIT_FAILURE;
    }

#ifndef _WIN32
  // Set the limit on the number of open files
  struct rlimit rl;
  rl.rlim_cur = 9;
  rl.rlim_max = 10;
  setrlimit(RLIMIT_NOFILE, &rl);
#endif

  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  // Read the same file multiple times to check for memory leaks and/or
  // file descriptor leaks
  for (int i = 0; i < 9; i++)
    {
    vtkSmartPointer<vtkTIFFReader> TIFFReader =
      vtkSmartPointer<vtkTIFFReader>::New();
    TIFFReader->AddObserver(vtkCommand::ErrorEvent,errorObserver);
    TIFFReader->SetFileName(argv[1]);
    TIFFReader->Update();
    std::cout << i << std::endl;
    if (errorObserver->GetError())
      {
      std::cout << "ERROR: " << errorObserver->GetErrorMessage() << std::endl;
      return EXIT_FAILURE;
      }
    errorObserver->Clear();
    }
  return EXIT_SUCCESS;
}
