/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALRasterConverter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGDALRasterConverter.h"

// VTK includes
#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUniformGrid.h"

#include <cpl_string.h>
#include <gdal_priv.h>
#include <memory>
#include <ogr_spatialref.h>
#include <sstream>
#include <vector>

// Preprocessor directive enable/disable row inversion (y-flip)
// Although vtkImageData and GDALDataset have different origin position,
// reprojection of NLCD imagery only "works" if row inversion is not
// applied when converting between formats.
#define INVERT_ROWS 0

vtkStandardNewMacro(vtkGDALRasterConverter);

//----------------------------------------------------------------------------
class vtkGDALRasterConverter::vtkGDALRasterConverterInternal
{
public:
  GDALDataType ToGDALDataType(int vtkDataType);

  template <typename VTK_TYPE>
  void CopyToVTK(GDALDataset* gdalData, vtkDataArray* vtkData, vtkUniformGrid* uniformGridData);

  template <typename GDAL_TYPE>
  void FindDataRange(GDALRasterBand* band, double* minValue, double* maxValue);
};

//----------------------------------------------------------------------------
// Translates vtk data type to GDAL data type
GDALDataType vtkGDALRasterConverter::vtkGDALRasterConverterInternal::ToGDALDataType(int vtkDataType)
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
// Copies contents of GDALDataset to vtkDataArray
template <typename VTK_TYPE>
void vtkGDALRasterConverter::vtkGDALRasterConverterInternal::CopyToVTK(
  GDALDataset* dataset, vtkDataArray* array, vtkUniformGrid* uniformGridData)
{
  // Initialize array storage
  int stride = dataset->GetRasterCount();
  array->SetNumberOfComponents(stride);
  int xSize = dataset->GetRasterXSize() - 1;
  int ySize = dataset->GetRasterYSize() - 1;
  int numElements = xSize * ySize;
  array->SetNumberOfTuples(numElements);

  // Copy from GDAL to vtkImageData, one band at a time
  VTK_TYPE* buffer = new VTK_TYPE[numElements];
  for (int i = 0; i < stride; i++)
  {
    // Copy one band from dataset to buffer
    GDALRasterBand* band = dataset->GetRasterBand(i + 1);
    GDALDataType gdalDataType = band->GetRasterDataType();
    CPLErr err =
      band->RasterIO(GF_Read, 0, 0, xSize, ySize, buffer, xSize, ySize, gdalDataType, 0, 0);
    if (err == CE_Failure)
    {
      std::cerr << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n"
                << CPLGetLastErrorMsg() << std::endl;
      return;
    }

    int hasNoDataValue = 0;
    double noDataValue = band->GetNoDataValue(&hasNoDataValue);

    // Copy data from buffer to vtkDataArray
    // Traverse by gdal row & column to make y-inversion easier
    for (int row = 0, index = 0; row < ySize; row++)
    {
#if INVERT_ROWS
      // GDAL data starts at top-left, vtk at bottom-left
      // So need to invert in the y direction
      int targetRow = ySize - row - 1;
#else
      int targetRow = row;
#endif
      int offset = targetRow * xSize;
      for (int col = 0; col < xSize; col++)
      {
        array->SetComponent(offset + col, i, buffer[index]);
        if (hasNoDataValue && (static_cast<double>(buffer[index]) == noDataValue))
        {
          std::cout << "Blank Point at col, row: " << col << ", " << row << std::endl;
          uniformGridData->BlankPoint(col, row, 0);
        }
        index++;
      }
    }

    // Check for color table
    if (band->GetColorInterpretation() == GCI_PaletteIndex)
    {
      GDALColorTable* gdalTable = band->GetColorTable();
      if (gdalTable->GetPaletteInterpretation() != GPI_RGB)
      {
        std::cerr << "Color table palette type not supported "
                  << gdalTable->GetPaletteInterpretation() << std::endl;
        continue;
      }

      vtkNew<vtkLookupTable> colorTable;
      char** categoryNames = band->GetCategoryNames();

      colorTable->IndexedLookupOn();
      int numEntries = gdalTable->GetColorEntryCount();
      colorTable->SetNumberOfTableValues(numEntries);
      std::stringstream ss;
      for (int j = 0; j < numEntries; ++j)
      {
        const GDALColorEntry* gdalEntry = gdalTable->GetColorEntry(j);
        double r = static_cast<double>(gdalEntry->c1) / 255.0;
        double g = static_cast<double>(gdalEntry->c2) / 255.0;
        double b = static_cast<double>(gdalEntry->c3) / 255.0;
        double a = static_cast<double>(gdalEntry->c4) / 255.0;
        colorTable->SetTableValue(j, r, g, b, a);

        // Copy category name to lookup table annotation
        if (categoryNames)
        {
          // Only use non-empty names
          if (strlen(categoryNames[j]) > 0)
          {
            colorTable->SetAnnotation(vtkVariant(j), categoryNames[j]);
          }
        }
        else
        {
          // Create default annotation
          ss.str("");
          ss.clear();
          ss << "Category " << j;
          colorTable->SetAnnotation(vtkVariant(j), ss.str());
        }
      }

      // colorTable->Print(std::cout);
      array->SetLookupTable(colorTable.GetPointer());
    }
  }
  delete[] buffer;
}

//----------------------------------------------------------------------------
// Iterate overall values in raster band to find min & max
template <typename VTK_TYPE>
void vtkGDALRasterConverter::vtkGDALRasterConverterInternal::FindDataRange(
  GDALRasterBand* band, double* minValue, double* maxValue)
{
  int xSize = band->GetDataset()->GetRasterXSize();
  int ySize = band->GetDataset()->GetRasterYSize();
  VTK_TYPE* buffer = new VTK_TYPE[xSize * ySize];
  GDALDataType gdalDataType = band->GetRasterDataType();
  CPLErr err =
    band->RasterIO(GF_Read, 0, 0, xSize, ySize, buffer, xSize, ySize, gdalDataType, 0, 0);
  if (err == CE_Failure)
  {
    std::cerr << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n"
              << CPLGetLastErrorMsg() << std::endl;
    return;
  }

  *minValue = VTK_DOUBLE_MAX;
  *maxValue = VTK_DOUBLE_MIN;
  for (int i = 0; i < xSize * ySize; i++)
  {
    *minValue = *minValue < buffer[i] ? *minValue : buffer[i];
    *maxValue = *maxValue > buffer[i] ? *maxValue : buffer[i];
  }

  delete[] buffer;
}

//----------------------------------------------------------------------------
// Copy vtkDataArray contents to GDAL raster bands
struct StaticCopyToGDAL
{
  template <typename ArrayT>
  void operator()(ArrayT* array, GDALDataset* gdalData)
  {
    using T = vtk::GetAPIType<ArrayT>;

    // If data includes a lookup table, copy that
    std::unique_ptr<GDALColorTable> gdalColorTable;
    vtkLookupTable* inputColorTable = array->GetLookupTable();
    if (inputColorTable)
    {
      gdalColorTable.reset(new GDALColorTable);
      vtkIdType tableSize = inputColorTable->GetNumberOfTableValues();
      double inputColor[4] = { 0.0, 0.0, 0.0, 1.0 };
      GDALColorEntry gdalColor;
      for (vtkIdType i = 0; i < tableSize; ++i)
      {
        inputColorTable->GetTableValue(i, inputColor);
        gdalColor.c1 = static_cast<short>(inputColor[0] * 255.0);
        gdalColor.c2 = static_cast<short>(inputColor[1] * 255.0);
        gdalColor.c3 = static_cast<short>(inputColor[2] * 255.0);
        gdalColor.c4 = static_cast<short>(inputColor[3] * 255.0);
        gdalColorTable.get()->SetColorEntry(i, &gdalColor);
      }
    }

    // Create local buffer
    int stride = array->GetNumberOfComponents();
    vtkIdType numElements = array->GetNumberOfTuples();
    std::vector<T> buffer(static_cast<size_t>(numElements));
    int xSize = gdalData->GetRasterXSize();
    int ySize = gdalData->GetRasterYSize();

    // Copy each vtk component to separate gdal band
    for (int i = 0; i < stride; i++)
    {
      GDALRasterBand* band = gdalData->GetRasterBand(i + 1);
      if (gdalColorTable)
      {
        band->SetColorTable(gdalColorTable.get());
        band->SetColorInterpretation(GCI_PaletteIndex);
      }

      // Create a tuple range for the array:
      const auto tuples = vtk::DataArrayTupleRange(array);
      auto tupleIter = tuples.cbegin();

      // Copy data from vtk iterator to buffer
      // Traverse by gdal row & column to make inversion easier
      for (int row = 0; row < ySize; row++)
      {
#if INVERT_ROWS
        // GDAL data starts at top-left, vtk at bottom-left
        // So need to invert in the y direction
        int targetRow = ySize - row - 1;
#else
        int targetRow = row;
#endif
        int offset = targetRow * xSize;
        for (int col = 0; col < xSize; col++)
        {
          buffer[offset + col] = (*tupleIter++)[i];
        }
      }

      // Copy from buffer to GDAL band
      GDALDataType gdalDataType = band->GetRasterDataType();
      CPLErr err = band->RasterIO(
        GF_Write, 0, 0, xSize, ySize, buffer.data(), xSize, ySize, gdalDataType, 0, 0);
      if (err == CE_Failure)
      {
        std::cerr << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n"
                  << CPLGetLastErrorMsg() << std::endl;
        return;
      }
    }
  }
};

//----------------------------------------------------------------------------
vtkGDALRasterConverter::vtkGDALRasterConverter()
{
  this->Internal = new vtkGDALRasterConverterInternal();
  this->NoDataValue = vtkMath::Nan();
}

//----------------------------------------------------------------------------
vtkGDALRasterConverter::~vtkGDALRasterConverter()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkGDALRasterConverter" << std::endl;
}

//----------------------------------------------------------------------------
// Copy image data contents, origin, & spacing to GDALDataset
bool vtkGDALRasterConverter::CopyToGDAL(vtkImageData* input, GDALDataset* output, int flipAxis[3])
{
  // Check that both images have the same dimensions
  int* inputDimensions = input->GetDimensions();
  if (output->GetRasterXSize() != inputDimensions[0] - 1 ||
    output->GetRasterYSize() != inputDimensions[1] - 1)
  {
    vtkErrorMacro(<< "Image dimensions do not match.");
    return false;
  }

  // Initialize geo transform
  double* origin = input->GetOrigin();
  double* spacing = input->GetSpacing();
  this->SetGDALGeoTransform(output, origin, spacing, flipAxis);

  // Check for NO_DATA_VALUE array
  int index = -1;
  vtkDataArray* array = input->GetFieldData()->GetArray("NO_DATA_VALUE", index);
  vtkDoubleArray* noDataArray = vtkDoubleArray::SafeDownCast(array);
  if (noDataArray)
  {
    for (int i = 0; i < array->GetNumberOfTuples(); i++)
    {
      double value = noDataArray->GetTuple1(i);
      if (!vtkMath::IsNan(value))
      {
        GDALRasterBand* band = output->GetRasterBand(i + 1);
        band->SetNoDataValue(value);
      }
    }
  } // if (noDataArray)

  // Copy scalars to gdal bands
  array = input->GetCellData()->GetScalars();
  using Dispatcher = vtkArrayDispatch::Dispatch;
  StaticCopyToGDAL worker;
  if (!Dispatcher::Execute(array, worker, output))
  { // Fallback for unknown array types:
    worker(array, output);
  }

  // Finis
  return true;
}

//----------------------------------------------------------------------------
GDALDataset* vtkGDALRasterConverter::CreateGDALDataset(
  vtkImageData* imageData, const char* mapProjection, int flipAxis[3])
{
  int* dimensions = imageData->GetDimensions();
  vtkDataArray* array = imageData->GetCellData()->GetScalars();
  int vtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  GDALDataset* dataset =
    this->CreateGDALDataset(dimensions[0] - 1, dimensions[1] - 1, vtkDataType, rasterCount);
  this->CopyToGDAL(imageData, dataset, flipAxis);
  this->SetGDALProjection(dataset, mapProjection);
  this->SetGDALGeoTransform(dataset, imageData->GetOrigin(), imageData->GetSpacing(), flipAxis);
  return dataset;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkGDALRasterConverter::CreateVTKUniformGrid(GDALDataset* dataset)
{
  // Set vtk origin & spacing from GDALGeoTransform
  double geoTransform[6] = {};
  if (dataset->GetGeoTransform(geoTransform) != CE_None)
  {
    vtkErrorMacro(<< "Error calling GetGeoTransform()");
    return nullptr;
  }

  // Initialize image
  vtkUniformGrid* image = vtkUniformGrid::New();
  int imageDimensions[3];
  imageDimensions[0] = dataset->GetRasterXSize();
  imageDimensions[1] = dataset->GetRasterYSize();
  imageDimensions[2] = 1;
  image->SetDimensions(imageDimensions);

  double origin[3];
  origin[0] = geoTransform[0];
  origin[1] = geoTransform[3];
  origin[2] = 0.0;
  image->SetOrigin(origin);

  double spacing[3];
  spacing[0] = geoTransform[1];
  spacing[1] = geoTransform[5];
  spacing[2] = 0.0;
  image->SetSpacing(spacing);

  // Copy pixel data
  int rasterCount = dataset->GetRasterCount();
  if (rasterCount < 1)
  {
    return nullptr;
  }

  vtkDataArray* array = nullptr;
  switch (dataset->GetRasterBand(1)->GetRasterDataType())
  {
    case GDT_Byte:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT8);
      this->Internal->CopyToVTK<vtkTypeUInt8>(dataset, array, image);
      break;

    case GDT_UInt16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT16);
      this->Internal->CopyToVTK<vtkTypeUInt16>(dataset, array, image);
      break;

    case GDT_Int16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT16);
      this->Internal->CopyToVTK<vtkTypeInt16>(dataset, array, image);
      break;

    case GDT_UInt32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT32);
      this->Internal->CopyToVTK<vtkTypeUInt32>(dataset, array, image);
      break;

    case GDT_Int32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT32);
      this->Internal->CopyToVTK<vtkTypeInt32>(dataset, array, image);
      break;

    case GDT_Float32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT32);
      this->Internal->CopyToVTK<vtkTypeFloat32>(dataset, array, image);
      break;

    case GDT_Float64:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT64);
      this->Internal->CopyToVTK<vtkTypeFloat64>(dataset, array, image);
      break;
    default:
      break;
  }

  if (!array)
  {
    return nullptr;
  }

  image->GetCellData()->SetScalars(array);
  array->Delete();

  return image;
}

//----------------------------------------------------------------------------
GDALDataset* vtkGDALRasterConverter::CreateGDALDataset(
  int xDim, int yDim, int vtkDataType, int numberOfBands)
{
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("MEM");
  GDALDataType gdalType = this->Internal->ToGDALDataType(vtkDataType);
  GDALDataset* dataset = driver->Create("", xDim, yDim, numberOfBands, gdalType, nullptr);
  return dataset;
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::CopyBandInfo(GDALDataset* src, GDALDataset* dest)
{
  // Copy color interpretation and color table info
  int numSrcBands = src->GetRasterCount();
  for (int i = 0; i < numSrcBands; i++)
  {
    int index = i + 1;
    GDALRasterBand* srcBand = src->GetRasterBand(index);
    GDALRasterBand* destBand = dest->GetRasterBand(index);
    destBand->SetColorInterpretation(srcBand->GetColorInterpretation());

    GDALColorTable* colorTable = srcBand->GetColorTable();
    if (colorTable)
    {
      destBand->SetColorTable(colorTable);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::SetGDALProjection(GDALDataset* dataset, const char* projectionString)
{
  // Use OGRSpatialReference to convert to WKT
  OGRSpatialReference ref;
  ref.SetFromUserInput(projectionString);
  char* wkt = nullptr;
  ref.exportToWkt(&wkt);
  // std::cout << "Projection WKT: " << wkt << std::endl;
  dataset->SetProjection(wkt);
  CPLFree(wkt);
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::SetGDALGeoTransform(
  GDALDataset* dataset, double origin[2], double spacing[2], int flipAxis[2])
{
  double geoTransform[6];
  geoTransform[0] = origin[0];
  geoTransform[1] = flipAxis[0] ? -spacing[0] : spacing[0];
  geoTransform[2] = 0.0;
  geoTransform[3] = origin[1];
  geoTransform[4] = 0.0;
  geoTransform[5] = flipAxis[1] ? -spacing[1] : spacing[1];
  dataset->SetGeoTransform(geoTransform);
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::CopyNoDataValues(GDALDataset* src, GDALDataset* dst)
{
  // Check that raster count is consistent and > 0
  int numSrcBands = src->GetRasterCount();
  int numDstBands = dst->GetRasterCount();
  if (numSrcBands != numDstBands)
  {
    vtkWarningMacro("raster count different between src & dst datasets");
    return;
  }

  if (numSrcBands == 0)
  {
    return;
  }

  double noDataValue = vtkMath::Nan();
  int success = -1;
  for (int i = 0; i < numSrcBands; i++)
  {
    int index = i + 1;
    GDALRasterBand* srcBand = src->GetRasterBand(index);
    noDataValue = srcBand->GetNoDataValue(&success);
    if (success)
    {
      GDALRasterBand* dstBand = dst->GetRasterBand(index);
      dstBand->SetNoDataValue(noDataValue);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::WriteTifFile(GDALDataset* dataset, const char* filename)
{
  const char* fmt = "GTiff";
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(fmt);
  if (driver == nullptr)
  {
    vtkErrorMacro(<< "Cannot write GTiff file. GDALDriver is a nullptr");
    std::cout << "Cannot write GTiff file." << std::endl;
    return;
  }

  // Copy dataset to GTiFF driver, which creates file
  GDALDataset* copy = driver->CreateCopy(filename, dataset, false, nullptr, nullptr, nullptr);
  GDALClose(copy);
}

//----------------------------------------------------------------------------
bool vtkGDALRasterConverter::FindDataRange(
  GDALDataset* dataset, int bandId, double* minValue, double* maxValue)
{
  if ((bandId < 1) || (bandId > dataset->GetRasterCount()))
  {
    return false;
  }
  GDALRasterBand* band = dataset->GetRasterBand(bandId);
  GDALDataType gdalDataType = band->GetRasterDataType();
  switch (gdalDataType)
  {
    case GDT_Byte:
      this->Internal->FindDataRange<vtkTypeUInt8>(band, minValue, maxValue);
      break;

    case GDT_Int16:
      this->Internal->FindDataRange<vtkTypeInt16>(band, minValue, maxValue);
      break;

    case GDT_UInt16:
      this->Internal->FindDataRange<vtkTypeUInt16>(band, minValue, maxValue);
      break;

    case GDT_UInt32:
      this->Internal->FindDataRange<vtkTypeUInt32>(band, minValue, maxValue);
      break;

    case GDT_Int32:
      this->Internal->FindDataRange<vtkTypeInt32>(band, minValue, maxValue);
      break;

    case GDT_Float32:
      this->Internal->FindDataRange<vtkTypeFloat32>(band, minValue, maxValue);
      break;

    case GDT_Float64:
      this->Internal->FindDataRange<vtkTypeFloat64>(band, minValue, maxValue);
      break;
    default:
      break;
  }

  return true;
}
