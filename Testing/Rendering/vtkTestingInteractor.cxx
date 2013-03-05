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
#include "vtkTestingInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindow.h"
#include "vtkTesting.h"

vtkStandardNewMacro(vtkTestingInteractor);

int          vtkTestingInteractor::TestReturnStatus = -1;
double       vtkTestingInteractor::ErrorThreshold = 10.0;
std::string  vtkTestingInteractor::ValidBaseline;
std::string  vtkTestingInteractor::TempDirectory;
std::string  vtkTestingInteractor::DataDirectory;

//----------------------------------------------------------------------------------
// Start normally starts an event loop. This interator uses vtkTesting
// to grab the render window and compare the results to a baseline image
void vtkTestingInteractor::Start()
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->SetRenderWindow(this->GetRenderWindow());

  // Location of the temp directory for testing
  testing->AddArgument("-T");
  testing->AddArgument(vtkTestingInteractor::TempDirectory.c_str());

  // Location of the Data directory. If NOTFOUND, suppress regression
  // testing
  if (vtkTestingInteractor::DataDirectory != "VTK_DATA_ROOT-NOTFOUND")
    {
    testing->AddArgument("-D");
    testing->AddArgument(vtkTestingInteractor::DataDirectory.c_str());

    // The name of the valid baseline image
    testing->AddArgument("-V");
    std::string valid = vtkTestingInteractor::ValidBaseline;
    testing->AddArgument(valid.c_str());

    // Regression test the image
    vtkTestingInteractor::TestReturnStatus =
      testing->RegressionTest(vtkTestingInteractor::ErrorThreshold);
    }

}

//----------------------------------------------------------------------------------
void vtkTestingInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

}
