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
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkGDAL.h>
#include <vtkGDALRasterReader.h>
#include <vtkInformation.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkUniformGrid.h>

#include <iostream>
#include <limits>

// Main program
int TestGDALRasterNoDataValue(int argc, char** argv)
{
  if (argc < 3)
  {
    std::cerr << "Expected TestName -D InputFile.tif" << std::endl;
    return -1;
  }

  std::string inputFileName(argv[2]);

  // Create reader to read shape file.
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  vtkUniformGrid* rasterImage = vtkUniformGrid::SafeDownCast(reader->GetOutput());

  int numErrors = 0;

  double* bounds = rasterImage->GetBounds();
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], -73.7583450) ||
    !vtkMathUtilities::FuzzyCompare(bounds[1], -72.7583450) ||
    !vtkMathUtilities::FuzzyCompare(bounds[2], 42.8496040) ||
    !vtkMathUtilities::FuzzyCompare(bounds[3], 43.8496040))
  {
    std::cerr << "Bounds do not match what is reported by gdalinfo." << std::endl;
    ++numErrors;
  }
  if (!rasterImage->HasAnyBlankCells())
  {
    std::cerr << "Error image has no blank cells" << std::endl;
    ++numErrors;
  }

  double* scalarRange = rasterImage->GetScalarRange();

  if ((scalarRange[0] < -888.5) || (scalarRange[0]) > -887.5)
  {
    std::cerr << "Error scalarRange[0] should be -888.0, not " << scalarRange[0] << std::endl;
    ++numErrors;
  }

  if ((scalarRange[1] < 9998.5) || (scalarRange[1] > 9999.5))
  {
    std::cerr << "Error scalarRange[1] should be 9999.0, not " << scalarRange[1] << std::endl;
    ++numErrors;
  }

  // test that we read the NoData value correctly
  double nodata = reader->GetInvalidValue(0);
  double expectedNodata = -3.40282346638529993e+38;
  double tolerance = 1e+26;
  if (!vtkMathUtilities::FuzzyCompare(nodata, expectedNodata, tolerance))
  {
    std::cerr << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "Error NoData value. Found: " << nodata << ". Expected: " << expectedNodata
              << std::endl;
    ++numErrors;
  }

  // test that we read a flip for the Y axis
  reader->UpdateInformation();

  // Do we have the meta-data created by the reader at the end
  // of the pipeline?
  vtkInformation* outInfo = reader->GetOutputInformation(0);
  if (!outInfo->Has(vtkGDAL::FLIP_AXIS()))
  {
    std::cerr << "Error: There is no FLIP_AXIS key" << std::endl;
    ++numErrors;
  }
  int flipAxis[3];
  outInfo->Get(vtkGDAL::FLIP_AXIS(), flipAxis);
  if (flipAxis[0] != 0 || flipAxis[1] != 0)
  {
    std::cerr << "Error: Wrong flipAxis for " << inputFileName << ": " << flipAxis[0] << ", "
              << flipAxis[1] << std::endl;
    ++numErrors;
  }

  if (!outInfo->Has(vtkGDAL::MAP_PROJECTION()))
  {
    std::cerr << "Error: There is no MAP_PROJECTION key" << std::endl;
    ++numErrors;
  }
  std::string expectedMapProjection(
    "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\","
    "SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],"
    "AUTHORITY[\"EPSG\",\"6326\"]],"
    "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433],AUTHORITY[\"EPSG\",\"4326\"]]");
  if (expectedMapProjection != std::string(outInfo->Get(vtkGDAL::MAP_PROJECTION())))
  {
    std::cerr << "Error: Different MAP_PROJECTION value than expected. Value:\n"
              << outInfo->Get(vtkGDAL::MAP_PROJECTION()) << "\nExpected:\n"
              << expectedMapProjection << "\n";
  }

  // std::cout << "numErrors: " << numErrors << std::endl;
  return numErrors;
}
