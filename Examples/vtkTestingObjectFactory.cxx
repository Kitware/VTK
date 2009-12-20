/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestingObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestingObjectFactory.h"
#include "vtkTesting.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindow.h"
#include "vtkPNGWriter.h"
#include "vtkWindowToImageFilter.h"

vtkCxxRevisionMacro(vtkTestingInteractor, "1.2");
vtkCxxRevisionMacro(vtkTestingObjectFactory, "1.2");

int            vtkTestingInteractor::TestReturnStatus = -1;
vtkstd::string vtkTestingInteractor::TestName;
vtkstd::string vtkTestingInteractor::TempDirectory;
vtkstd::string vtkTestingInteractor::BaselineDirectory;

VTK_CREATE_CREATE_FUNCTION(vtkTestingInteractor);

vtkTestingObjectFactory::vtkTestingObjectFactory()
{
  this->RegisterOverride("vtkRenderWindowInteractor",
                         "vtkTestingInteractor",
                         "Overrides for testing",
                         1,
                         vtkObjectFactoryCreatevtkTestingInteractor);
}

// Start normally starts an event loop. This interator uses vtkTesting
// to grab the render window and compare the results to a baseline image
void vtkTestingInteractor::Start()
{
  vtkSmartPointer<vtkTesting> testing =
    vtkSmartPointer<vtkTesting>::New();
  testing->SetRenderWindow(this->GetRenderWindow());
  testing->AddArgument("-T");
  testing->AddArgument(vtkTestingInteractor::TempDirectory.c_str());
  testing->AddArgument("-B");
  testing->AddArgument(vtkTestingInteractor::BaselineDirectory.c_str());
  testing->AddArgument("-V");
  vtkstd::string valid = vtkTestingInteractor::TestName + vtkstd::string(".png");
  testing->AddArgument(valid.c_str());
//  testing->AddArgument("-FrontBuffer");
  vtkTestingInteractor::TestReturnStatus = testing->RegressionTest(10);
}
