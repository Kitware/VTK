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
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestGraphLayoutView(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/treetest.xml");

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(file);
  reader->SetMaskArrays(true);
  reader->Update();
  vtkTree* t = reader->GetOutput();
  VTK_CREATE(vtkIdTypeArray, dist);
  dist->SetName("distance");
  for (vtkIdType i = 0; i < t->GetNumberOfEdges(); i++)
    {
    dist->InsertNextValue(i);
    }
  t->GetEdgeData()->AddArray(dist);
  
  VTK_CREATE(vtkStringToNumeric, numeric);
  numeric->SetInput(t);
  
  // Graph layout view
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkGraphLayoutView, view);
  view->SetLayoutStrategyToCircular();
  view->SetVertexLabelArrayName("name");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetupRenderWindow(win);
  view->AddRepresentationFromInputConnection(numeric->GetOutputPort());
  
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }

  delete [] file;
  return !retVal;
}
