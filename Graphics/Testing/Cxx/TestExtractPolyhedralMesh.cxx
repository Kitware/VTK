/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractPolyhedralMesh.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGridReader.h"
#include "vtkExtractEdges.h"
#include "vtkProperty.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

// Test of vtkPolyhedron. A structured grid is converted to a polyhedral
// mesh.
int TestExtractPolyhedralMesh( int argc, char* argv[] )
{
  //char* fname = vtkTestUtilities::ExpandDataFileName( 
  //	  argc, argv, "c:/d/VTK/VTKData/Data/SampleStructGrid.vtk" );
  char fname[] = "c:/d/VTK/VTKData/Data/SampleStructGrid.vtk";
  vtkSmartPointer<vtkStructuredGridReader> reader =
    vtkSmartPointer<vtkStructuredGridReader>::New();
  reader->SetFileName(fname);
  reader->Update();

  vtkSmartPointer<vtkExtractPolyhedralMesh> extract =
    vtkSmartPointer<vtkExtractPolyhedralMesh>::New();
  extract->SetInputConnection(reader->GetOutputPort());
  extract->Update();

  vtkSmartPointer<vtkDataSetMapper> mapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(extract->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = 
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Okay let's extract some edges
  vtkSmartPointer<vtkExtractEdges> edges =
    vtkSmartPointer<vtkExtractEdges>::New();
  edges->SetInputConnection(extract->GetOutputPort());
  
  vtkSmartPointer<vtkDataSetMapper> eMapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  eMapper->SetInputConnection(edges->GetOutputPort());

  vtkSmartPointer<vtkActor> eActor = 
    vtkSmartPointer<vtkActor>::New();
  eActor->SetMapper(eMapper);
  eActor->GetProperty()->SetColor(0,0,0);

  // Create rendering infrastructure
  vtkSmartPointer<vtkRenderer> ren = 
    vtkSmartPointer<vtkRenderer>::New();
  ren->AddActor(actor);
  ren->AddActor(eActor);

  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren);

  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
