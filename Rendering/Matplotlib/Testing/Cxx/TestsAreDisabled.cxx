/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestsAreDisabled

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkSetGet.h>

//----------------------------------------------------------------------------
int TestsAreDisabled(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  std::cout << "Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests." << std::endl;
  return EXIT_SUCCESS;
}
