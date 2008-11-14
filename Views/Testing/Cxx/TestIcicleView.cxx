/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIcicleView.cxx

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

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectionLink.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkIcicleView.h"
#include "vtkXMLTreeReader.h"
#include "vtkViewTheme.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using vtkstd::string;

int TestIcicleView(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot+"/Data/Infovis/XML/vtkclasses.xml";
  
  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file.c_str());

  // Tree icicle view
  VTK_CREATE(vtkRenderWindow, win);
  win->SetMultiSamples(0);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkIcicleView, view);
  view->SetRepresentationFromInput(reader->GetOutput());
  view->SetSizeArrayName("size");
  view->SetColorArrayName("vertex id");
  view->SetLabelArrayName("id");
  view->SetHoverArrayName("id");
  view->Update();
  view->SetupRenderWindow(win);

    // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();
 
  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
