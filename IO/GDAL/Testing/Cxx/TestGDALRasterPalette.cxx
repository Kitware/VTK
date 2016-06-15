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
#include <vtkDataArray.h>
#include <vtkGDALRasterReader.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkUniformGrid.h>

#include <iostream>

// Main program
int TestGDALRasterPalette(int argc, char** argv)
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
  vtkUniformGrid *image = vtkUniformGrid::SafeDownCast(reader->GetOutput());

  // Check that reader generated point scalars
  if (image->GetPointData()->GetNumberOfArrays() < 1)
    {
    std::cerr << "ERROR: Missing point data scalars" << std::endl;
    return 1;
    }
  if (image->GetPointData()->GetScalars()->GetSize() != 300*300)
    {
    std::cerr << "ERROR: Point data scalars wrong size, not."
              << (300*300) << ". Instead "
              << image->GetPointData()->GetScalars()->GetSize() << std::endl;
    return 1;
    }

  // Check that reader generated color table
  vtkLookupTable *colorTable =
    image->GetPointData()->GetScalars()->GetLookupTable();
  if (!colorTable)
    {
    std::cerr << "ERROR: Missing color table" << std::endl;
    return 1;
    }
  if (colorTable->GetNumberOfAvailableColors() != 256)
    {
    std::cerr << "ERROR: Color table does not have 256 colors."
              << " Instead has " <<  colorTable->GetNumberOfAvailableColors()
              << std::endl;
    return 1;
    }

  return 0;
}
