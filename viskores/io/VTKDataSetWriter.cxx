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

#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/CellShape.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Field.h>

#include <viskores/io/ErrorIO.h>

#include <viskores/io/internal/Endian.h>
#include <viskores/io/internal/VTKDataSetTypes.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

struct CallForBaseTypeFunctor
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T t,
                  bool& success,
                  Functor functor,
                  const viskores::cont::UnknownArrayHandle& array,
                  Args&&... args)
  {
    if (!array.IsBaseComponentType<T>())
    {
      return;
    }

    success = true;

    functor(t, array, std::forward<Args>(args)...);
  }
};

template <typename Functor, typename... Args>
void CallForBaseType(Functor&& functor,
                     const viskores::cont::UnknownArrayHandle& array,
                     Args&&... args)
{
  bool success = true;
  viskores::ListForEach(CallForBaseTypeFunctor{},
                        viskores::TypeListScalarAll{},
                        success,
                        std::forward<Functor>(functor),
                        array,
                        std::forward<Args>(args)...);
  if (!success)
  {
    std::ostringstream out;
    out << "Unrecognized base type in array to be written out.\nArray: ";
    array.PrintSummary(out);

    throw viskores::cont::ErrorBadValue(out.str());
  }
}

template <typename T>
using ArrayHandleRectilinearCoordinates =
  viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T>,
                                              viskores::cont::ArrayHandle<T>,
                                              viskores::cont::ArrayHandle<T>>;

struct OutputArrayDataFunctor
{
  template <typename T>
  VISKORES_CONT void operator()(T,
                                const viskores::cont::UnknownArrayHandle& array,
                                std::ostream& out,
                                viskores::io::FileType fileType) const
  {
    auto componentArray = array.ExtractArrayFromComponents<T>();
    auto portal = componentArray.ReadPortal();
    switch (fileType)
    {
      case viskores::io::FileType::ASCII:
        this->OutputAsciiArray(portal, out);
        break;
      case viskores::io::FileType::BINARY:
        this->OutputBinaryArray(portal, out);
        break;
    }
  }

  template <typename PortalType>
  void OutputAsciiArray(const PortalType& portal, std::ostream& out) const
  {
    using T = typename PortalType::ValueType::ComponentType;

    viskores::Id numValues = portal.GetNumberOfValues();
    for (viskores::Id valueIndex = 0; valueIndex < numValues; ++valueIndex)
    {
      auto value = portal.Get(valueIndex);
      for (viskores::IdComponent cIndex = 0; cIndex < value.GetNumberOfComponents(); ++cIndex)
      {
        out << ((cIndex == 0) ? "" : " ");
        if (std::numeric_limits<T>::is_integer && sizeof(T) == 1)
        {
          out << static_cast<int>(value[cIndex]);
        }
        else
        {
          out << value[cIndex];
        }
      }
      out << "\n";
    }
  }

  template <typename PortalType>
  void OutputBinaryArray(const PortalType& portal, std::ostream& out) const
  {
    using T = typename PortalType::ValueType::ComponentType;

    viskores::Id numValues = portal.GetNumberOfValues();
    std::vector<T> tuple;
    for (viskores::Id valueIndex = 0; valueIndex < numValues; ++valueIndex)
    {
      auto value = portal.Get(valueIndex);
      tuple.resize(static_cast<std::size_t>(value.GetNumberOfComponents()));
      for (viskores::IdComponent cIndex = 0; cIndex < value.GetNumberOfComponents(); ++cIndex)
      {
        tuple[static_cast<std::size_t>(cIndex)] = value[cIndex];
      }
      if (viskores::io::internal::IsLittleEndian())
      {
        viskores::io::internal::FlipEndianness(tuple);
      }
      out.write(reinterpret_cast<const char*>(tuple.data()),
                static_cast<std::streamsize>(tuple.size() * sizeof(T)));
    }
  }
};

void OutputArrayData(const viskores::cont::UnknownArrayHandle& array,
                     std::ostream& out,
                     viskores::io::FileType fileType)
{
  CallForBaseType(OutputArrayDataFunctor{}, array, out, fileType);
}

struct GetFieldTypeNameFunctor
{
  template <typename Type>
  void operator()(Type, const viskores::cont::UnknownArrayHandle& array, std::string& name) const
  {
    if (array.IsBaseComponentType<Type>())
    {
      name = viskores::io::internal::DataTypeName<Type>::Name();
    }
  }
};

std::string GetFieldTypeName(const viskores::cont::UnknownArrayHandle& array)
{
  std::string name;
  CallForBaseType(GetFieldTypeNameFunctor{}, array, name);
  return name;
}

template <viskores::IdComponent DIM>
void WriteDimensions(std::ostream& out, const viskores::cont::CellSetStructured<DIM>& cellSet)
{
  auto pointDimensions = cellSet.GetPointDimensions();
  using VTraits = viskores::VecTraits<decltype(pointDimensions)>;

  out << "DIMENSIONS ";
  out << VTraits::GetComponent(pointDimensions, 0) << " ";
  out << (DIM > 1 ? VTraits::GetComponent(pointDimensions, 1) : 1) << " ";
  out << (DIM > 2 ? VTraits::GetComponent(pointDimensions, 2) : 1) << "\n";
}

void WritePoints(std::ostream& out,
                 const viskores::cont::DataSet& dataSet,
                 viskores::io::FileType fileType)
{
  ///\todo: support other coordinate systems
  int cindex = 0;
  auto cdata = dataSet.GetCoordinateSystem(cindex).GetData();

  std::string typeName = GetFieldTypeName(cdata);

  viskores::Id npoints = cdata.GetNumberOfValues();
  out << "POINTS " << npoints << " " << typeName << " " << '\n';

  OutputArrayData(cdata, out, fileType);
}

template <class CellSetType>
void WriteExplicitCellsAscii(std::ostream& out, const CellSetType& cellSet)
{
  viskores::Id nCells = cellSet.GetNumberOfCells();

  viskores::Id conn_length = 0;
  for (viskores::Id i = 0; i < nCells; ++i)
  {
    conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
  }

  out << "CELLS " << nCells << " " << conn_length << '\n';

  for (viskores::Id i = 0; i < nCells; ++i)
  {
    viskores::cont::ArrayHandle<viskores::Id> ids;
    viskores::Id nids = cellSet.GetNumberOfPointsInCell(i);
    cellSet.GetIndices(i, ids);
    out << nids;
    auto IdPortal = ids.ReadPortal();
    for (int j = 0; j < nids; ++j)
    {
      out << " " << IdPortal.Get(j);
    }
    out << '\n';
  }

  out << "CELL_TYPES " << nCells << '\n';
  for (viskores::Id i = 0; i < nCells; ++i)
  {
    viskores::Id shape = cellSet.GetCellShape(i);
    out << shape << '\n';
  }
}

template <class CellSetType>
void WriteExplicitCellsBinary(std::ostream& out, const CellSetType& cellSet)
{
  viskores::Id nCells = cellSet.GetNumberOfCells();

  viskores::Id conn_length = 0;
  for (viskores::Id i = 0; i < nCells; ++i)
  {
    conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
  }

  out << "CELLS " << nCells << " " << conn_length << '\n';

  std::vector<viskores::Int32> buffer;
  buffer.reserve(static_cast<std::size_t>(conn_length));
  for (viskores::Id i = 0; i < nCells; ++i)
  {
    viskores::cont::ArrayHandle<viskores::Id> ids;
    viskores::Id nids = cellSet.GetNumberOfPointsInCell(i);
    cellSet.GetIndices(i, ids);
    buffer.push_back(static_cast<viskores::Int32>(nids));
    auto IdPortal = ids.ReadPortal();
    for (int j = 0; j < nids; ++j)
    {
      buffer.push_back(static_cast<viskores::Int32>(IdPortal.Get(j)));
    }
  }
  if (viskores::io::internal::IsLittleEndian())
  {
    viskores::io::internal::FlipEndianness(buffer);
  }
  VISKORES_ASSERT(static_cast<viskores::Id>(buffer.size()) == conn_length);
  out.write(reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size() * sizeof(viskores::Int32)));

  out << "CELL_TYPES " << nCells << '\n';
  buffer.resize(0);
  buffer.reserve(static_cast<std::size_t>(nCells));
  for (viskores::Id i = 0; i < nCells; ++i)
  {
    buffer.push_back(static_cast<viskores::Int32>(cellSet.GetCellShape(i)));
  }
  if (viskores::io::internal::IsLittleEndian())
  {
    viskores::io::internal::FlipEndianness(buffer);
  }
  VISKORES_ASSERT(static_cast<viskores::Id>(buffer.size()) == nCells);
  out.write(reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size() * sizeof(viskores::Int32)));
}

template <class CellSetType>
void WriteExplicitCells(std::ostream& out,
                        const CellSetType& cellSet,
                        viskores::io::FileType fileType)
{
  switch (fileType)
  {
    case viskores::io::FileType::ASCII:
      WriteExplicitCellsAscii(out, cellSet);
      break;
    case viskores::io::FileType::BINARY:
      WriteExplicitCellsBinary(out, cellSet);
      break;
  }
}

void WritePointFields(std::ostream& out,
                      const viskores::cont::DataSet& dataSet,
                      viskores::io::FileType fileType)
{
  bool wrote_header = false;
  for (viskores::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
  {
    const viskores::cont::Field field = dataSet.GetField(f);

    if (field.GetAssociation() != viskores::cont::Field::Association::Points)
    {
      continue;
    }

    if (field.GetName() == dataSet.GetCoordinateSystemName())
    {
      // Do not write out the first coordinate system as a field.
      continue;
    }

    viskores::Id npoints = field.GetNumberOfValues();
    int ncomps = field.GetData().GetNumberOfComponentsFlat();

    if (!wrote_header)
    {
      out << "POINT_DATA " << npoints << '\n';
      wrote_header = true;
    }

    std::string typeName = GetFieldTypeName(field.GetData());
    std::string name = field.GetName();
    for (auto& c : name)
    {
      if (std::isspace(c))
      {
        c = '_';
      }
    }
    out << "SCALARS " << name << " " << typeName << " " << ncomps << '\n';
    out << "LOOKUP_TABLE default" << '\n';

    OutputArrayData(field.GetData(), out, fileType);
  }
}

void WriteCellFields(std::ostream& out,
                     const viskores::cont::DataSet& dataSet,
                     viskores::io::FileType fileType)
{
  bool wrote_header = false;
  for (viskores::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
  {
    const viskores::cont::Field field = dataSet.GetField(f);
    if (!field.IsCellField())
    {
      continue;
    }


    viskores::Id ncells = field.GetNumberOfValues();
    int ncomps = field.GetData().GetNumberOfComponentsFlat();

    if (!wrote_header)
    {
      out << "CELL_DATA " << ncells << '\n';
      wrote_header = true;
    }

    std::string typeName = GetFieldTypeName(field.GetData());

    std::string name = field.GetName();
    for (auto& c : name)
    {
      if (std::isspace(c))
      {
        c = '_';
      }
    }

    out << "SCALARS " << name << " " << typeName << " " << ncomps << '\n';
    out << "LOOKUP_TABLE default" << '\n';

    OutputArrayData(field.GetData(), out, fileType);
  }
}

template <class CellSetType>
void WriteDataSetAsUnstructured(std::ostream& out,
                                const viskores::cont::DataSet& dataSet,
                                const CellSetType& cellSet,
                                viskores::io::FileType fileType)
{
  out << "DATASET UNSTRUCTURED_GRID" << '\n';
  WritePoints(out, dataSet, fileType);
  WriteExplicitCells(out, cellSet, fileType);
}

template <viskores::IdComponent DIM>
void WriteDataSetAsStructuredPoints(
  std::ostream& out,
  const viskores::cont::ArrayHandleUniformPointCoordinates& points,
  const viskores::cont::CellSetStructured<DIM>& cellSet)
{
  out << "DATASET STRUCTURED_POINTS\n";

  WriteDimensions(out, cellSet);

  auto portal = points.ReadPortal();
  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();
  out << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";
  out << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";
}

template <typename T, viskores::IdComponent DIM>
void WriteDataSetAsRectilinearGrid(std::ostream& out,
                                   const ArrayHandleRectilinearCoordinates<T>& points,
                                   const viskores::cont::CellSetStructured<DIM>& cellSet,
                                   viskores::io::FileType fileType)
{
  out << "DATASET RECTILINEAR_GRID\n";

  WriteDimensions(out, cellSet);

  std::string typeName = viskores::io::internal::DataTypeName<T>::Name();
  viskores::cont::ArrayHandle<T> dimArray;

  dimArray = points.GetFirstArray();
  out << "X_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);

  dimArray = points.GetSecondArray();
  out << "Y_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);

  dimArray = points.GetThirdArray();
  out << "Z_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);
}

template <viskores::IdComponent DIM>
void WriteDataSetAsStructuredGrid(std::ostream& out,
                                  const viskores::cont::DataSet& dataSet,
                                  const viskores::cont::CellSetStructured<DIM>& cellSet,
                                  viskores::io::FileType fileType)
{
  out << "DATASET STRUCTURED_GRID" << '\n';

  WriteDimensions(out, cellSet);

  WritePoints(out, dataSet, fileType);
}

template <viskores::IdComponent DIM>
void WriteDataSetAsStructured(std::ostream& out,
                              const viskores::cont::DataSet& dataSet,
                              const viskores::cont::CellSetStructured<DIM>& cellSet,
                              viskores::io::FileType fileType)
{
  ///\todo: support rectilinear

  // Type of structured grid (uniform, rectilinear, curvilinear) is determined by coordinate system
  auto coordSystem = dataSet.GetCoordinateSystem().GetData();
  if (coordSystem.IsType<viskores::cont::ArrayHandleUniformPointCoordinates>())
  {
    // uniform is written as "structured points"
    WriteDataSetAsStructuredPoints(
      out,
      coordSystem.AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>(),
      cellSet);
  }
  else if (coordSystem.IsType<ArrayHandleRectilinearCoordinates<viskores::Float32>>())
  {
    WriteDataSetAsRectilinearGrid(
      out,
      coordSystem.AsArrayHandle<ArrayHandleRectilinearCoordinates<viskores::Float32>>(),
      cellSet,
      fileType);
  }
  else if (coordSystem.IsType<ArrayHandleRectilinearCoordinates<viskores::Float64>>())
  {
    WriteDataSetAsRectilinearGrid(
      out,
      coordSystem.AsArrayHandle<ArrayHandleRectilinearCoordinates<viskores::Float64>>(),
      cellSet,
      fileType);
  }
  else
  {
    // Curvilinear is written as "structured grid"
    WriteDataSetAsStructuredGrid(out, dataSet, cellSet, fileType);
  }
}

void Write(std::ostream& out,
           const viskores::cont::DataSet& dataSet,
           viskores::io::FileType fileType)
{
  // The Paraview parser cannot handle scientific notation:
  out << std::fixed;
  // This causes a big problem for the dataset writer.
  // Fixed point and floating point are fundamentally different.
  // This is a workaround, but until Paraview supports parsing floats,
  // this is as good as we can do.
#ifdef VISKORES_USE_DOUBLE_PRECISION
  out << std::setprecision(18);
#else
  out << std::setprecision(10);
#endif
  out << "# vtk DataFile Version 3.0" << '\n';
  out << "vtk output" << '\n';
  switch (fileType)
  {
    case viskores::io::FileType::ASCII:
      out << "ASCII" << '\n';
      break;
    case viskores::io::FileType::BINARY:
      out << "BINARY" << '\n';
      break;
  }

  viskores::cont::UnknownCellSet cellSet = dataSet.GetCellSet();
  if (cellSet.IsType<viskores::cont::CellSetExplicit<>>())
  {
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetExplicit<>>(), fileType);
  }
  else if (cellSet.IsType<viskores::cont::CellSetStructured<1>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetStructured<1>>(), fileType);
  }
  else if (cellSet.IsType<viskores::cont::CellSetStructured<2>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetStructured<2>>(), fileType);
  }
  else if (cellSet.IsType<viskores::cont::CellSetStructured<3>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetStructured<3>>(), fileType);
  }
  else if (cellSet.IsType<viskores::cont::CellSetSingleType<>>())
  {
    // these function just like explicit cell sets
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetSingleType<>>(), fileType);
  }
  else if (cellSet.IsType<viskores::cont::CellSetExtrude>())
  {
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<viskores::cont::CellSetExtrude>(), fileType);
  }
  else
  {
    throw viskores::cont::ErrorBadType("Could not determine type to write out.");
  }

  WritePointFields(out, dataSet, fileType);
  WriteCellFields(out, dataSet, fileType);
}

} // anonymous namespace

namespace viskores
{
namespace io
{

VTKDataSetWriter::VTKDataSetWriter(const char* fileName)
  : FileName(fileName)
{
}

VTKDataSetWriter::VTKDataSetWriter(const std::string& fileName)
  : FileName(fileName)
{
}

void VTKDataSetWriter::WriteDataSet(const viskores::cont::DataSet& dataSet) const
{
  if (dataSet.GetNumberOfCoordinateSystems() < 1)
  {
    throw viskores::cont::ErrorBadValue(
      "DataSet has no coordinate system, which is not supported by VTK file format.");
  }
  try
  {
    std::ofstream fileStream(this->FileName.c_str(), std::fstream::trunc | std::fstream::binary);
    Write(fileStream, dataSet, this->GetFileType());
    fileStream.close();
  }
  catch (std::ofstream::failure& error)
  {
    throw viskores::io::ErrorIO(error.what());
  }
}

viskores::io::FileType VTKDataSetWriter::GetFileType() const
{
  return this->FileType;
}

void VTKDataSetWriter::SetFileType(viskores::io::FileType type)
{
  this->FileType = type;
}

}
} // namespace viskores::io
