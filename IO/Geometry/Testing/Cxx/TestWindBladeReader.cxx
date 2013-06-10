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
// .NAME Test of vtkWindBladeReader
// .SECTION Description
// Tests the vtkWindBladeReader.

#include "vtkWindBladeReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTestUtilities.h"

void AddColor(vtkDataSet* grid)
{
  vtkFloatArray* color = vtkFloatArray::New();
  color->SetNumberOfTuples(grid->GetNumberOfPoints());
  for(vtkIdType i=0;i<grid->GetNumberOfPoints();i++)
    {
    color->SetValue(i, 1.);
    }
  color->SetName("Density");
  grid->GetPointData()->AddArray(color);
  grid->GetPointData()->SetScalars(color);
  color->Delete();
}

int TestWindBladeReader( int argc, char *argv[] )
{
  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/WindBladeReader/test1_topo.wind");

  // Create the reader.
  vtkSmartPointer<vtkWindBladeReader> reader = vtkSmartPointer<vtkWindBladeReader>::New();
  reader->SetFilename(fname);
  delete [] fname;

  // Convert to PolyData.
  vtkGeometryFilter* fieldGeometryFilter = vtkGeometryFilter::New();
  fieldGeometryFilter->SetInputConnection(reader->GetOutputPort());
  vtkGeometryFilter* bladeGeometryFilter = vtkGeometryFilter::New();
  bladeGeometryFilter->SetInputConnection(reader->GetOutputPort(1));
  vtkGeometryFilter* groundGeometryFilter = vtkGeometryFilter::New();
  groundGeometryFilter->SetInputConnection(reader->GetOutputPort(2));

  fieldGeometryFilter->UpdateInformation();
  vtkExecutive* executive = fieldGeometryFilter->GetExecutive();
  vtkInformationVector* inputVector =
    executive->GetInputInformation(0);
  double timeReq = 10;
  inputVector->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);

  bladeGeometryFilter->UpdateInformation();
  executive = bladeGeometryFilter->GetExecutive();
  inputVector = executive->GetInputInformation(0);
  inputVector->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);

  reader->Update();
  bladeGeometryFilter->Update();
  groundGeometryFilter->Update();
  AddColor(bladeGeometryFilter->GetOutput());
  AddColor(groundGeometryFilter->GetOutput());

  // Create a mapper.
  vtkPolyDataMapper* fieldMapper = vtkPolyDataMapper::New();
  fieldMapper->SetInputConnection(
        fieldGeometryFilter->GetOutputPort());
  fieldMapper->ScalarVisibilityOn();
  fieldMapper->SetColorModeToMapScalars();
  fieldMapper->SetScalarRange(.964, 1.0065);
  fieldMapper->SetScalarModeToUsePointFieldData ();
  fieldMapper->SelectColorArray("Density");

  vtkPolyDataMapper* bladeMapper = vtkPolyDataMapper::New();
  bladeMapper->SetInputConnection(
        bladeGeometryFilter->GetOutputPort());
  bladeMapper->ScalarVisibilityOn();
  vtkPolyDataMapper* groundMapper = vtkPolyDataMapper::New();
  groundMapper->SetInputConnection(
         groundGeometryFilter->GetOutputPort());
  groundMapper->ScalarVisibilityOn();

  // Create the actor.
  vtkActor* fieldActor = vtkActor::New();
  fieldActor->SetMapper(fieldMapper);
  vtkActor* bladeActor = vtkActor::New();
  bladeActor->SetMapper(bladeMapper);
  double position[3];
  bladeActor->GetPosition(position);
  bladeActor->RotateZ(90);
  bladeActor->SetPosition(position[0]+100, position[1]+100, position[2]-150);
  vtkActor* groundActor = vtkActor::New();
  groundActor->SetMapper(groundMapper);

  // Basic visualisation.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkCamera* camera = ren->GetActiveCamera();
  double bounds[6];
  reader->GetFieldOutput()->GetBounds(bounds);
  bounds[2] -= 150;
  ren->ResetCamera(bounds);
  camera->Elevation(-90);
  camera->Zoom(1.2);

  ren->AddActor(fieldActor);
  ren->AddActor(bladeActor);
  ren->AddActor(groundActor);
  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  fieldActor->Delete();
  bladeActor->Delete();
  groundActor->Delete();
  fieldMapper->Delete();
  bladeMapper->Delete();
  groundMapper->Delete();
  fieldGeometryFilter->Delete();
  bladeGeometryFilter->Delete();
  groundGeometryFilter->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
