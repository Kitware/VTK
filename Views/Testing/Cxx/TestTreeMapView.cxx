/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeMapView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectionLink.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkTreeMapView.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTreeMapView(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/treetest.xml");

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file);
  reader->SetMaskArrays(true);

  delete [] file;

  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInputConnection(reader->GetOutputPort());
    
  // Tree map view
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkTreeMapView, view);
  view->SetSizeArrayName("size");
  view->SetColorArrayName("level");
  view->SetLabelArrayName("name");
  view->SetHoverArrayName("name");
  view->SetLayoutStrategyToSquarify();
  view->SetupRenderWindow(win);
  view->AddRepresentationFromInputConnection(numeric->GetOutputPort());
  
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
