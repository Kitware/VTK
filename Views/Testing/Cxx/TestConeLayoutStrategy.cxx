/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConeLayoutStrategy.cxx

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
#include "vtkCellData.h"
#include "vtkConeLayoutStrategy.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkXMLTreeReader.h"

using std::string;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestConeLayoutStrategy(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
//  string file = dataRoot + "/Data/treetest.xml";
  string file = dataRoot + "/Data/Infovis/XML/vtkclasses.xml";

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file.c_str());
  reader->SetMaskArrays(true);
  reader->Update();
  vtkTree* t = reader->GetOutput();
  VTK_CREATE(vtkStringArray, label);
  label->SetName("edge label");
  VTK_CREATE(vtkIdTypeArray, dist);
  dist->SetName("distance");
  for (vtkIdType i = 0; i < t->GetNumberOfEdges(); i++)
    {
    dist->InsertNextValue(i);
    switch (i % 3)
      {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
      }
    }
  t->GetEdgeData()->AddArray(dist);
  t->GetEdgeData()->AddArray(label);
  
  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInput(t);
  
  // Graph layout view
  VTK_CREATE(vtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  VTK_CREATE(vtkConeLayoutStrategy, strategy);
  strategy->SetSpacing(0.3);
  view->SetLayoutStrategy(strategy);
  view->SetVertexLabelArrayName("id");
  view->VertexLabelVisibilityOn();
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetEdgeLabelArrayName("edge label");
  view->EdgeLabelVisibilityOn();
  view->SetRepresentationFromInputConnection(numeric->GetOutputPort());

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
