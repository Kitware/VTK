/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMathTextRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextUtilities.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestRenderString(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  const char * str = "$\\hat{H}\\psi = \\left(-\\frac{\\hbar}{2m}\\nabla^2"
      " + V(r)\\right) \\psi = \\psi\\cdot E $";

  vtkNew<vtkImageData> image;
  vtkNew<vtkMathTextUtilities> utils;
  utils->SetScaleToPowerOfTwo(false);
  vtkNew<vtkTextProperty> tprop;
  tprop->SetColor(1, 1, 1);
  tprop->SetFontSize(50);

  vtkNew<vtkImageViewer2> viewer;
  utils->RenderString(str, image.GetPointer(), tprop.GetPointer(),
                      viewer->GetRenderWindow()->GetDPI());

  viewer->SetInputData(image.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  viewer->SetupInteractor(iren.GetPointer());

  viewer->Render();
  viewer->GetRenderer()->ResetCamera();
  viewer->GetRenderer()->GetActiveCamera()->Zoom(6.0);
  viewer->Render();

  viewer->GetRenderWindow()->SetMultiSamples(0);
  viewer->GetRenderWindow()->GetInteractor()->Initialize();
  viewer->GetRenderWindow()->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
