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
#include <fcntl.h>
#endif

int TestTIFFReaderMultiple(int argc, char *argv[])
{
  if ( argc <= 1 )
  {
    std::cout << "Usage: " << argv[0] << " <meta image file>" << endl;
    return EXIT_FAILURE;
  }

#ifndef _WIN32
  // See how many file descriptors are in use
  int fdUsedBefore = 1;
  for (int i = 0; i < 1024; ++i)
  {
    if (fcntl(i, F_GETFD) == -1)
    {
      break;
    }
      ++fdUsedBefore;
  }
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

#ifndef _WIN32
  // See how many file descriptors are in use
  int fdUsedAfter = 1;
  for (int i = 0; i < 1024; ++i)
  {
    if (fcntl(i, F_GETFD) == -1)
    {
      break;
    }
      ++fdUsedAfter;
  }
  if (fdUsedBefore != fdUsedAfter)
  {
    std::cout << "ERROR: the number of file descriptors used after the I/O ("
              << fdUsedAfter
              << ") does not equal the number used before the I/O ("
              << fdUsedBefore
              << ")" << std::endl;
    return EXIT_FAILURE;
  }
#endif
  return EXIT_SUCCESS;
}
