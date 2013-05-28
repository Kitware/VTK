/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkNetCDFCAMReader
// .SECTION Description
// Tests the vtkNetCDFCAMReader.

#include "vtkNetCDFCAMReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestNetCDFCAMReader( int argc, char *argv[] )
{
  // Read file names.
  char* pointsFileName = vtkTestUtilities::ExpandDataFileName(
    argc, argv,"Data/NetCDF/CAMReaderPoints.nc");
  char* connectivityFileName = vtkTestUtilities::ExpandDataFileName(
    argc, argv,"Data/NetCDF/CAMReaderConnectivity.nc");

  // Create the reader.
  vtkNew<vtkNetCDFCAMReader> reader;
  reader->SetFileName(pointsFileName);
  reader->SetConnectivityFileName(connectivityFileName);
  delete []pointsFileName;
  pointsFileName = NULL;
  delete []connectivityFileName;
  connectivityFileName = NULL;
  reader->Update();

  // Convert to PolyData.
  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(reader->GetOutputPort());

  // Create a mapper and LUT.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(205, 250);
  mapper->SetScalarModeToUsePointFieldData ();
  mapper->SelectColorArray("T");

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkCamera> camera;
  ren->ResetCamera(reader->GetOutput()->GetBounds());
  camera->Zoom(8);

  ren->AddActor(actor.GetPointer());
  ren->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
