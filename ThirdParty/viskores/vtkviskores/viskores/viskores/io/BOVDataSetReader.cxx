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

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/io/ErrorIO.h>

#include <fstream>
#include <sstream>

namespace
{

enum class DataFormat
{
  ByteData,
  ShortData,
  IntegerData,
  FloatData,
  DoubleData
};

template <typename T>
void ReadBuffer(const std::string& fName, const viskores::Id& sz, std::vector<T>& buff)
{
  FILE* fp = fopen(fName.c_str(), "rb");
  size_t readSize = static_cast<size_t>(sz);
  if (fp == nullptr)
  {
    throw viskores::io::ErrorIO("Unable to open data file: " + fName);
  }
  buff.resize(readSize);
  size_t nread = fread(&buff[0], sizeof(T), readSize, fp);
  if (nread != readSize)
  {
    throw viskores::io::ErrorIO("Data file read failed: " + fName);
  }
  fclose(fp);
}

template <typename T>
void ReadScalar(const std::string& fName,
                const viskores::Id& nTuples,
                viskores::cont::ArrayHandle<T>& var)
{
  std::vector<T> buff;
  ReadBuffer(fName, nTuples, buff);
  var.Allocate(nTuples);
  auto writePortal = var.WritePortal();
  for (viskores::Id i = 0; i < nTuples; i++)
  {
    writePortal.Set(i, buff[static_cast<size_t>(i)]);
  }
}

template <typename T>
void ReadVector(const std::string& fName,
                const viskores::Id& nTuples,
                viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& var)
{
  std::vector<T> buff;
  ReadBuffer(fName, nTuples * 3, buff);

  var.Allocate(nTuples);
  viskores::Vec<T, 3> v;
  auto writePortal = var.WritePortal();
  for (viskores::Id i = 0; i < nTuples; i++)
  {
    v[0] = buff[static_cast<size_t>(i * 3 + 0)];
    v[1] = buff[static_cast<size_t>(i * 3 + 1)];
    v[2] = buff[static_cast<size_t>(i * 3 + 2)];
    writePortal.Set(i, v);
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

  std::ifstream stream(this->FileName);
  if (stream.fail())
    throw viskores::io::ErrorIO("Failed to open file: " + this->FileName);

  DataFormat dataFormat = DataFormat::ByteData;
  std::string bovFile, line, token, options, variableName;
  viskores::Id numComponents = 1;
  viskores::Id3 dim;
  viskores::Vec3f origin(0, 0, 0);
  viskores::Vec3f spacing(1, 1, 1);
  bool spacingSet = false;

  while (stream.good())
  {
    std::getline(stream, line);
    if (line.size() == 0 || line[0] == '#')
      continue;
    //std::cout<<"::"<<line<<"::"<<std::endl;
    std::size_t pos = line.find(":");
    if (pos == std::string::npos)
      throw viskores::io::ErrorIO("Unsupported option: " + line);
    token = line.substr(0, pos);
    options = line.substr(pos + 1, line.size() - 1);
    //std::cout<<token<<"::"<<options<<std::endl;

    std::stringstream strStream(options);

    //Format supports both space and "_" separated tokens...
    if (token.find("DATA") != std::string::npos && token.find("FILE") != std::string::npos)
    {
      strStream >> bovFile >> std::ws;
    }
    else if (token.find("DATA") != std::string::npos && token.find("SIZE") != std::string::npos)
    {
      strStream >> dim[0] >> dim[1] >> dim[2] >> std::ws;
    }
    else if (token.find("BRICK") != std::string::npos && token.find("ORIGIN") != std::string::npos)
    {
      strStream >> origin[0] >> origin[1] >> origin[2] >> std::ws;
    }

    //DRP
    else if (token.find("BRICK") != std::string::npos && token.find("SIZE") != std::string::npos)
    {
      strStream >> spacing[0] >> spacing[1] >> spacing[2] >> std::ws;
      spacingSet = true;
    }
    else if (token.find("DATA") != std::string::npos && token.find("FORMAT") != std::string::npos)
    {
      std::string opt;
      strStream >> opt >> std::ws;
      if (opt.find("FLOAT") != std::string::npos || opt.find("REAL") != std::string::npos)
        dataFormat = DataFormat::FloatData;
      else if (opt.find("DOUBLE") != std::string::npos)
        dataFormat = DataFormat::DoubleData;
      else
        throw viskores::io::ErrorIO("Unsupported data type: " + token);
    }
    else if (token.find("DATA") != std::string::npos &&
             token.find("COMPONENTS") != std::string::npos)
    {
      strStream >> numComponents >> std::ws;
      if (numComponents != 1 && numComponents != 3)
        throw viskores::io::ErrorIO("Unsupported number of components");
    }
    else if (token.find("VARIABLE") != std::string::npos &&
             token.find("PALETTE") == std::string::npos)
    {
      strStream >> variableName >> std::ws;
      if (variableName[0] == '"')
        variableName = variableName.substr(1, variableName.size() - 2);
    }
  }

  if (spacingSet)
  {
    spacing[0] = (spacing[0]) / static_cast<viskores::FloatDefault>(dim[0] - 1);
    spacing[1] = (spacing[1]) / static_cast<viskores::FloatDefault>(dim[1] - 1);
    spacing[2] = (spacing[2]) / static_cast<viskores::FloatDefault>(dim[2] - 1);
  }

  std::string fullPathDataFile;
  std::size_t pos = FileName.rfind("/");
  if (pos != std::string::npos)
  {
    std::string baseDir;
    baseDir = this->FileName.substr(0, pos);
    fullPathDataFile = baseDir + "/" + bovFile;
  }
  else
    fullPathDataFile = bovFile;


  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  this->DataSet = dataSetBuilder.Create(dim, origin, spacing);

  viskores::Id numTuples = dim[0] * dim[1] * dim[2];
  if (numComponents == 1)
  {
    if (dataFormat == DataFormat::FloatData)
    {
      viskores::cont::ArrayHandle<viskores::Float32> var;
      ReadScalar(fullPathDataFile, numTuples, var);
      this->DataSet.AddPointField(variableName, var);
    }
    else if (dataFormat == DataFormat::DoubleData)
    {
      viskores::cont::ArrayHandle<viskores::Float64> var;
      ReadScalar(fullPathDataFile, numTuples, var);
      this->DataSet.AddPointField(variableName, var);
    }
  }
  else if (numComponents == 3)
  {
    if (dataFormat == DataFormat::FloatData)
    {
      viskores::cont::ArrayHandle<viskores::Vec3f_32> var;
      ReadVector(fullPathDataFile, numTuples, var);
      this->DataSet.AddPointField(variableName, var);
    }
    else if (dataFormat == DataFormat::DoubleData)
    {
      viskores::cont::ArrayHandle<viskores::Vec3f_64> var;
      ReadVector(fullPathDataFile, numTuples, var);
      this->DataSet.AddPointField(variableName, var);
    }
  }

  this->Loaded = true;
}
}
} // namespace viskores::io
