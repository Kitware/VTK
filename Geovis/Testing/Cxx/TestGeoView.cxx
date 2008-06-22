/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGeoView.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

int TestGeoView(int argc, char* argv[])
{
  // Create the geo view.
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkGeoView, view);
  view->SetupRenderWindow(win);
  win->SetSize(400,400);

  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/NE2_ps_bath_small.jpg");
  view->AddDefaultImageRepresentation(fname);
  
  view->Update();
  
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    // Interact with data.
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }

  delete [] fname;
  return !retVal;
}

