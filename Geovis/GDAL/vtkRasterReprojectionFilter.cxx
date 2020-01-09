/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRasterReprojectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRasterReprojectionFilter.h"

// VTK includes
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkGDAL.h"
#include "vtkGDALRasterConverter.h"
#include "vtkGDALRasterReprojection.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"

// GDAL includes
#include <gdal_priv.h>

// STL includes
#include <algorithm>

vtkStandardNewMacro(vtkRasterReprojectionFilter);

//----------------------------------------------------------------------------
class vtkRasterReprojectionFilter::vtkRasterReprojectionFilterInternal
{
public:
  vtkRasterReprojectionFilterInternal();
  ~vtkRasterReprojectionFilterInternal();

  vtkGDALRasterConverter* GDALConverter;
  vtkGDALRasterReprojection* GDALReprojection;

  // Data saved during RequestInformation()
  int InputImageExtent[6];
  double OutputImageGeoTransform[6];
};

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::vtkRasterReprojectionFilterInternal::
  vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter = vtkGDALRasterConverter::New();
  this->GDALReprojection = vtkGDALRasterReprojection::New();
  std::fill(this->InputImageExtent, this->InputImageExtent + 6, 0);
  std::fill(this->OutputImageGeoTransform, this->OutputImageGeoTransform + 6, 0.0);
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::vtkRasterReprojectionFilterInternal::
  ~vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter->Delete();
  this->GDALReprojection->Delete();
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::vtkRasterReprojectionFilter()
{
  this->Internal = new vtkRasterReprojectionFilterInternal;
  this->InputProjection = nullptr;
  this->FlipAxis[0] = this->FlipAxis[1] = this->FlipAxis[2] = 0;
  this->OutputProjection = nullptr;
  this->OutputDimensions[0] = this->OutputDimensions[1] = 0;
  this->NoDataValue = vtkMath::Nan();
  this->MaxError = 0.0;
  this->ResamplingAlgorithm = 0;

  // Enable all the drivers.
  GDALAllRegister();
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::~vtkRasterReprojectionFilter()
{
  if (this->InputProjection)
  {
    delete[] this->InputProjection;
  }
  if (this->OutputProjection)
  {
    delete[] this->OutputProjection;
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkRasterReprojectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputProjection: ";
  if (this->InputProjection)
  {
    os << this->InputProjection;
  }
  else
  {
    os << "(not specified)";
  }
  os << "\n";

  os << indent << "OutputProjection: ";
  if (this->OutputProjection)
  {
    os << this->OutputProjection;
  }
  else
  {
    os << "(not specified)";
  }
  os << "\n";

  os << indent << "OutputDimensions: " << OutputDimensions[0] << ", " << OutputDimensions[1] << "\n"
     << indent << "NoDataValue: " << this->NoDataValue << "\n"
     << indent << "MaxError: " << this->MaxError << "\n"
     << indent << "ResamplingAlgorithm: " << this->ResamplingAlgorithm << "\n"
     << indent << "FlipAxis: " << this->FlipAxis[0] << ", " << this->FlipAxis[1] << "\n"
     << std::endl;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input image data
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inDataObject = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!inDataObject)
  {
    return 0;
  }

  vtkImageData* inImageData = vtkImageData::SafeDownCast(inDataObject);
  if (!inImageData)
  {
    return 0;
  }
  // std::cout << "RequestData() has image to reproject!" << std::endl;

  // Get the output image
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    return 0;
  }

  // Convert input image to GDALDataset
  GDALDataset* inputGDAL = this->Internal->GDALConverter->CreateGDALDataset(
    inImageData, this->InputProjection, this->FlipAxis);

  if (this->Debug)
  {
    std::string tifFileName = "inputGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(inputGDAL, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;

    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(inputGDAL, 1, &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
  }

  // Construct GDAL dataset for output image
  vtkDataArray* array = inImageData->GetCellData()->GetScalars();
  int vtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  GDALDataset* outputGDAL = this->Internal->GDALConverter->CreateGDALDataset(
    this->OutputDimensions[0], this->OutputDimensions[1], vtkDataType, rasterCount);
  this->Internal->GDALConverter->CopyBandInfo(inputGDAL, outputGDAL);
  this->Internal->GDALConverter->SetGDALProjection(outputGDAL, this->OutputProjection);
  outputGDAL->SetGeoTransform(this->Internal->OutputImageGeoTransform);
  this->Internal->GDALConverter->CopyNoDataValues(inputGDAL, outputGDAL);

  // Apply the reprojection
  this->Internal->GDALReprojection->SetMaxError(this->MaxError);
  this->Internal->GDALReprojection->SetResamplingAlgorithm(this->ResamplingAlgorithm);
  this->Internal->GDALReprojection->Reproject(inputGDAL, outputGDAL);

  if (this->Debug)
  {
    std::string tifFileName = "reprojectGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(outputGDAL, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;
    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(outputGDAL, 1, &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
  }

  // Done with input GDAL dataset
  GDALClose(inputGDAL);

  // Convert output dataset to vtkUniformGrid
  vtkUniformGrid* reprojectedImage =
    this->Internal->GDALConverter->CreateVTKUniformGrid(outputGDAL);

  // Done with output GDAL dataset
  GDALClose(outputGDAL);

  // Update pipeline output instance
  vtkUniformGrid* output = vtkUniformGrid::GetData(outInfo);
  output->ShallowCopy(reprojectedImage);

  reprojectedImage->Delete();
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // Set input extent to values saved in last RequestInformation() call
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), this->Internal->InputImageExtent, 6);
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) ||
    !inInfo->Has(vtkDataObject::SPACING()) || !inInfo->Has(vtkDataObject::ORIGIN()))
  {
    vtkErrorMacro("Input information missing");
    return VTK_ERROR;
  }
  int* inputDataExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  std::copy(inputDataExtent, inputDataExtent + 6, this->Internal->InputImageExtent);

  double* inputOrigin = inInfo->Get(vtkDataObject::ORIGIN());
  double* inputSpacing = inInfo->Get(vtkDataObject::SPACING());
  // std::cout << "Whole extent: " << inputDataExtent[0]
  //           << ", " << inputDataExtent[1]
  //           << ", " << inputDataExtent[2]
  //           << ", " << inputDataExtent[3]
  //           << ", " << inputDataExtent[4]
  //           << ", " << inputDataExtent[5]
  //           << std::endl;
  // std::cout << "Input spacing: " << inputSpacing[0]
  //           << ", " << inputSpacing[1]
  //           << ", " << inputSpacing[2] << std::endl;
  // std::cout << "Input origin: " << inputOrigin[0]
  //           << ", " << inputOrigin[1]
  //           << ", " << inputOrigin[2] << std::endl;

  // InputProjection can be overridden, so only get from pipeline if needed
  if (!this->InputProjection)
  {
    if (!inInfo->Has(vtkGDAL::MAP_PROJECTION()))
    {
      vtkErrorMacro("No map-projection for input image");
      return VTK_ERROR;
    }
    this->SetInputProjection(inInfo->Get(vtkGDAL::MAP_PROJECTION()));
  }
  if (!inInfo->Has(vtkGDAL::FLIP_AXIS()))
  {
    vtkErrorMacro("No flip information for GDAL raster input image");
    return VTK_ERROR;
  }
  inInfo->Get(vtkGDAL::FLIP_AXIS(), this->FlipAxis);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return VTK_ERROR;
  }

  // Validate current settings
  if (!this->OutputProjection)
  {
    vtkErrorMacro("No output projection specified");
    return VTK_ERROR;
  }

  // Create GDALDataset to compute suggested output
  int xDim = inputDataExtent[1] - inputDataExtent[0] + 1;
  int yDim = inputDataExtent[3] - inputDataExtent[2] + 1;
  GDALDataset* gdalDataset =
    this->Internal->GDALConverter->CreateGDALDataset(xDim, yDim, VTK_UNSIGNED_CHAR, 1);
  this->Internal->GDALConverter->SetGDALProjection(gdalDataset, this->InputProjection);
  this->Internal->GDALConverter->SetGDALGeoTransform(
    gdalDataset, inputOrigin, inputSpacing, this->FlipAxis);

  int nPixels = 0;
  int nLines = 0;
  this->Internal->GDALReprojection->SuggestOutputDimensions(gdalDataset, this->OutputProjection,
    this->Internal->OutputImageGeoTransform, &nPixels, &nLines);
  GDALClose(gdalDataset);

  if ((this->OutputDimensions[0] < 1) || (this->OutputDimensions[1] < 1))
  {
    this->OutputDimensions[0] = nPixels;
    this->OutputDimensions[1] = nLines;
  }

  // Set output info
  int outputDataExtent[6] = {};
  outputDataExtent[1] = this->OutputDimensions[0] - 1;
  outputDataExtent[3] = this->OutputDimensions[1] - 1;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outputDataExtent, 6);

  double outputImageOrigin[3] = {};
  outputImageOrigin[0] = this->Internal->OutputImageGeoTransform[0];
  outputImageOrigin[1] = this->Internal->OutputImageGeoTransform[3];
  outputImageOrigin[2] = 1.0;
  outInfo->Set(vtkDataObject::SPACING(), outputImageOrigin, 3);

  double outputImageSpacing[3] = {};
  outputImageSpacing[0] = this->Internal->OutputImageGeoTransform[1];
  outputImageSpacing[1] = -this->Internal->OutputImageGeoTransform[5];
  outputImageSpacing[2] = 1.0;
  outInfo->Set(vtkDataObject::ORIGIN(), outputImageSpacing, 3);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    return VTK_OK;
  }
  else
  {
    vtkErrorMacro("Input port: " << port << " is not a valid port");
    return VTK_ERROR;
  }
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUniformGrid");
    return VTK_OK;
  }
  else
  {
    vtkErrorMacro("Output port: " << port << " is not a valid port");
    return VTK_ERROR;
  }
}
