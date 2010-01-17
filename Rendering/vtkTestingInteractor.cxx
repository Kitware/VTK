/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestingInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"
#include "vtkTestingInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkTestingInteractor, "1.4");
vtkStandardNewMacro(vtkTestingInteractor);

int            vtkTestingInteractor::TestReturnStatus = -1;
vtkstd::string vtkTestingInteractor::TestName;
vtkstd::string vtkTestingInteractor::TempDirectory;
vtkstd::string vtkTestingInteractor::BaselineDirectory;
vtkstd::string vtkTestingInteractor::DataDirectory;

// Start normally starts an event loop. This interator uses vtkTesting
// to grab the render window and compare the results to a baseline image
void vtkTestingInteractor::Start()
{
  vtkSmartPointer<vtkTesting> testing =
    vtkSmartPointer<vtkTesting>::New();
  testing->SetRenderWindow(this->GetRenderWindow());

  // Location of the temp directory for testing
  testing->AddArgument("-T");
  testing->AddArgument(vtkTestingInteractor::TempDirectory.c_str());

  // Location of the Data directory
  testing->AddArgument("-D");
  testing->AddArgument(vtkTestingInteractor::DataDirectory.c_str());

  // Location of the Baseline directory
  testing->AddArgument("-B");
  testing->AddArgument(vtkTestingInteractor::BaselineDirectory.c_str());

  // The name of the valid baseline image
  testing->AddArgument("-V");
  vtkstd::string valid = vtkTestingInteractor::TestName + vtkstd::string(".png");
  testing->AddArgument(valid.c_str());

  // Regression test the image
  vtkTestingInteractor::TestReturnStatus = testing->RegressionTest(40);
}
