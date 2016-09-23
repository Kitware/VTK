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
#include <vtkNew.h>
#include <vtkUniformGrid.h>

#include <iostream>

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

  vtkUniformGrid *rasterImage = vtkUniformGrid::SafeDownCast(
    reader->GetOutput());

  int numErrors = 0;

  if (!rasterImage->HasAnyBlankPoints())
  {
    std::cerr << "Error image has no blank points" << std::endl;
    ++numErrors;
  }

  if (!rasterImage->HasAnyBlankCells())
  {
    std::cerr << "Error image has no blank cells" << std::endl;
    ++numErrors;
  }

  double *scalarRange = rasterImage->GetScalarRange();

  if ((scalarRange[0] < -888.5) || (scalarRange[0]) > -887.5)
  {
    std::cerr << "Error scalarRange[0] should be -888.0, not "
              << scalarRange[0] << std::endl;
    ++numErrors;
  }

  if ((scalarRange[1] < 9998.5) || (scalarRange[1] > 9999.5))
  {
    std::cerr << "Error scalarRange[1] should be 9999.0, not "
              << scalarRange[1] << std::endl;
    ++numErrors;
  }

  //std::cout << "numErrors: " << numErrors << std::endl;
  return numErrors;
}
