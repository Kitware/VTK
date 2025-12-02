// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example creates a couple of class instances and print them to
// the standard output. No rendering window is created.
//

//
// First include the required header files for the vtk classes we are using
//
#include "vtkBar.h"
#include "vtkBar2.h"
#include "vtkImageFoo.h"

#include <iostream>

int main()
{

  //
  // Next we create an instance of vtkBar
  //
  std::cout << "Create vtkBar object and print it." << endl;

  vtkBar* bar = vtkBar::New();
  bar->Print(std::cout);

  //
  // Then we create an instance of vtkBar2
  //
  std::cout << "Create vtkBar2 object and print it." << endl;

  vtkBar2* bar2 = vtkBar2::New();
  bar2->Print(std::cout);

  //
  // And we create an instance of vtkImageFoo
  //
  std::cout << "Create vtkImageFoo object and print it." << endl;

  vtkImageFoo* imagefoo = vtkImageFoo::New();
  imagefoo->Print(std::cout);

  std::cout << "Looks good ?" << endl;

  //
  // Free up any objects we created
  //
  bar->Delete();
  bar2->Delete();
  imagefoo->Delete();

  return 0;
}
