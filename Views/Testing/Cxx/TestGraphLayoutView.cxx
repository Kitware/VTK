/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphLayoutView.cxx

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

#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkGraphLayoutView.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkXMLTreeReader.h"

#include <vtksys/stl/vector>
using vtksys_stl::vector;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


class TestGraphLayoutViewUpdater : public vtkCommand
{
public:
  static TestGraphLayoutViewUpdater* New()
  { return new TestGraphLayoutViewUpdater; }
  
  void SetView(vtkView* view)
  {
    this->View = view;
    view->AddObserver(vtkCommand::SelectionChangedEvent, this);
  }
  
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    this->View->Update();
  }
private:
  TestGraphLayoutViewUpdater() { }  
  ~TestGraphLayoutViewUpdater() { }
  vtkView* View;
};

int TestGraphLayoutView(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/treetest.xml");

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file);
  reader->SetMaskArrays(true);
  
  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInputConnection(reader->GetOutputPort());
  
  // Graph layout view
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkGraphLayoutView, view);
  view->SetVertexLabelArrayName("name");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetupRenderWindow(win);
  view->AddRepresentationFromInputConnection(numeric->GetOutputPort());
  
  VTK_CREATE(TestGraphLayoutViewUpdater, update);
  update->SetView(view);
    
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
