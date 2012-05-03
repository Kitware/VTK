/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIcicleView.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataRepresentation.h"
#include "vtkIcicleView.h"
#include "vtkRenderWindow.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
using std::string;

int TestIcicleView(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/smalltest.xml";

  // We need to put the graph and tree edges in different domains.
  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(treeFileName.c_str());

  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInputConnection(reader->GetOutputPort());

  VTK_CREATE(vtkIcicleView, view);
  view->DisplayHoverTextOff();
  view->SetTreeFromInputConnection(numeric->GetOutputPort());

  view->SetAreaColorArrayName("size");
  view->ColorAreasOn();
  view->SetAreaLabelArrayName("label");
  view->AreaLabelVisibilityOn();
  view->SetAreaHoverArrayName("label");
  view->SetAreaSizeArrayName("size");

  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

 return !retVal;
}
