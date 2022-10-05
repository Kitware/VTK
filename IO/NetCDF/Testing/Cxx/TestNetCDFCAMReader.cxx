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
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include <set>

int TestNetCDFCAMReader(int argc, char* argv[])
{
  // Read file names.
  char* pointsFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/CAMReaderPoints.nc");
  char* connectivityFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/CAMReaderConnectivity.nc");

  // Create the reader.
  vtkNew<vtkNetCDFCAMReader> reader;
  reader->SetFileName(pointsFileName);
  reader->SetConnectivityFileName(connectivityFileName);
  delete[] pointsFileName;
  pointsFileName = nullptr;
  delete[] connectivityFileName;
  connectivityFileName = nullptr;
  reader->Update();

  // Check that the lev variable is loaded correctly
  auto output = reader->GetOutput()->GetPointData();
  auto lev = vtkFloatArray::SafeDownCast(output->GetAbstractArray("lev"));
  const vtkIdType numTuples = lev->GetNumberOfTuples();

  std::set<float> expectedLevels = { 3.54463800000002, 7.38881300000002, 13.9672100000001, 23.94463,
    37.2302900000001, 53.1146000000002, 70.0591400000001, 85.4391200000001, 100.514690000001,
    118.25033, 139.11538, 163.66205, 192.539940000001, 226.51321, 266.48106, 313.501270000001,
    368.81799, 433.895230000001, 510.455250000002, 600.524100000001, 696.796239999999,
    787.702010000002, 867.160710000001, 929.648975, 970.554785000003, 992.556100000005 };

  // Valid that the level values valid
  for (auto tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
  {
    auto level = *lev->GetTuple(tupleIdx);
    if (expectedLevels.count(level) != 1)
    {
      std::cerr << "Invalid level value:" << level << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Convert to PolyData.
  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(reader->GetOutputPort());

  // Create a mapper and LUT.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(205, 250);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("T");

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkCamera> camera;
  ren->ResetCamera(reader->GetOutput()->GetBounds());
  camera->Zoom(8);

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
