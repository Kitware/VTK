/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRaster.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGDALRasterConverter.h"

#include <vtkGDALRasterReader.h>
#include <vtkImageData.h>
#include <vtkNew.h>

#undef LT_OBJDIR // fixes compiler warning (collision w/vtkIOStream.h)
#include <gdal_priv.h>

#include <iostream>

//----------------------------------------------------------------------------
GDALDataType vtkDataTypeToGDAL(int vtkDataType)
{
  GDALDataType gdalType = GDT_Unknown;

  switch (vtkDataType)
  {
    case VTK_TYPE_UINT8:
      gdalType = GDT_Byte;
      break;
    case VTK_TYPE_INT16:
      gdalType = GDT_Int16;
      break;
    case VTK_TYPE_UINT16:
      gdalType = GDT_UInt16;
      break;
    case VTK_TYPE_INT32:
      gdalType = GDT_Int32;
      break;
    case VTK_TYPE_UINT32:
      gdalType = GDT_UInt32;
      break;
    case VTK_TYPE_FLOAT32:
      gdalType = GDT_Float32;
      break;
    case VTK_TYPE_FLOAT64:
      gdalType = GDT_Float64;
      break;
  }

  return gdalType;
}

//----------------------------------------------------------------------------
int TestVTKToGDAL(const char* inputFilename)
{
  GDALAllRegister(); // shouldn't need this

  // Load input file
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(inputFilename);
  reader->Update();

  std::cout << "Driver short " << reader->GetDriverShortName() << ", long "
            << reader->GetDriverLongName() << "\n";
  std::cout << "Projection string: " << reader->GetProjectionString() << "\n";
  std::cout << "Corner points:"
            << "\n";
  const double* corners = reader->GetGeoCornerPoints();
  for (int i = 0, index = 0; i < 4; i++, index += 2)
  {
    std::cout << "  " << std::setprecision(12) << corners[index] << ", "
              << std::setprecision(12) << corners[index + 1] << "\n";
  }
  std::cout << "Delta longitude: " << std::setprecision(12)
            << (corners[4] - corners[0]) << "\n";
  std::cout << "Delta latitude:  " << std::setprecision(12)
            << (corners[5] - corners[1]) << "\n";

  int dim[2];
  reader->GetRasterDimensions(dim);
  std::cout << "Raster dimensions: " << dim[0] << ", " << dim[1] << "\n";

  vtkImageData* rasterData = reader->GetOutput();
  std::cout << "Scalar type: " << rasterData->GetScalarType() << " = "
            << rasterData->GetScalarTypeAsString() << "\n";
  std::cout << "Scalar size: " << rasterData->GetScalarSize() << " bytes"
            << "\n";
  int* rasterDim = rasterData->GetDimensions();
  std::cout << "Raster dimensions: " << rasterDim[0] << ", " << rasterDim[1]
            << "\n";
  double* range = rasterData->GetScalarRange();
  std::cout << "Scalar range: " << range[0] << ", " << range[1] << "\n";

  std::cout << std::endl;

  // Convert back to GDALDataset and write as tif file
  vtkNew<vtkGDALRasterConverter> converter;
  double noDataValue = reader->GetInvalidValue();
  converter->SetNoDataValue(noDataValue);
  GDALDataset* gdalData =
    converter->CreateGDALDataset(rasterData, reader->GetProjectionString());
  converter->WriteTifFile(gdalData, "converted.tif");
  GDALClose(gdalData);

  return 0;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "\n"
              << "Usage: TestGDALRasterConvert  inputfile"
              << "\n"
              << std::endl;
    return -1;
  }

  int result = TestVTKToGDAL(argv[1]);
  std::cout << "Finis" << std::endl;
  return result;
}
