//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/io/BOVDataSetReader.h>

#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/FileUtils.h>
#include <viskores/io/internal/Endian.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

namespace
{

enum class DataFormat
{
  Byte,
  Short,
  Integer,
  Float,
  Double
};

enum class Centering
{
  Nodal,
  Zonal
};

struct BOVHeader
{
  BOVHeader()
    : DataSize(0, 0, 0)
    , Origin(0, 0, 0)
    , BrickSize(1, 1, 1)
  {
  }

  std::string DataFile;
  std::string VariableName;
  viskores::Id3 DataSize;
  viskores::Vec3f Origin;
  viskores::Vec3f BrickSize;
  viskores::IdComponent NumComponents = 1;
  viskores::Id ByteOffset = 0;
  DataFormat Format = DataFormat::Float;
  Centering FieldCentering = Centering::Nodal;
  bool HasDataFile = false;
  bool HasDataSize = false;
  bool HasFormat = false;
  bool HasVariable = false;
  bool HasBrickSize = false;
  bool HasEndian = false;
  bool DataIsLittleEndian = false;
};

std::string Trim(const std::string& value)
{
  auto begin = std::find_if_not(
    value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
  auto end = std::find_if_not(
    value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; });

  if (begin >= end.base())
    return std::string();
  return std::string(begin, end.base());
}

std::string ToUpper(const std::string& value)
{
  std::string result = value;
  std::transform(result.begin(),
                 result.end(),
                 result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return result;
}

std::string NormalizeKey(const std::string& key)
{
  std::string normalized;
  bool lastWasSpace = true;
  for (char ch : Trim(key))
  {
    const auto uch = static_cast<unsigned char>(ch);
    if (ch == '_' || std::isspace(uch) != 0)
    {
      if (!lastWasSpace)
      {
        normalized.push_back(' ');
        lastWasSpace = true;
      }
    }
    else
    {
      normalized.push_back(static_cast<char>(std::toupper(uch)));
      lastWasSpace = false;
    }
  }
  return Trim(normalized);
}

std::string ParseStringOption(const std::string& option, const std::string& optionName)
{
  const std::string value = Trim(option);
  if (value.empty())
    throw viskores::io::ErrorIO("Missing value for " + optionName);

  if (value[0] == '"' || value[0] == '\'')
  {
    const char quote = value[0];
    const std::string::size_type endQuote = value.find_last_of(quote);
    if (endQuote == 0)
      throw viskores::io::ErrorIO("Unterminated quoted value for " + optionName);
    return value.substr(1, endQuote - 1);
  }

  std::stringstream stream(value);
  std::string result;
  stream >> result;
  return result;
}

viskores::Id3 ParseId3Option(const std::string& option, const std::string& optionName)
{
  std::stringstream stream(option);
  viskores::Id3 result;
  stream >> result[0] >> result[1] >> result[2];
  if (!stream)
    throw viskores::io::ErrorIO("Invalid value for " + optionName + ": " + Trim(option));
  return result;
}

viskores::Vec3f ParseVec3fOption(const std::string& option, const std::string& optionName)
{
  std::stringstream stream(option);
  viskores::Vec3f result;
  stream >> result[0] >> result[1] >> result[2];
  if (!stream)
    throw viskores::io::ErrorIO("Invalid value for " + optionName + ": " + Trim(option));
  return result;
}

viskores::Id ParseIdOption(const std::string& option, const std::string& optionName)
{
  std::stringstream stream(option);
  viskores::Id result;
  stream >> result;
  if (!stream || result < 0)
    throw viskores::io::ErrorIO("Invalid value for " + optionName + ": " + Trim(option));
  return result;
}

viskores::IdComponent ParseDataComponentsOption(const std::string& option)
{
  const std::string value = ToUpper(Trim(option));
  if (value == "COMPLEX")
    return 2;

  std::stringstream stream(value);
  viskores::Id result;
  stream >> result;
  if (!stream || result < 1)
    throw viskores::io::ErrorIO("Invalid value for DATA_COMPONENTS: " + Trim(option));

  char extra = '\0';
  if (stream >> extra)
    throw viskores::io::ErrorIO("Invalid value for DATA_COMPONENTS: " + Trim(option));

  if (result > std::numeric_limits<viskores::IdComponent>::max())
    throw viskores::io::ErrorIO("DATA_COMPONENTS is too large");

  return static_cast<viskores::IdComponent>(result);
}

DataFormat ParseDataFormat(const std::string& option)
{
  const std::string value = ToUpper(Trim(option));
  if (value.find("BYTE") != std::string::npos)
    return DataFormat::Byte;
  if (value.find("SHORT") != std::string::npos)
    return DataFormat::Short;
  if (value == "INT" || value == "INTS" || value.find("INTEGER") != std::string::npos)
    return DataFormat::Integer;
  if (value.find("FLOAT") != std::string::npos || value.find("REAL") != std::string::npos)
    return DataFormat::Float;
  if (value.find("DOUBLE") != std::string::npos)
    return DataFormat::Double;
  throw viskores::io::ErrorIO("Unsupported DATA_FORMAT: " + Trim(option));
}

Centering ParseCentering(const std::string& option)
{
  const std::string value = ToUpper(Trim(option));
  if (value.find("NODAL") != std::string::npos || value.find("NODE") != std::string::npos ||
      value.find("POINT") != std::string::npos)
    return Centering::Nodal;
  if (value.find("ZONAL") != std::string::npos || value.find("ZONE") != std::string::npos ||
      value.find("CELL") != std::string::npos)
    return Centering::Zonal;
  throw viskores::io::ErrorIO("Unsupported CENTERING: " + Trim(option));
}

bool ParseDataEndianIsLittle(const std::string& option)
{
  const std::string value = ToUpper(Trim(option));
  if (value.find("LITTLE") != std::string::npos)
    return true;
  if (value.find("BIG") != std::string::npos)
    return false;
  throw viskores::io::ErrorIO("Unsupported DATA_ENDIAN: " + Trim(option));
}

bool ParseBoolOption(const std::string& option, const std::string& optionName)
{
  const std::string value = ToUpper(Trim(option));
  if (value == "TRUE" || value == "YES" || value == "ON" || value == "1")
    return true;
  if (value == "FALSE" || value == "NO" || value == "OFF" || value == "0")
    return false;
  throw viskores::io::ErrorIO("Invalid value for " + optionName + ": " + Trim(option));
}

void ValidateHeader(const BOVHeader& header)
{
  if (!header.HasDataFile || header.DataFile.empty())
    throw viskores::io::ErrorIO("Missing required DATA_FILE option");
  if (!header.HasDataSize)
    throw viskores::io::ErrorIO("Missing required DATA_SIZE option");
  if (!header.HasFormat)
    throw viskores::io::ErrorIO("Missing required DATA_FORMAT option");
  if (!header.HasVariable || header.VariableName.empty())
    throw viskores::io::ErrorIO("Missing required VARIABLE option");
  if (header.NumComponents < 1)
    throw viskores::io::ErrorIO("DATA_COMPONENTS must be >= 1");

  for (viskores::IdComponent i = 0; i < 3; ++i)
    if (header.DataSize[i] < 1)
      throw viskores::io::ErrorIO("DATA_SIZE values must be positive");

  if (header.FieldCentering == Centering::Nodal && header.DataSize[0] == 1 &&
      header.DataSize[1] == 1 && header.DataSize[2] == 1)
    throw viskores::io::ErrorIO("NODAL DATA_SIZE must define at least one structured cell axis");
}

BOVHeader ParseHeaderFile(const std::string& fileName)
{
  std::ifstream stream(fileName);
  if (stream.fail())
    throw viskores::io::ErrorIO("Failed to open file: " + fileName);

  BOVHeader header;
  std::string line;
  while (std::getline(stream, line))
  {
    line = Trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    const std::string::size_type pos = line.find(':');
    if (pos == std::string::npos)
      throw viskores::io::ErrorIO("Unsupported option: " + line);

    const std::string token = NormalizeKey(line.substr(0, pos));
    const std::string options = line.substr(pos + 1);

    if (token == "DATA FILE")
    {
      header.DataFile = ParseStringOption(options, "DATA_FILE");
      header.HasDataFile = true;
    }
    else if (token == "DATA SIZE")
    {
      header.DataSize = ParseId3Option(options, "DATA_SIZE");
      header.HasDataSize = true;
    }
    else if (token == "BRICK ORIGIN")
      header.Origin = ParseVec3fOption(options, "BRICK_ORIGIN");
    else if (token == "BRICK SIZE")
    {
      header.BrickSize = ParseVec3fOption(options, "BRICK_SIZE");
      header.HasBrickSize = true;
    }
    else if (token == "DATA FORMAT")
    {
      header.Format = ParseDataFormat(options);
      header.HasFormat = true;
    }
    else if (token == "DATA COMPONENTS")
    {
      header.NumComponents = ParseDataComponentsOption(options);
    }
    else if (token == "VARIABLE")
    {
      header.VariableName = ParseStringOption(options, "VARIABLE");
      header.HasVariable = true;
    }
    else if (token == "DATA ENDIAN")
    {
      header.DataIsLittleEndian = ParseDataEndianIsLittle(options);
      header.HasEndian = true;
    }
    else if (token == "BYTE OFFSET")
      header.ByteOffset = ParseIdOption(options, "BYTE_OFFSET");
    else if (token == "CENTERING")
      header.FieldCentering = ParseCentering(options);
    else if (token == "DIVIDE BRICK" && ParseBoolOption(options, "DIVIDE_BRICK"))
      throw viskores::io::ErrorIO("DIVIDE_BRICK: TRUE is not supported");
  }

  ValidateHeader(header);
  return header;
}

std::string ResolveDataFilePath(const std::string& bovFileName, const std::string& dataFileName)
{
  if (viskores::io::IsAbsolutePath(dataFileName))
    return dataFileName;

  const std::string baseDir = viskores::io::ParentPath(bovFileName);
  if (baseDir.empty())
    return dataFileName;
  return viskores::io::MergePaths(baseDir, dataFileName);
}

viskores::Id Product(const viskores::Id3& dims)
{
  return dims[0] * dims[1] * dims[2];
}

viskores::Id SafeMultiply(viskores::Id lhs, viskores::Id rhs, const std::string& context)
{
  if (lhs < 0 || rhs < 0)
    throw viskores::io::ErrorIO("Negative value while computing " + context);
  if (lhs != 0 && rhs > (std::numeric_limits<viskores::Id>::max() / lhs))
    throw viskores::io::ErrorIO("Overflow while computing " + context);
  return lhs * rhs;
}

viskores::Id3 GetPointDimensions(const BOVHeader& header)
{
  if (header.FieldCentering == Centering::Zonal)
    return viskores::Id3(header.DataSize[0] + 1, header.DataSize[1] + 1, header.DataSize[2] + 1);
  return header.DataSize;
}

viskores::Vec3f GetSpacing(const BOVHeader& header, const viskores::Id3& pointDimensions)
{
  viskores::Vec3f spacing(1, 1, 1);
  if (!header.HasBrickSize)
    return spacing;

  for (viskores::IdComponent i = 0; i < 3; ++i)
  {
    const viskores::Id divisor =
      (header.FieldCentering == Centering::Zonal) ? header.DataSize[i] : (header.DataSize[i] - 1);
    spacing[i] =
      (divisor > 0) ? (header.BrickSize[i] / static_cast<viskores::FloatDefault>(divisor)) : 1.0f;
    if (pointDimensions[i] > 1 && spacing[i] <= 0)
      throw viskores::io::ErrorIO("BRICK_SIZE values must produce positive spacing");
  }

  return spacing;
}

template <typename T>
void ReadBuffer(const std::string& fName,
                const viskores::Id& sz,
                const viskores::Id& byteOffset,
                bool swapEndian,
                std::vector<T>& buff)
{
  const size_t readSize = static_cast<size_t>(sz);
  std::ifstream stream(fName, std::ios::binary);
  if (stream.fail())
    throw viskores::io::ErrorIO("Unable to open data file: " + fName);

  if (byteOffset > 0)
  {
    stream.seekg(static_cast<std::streamoff>(byteOffset), std::ios::beg);
    if (stream.fail())
      throw viskores::io::ErrorIO("Unable to seek data file: " + fName);
  }

  buff.resize(readSize);
  stream.read(reinterpret_cast<char*>(buff.data()),
              static_cast<std::streamsize>(sizeof(T) * readSize));
  if (stream.fail())
    throw viskores::io::ErrorIO("Data file read failed: " + fName);

  if (swapEndian && sizeof(T) > 1)
    viskores::io::internal::FlipEndianness(buff);
}

template <typename T>
void ReadScalar(const std::string& fName,
                const viskores::Id& nTuples,
                const viskores::Id& byteOffset,
                bool swapEndian,
                viskores::cont::ArrayHandle<T>& var)
{
  std::vector<T> buff;
  ReadBuffer(fName, nTuples, byteOffset, swapEndian, buff);
  var.Allocate(nTuples);
  auto writePortal = var.WritePortal();
  for (viskores::Id i = 0; i < nTuples; i++)
    writePortal.Set(i, buff[static_cast<size_t>(i)]);
}

template <typename T>
void ReadVector(const std::string& fName,
                const viskores::Id& nTuples,
                viskores::IdComponent numComponents,
                const viskores::Id& byteOffset,
                bool swapEndian,
                viskores::cont::ArrayHandle<viskores::Vec<T, 2>>& var)
{
  if (numComponents != 2)
    throw viskores::io::ErrorIO("Internal error: invalid Vec2 component count");

  std::vector<T> buff;
  ReadBuffer(fName, SafeMultiply(nTuples, 2, "BOV Vec2 tuple count"), byteOffset, swapEndian, buff);

  var.Allocate(nTuples);
  auto writePortal = var.WritePortal();
  for (viskores::Id i = 0; i < nTuples; i++)
  {
    writePortal.Set(
      i,
      viskores::Vec<T, 2>(buff[static_cast<size_t>(i * 2)], buff[static_cast<size_t>(i * 2 + 1)]));
  }
}

template <typename T>
void ReadVector(const std::string& fName,
                const viskores::Id& nTuples,
                viskores::IdComponent numComponents,
                const viskores::Id& byteOffset,
                bool swapEndian,
                viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& var)
{
  if (numComponents != 3)
    throw viskores::io::ErrorIO("Internal error: invalid Vec3 component count");

  std::vector<T> buff;
  ReadBuffer(fName, SafeMultiply(nTuples, 3, "BOV Vec3 tuple count"), byteOffset, swapEndian, buff);

  var.Allocate(nTuples);
  auto writePortal = var.WritePortal();
  for (viskores::Id i = 0; i < nTuples; i++)
  {
    writePortal.Set(i,
                    viskores::Vec<T, 3>(buff[static_cast<size_t>(i * 3)],
                                        buff[static_cast<size_t>(i * 3 + 1)],
                                        buff[static_cast<size_t>(i * 3 + 2)]));
  }
}

template <typename T>
void ReadRuntimeVector(const std::string& fName,
                       const viskores::Id& nTuples,
                       viskores::IdComponent numComponents,
                       const viskores::Id& byteOffset,
                       bool swapEndian,
                       viskores::cont::ArrayHandleRuntimeVec<T>& var)
{
  if (numComponents < 1)
    throw viskores::io::ErrorIO("DATA_COMPONENTS must be >= 1");

  std::vector<T> buff;
  ReadBuffer(
    fName,
    SafeMultiply(nTuples, static_cast<viskores::Id>(numComponents), "BOV runtime tuple count"),
    byteOffset,
    swapEndian,
    buff);
  var = viskores::cont::make_ArrayHandleRuntimeVecMove(numComponents, std::move(buff));
}

template <typename ArrayHandleType>
void AddField(viskores::cont::DataSet& dataSet, const BOVHeader& header, const ArrayHandleType& var)
{
  if (header.FieldCentering == Centering::Zonal)
    dataSet.AddCellField(header.VariableName, var);
  else
    dataSet.AddPointField(header.VariableName, var);
}

template <typename T>
void ReadAndAddScalarField(viskores::cont::DataSet& dataSet,
                           const BOVHeader& header,
                           const std::string& dataFileName,
                           viskores::Id numTuples,
                           bool swapEndian)
{
  viskores::cont::ArrayHandle<T> var;
  ReadScalar(dataFileName, numTuples, header.ByteOffset, swapEndian, var);
  AddField(dataSet, header, var);
}

template <typename T, viskores::IdComponent NUM_COMPONENTS>
void ReadAndAddVectorField(viskores::cont::DataSet& dataSet,
                           const BOVHeader& header,
                           const std::string& dataFileName,
                           viskores::Id numTuples,
                           bool swapEndian)
{
  viskores::cont::ArrayHandle<viskores::Vec<T, NUM_COMPONENTS>> var;
  ReadVector(dataFileName, numTuples, header.NumComponents, header.ByteOffset, swapEndian, var);
  AddField(dataSet, header, var);
}

template <typename T>
void ReadAndAddRuntimeVectorField(viskores::cont::DataSet& dataSet,
                                  const BOVHeader& header,
                                  const std::string& dataFileName,
                                  viskores::Id numTuples,
                                  bool swapEndian)
{
  viskores::cont::ArrayHandleRuntimeVec<T> var;
  ReadRuntimeVector(
    dataFileName, numTuples, header.NumComponents, header.ByteOffset, swapEndian, var);
  AddField(dataSet, header, var);
}

void ReadAndAddField(viskores::cont::DataSet& dataSet,
                     const BOVHeader& header,
                     const std::string& dataFileName)
{
  const viskores::Id numTuples = Product(header.DataSize);
  const bool swapEndian =
    header.HasEndian && (header.DataIsLittleEndian != viskores::io::internal::IsLittleEndian());

  if (header.NumComponents == 1)
  {
    switch (header.Format)
    {
      case DataFormat::Byte:
        ReadAndAddScalarField<viskores::UInt8>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Short:
        ReadAndAddScalarField<viskores::Int16>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Integer:
        ReadAndAddScalarField<viskores::Int32>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Float:
        ReadAndAddScalarField<viskores::Float32>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Double:
        ReadAndAddScalarField<viskores::Float64>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
    }
  }
  else if (header.NumComponents == 2)
  {
    switch (header.Format)
    {
      case DataFormat::Byte:
        ReadAndAddVectorField<viskores::UInt8, 2>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Short:
        ReadAndAddVectorField<viskores::Int16, 2>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Integer:
        ReadAndAddVectorField<viskores::Int32, 2>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Float:
        ReadAndAddVectorField<viskores::Float32, 2>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Double:
        ReadAndAddVectorField<viskores::Float64, 2>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
    }
  }
  else if (header.NumComponents == 3)
  {
    switch (header.Format)
    {
      case DataFormat::Byte:
        ReadAndAddVectorField<viskores::UInt8, 3>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Short:
        ReadAndAddVectorField<viskores::Int16, 3>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Integer:
        ReadAndAddVectorField<viskores::Int32, 3>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Float:
        ReadAndAddVectorField<viskores::Float32, 3>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Double:
        ReadAndAddVectorField<viskores::Float64, 3>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
    }
  }
  else
  {
    switch (header.Format)
    {
      case DataFormat::Byte:
        ReadAndAddRuntimeVectorField<viskores::UInt8>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Short:
        ReadAndAddRuntimeVectorField<viskores::Int16>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Integer:
        ReadAndAddRuntimeVectorField<viskores::Int32>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Float:
        ReadAndAddRuntimeVectorField<viskores::Float32>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
      case DataFormat::Double:
        ReadAndAddRuntimeVectorField<viskores::Float64>(
          dataSet, header, dataFileName, numTuples, swapEndian);
        break;
    }
  }
}

} // anonymous namespace

namespace viskores
{
namespace io
{

BOVDataSetReader::BOVDataSetReader(const char* fileName)
  : FileName(fileName)
  , Loaded(false)
  , DataSet()
{
}

BOVDataSetReader::BOVDataSetReader(const std::string& fileName)
  : FileName(fileName)
  , Loaded(false)
  , DataSet()
{
}

const viskores::cont::DataSet& BOVDataSetReader::ReadDataSet()
{
  try
  {
    this->LoadFile();
  }
  catch (std::ifstream::failure& e)
  {
    std::string message("IO Error: ");
    throw viskores::io::ErrorIO(message + e.what());
  }
  return this->DataSet;
}

void BOVDataSetReader::LoadFile()
{
  if (this->Loaded)
    return;

  const BOVHeader header = ParseHeaderFile(this->FileName);
  const viskores::Id3 pointDimensions = GetPointDimensions(header);
  const viskores::Vec3f spacing = GetSpacing(header, pointDimensions);
  const std::string fullPathDataFile = ResolveDataFilePath(this->FileName, header.DataFile);

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  this->DataSet = dataSetBuilder.Create(pointDimensions, header.Origin, spacing);
  ReadAndAddField(this->DataSet, header, fullPathDataFile);

  this->Loaded = true;
}
}
} // namespace viskores::io
