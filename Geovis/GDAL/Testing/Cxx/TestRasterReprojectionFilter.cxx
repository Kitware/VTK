/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRasterReprojectionFiltercxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test for the vtkRasterReprojectionFilter using GDAL

#include "vtkCellDataToPointData.h"
#include "vtkGDALRasterReader.h"
#include "vtkImageActor.h"
#include "vtkImageMapToColors.h"
#include "vtkImageMapper3D.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkRasterReprojectionFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

int TestRasterReprojectionFilter(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/sa052483.tif");

  // Load input file
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // test that we read the NoData value correctly
  reader->Update();
  double nodata = reader->GetInvalidValue(0);
  double expectedNodata = -32768;
  if (nodata != expectedNodata)
  {
    std::cerr << "Error NoData value. Found: " << nodata << ". Expected: " << expectedNodata
              << std::endl;
    return 1;
  }

  // Apply reprojection filter
  vtkNew<vtkRasterReprojectionFilter> filter;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->SetOutputProjection("EPSG:3857");

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(256);
  lut->SetRange(296, 334);
  lut->SetRampToLinear();
  lut->Build();

  vtkNew<vtkCellDataToPointData> c2p1;
  c2p1->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkImageMapToColors> c;
  c->SetLookupTable(lut);
  c->SetInputConnection(c2p1->GetOutputPort());
  vtkNew<vtkImageActor> inputSlice;
  inputSlice->GetMapper()->SetInputConnection(c->GetOutputPort());
  vtkNew<vtkRenderer> leftRen;
  leftRen->SetViewport(0, 0, 0.5, 1);
  leftRen->SetBackground(0.2, 0.2, 0.2);
  leftRen->AddActor(inputSlice);

  vtkNew<vtkCellDataToPointData> c2p2;
  c2p2->SetInputConnection(filter->GetOutputPort());
  vtkNew<vtkImageMapToColors> co;
  co->SetLookupTable(lut);
  co->SetInputConnection(c2p2->GetOutputPort());
  vtkNew<vtkImageActor> outputSlice;
  outputSlice->GetMapper()->SetInputConnection(co->GetOutputPort());
  vtkNew<vtkRenderer> rightRen;
  rightRen->SetViewport(0.5, 0, 1, 1);
  rightRen->AddActor(outputSlice);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->AddRenderer(leftRen);
  renWin->AddRenderer(rightRen);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  leftRen->ResetCamera();
  rightRen->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
