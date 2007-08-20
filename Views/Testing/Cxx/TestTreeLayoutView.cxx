/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeLayoutView.cxx

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
#include "vtkGraphLayoutView.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectionLink.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkTreeLayoutView.h"
#include "vtkTreeMapView.h"
#include "vtkXMLTreeReader.h"

#include <vtksys/stl/vector>
using vtksys_stl::vector;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

const char* xml =
"<a name=\"name a\">"
"  <b name=\"name b\" size=\"1\"/>"
"  <c name=\"name c\" size=\"1\"/>"
"  <d name=\"name d\" size=\"1\"/>"
"  <e name=\"name e\" size=\"1.1\"/>"
"  <f name=\"name f\" size=\"1.234\"/>"
"</a>";

const char* xml2 =
"<node1 name=\"name1\">"
"  <node2 name=\"name2\" size=\"1\">"
"    <node3 name=\"name3\" size=\"1\">"
"       <node4 name=\"name4\" size=\"1\"/>"
"    </node3>"
"  </node2>"
"</node1>";

class TestTreeLayoutViewUpdater : public vtkCommand
{
public:
  static TestTreeLayoutViewUpdater* New()
  { return new TestTreeLayoutViewUpdater; }
  
  void AddView(vtkView* view)
  {
    this->Views.push_back(view);
    view->AddObserver(vtkCommand::SelectionChangedEvent, this);
  }
  
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    for (unsigned int i = 0; i < this->Views.size(); i++)
      {
      this->Views[i]->Update();
      }
  }
private:
  TestTreeLayoutViewUpdater() { }  
  ~TestTreeLayoutViewUpdater() { }
  vector<vtkView*> Views;
};

int TestTreeLayoutView(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/treetest.xml");

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file);
  reader->SetMaskArrays(true);
  
  VTK_CREATE(vtkXMLTreeReader, reader2);
  reader2->SetXMLString(xml);
  reader2->SetMaskArrays(true);
  
  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInputConnection(reader->GetOutputPort());
  
  VTK_CREATE(vtkSelectionLink, link);
  
  VTK_CREATE(TestTreeLayoutViewUpdater, updater);
  
  // Tree layout view
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkTreeLayoutView, view);
  view->SetLabelArrayName("name");
  view->LabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetLeafSpacing(0.9);
  view->SetRadial(true);
  view->SetAngle(360);
  view->SetLogSpacingValue(1.0);
  view->SetupRenderWindow(win);
  view->AddRepresentationFromInputConnection(numeric->GetOutputPort());
  view->GetRepresentation()->SetSelectionLink(link);
  view->Update();
  updater->AddView(view);
  
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    // Make more views to play with if it is interactive :)
    
    // Tree map view
    VTK_CREATE(vtkRenderWindow, win2);
    VTK_CREATE(vtkRenderWindowInteractor, iren2);
    iren2->SetRenderWindow(win2);
    VTK_CREATE(vtkTreeMapView, view2);
    view2->SetSizeArrayName("size");
    view2->SetColorArrayName("level");
    view2->SetLabelArrayName("name");
    view2->SetHoverArrayName("name");
    view2->SetupRenderWindow(win2);
    view2->AddRepresentationFromInputConnection(reader->GetOutputPort());
    view2->GetRepresentation()->SetSelectionLink(link);
    view2->Update();
    updater->AddView(view2);

    // Graph layout view
    VTK_CREATE(vtkRenderWindow, win4);
    VTK_CREATE(vtkRenderWindowInteractor, iren4);
    iren4->SetRenderWindow(win4);
    VTK_CREATE(vtkGraphLayoutView, view4);
    view4->SetVertexLabelArrayName("name");
    view4->VertexLabelVisibilityOn();
    view4->SetupRenderWindow(win4);
    view4->AddRepresentationFromInputConnection(reader->GetOutputPort());
    view4->GetRepresentation()->SetSelectionLink(link);
    view4->Update();
    updater->AddView(view4);
    
#if 0
    // Test changing the input connection.
    view->GetRepresentation()->SetInputConnection(reader2->GetOutputPort());
    view2->GetRepresentation()->SetInputConnection(reader2->GetOutputPort());
    view4->GetRepresentation()->SetInputConnection(reader2->GetOutputPort());

    view->Update();
    view2->Update();
    view4->Update();
    
    // Test changing the pipeline.
    reader2->SetXMLString(xml2);
    
    view->Update();
    view2->Update();
    view4->Update();
#endif

    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
