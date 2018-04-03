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

#include "vtkRasterReprojectionFilter.h"

#include <vtkGDALRasterReader.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkImageViewer2.h>
#include <vtkNew.h>
#include <vtkRasterReprojectionFilter.h>
#include <vtkUniformGrid.h>
#include <vtkXMLImageDataWriter.h>

#undef LT_OBJDIR // fixes compiler warning (collision w/vtkIOStream.h)
#include <gdal_priv.h>

#include <iostream>

//----------------------------------------------------------------------------
int TestRasterReprojectionFilter(const char* inputFilename)
{
  GDALAllRegister(); // shouldn't need this

  // Load input file
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(inputFilename);
  //reader->Update();

  // Apply reprojection filter
  vtkNew<vtkRasterReprojectionFilter> filter;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->SetOutputProjection("EPSG:3857");
  //filter->DebugOn();
  //filter->Update();

  // Capture minimal statistics
  vtkNew<vtkImageAccumulate> accumulator;
  accumulator->SetInputConnection(filter->GetOutputPort());
  accumulator->SetComponentExtent(0, 1, 0, 1, 0, 0);
  accumulator->Update();

  double* min = accumulator->GetMin();
  double* mean = accumulator->GetMean();
  double* max = accumulator->GetMax();
  double* std = accumulator->GetStandardDeviation();
  vtkIdType count = accumulator->GetVoxelCount();

  std::cout << "Accumulator results:"
            << "\n"
            << "  Voxel count: " << count
            << "  Min, Mean, Max StdDev:  " << min[0] << ", " << mean[0] << ", "
            << max[0] << ", " << std[0] << "\n";

  // Write image to file
  const char* outputFilename = "image.vti";
  vtkNew<vtkXMLImageDataWriter> writer;
  writer->SetFileName(outputFilename);
  writer->SetInputConnection(filter->GetOutputPort());
  writer->SetDataModeToAscii();
  writer->Write();
  std::cout << "Wrote " << outputFilename << std::endl;

  // Display input image
  vtkImageData* inputImage = reader->GetOutput();
  double* scalarRange = inputImage->GetScalarRange();
  double colorLevel = 0.5 * (scalarRange[0] + scalarRange[1]);
  double colorRange = scalarRange[1] - scalarRange[0];

  vtkImageViewer2* inputViewer = vtkImageViewer2::New();
  inputViewer->SetInputData(inputImage);
  inputViewer->SetColorLevel(colorLevel);
  inputViewer->SetColorWindow(colorRange);
  inputViewer->Render();
  int* inputDims = inputImage->GetDimensions();
  std::cout << "Input image " << inputDims[0] << " x " << inputDims[1]
            << std::endl;

  // Display reprojected image
  vtkImageData* outputImage = filter->GetOutput();
  vtkImageViewer2* outputViewer = vtkImageViewer2::New();
  outputViewer->SetInputData(outputImage);
  outputViewer->SetColorLevel(colorLevel);
  outputViewer->SetColorWindow(colorRange);
  outputViewer->Render();
  int* outputDims = outputImage->GetDimensions();
  std::cout << "Output image " << outputDims[0] << " x " << outputDims[1]
            << std::endl;

  std::cout << "Hit any key plus <ENTER> to exit: ";
  char c;
  std::cin >> c;

  outputViewer->Delete();
  inputViewer->Delete();

  // Dump info
  //vtkUniformGrid *output = vtkUniformGrid::SafeDownCast(filter->GetOutput());
  //output->Print(std::cout);

  return 0;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "\n"
              << "Usage: TestRasterReprojectionFilter  inputfile"
              << "\n"
              << std::endl;
    return -1;
  }

  int result = TestRasterReprojectionFilter(argv[1]);
  std::cout << "Finis" << std::endl;
  return result;
}
