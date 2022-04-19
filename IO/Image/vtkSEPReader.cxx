/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSEPReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSEPReader.h"

#include <vtkByteSwap.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkExtentTranslator.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerRequestKey.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkStringArray.h>
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstdint>

namespace details
{
static constexpr std::size_t DataFormatSize[] = { 4, 4, 8 };

inline void SwapByteOrder4(char* data)
{
  std::swap(data[0], data[3]);
  std::swap(data[1], data[2]);
}
inline void SwapByteOrder8(char* data)
{
  std::swap(data[0], data[7]);
  std::swap(data[1], data[6]);
  std::swap(data[2], data[5]);
  std::swap(data[3], data[4]);
}

void TrimString(std::string& s)
{
  // trim trailing spaces
  std::size_t pos = s.find_last_not_of(" \t");
  if (pos != std::string::npos)
  {
    s = s.substr(0, pos + 1);
  }
  // trim leading spaces
  pos = s.find_first_not_of(" \t");
  if (pos != std::string::npos)
  {
    s = s.substr(pos);
  }
}

constexpr EndiannessType GetEndianessType()
{
#ifdef VTK_WORDS_BIGENDIAN
  return EndiannessType::SEP_BIG_ENDIAN;
#else
  return EndiannessType::SEP_LITTLE_ENDIAN;
#endif
}

bool DimensionIsInRange(int dim)
{
  return dim >= 0 && dim < SEP_READER_MAX_DIMENSION;
}
}

std::ostream& operator<<(std::ostream& os, details::EndiannessType& type)
{
  switch (type)
  {
    case details::EndiannessType::SEP_BIG_ENDIAN:
      os << "Big Endian";
      break;
    case details::EndiannessType::SEP_LITTLE_ENDIAN:
      os << "Little Endian";
      break;
  }
  return os;
}

vtkStandardNewMacro(vtkSEPReader);

//----------------------------------------------------------------------------
vtkSEPReader::vtkSEPReader()
{
  std::fill_n(this->DataSpacing, details::SEP_READER_MAX_DIMENSION, 1.0);
  std::fill_n(
    this->Dimensions, details::SEP_READER_MAX_DIMENSION, details::SEP_READER_MAX_DIMENSION);

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
void vtkSEPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  const auto PrintDataType = [](const DataFormatType type) {
    switch (type)
    {
      case DataFormatType::XDR_DOUBLE:
        return "float32 [double]";
      case DataFormatType::XDR_FLOAT:
        return "float16 [float]";
      case DataFormatType::XDR_INT:
      default:
        return "int32";
    }
  };

  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (!this->FileName.empty() ? this->FileName : "(none)")
     << std::endl;

  os << indent << "Endianness: " << this->Endianness << std::endl;
  os << indent << "DataType: " << PrintDataType(this->DataFormat) << std::endl;
  os << indent << "ESize: " << this->ESize << std::endl;
  os << indent << "DataFileType: " << this->DataFileType << std::endl;
  os << indent << "BinaryFilename: " << this->BinaryFilename << std::endl;
  os << indent << "FixedDimension1ArrayId: " << this->FixedDimension1ArrayId << std::endl;
  os << indent << "FixedDimension2ArrayId: " << this->FixedDimension1ArrayId << std::endl;
  os << indent << "FixedDimension1: " << this->FixedDimension1 << std::endl;
  os << indent << "FixedDimension2: " << this->FixedDimension2 << std::endl;

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", " << this->Dimensions[1] << ", "
     << this->Dimensions[2] << ")" << std::endl;

  os << indent << "DataSpacing: (" << this->DataSpacing[0] << ", " << this->DataSpacing[1] << ", "
     << this->DataSpacing[2] << ")" << std::endl;

  os << indent << "DataOrigin: (" << this->DataOrigin[0] << ", " << this->DataOrigin[1] << ", "
     << this->DataOrigin[2] << ")" << std::endl;

  os << indent << "ExtentSplitMode: " << this->ExtentSplitMode << std::endl;

  os << indent << "Labels: (" << this->Label[0] << ", " << this->Label[1] << ", " << this->Label[2]
     << ")" << std::endl;
}

//----------------------------------------------------------------------------
int vtkSEPReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  this->ReadHeader();

  this->AllDimensions->SetNumberOfValues(this->ESize);
  this->AllRanges->SetNumberOfValues(this->ESize * 2 + 2);
  this->AllRanges->SetValue(0, "Dimension");
  this->AllRanges->SetValue(1, "Size");
  for (int i = 0; i < this->ESize; i++)
  {
    this->AllDimensions->SetValue(i, this->Label[i]);
    this->AllRanges->SetValue((i + 1) * 2, this->Label[i]);
    this->AllRanges->SetValue((i + 1) * 2 + 1, std::to_string(this->Dimensions[i]));
    this->FixedDimRange[1] = std::max(this->FixedDimRange[1], this->Dimensions[i]);
  }

  const auto AssignDimensionId = [this](const std::string& Name) -> int {
    auto iter = std::find(std::begin(this->Label), std::end(this->Label), Name);
    return std::distance(this->Label, iter);
  };

  this->XArrayId = AssignDimensionId(this->XDimension);
  this->YArrayId = AssignDimensionId(this->YDimension);
  this->ZArrayId = AssignDimensionId(this->ZDimension);

  this->FixedDimension1ArrayId = AssignDimensionId(this->FixedDimension1);
  this->FixedDimension2ArrayId = AssignDimensionId(this->FixedDimension2);

  this->OutputSpacing[0] = this->DataSpacing[this->XArrayId];
  this->OutputSpacing[1] = this->DataSpacing[this->YArrayId];
  this->OutputSpacing[2] = this->DataSpacing[this->ZArrayId];

  this->OutputOrigin[0] = this->DataOrigin[this->XArrayId];
  this->OutputOrigin[1] = this->DataOrigin[this->YArrayId];
  this->OutputOrigin[2] = this->DataOrigin[this->ZArrayId];

  auto extent = this->ComputeExtent();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent.data(), 6);
  outInfo->Set(vtkDataObject::SPACING(), this->OutputSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), this->OutputOrigin, 3);
  outInfo->Set(vtkExtentTranslator::UPDATE_SPLIT_MODE(), this->ExtentSplitMode);

  return true;
}

//----------------------------------------------------------------------------
std::array<std::int32_t, 6> vtkSEPReader::ComputeExtent() const
{
  std::array<std::int32_t, 6> extent = {
    0,
    this->Dimensions[this->XArrayId] - 1,
    0,
    this->Dimensions[this->YArrayId] - 1,
    0,
    this->Dimensions[this->ZArrayId] - 1,
  };

  if (this->OutputGridDimension == 2)
  {
    extent[5] = 0;
  }

  return extent;
}

//----------------------------------------------------------------------------
int vtkSEPReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* imageData = vtkImageData::GetData(outputVector, 0);

  int* updateExtent = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  return this->ReadData(imageData, updateExtent);
}

//----------------------------------------------------------------------------
bool vtkSEPReader::ReadHeader()
{
  if (this->FileName.empty())
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    return false;
  }

  // Open the new file
  vtkDebugMacro(<< "Initialize: opening file " << this->FileName);
  vtksys::ifstream file;
  vtksys::SystemTools::Stat_t fs;
  if (!vtksys::SystemTools::Stat(this->FileName, &fs))
  {
#ifdef _WIN32
    file.open(this->FileName.c_str(), ios::in | ios::binary);
#else
    file.open(this->FileName.c_str(), ios::in);
#endif
  }

  if (file.fail())
  {
    vtkErrorMacro(<< "Initialize: Could not open file " << this->FileName);
    return false;
  }

  std::string line;
  while (vtksys::SystemTools::GetLineFromStream(file, line))
  {
    std::vector<std::string> splittedLine = vtksys::SystemTools::SplitString(line, '=');
    if (splittedLine.size() == 2)
    {
      std::string key = splittedLine[0];
      std::string value = splittedLine[1];
      details::TrimString(key);
      details::TrimString(value);
      if (key.length() == 2 && key[0] == 'n')
      {
        this->Dimensions[key[1] - '0' - 1] = atoi(value.c_str());
      }
      else if (key.length() == 2 && key[0] == 'd')
      {
        this->DataSpacing[key[1] - '0' - 1] = atof(value.c_str());
      }
      else if (key.length() == 2 && key[0] == 'o')
      {
        this->DataOrigin[key[1] - '0' - 1] = atof(value.c_str());
      }
      else if (vtksys::SystemTools::StringStartsWith(key.c_str(), "label"))
      {
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
        this->Label[key[5] - '0' - 1] = value;
      }
      else if (key == "esize")
      {
        this->ESize = atoi(value.c_str());
      }
      else if (key == "data_format")
      {
        vtksys::SystemTools::ReplaceString(value, "\"", "");
        if (value == "xdr_float" || value == "native_float")
        {
          this->DataFormat = DataFormatType::XDR_FLOAT;
        }
        else if (value == "xdr_double" || value == "native_double")
        {
          this->DataFormat = DataFormatType::XDR_DOUBLE;
        }
        else if (value == "xdr_int" || value == "native_int")
        {
          this->DataFormat = DataFormatType::XDR_INT;
        }
        if (value.substr(0, 3) == "xdr")
        {
          this->Endianness = details::EndiannessType::SEP_BIG_ENDIAN;
        }
      }
      else if (key == "endian")
      {
        vtksys::SystemTools::ReplaceString(value, "\"", "");
        if (value == "little")
        {
          this->Endianness = details::EndiannessType::SEP_LITTLE_ENDIAN;
        }
        else if (value == "big")
        {
          this->Endianness = details::EndiannessType::SEP_BIG_ENDIAN;
        }
      }
      else if (key == "data_filetype")
      {
        this->DataFileType = value;
      }
      else if (key == "in")
      {
        std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
        if (path.empty())
        {
          this->BinaryFilename = value;
        }
        else
        {
          this->BinaryFilename = path + "/" + value;
        }
      }
    }
  }

  if (this->Label[0].empty())
  {
    vtkWarningMacro("Could not find the 1st dimension Label in "
      << this->FileName << ". Assigning default value " << this->XDimension);
    this->Label[0] = this->XDimension;
  }
  if (this->Label[1].empty())
  {
    vtkWarningMacro("Could not find the 2nd dimension Label in "
      << this->FileName << ". Assigning default value " << this->YDimension);
    this->Label[1] = this->YDimension;
  }

  if (this->OutputGridDimension == 3)
  {
    if (this->Label[2].empty())
    {
      vtkWarningMacro("Could not find the 3rd dimension Label in "
        << this->FileName << ". Assigning default value " << this->ZDimension);
      this->Label[2] = this->ZDimension;
    }
    if (this->Label[3].empty())
    {
      vtkWarningMacro("Could not find the 1st fixed dimension Label in "
        << this->FileName << ". Assigning default value " << this->FixedDimension1);
      this->Label[3] = this->FixedDimension1;
    }
  }
  else
  {
    if (this->Label[2].empty())
    {
      vtkWarningMacro("Could not find the 1st fixed dimension Label in "
        << this->FileName << ". Assigning default value " << this->FixedDimension1);
      this->Label[2] = this->FixedDimension1;
    }
    if (this->Label[3].empty())
    {
      vtkWarningMacro("Could not find the 2nd fixed dimension Label in "
        << this->FileName << ". Assigning default value " << this->FixedDimension2);
      this->Label[3] = this->FixedDimension2;
    }
  }

  for (int i = 4; i < details::SEP_READER_MAX_DIMENSION; ++i)
  {
    if (this->Label[i].empty())
    {
      this->Label[i] = "Dimension " + std::to_string(i + 1);
    }
  }

  file.close();

  return true;
}

//----------------------------------------------------------------------------
bool vtkSEPReader::ReadData(vtkImageData* imageData, int updateExtents[6])
{
  vtkDebugMacro(<< "Read data: opening file " << this->BinaryFilename);

  FILE* dataFile = vtksys::SystemTools::Fopen(this->BinaryFilename, "rb");

  if (!dataFile)
  {
    vtkErrorMacro(<< "Unable to open " << this->BinaryFilename << "!");
    return false;
  }

  vtkSmartPointer<vtkDataArray> scalars;
  switch (this->DataFormat)
  {
    case DataFormatType::XDR_FLOAT:
      scalars = vtkSmartPointer<vtkFloatArray>::New();
      break;
    case DataFormatType::XDR_INT:
      scalars = vtkSmartPointer<vtkIntArray>::New();
      break;
    case DataFormatType::XDR_DOUBLE:
      scalars = vtkSmartPointer<vtkDoubleArray>::New();
      break;
    default:
      vtkErrorMacro(<< "Unknown data type!");
      return false;
  }

  imageData->SetExtent(updateExtents);
  imageData->SetSpacing(this->OutputSpacing);
  imageData->SetOrigin(this->OutputOrigin);

  // compute the image dimensions
  int imgDims[3];
  imageData->GetDimensions(imgDims);

  // get the piece of data corresponding to the update extents
  std::size_t nbPoints = imageData->GetNumberOfPoints();
  char* data =
    new char[nbPoints * details::DataFormatSize[static_cast<std::size_t>(this->DataFormat)]];

  int DimensionsOffset[details::SEP_READER_MAX_DIMENSION];
  uint64_t acc = 1;
  for (int t = 0; t < details::SEP_READER_MAX_DIMENSION; t++)
  {
    DimensionsOffset[t] = acc;
    acc *= this->Dimensions[t];
  }

  const auto GetConstantOffsetValue = [this](const int fixedValue, const int dimensionArrayId) {
    if (details::DimensionIsInRange(fixedValue))
    {
      if (fixedValue >= dimensionArrayId)
      {
        vtkWarningMacro("Value entered for fixed dimension 1 (" + std::to_string(fixedValue) +
          ") is greater than the size of the chosen dimension (" +
          std::to_string(dimensionArrayId) + ").");
        return dimensionArrayId;
      }

      return fixedValue;
    }

    return 0;
  };

  const int fixedValue1 = GetConstantOffsetValue(
    this->FixedDimensionValue1, this->Dimensions[this->FixedDimension1ArrayId]);

  bool enableIDim = (this->XArrayId != this->FixedDimension1ArrayId);
  bool enableJDim =
    (this->YArrayId != this->FixedDimension1ArrayId && this->YArrayId != this->XArrayId);
  bool enableKDim = (this->ZArrayId != this->FixedDimension1ArrayId &&
    this->ZArrayId != this->XArrayId && this->ZArrayId != this->YArrayId);

  int minK = updateExtents[4], maxK = updateExtents[5];
  int fixedValue2 = 0;
  if (this->OutputGridDimension == 2)
  {
    // In 2D, there is no  varying k dimension, but a second fixed scalar
    minK = maxK = 0;
    enableIDim &= this->XArrayId != this->FixedDimension2ArrayId;
    enableJDim &= this->YArrayId != this->FixedDimension2ArrayId;
    enableKDim = false;
    fixedValue2 = GetConstantOffsetValue(
      this->FixedDimensionValue2, this->Dimensions[this->FixedDimension2ArrayId]);
  }

  char* dataPtr = data;
  for (int k = minK; k <= maxK; k++)
  {
    const int kVal = enableKDim ? k : 0;
    for (int j = updateExtents[2]; j <= updateExtents[3]; j++)
    {
      const int jVal = enableJDim ? j : 0;
      for (int i = updateExtents[0]; i <= updateExtents[1]; i++)
      {
        const int iVal = enableIDim ? i : 0;
        const int offset = iVal * DimensionsOffset[this->XArrayId] +
          jVal * DimensionsOffset[this->YArrayId] + kVal * DimensionsOffset[this->ZArrayId] +
          fixedValue1 * DimensionsOffset[this->FixedDimension1ArrayId] +
          fixedValue2 * DimensionsOffset[this->FixedDimension2ArrayId];
        this->ReadDataPiece(dataFile, dataPtr, offset, 1);
      }
    }
  }

  fclose(dataFile);

  // manage the endian
  if (scalars && details::GetEndianessType() != this->Endianness)
  {
    if (this->DataFormat == DataFormatType::XDR_FLOAT ||
      this->DataFormat == DataFormatType::XDR_INT)
    {
      for (std::size_t i = 0; i < nbPoints; i++)
      {
        details::SwapByteOrder4(data + i * 4);
      }
    }
    else if (this->DataFormat == DataFormatType::XDR_DOUBLE)
    {
      for (std::size_t i = 0; i < nbPoints; i++)
      {
        details::SwapByteOrder8(data + i * 8);
      }
    }
  }

  scalars->SetVoidArray(
    data, static_cast<vtkIdType>(nbPoints), 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
  scalars->SetName("ImageScalars");
  imageData->GetPointData()->SetScalars(scalars);

  return true;
}

//----------------------------------------------------------------------------
void vtkSEPReader::ReadDataPiece(FILE* file, char*& dataOutput, vtkIdType offset, vtkIdType range)
{
  std::size_t len = details::DataFormatSize[static_cast<std::size_t>(this->DataFormat)];
  static_cast<void>(fseek(file, static_cast<long int>(offset * len), SEEK_SET));

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

  size_t amountRead = fread(dataOutput, len, range, file);
  static_cast<void>(amountRead);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  dataOutput += range * len;
}

//------------------------------------------------------------------------------
bool vtkSEPReader::CanReadFile(const char* filename)
{
  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(filename);
  return extension == ".H";
}
