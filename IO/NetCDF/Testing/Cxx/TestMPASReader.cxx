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
// .NAME Test of vtkMPASReader
// .SECTION Description
// Tests the vtkMPASReader.

#include "vtkMPASReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkExecutive.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"




#include "vtkPointData.h"
#include "vtkDataArray.h"

int TestMPASReader( int argc, char *argv[] )
{
  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Read file names.
  char* fName = vtkTestUtilities::ExpandDataFileName(
    argc, argv,"Data/NetCDF/MPASReader.nc");
  std::string fileName(fName);
  delete []fName;
  fName = NULL;

  // make 2 loops for 2 actors since the reader can read in the file
  // as an sphere or as a plane
  for(int i=0;i<2;i++)
  {
    // Create the reader.
    vtkNew<vtkMPASReader> reader;
    reader->SetFileName(fileName.c_str());

    // Convert to PolyData.
    vtkNew<vtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputConnection(reader->GetOutputPort());

    geometryFilter->UpdateInformation();
    vtkExecutive* executive = geometryFilter->GetExecutive();
    vtkInformationVector* inputVector =
      executive->GetInputInformation(0);
    double timeReq = 0;
    inputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    reader->Update();
    reader->EnableAllCellArrays();
    reader->EnableAllPointArrays();
    reader->SetProjectLatLon( (i != 0) );
    reader->SetVerticalLevel(i);
    reader->Update();

    int* values = reader->GetVerticalLevelRange();
    if(values[0] != 0 || values[1] != 3)
    {
      vtkGenericWarningMacro("Vertical level range is incorrect.");
      return 1;
    }
    values = reader->GetLayerThicknessRange();
    if(values[0] != 0 || values[1] != 200000)
    {
      vtkGenericWarningMacro("Layer thickness range is incorrect.");
      return 1;
    }
    values = reader->GetCenterLonRange();
    if(values[0] != 0 || values[1] != 360)
    {
      vtkGenericWarningMacro("Center lon range is incorrect.");
      return 1;
    }

    // Create a mapper and LUT.
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geometryFilter->GetOutputPort());
    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarRange(0.0116, 199.9);
    mapper->SetScalarModeToUsePointFieldData ();
    mapper->SelectColorArray("ke");

    // Create the actor.
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());
    if(i == 1)
    {
      actor->SetScale(30000);
      actor->AddPosition(4370000, 0, 0);
    }
    ren->AddActor(actor.GetPointer());
  }

  vtkNew<vtkCamera> camera;
  ren->ResetCamera(-4370000, 12370000, -6370000, 6370000, -6370000, 6370000);
  camera->Zoom(8);

  ren->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  cerr << !retVal << " is the return val\n";
  return !retVal;
}
