/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkGDALRasterReader.h>

// VTK includes
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkImageActor.h>
#include <vtkInformation.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTestUtilities.h>
#include <vtkUniformGrid.h>

// C++ includes
#include <iterator>
#include <sstream>

// Main program
int TestGDALRasterReader(int argc, char** argv)
{
  const char* rasterFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/raster.tif");

  // Create reader to read shape file.
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  // extent in points
  int* extent =
    reader->GetOutputInformation(0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  std::ostream_iterator<int> out_it(std::cout, " ");
  std::cout << "Point extents: ";
  std::copy(extent, extent + 6, out_it);
  std::cout << "\n";
  // raster dimensions in cells (pixels)
  int* rasterdims = reader->GetRasterDimensions();
  std::cout << "Cell dimensions: ";
  std::copy(rasterdims, rasterdims + 2, out_it);
  std::cout << std::endl;
  if (extent[1] - extent[0] != rasterdims[0] || extent[3] - extent[2] != rasterdims[1])
  {
    std::cerr << "Error: Number of cells should be one less than the number of points\n";
    return 1;
  }

  // test if we read all 3 bands with CollateBands=0 (default is 1)
  reader->SetCollateBands(0);
  reader->Update();
  vtkUniformGrid* data = vtkUniformGrid::SafeDownCast(reader->GetOutput());
  if (data->GetCellData()->GetNumberOfArrays() != 3)
  {
    std::cerr << "Error: Expecting 3 scalar arrays\n";
    return 1;
  }

  // test if we read only 2 bands once we deselected the first band
  reader->SetCellArrayStatus(reader->GetCellArrayName(0), 0);
  reader->Update();
  data = vtkUniformGrid::SafeDownCast(reader->GetOutput());
  if (data->GetCellData()->GetNumberOfArrays() != 2)
  {
    std::cerr << "Error: Expecting two scalar arrays\n";
    return 1;
  }

  // collate bands
  reader->SetCollateBands(1);
  reader->SetCellArrayStatus(reader->GetCellArrayName(0), 1);
  reader->Update();
  delete[] rasterFileName;

  // We need a renderer
  vtkNew<vtkRenderer> renderer;

  // Get the data
  vtkNew<vtkCellDataToPointData> c2p;
  c2p->SetInputDataObject(reader->GetOutput());
  c2p->Update();

  vtkNew<vtkImageActor> actor;
  actor->SetInputData(vtkUniformGrid::SafeDownCast(c2p->GetOutput()));
  renderer->AddActor(actor);

  // Create a render window, and an interactor
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderWindow->SetSize(400, 400);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
