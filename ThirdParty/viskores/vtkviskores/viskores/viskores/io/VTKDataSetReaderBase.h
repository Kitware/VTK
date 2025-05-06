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
#ifndef viskores_io_VTKDataSetReaderBase_h
#define viskores_io_VTKDataSetReaderBase_h

#include <viskores/Types.h>
#include <viskores/cont/DataSet.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/viskores_io_export.h>

#include <viskores/io/internal/Endian.h>
#include <viskores/io/internal/VTKDataSetStructures.h>
#include <viskores/io/internal/VTKDataSetTypes.h>

#include <fstream>
#include <sstream>

namespace viskores
{
namespace io
{

namespace internal
{

struct VTKDataSetFile
{
  std::string FileName;
  viskores::Id2 Version;
  std::string Title;
  bool IsBinary;
  viskores::io::internal::DataSetStructure Structure;
  std::ifstream Stream;
};

inline void parseAssert(bool condition)
{
  if (!condition)
  {
    throw viskores::io::ErrorIO("Parse Error");
  }
}

template <typename T>
struct StreamIOType
{
  using Type = T;
};
template <>
struct StreamIOType<viskores::Int8>
{
  using Type = viskores::Int16;
};
template <>
struct StreamIOType<viskores::UInt8>
{
  using Type = viskores::UInt16;
};

inline viskores::cont::UnknownCellSet CreateCellSetStructured(const viskores::Id3& dim)
{
  if (dim[0] > 1 && dim[1] > 1 && dim[2] > 1)
  {
    viskores::cont::CellSetStructured<3> cs;
    cs.SetPointDimensions(viskores::make_Vec(dim[0], dim[1], dim[2]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] > 1 && dim[2] <= 1)
  {
    viskores::cont::CellSetStructured<2> cs;
    cs.SetPointDimensions(viskores::make_Vec(dim[0], dim[1]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] <= 1 && dim[2] <= 1)
  {
    viskores::cont::CellSetStructured<1> cs;
    cs.SetPointDimensions(dim[0]);
    return cs;
  }

  std::stringstream ss;
  ss << "Unsupported dimensions: (" << dim[0] << ", " << dim[1] << ", " << dim[2]
     << "), 2D structured datasets should be on X-Y plane and "
     << "1D structured datasets should be along X axis";
  throw viskores::io::ErrorIO(ss.str());
}

} // namespace internal

class VISKORES_IO_EXPORT VTKDataSetReaderBase
{
protected:
  std::unique_ptr<internal::VTKDataSetFile> DataFile;
  viskores::cont::DataSet DataSet;

private:
  bool Loaded;
  viskores::cont::ArrayHandle<viskores::Id> CellsPermutation;

  friend class VTKDataSetReader;

public:
  explicit VISKORES_CONT VTKDataSetReaderBase(const char* fileName);

  explicit VISKORES_CONT VTKDataSetReaderBase(const std::string& fileName);

  virtual VISKORES_CONT ~VTKDataSetReaderBase();

  VTKDataSetReaderBase(const VTKDataSetReaderBase&) = delete;
  void operator=(const VTKDataSetReaderBase&) = delete;

  /// @brief Load data from the file and return it in a `DataSet` object.
  VISKORES_CONT const viskores::cont::DataSet& ReadDataSet();

  const viskores::cont::DataSet& GetDataSet() const { return this->DataSet; }

  virtual VISKORES_CONT void PrintSummary(std::ostream& out) const;

protected:
  VISKORES_CONT void ReadPoints();

  VISKORES_CONT void ReadCells(viskores::cont::ArrayHandle<viskores::Id>& connectivity,
                               viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices);

  VISKORES_CONT void ReadShapes(viskores::cont::ArrayHandle<viskores::UInt8>& shapes);

  VISKORES_CONT void ReadAttributes();

  void SetCellsPermutation(const viskores::cont::ArrayHandle<viskores::Id>& permutation)
  {
    this->CellsPermutation = permutation;
  }

  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> GetCellsPermutation() const
  {
    return this->CellsPermutation;
  }

  VISKORES_CONT void TransferDataFile(VTKDataSetReaderBase& reader)
  {
    reader.DataFile.swap(this->DataFile);
    this->DataFile.reset(nullptr);
  }

  VISKORES_CONT virtual void CloseFile();

  VISKORES_CONT virtual void Read() = 0;

private:
  VISKORES_CONT void OpenFile();
  VISKORES_CONT void ReadHeader();
  VISKORES_CONT void AddField(const std::string& name,
                              viskores::cont::Field::Association association,
                              viskores::cont::UnknownArrayHandle& data);
  VISKORES_CONT void ReadScalars(viskores::cont::Field::Association association,
                                 std::size_t numElements);
  VISKORES_CONT void ReadColorScalars(viskores::cont::Field::Association association,
                                      std::size_t numElements);
  VISKORES_CONT void ReadLookupTable();
  VISKORES_CONT void ReadTextureCoordinates(viskores::cont::Field::Association association,
                                            std::size_t numElements);
  VISKORES_CONT void ReadVectors(viskores::cont::Field::Association association,
                                 std::size_t numElements);
  VISKORES_CONT void ReadTensors(viskores::cont::Field::Association association,
                                 std::size_t numElements);
  VISKORES_CONT void ReadFields(viskores::cont::Field::Association association,
                                std::size_t expectedNumElements);
  VISKORES_CONT void ReadGlobalOrPedigreeIds(viskores::cont::Field::Association association,
                                             std::size_t numElements);

protected:
  VISKORES_CONT void ReadGlobalFields(std::vector<viskores::Float32>* visitBounds = nullptr);

private:
  class SkipArrayVariant;
  class ReadArrayVariant;

  //Make the Array parsing methods protected so that derived classes
  //can call the methods.
protected:
  VISKORES_CONT void DoSkipArrayVariant(std::string dataType,
                                        std::size_t numElements,
                                        viskores::IdComponent numComponents);
  VISKORES_CONT viskores::cont::UnknownArrayHandle DoReadArrayVariant(
    viskores::cont::Field::Association association,
    std::string dataType,
    std::size_t numElements,
    viskores::IdComponent numComponents);

  template <typename T>
  VISKORES_CONT void ReadArray(std::vector<T>& buffer)
  {
    using ComponentType = typename viskores::VecTraits<T>::ComponentType;
    constexpr viskores::IdComponent numComponents = viskores::VecTraits<T>::NUM_COMPONENTS;

    std::size_t numElements = buffer.size();
    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.read(reinterpret_cast<char*>(&buffer[0]),
                                  static_cast<std::streamsize>(numElements * sizeof(T)));
      if (viskores::io::internal::IsLittleEndian())
      {
        viskores::io::internal::FlipEndianness(buffer);
      }
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (viskores::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
          viskores::VecTraits<T>::SetComponent(buffer[i], j, static_cast<ComponentType>(val));
        }
      }
    }
    this->DataFile->Stream >> std::ws;
    this->SkipArrayMetaData(numComponents);
  }

  template <viskores::IdComponent NumComponents>
  VISKORES_CONT void ReadArray(
    std::vector<viskores::Vec<viskores::io::internal::DummyBitType, NumComponents>>& buffer)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "Support for data type 'bit' is not implemented. Skipping.");
    this->SkipArray(buffer.size(),
                    viskores::Vec<viskores::io::internal::DummyBitType, NumComponents>());
    buffer.clear();
  }

  VISKORES_CONT void ReadArray(std::vector<viskores::io::internal::DummyBitType>& buffer);

  template <typename T>
  void SkipArray(std::size_t numElements, T)
  {
    using ComponentType = typename viskores::VecTraits<T>::ComponentType;
    constexpr viskores::IdComponent numComponents = viskores::VecTraits<T>::NUM_COMPONENTS;

    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements * sizeof(T)),
                                   std::ios_base::cur);
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (viskores::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
        }
      }
    }
    this->DataFile->Stream >> std::ws;
    this->SkipArrayMetaData(numComponents);
  }

  template <viskores::IdComponent NumComponents>
  void SkipArray(std::size_t numElements,
                 viskores::Vec<viskores::io::internal::DummyBitType, NumComponents>)
  {
    this->SkipArray(numElements * static_cast<std::size_t>(NumComponents),
                    viskores::io::internal::DummyBitType(),
                    NumComponents);
  }

  VISKORES_CONT void SkipArray(std::size_t numElements,
                               viskores::io::internal::DummyBitType,
                               viskores::IdComponent numComponents = 1);

  VISKORES_CONT void SkipStringArray(std::size_t numStrings);

  VISKORES_CONT void SkipArrayMetaData(viskores::IdComponent numComponents);
};
}
} // viskores::io

#endif // viskores_io_VTKDataSetReaderBase_h
