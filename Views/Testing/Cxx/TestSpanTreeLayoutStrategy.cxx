/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSpanTreeLayoutStrategy.cxx

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

#include "vtkSpanTreeLayoutStrategy.h"
#include "vtkGraphLayoutView.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkXGMLReader.h"

using vtkstd::string;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestSpanTreeLayoutStrategy(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot + "/Data/Infovis/fsm.gml";

  VTK_CREATE(vtkXGMLReader, reader);
  reader->SetFileName(file.c_str());
  reader->Update();

  // Graph layout view
  VTK_CREATE(vtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  view->SetLayoutStrategyToSpanTree();
  view->SetVertexLabelArrayName("vertex id");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("vertex id");
  view->SetColorVertices(true);
  view->SetRepresentationFromInputConnection(reader->GetOutputPort());

  view->ResetCamera();
  view->GetRenderWindow()->SetSize( 600, 600 );
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->SetInteractionModeTo3D();
  view->SetLabelPlacementModeToNoOverlap();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

 return !retVal;
}
