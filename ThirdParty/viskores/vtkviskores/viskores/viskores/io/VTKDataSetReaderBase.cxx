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

#include <viskores/io/VTKDataSetReaderBase.h>

#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleOffsetsToNumComponents.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{

inline void PrintVTKDataFileSummary(const viskores::io::internal::VTKDataSetFile& df,
                                    std::ostream& out)
{
  out << "\tFile: " << df.FileName << std::endl;
  out << "\tVersion: " << df.Version[0] << "." << df.Version[0] << std::endl;
  out << "\tTitle: " << df.Title << std::endl;
  out << "\tFormat: " << (df.IsBinary ? "BINARY" : "ASCII") << std::endl;
  out << "\tDataSet type: " << viskores::io::internal::DataSetStructureString(df.Structure)
      << std::endl;
}

} // anonymous namespace

namespace viskores
{
namespace io
{

VTKDataSetReaderBase::VTKDataSetReaderBase(const char* fileName)
  : DataFile(new internal::VTKDataSetFile)
  , DataSet()
  , Loaded(false)
{
  this->DataFile->FileName = fileName;
}

VTKDataSetReaderBase::VTKDataSetReaderBase(const std::string& fileName)
  : DataFile(new internal::VTKDataSetFile)
  , DataSet()
  , Loaded(false)
{
  this->DataFile->FileName = fileName;
}

VTKDataSetReaderBase::~VTKDataSetReaderBase() {}

const viskores::cont::DataSet& VTKDataSetReaderBase::ReadDataSet()
{
  if (!this->Loaded)
  {
    try
    {
      this->OpenFile();
      this->ReadHeader();
      this->Read();
      this->CloseFile();
      this->Loaded = true;
    }
    catch (std::ifstream::failure& e)
    {
      std::string message("IO Error: ");
      throw viskores::io::ErrorIO(message + e.what());
    }
  }

  return this->DataSet;
}

void VTKDataSetReaderBase::PrintSummary(std::ostream& out) const
{
  out << "VTKDataSetReader" << std::endl;
  PrintVTKDataFileSummary(*this->DataFile.get(), out);
  this->DataSet.PrintSummary(out);
}

void VTKDataSetReaderBase::ReadPoints()
{
  std::string dataType;
  std::size_t numPoints;
  this->DataFile->Stream >> numPoints >> dataType >> std::ws;

  viskores::cont::UnknownArrayHandle points =
    this->DoReadArrayVariant(viskores::cont::Field::Association::Points, dataType, numPoints, 3);

  this->DataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", points));
}

void VTKDataSetReaderBase::ReadCells(viskores::cont::ArrayHandle<viskores::Id>& connectivity,
                                     viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices)
{
  if (this->DataFile->Version[0] < 5)
  {
    viskores::Id numCells, numInts;
    this->DataFile->Stream >> numCells >> numInts >> std::ws;

    connectivity.Allocate(numInts - numCells);
    numIndices.Allocate(numCells);

    std::vector<viskores::Int32> buffer(static_cast<std::size_t>(numInts));
    this->ReadArray(buffer);

    viskores::Int32* buffp = buffer.data();
    auto connectivityPortal = connectivity.WritePortal();
    auto numIndicesPortal = numIndices.WritePortal();
    for (viskores::Id i = 0, connInd = 0; i < numCells; ++i)
    {
      viskores::IdComponent numInds = static_cast<viskores::IdComponent>(*buffp++);
      numIndicesPortal.Set(i, numInds);
      for (viskores::IdComponent j = 0; j < numInds; ++j, ++connInd)
      {
        connectivityPortal.Set(connInd, static_cast<viskores::Id>(*buffp++));
      }
    }
  }
  else
  {
    viskores::Id offsetsSize, connSize;
    this->DataFile->Stream >> offsetsSize >> connSize >> std::ws;

    std::string tag, dataType;
    this->DataFile->Stream >> tag >> dataType >> std::ws;
    internal::parseAssert(tag == "OFFSETS");
    auto offsets =
      this->DoReadArrayVariant(viskores::cont::Field::Association::Any, dataType, offsetsSize, 1);
    offsets.CastAndCallForTypes<viskores::List<viskores::Int64, viskores::Int32>,
                                viskores::List<viskores::cont::StorageTagBasic>>(
      [&](const auto& offsetsAH)
      {
        // Convert on host. There will be several other passes of this array on the host anyway.
        numIndices.Allocate(offsetsSize - 1);
        auto offsetPortal = offsetsAH.ReadPortal();
        auto numIndicesPortal = numIndices.WritePortal();
        for (viskores::Id cellIndex = 0; cellIndex < offsetsSize - 1; ++cellIndex)
        {
          numIndicesPortal.Set(cellIndex,
                               static_cast<viskores::IdComponent>(offsetPortal.Get(cellIndex + 1) -
                                                                  offsetPortal.Get(cellIndex)));
        }
      });

    this->DataFile->Stream >> tag >> dataType >> std::ws;
    internal::parseAssert(tag == "CONNECTIVITY");
    auto conn =
      this->DoReadArrayVariant(viskores::cont::Field::Association::Any, dataType, connSize, 1);
    viskores::cont::ArrayCopyShallowIfPossible(conn, connectivity);
  }
}

void VTKDataSetReaderBase::ReadShapes(viskores::cont::ArrayHandle<viskores::UInt8>& shapes)
{
  std::string tag;
  viskores::Id numCells;
  this->DataFile->Stream >> tag >> numCells >> std::ws;
  internal::parseAssert(tag == "CELL_TYPES");

  shapes.Allocate(numCells);
  std::vector<viskores::Int32> buffer(static_cast<std::size_t>(numCells));
  this->ReadArray(buffer);

  viskores::Int32* buffp = buffer.data();
  auto shapesPortal = shapes.WritePortal();
  for (viskores::Id i = 0; i < numCells; ++i)
  {
    shapesPortal.Set(i, static_cast<viskores::UInt8>(*buffp++));
  }
}

void VTKDataSetReaderBase::ReadAttributes()
{
  if (this->DataFile->Stream.eof())
  {
    return;
  }

  viskores::cont::Field::Association association = viskores::cont::Field::Association::Any;
  std::size_t size;

  std::string tag;
  this->DataFile->Stream >> tag;
  while (!this->DataFile->Stream.eof())
  {
    if (tag == "POINT_DATA")
    {
      association = viskores::cont::Field::Association::Points;
    }
    else if (tag == "CELL_DATA")
    {
      association = viskores::cont::Field::Association::Cells;
    }
    else if (tag == "FIELD") // can see field in this position also
    {
      this->ReadGlobalFields(nullptr);
      this->DataFile->Stream >> tag;
      continue;
    }
    else
    {
      internal::parseAssert(false);
    }

    this->DataFile->Stream >> size;
    while (!this->DataFile->Stream.eof())
    {
      this->DataFile->Stream >> tag;
      if (tag == "SCALARS")
      {
        this->ReadScalars(association, size);
      }
      else if (tag == "COLOR_SCALARS")
      {
        this->ReadColorScalars(association, size);
      }
      else if (tag == "LOOKUP_TABLE")
      {
        this->ReadLookupTable();
      }
      else if (tag == "VECTORS" || tag == "NORMALS")
      {
        this->ReadVectors(association, size);
      }
      else if (tag == "TEXTURE_COORDINATES")
      {
        this->ReadTextureCoordinates(association, size);
      }
      else if (tag == "TENSORS")
      {
        this->ReadTensors(association, size);
      }
      else if (tag == "FIELD")
      {
        this->ReadFields(association, size);
      }
      else if (tag == "GLOBAL_IDS" || tag == "PEDIGREE_IDS")
      {
        this->ReadGlobalOrPedigreeIds(association, size);
      }
      else
      {
        break;
      }
    }
  }
}

void VTKDataSetReaderBase::CloseFile()
{
  this->DataFile->Stream.close();
}

void VTKDataSetReaderBase::OpenFile()
{
  this->DataFile->Stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    this->DataFile->Stream.open(this->DataFile->FileName.c_str(),
                                std::ios_base::in | std::ios_base::binary);
  }
  catch (std::ifstream::failure&)
  {
    std::string message("could not open file \"" + this->DataFile->FileName + "\"");
    throw viskores::io::ErrorIO(message);
  }
}

void VTKDataSetReaderBase::ReadHeader()
{
  char vstring[] = "# vtk DataFile Version";
  const std::size_t vlen = sizeof(vstring);

  // Read version line
  char vbuf[vlen];
  this->DataFile->Stream.read(vbuf, vlen - 1);
  vbuf[vlen - 1] = '\0';
  if (std::string(vbuf) != std::string(vstring))
  {
    throw viskores::io::ErrorIO("Incorrect file format.");
  }

  char dot;
  this->DataFile->Stream >> this->DataFile->Version[0] >> dot >> this->DataFile->Version[1];
  // skip rest of the line
  std::string skip;
  std::getline(this->DataFile->Stream, skip);

  if ((this->DataFile->Version[0] > 4) ||
      (this->DataFile->Version[0] == 4 && this->DataFile->Version[1] > 2))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "Reader may not correctly read >v4.2 files. Reading version "
                     << this->DataFile->Version[0] << "." << this->DataFile->Version[1] << ".\n");
  }

  // Read title line
  std::getline(this->DataFile->Stream, this->DataFile->Title);

  // Read format line
  this->DataFile->IsBinary = false;
  std::string format;
  this->DataFile->Stream >> format >> std::ws;
  if (format == "BINARY")
  {
    this->DataFile->IsBinary = true;
  }
  else if (format != "ASCII")
  {
    throw viskores::io::ErrorIO("Unsupported Format.");
  }

  // Read structure line
  std::string tag, structStr;
  this->DataFile->Stream >> tag >> structStr >> std::ws;
  internal::parseAssert(tag == "DATASET");

  this->DataFile->Structure = viskores::io::internal::DataSetStructureId(structStr);
  if (this->DataFile->Structure == viskores::io::internal::DATASET_UNKNOWN)
  {
    throw viskores::io::ErrorIO("Unsupported DataSet type.");
  }
}


void VTKDataSetReaderBase::AddField(const std::string& name,
                                    viskores::cont::Field::Association association,
                                    viskores::cont::UnknownArrayHandle& data)
{
  if (data.GetNumberOfValues() > 0)
  {
    switch (association)
    {
      case viskores::cont::Field::Association::Points:
      case viskores::cont::Field::Association::WholeDataSet:
        this->DataSet.AddField(viskores::cont::Field(name, association, data));
        break;
      case viskores::cont::Field::Association::Cells:
        this->DataSet.AddField(viskores::cont::Field(name, association, data));
        break;
      default:
        VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                       "Not recording field '" << name
                                               << "' because it has an unknown association");
        break;
    }
  }
}

void VTKDataSetReaderBase::ReadScalars(viskores::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName, dataType, lookupTableName;
  viskores::IdComponent numComponents = 1;
  this->DataFile->Stream >> dataName >> dataType;
  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag != "LOOKUP_TABLE")
  {
    try
    {
      numComponents = std::stoi(tag);
    }
    catch (std::invalid_argument&)
    {
      internal::parseAssert(false);
    }
    this->DataFile->Stream >> tag;
  }

  internal::parseAssert(tag == "LOOKUP_TABLE");
  this->DataFile->Stream >> lookupTableName >> std::ws;

  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadColorScalars(viskores::cont::Field::Association association,
                                            std::size_t numElements)
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                 "Support for COLOR_SCALARS is not implemented. Skipping.");

  std::string dataName;
  viskores::IdComponent numComponents;
  this->DataFile->Stream >> dataName >> numComponents >> std::ws;
  std::string dataType = this->DataFile->IsBinary ? "unsigned_char" : "float";
  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadLookupTable()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                 "Support for LOOKUP_TABLE is not implemented. Skipping.");

  std::string dataName;
  std::size_t numEntries;
  this->DataFile->Stream >> dataName >> numEntries >> std::ws;
  this->SkipArray(numEntries, viskores::Vec<viskores::io::internal::ColorChannel8, 4>());
}

void VTKDataSetReaderBase::ReadTextureCoordinates(viskores::cont::Field::Association association,
                                                  std::size_t numElements)
{
  std::string dataName;
  viskores::IdComponent numComponents;
  std::string dataType;
  this->DataFile->Stream >> dataName >> numComponents >> dataType >> std::ws;

  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadVectors(viskores::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;

  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, 3);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadTensors(viskores::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;

  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, 9);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadFields(viskores::cont::Field::Association association,
                                      std::size_t expectedNumElements)
{
  std::string dataName;
  viskores::Id numArrays;
  this->DataFile->Stream >> dataName >> numArrays >> std::ws;
  for (viskores::Id i = 0; i < numArrays; ++i)
  {
    std::size_t numTuples;
    viskores::IdComponent numComponents;
    std::string arrayName, dataType;
    this->DataFile->Stream >> arrayName >> numComponents >> numTuples >> dataType >> std::ws;
    if (numTuples == expectedNumElements)
    {
      viskores::cont::UnknownArrayHandle data =
        this->DoReadArrayVariant(association, dataType, numTuples, numComponents);
      this->AddField(arrayName, association, data);
    }
    else
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                     "Field " << arrayName
                              << "'s size does not match expected number of elements. Skipping");
    }
  }
}

void VTKDataSetReaderBase::ReadGlobalFields(std::vector<viskores::Float32>* visitBounds)
{
  std::string dataName;
  viskores::Id numArrays;
  this->DataFile->Stream >> dataName >> numArrays >> std::ws;
  for (viskores::Id i = 0; i < numArrays; ++i)
  {
    std::size_t numTuples;
    viskores::IdComponent numComponents;
    std::string arrayName, dataType;
    this->DataFile->Stream >> arrayName >> numComponents >> numTuples >> dataType >> std::ws;
    if (arrayName == "avtOriginalBounds" && visitBounds)
    {
      visitBounds->resize(6);
      internal::parseAssert(numComponents == 1 && numTuples == 6);
      // parse the bounds and fill the bounds vector
      this->ReadArray(*visitBounds);
    }
    else
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Support for global field " << arrayName << " not implemented. Skipping.");
      this->DoSkipArrayVariant(dataType, numTuples, numComponents);
    }
  }
}

void VTKDataSetReaderBase::ReadGlobalOrPedigreeIds(viskores::cont::Field::Association association,
                                                   std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;
  internal::parseAssert(dataType == "vtkIdType");
  // vtk writes vtkIdType as int

  viskores::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, "int", numElements, 1);
  this->AddField(dataName, association, data);

  this->SkipArrayMetaData(1);
}

class VTKDataSetReaderBase::SkipArrayVariant
{
public:
  SkipArrayVariant(VTKDataSetReaderBase* reader,
                   std::size_t numElements,
                   viskores::IdComponent numComponents)
    : Reader(reader)
    , TotalSize(numElements * static_cast<std::size_t>(numComponents))
  {
  }

  template <typename T>
  void operator()(T) const
  {
    this->Reader->SkipArray(this->TotalSize, T());
  }

protected:
  VTKDataSetReaderBase* Reader;
  std::size_t TotalSize;
};

class VTKDataSetReaderBase::ReadArrayVariant : public SkipArrayVariant
{
public:
  ReadArrayVariant(VTKDataSetReaderBase* reader,
                   viskores::cont::Field::Association association,
                   std::size_t numElements,
                   viskores::IdComponent numComponents,
                   viskores::cont::UnknownArrayHandle& data)
    : SkipArrayVariant(reader, numElements, numComponents)
    , Association(association)
    , NumComponents(numComponents)
    , Data(&data)
  {
  }

  template <typename T>
  void operator()(T) const
  {
    std::vector<T> buffer(this->TotalSize);
    this->Reader->ReadArray(buffer);
    if ((this->Association != viskores::cont::Field::Association::Cells) ||
        (this->Reader->GetCellsPermutation().GetNumberOfValues() < 1))
    {
      *this->Data =
        viskores::cont::make_ArrayHandleRuntimeVecMove(this->NumComponents, std::move(buffer));
    }
    else
    {
      // If we are reading data associated with a cell set, we need to (sometimes) permute the
      // data due to differences between VTK and Viskores cell shapes.
      auto permutation = this->Reader->GetCellsPermutation().ReadPortal();
      viskores::Id outSize = permutation.GetNumberOfValues();
      std::vector<T> permutedBuffer(static_cast<std::size_t>(outSize));
      for (viskores::Id outIndex = 0; outIndex < outSize; outIndex++)
      {
        std::size_t inIndex = static_cast<std::size_t>(permutation.Get(outIndex));
        permutedBuffer[static_cast<std::size_t>(outIndex)] = buffer[inIndex];
      }
      *this->Data = viskores::cont::make_ArrayHandleRuntimeVecMove(this->NumComponents,
                                                                   std::move(permutedBuffer));
    }
  }

private:
  viskores::cont::Field::Association Association;
  viskores::IdComponent NumComponents;
  viskores::cont::UnknownArrayHandle* Data;
};

void VTKDataSetReaderBase::DoSkipArrayVariant(std::string dataType,
                                              std::size_t numElements,
                                              viskores::IdComponent numComponents)
{
  // string requires some special handling
  if (dataType == "string" || dataType == "utf8_string")
  {
    const viskores::Id stringCount = numComponents * static_cast<viskores::Id>(numElements);
    this->SkipStringArray(stringCount);
  }
  else
  {
    viskores::io::internal::DataType typeId = viskores::io::internal::DataTypeId(dataType);
    viskores::io::internal::SelectTypeAndCall(typeId,
                                              SkipArrayVariant(this, numElements, numComponents));
  }
}

viskores::cont::UnknownArrayHandle VTKDataSetReaderBase::DoReadArrayVariant(
  viskores::cont::Field::Association association,
  std::string dataType,
  std::size_t numElements,
  viskores::IdComponent numComponents)
{
  // Create empty data to start so that the return can check if data were actually read
  viskores::cont::ArrayHandle<viskores::Float32> empty;
  viskores::cont::UnknownArrayHandle data(empty);

  // string requires some special handling
  if (dataType == "string" || dataType == "utf8_string")
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Warn,
      "Support for data type 'string' and 'utf8_string' is not implemented. Skipping.");
    const viskores::Id stringCount = numComponents * static_cast<viskores::Id>(numElements);
    this->SkipStringArray(stringCount);
  }
  else
  {
    viskores::io::internal::DataType typeId = viskores::io::internal::DataTypeId(dataType);
    viskores::io::internal::SelectTypeAndCall(
      typeId, ReadArrayVariant(this, association, numElements, numComponents, data));
  }

  return data;
}

void VTKDataSetReaderBase::ReadArray(std::vector<viskores::io::internal::DummyBitType>& buffer)
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                 "Support for data type 'bit' is not implemented. Skipping.");
  this->SkipArray(buffer.size(), viskores::io::internal::DummyBitType());
  buffer.clear();
}

void VTKDataSetReaderBase::SkipArray(std::size_t numElements,
                                     viskores::io::internal::DummyBitType,
                                     viskores::IdComponent numComponents)
{
  if (this->DataFile->IsBinary)
  {
    numElements = (numElements + 7) / 8;
    this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements), std::ios_base::cur);
  }
  else
  {
    for (std::size_t i = 0; i < numElements; ++i)
    {
      viskores::UInt16 val;
      this->DataFile->Stream >> val;
    }
  }
  this->DataFile->Stream >> std::ws;
  this->SkipArrayMetaData(numComponents);
}

void VTKDataSetReaderBase::SkipStringArray(std::size_t numStrings)
{
  if (this->DataFile->IsBinary)
  {
    for (std::size_t i = 0; i < numStrings; ++i)
    {
      auto firstByte = this->DataFile->Stream.peek();
      auto type = firstByte >> 6;
      switch (type)
      {
        case 3: // length stored in 1 byte
        {
          auto length = this->DataFile->Stream.get();
          length &= 0x3F;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        case 2: // length stored in 2 bytes
        {
          viskores::UInt16 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 2);
          std::swap(bytes[0], bytes[1]);
          length &= 0x3FFF;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        case 1: // length stored in 4 bytes
        {
          viskores::UInt32 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 4);
          std::reverse(bytes, bytes + 4);
          length &= 0x3FFFFFFF;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        default: // length stored in 8 bytes
        {
          viskores::UInt64 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 8);
          std::reverse(bytes, bytes + 8);
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
      }
    }
  }
  else
  {
    for (std::size_t i = 0; i < numStrings; ++i)
    {
      // ASCII mode stores one string per line
      this->DataFile->Stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  }
}

void VTKDataSetReaderBase::SkipArrayMetaData(viskores::IdComponent numComponents)
{
  if (!this->DataFile->Stream.good())
  {
    return;
  }

  auto begining = this->DataFile->Stream.tellg();

  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag != "METADATA")
  {
    this->DataFile->Stream.seekg(begining);
    return;
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "METADATA is not supported. Attempting to Skip.");

  this->DataFile->Stream >> tag >> std::ws;
  if (tag == "COMPONENT_NAMES")
  {
    std::string name;
    for (viskores::IdComponent i = 0; i < numComponents; ++i)
    {
      this->DataFile->Stream >> name >> std::ws;
    }
  }
  else if (tag == "INFORMATION")
  {
    int numKeys = 0;
    this->DataFile->Stream >> numKeys >> std::ws;

    // Skipping INFORMATION is tricky. The reader needs to be aware of the types of the
    // information, which is not provided in the file.
    // Here we will just skip until an empty line is found.
    // However, if there are no keys, then there is nothing to read (and the stream tends
    // to skip over empty lines.
    if (numKeys > 0)
    {
      std::string line;
      do
      {
        std::getline(this->DataFile->Stream, line);
      } while (this->DataFile->Stream.good() && !line.empty());

      // Eat any remaining whitespace after the INFORMATION to be ready to read the next token
      this->DataFile->Stream >> std::ws;
    }
  }
  else
  {
    internal::parseAssert(false);
  }
}
}
} // namespace viskores::io
