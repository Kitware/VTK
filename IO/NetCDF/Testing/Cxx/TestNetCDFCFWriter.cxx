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

#include "vtkNetCDFCFWriter.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkNetCDFCFReader.h"
#include "vtkNew.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkXMLImageDataReader.h"

int TestNetCDFCFWriter(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  if (!testHelper->IsFlagSpecified("-T"))
  {
    std::cerr << "Error: -T /path/to/temp_directory was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDirectory = testHelper->GetTempDirectory();

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName((dataRoot + "/Data/okanagan.vti").c_str());

  vtkNew<vtkNetCDFCFWriter> writer;
  writer->SetFileName((tempDirectory + "/okanagan.nc").c_str());
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFillValue(-9999);
  writer->SetAttributeType(vtkDataObject::POINT);
  writer->FillBlankedAttributesOn();
  writer->Write();

  vtkNew<vtkNetCDFCFReader> netcdfReader;
  netcdfReader->SetFileName((tempDirectory + "/okanagan.nc").c_str());
  netcdfReader->SphericalCoordinatesOff();
  netcdfReader->SetDimensions("(z, y, x)");
  netcdfReader->Update();
  vtkImageData* data = vtkImageData::SafeDownCast(netcdfReader->GetOutput());
  vtkNew<vtkUniformGrid> newData;
  newData->ShallowCopy(data);

  vtkNew<vtkDataSetSurfaceFilter> geometryFilter;
  // BUG 10/26/21: vtkGeometryFilter produces all values equal with 113
  // vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputData(newData);

  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1);
  lut->SetTableRange(-200, 125);

  // Create a mapper and LUT.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(34, 125);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("National_units");

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera(reader->GetOutput()->GetBounds());
  vtkCamera* camera = ren->GetActiveCamera();
  camera->Azimuth(180);
  camera->Zoom(1.6);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
