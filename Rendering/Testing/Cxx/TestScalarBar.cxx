/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkPLOT3DReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridGeometryFilter.h"

#include "vtkTestUtilities.h"

int TestScalarBar( int argc, char *argv[] )
{
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  char* fname2 = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");
  
  // Start by loading some data.
  vtkSmartPointer<vtkPLOT3DReader> pl3d =
    vtkSmartPointer<vtkPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname);
  pl3d->SetQFileName(fname2);
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();
  
  delete [] fname;
  delete [] fname2;
  
  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridGeometryFilter> outline =
    vtkSmartPointer<vtkStructuredGridGeometryFilter>::New();
  outline->SetInputConnection(pl3d->GetOutputPort());
  outline->SetExtent(0,100,0,100,9,9);

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  // Create the RenderWindow, Renderer and all Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
 
  vtkSmartPointer<vtkScalarBarActor> scalarBar1 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar1->SetTitle("Temperature");
  scalarBar1->SetLookupTable(outlineMapper->GetLookupTable());
  scalarBar1->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar1->GetPositionCoordinate()->SetValue(0.6, 0.1);
  scalarBar1->SetHeight(0.5);
   
  vtkSmartPointer<vtkScalarBarActor> scalarBar2 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar2->SetTitle("Temperature");
  scalarBar2->SetLookupTable(outlineMapper->GetLookupTable());
  scalarBar2->SetOrientationToHorizontal();
  scalarBar2->SetWidth(0.5);
  scalarBar2->SetHeight(0.2);
  scalarBar2->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar2->GetPositionCoordinate()->SetValue(0.05, 0.05);
  
  vtkSmartPointer<vtkScalarBarActor> scalarBar3 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar3->SetTitle("Temperature");
  scalarBar3->SetLookupTable(outlineMapper->GetLookupTable());
  scalarBar3->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar3->GetPositionCoordinate()->SetValue(0.8, 0.1);
  scalarBar3->SetHeight(0.5);

  ren1->AddActor(outlineActor);
  ren1->AddActor(scalarBar1);
  ren1->AddActor(scalarBar2);
  ren1->AddActor(scalarBar3);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(700, 500);

  // render the image
  iren->Initialize();
  renWin->Render();
  iren->Start();
  
  return EXIT_SUCCESS;
}
