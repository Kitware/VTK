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

#include <viskores/io/VTKPolyDataReader.h>

#include <viskores/cont/ConvertNumComponentsToOffsets.h>

namespace
{

template <typename T>
inline viskores::cont::ArrayHandle<T> ConcatinateArrayHandles(
  const std::vector<viskores::cont::ArrayHandle<T>>& arrays)
{
  viskores::Id size = 0;
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    size += arrays[i].GetNumberOfValues();
  }

  viskores::cont::ArrayHandle<T> out;
  out.Allocate(size);

  auto outp = viskores::cont::ArrayPortalToIteratorBegin(out.WritePortal());
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    std::copy(viskores::cont::ArrayPortalToIteratorBegin(arrays[i].ReadPortal()),
              viskores::cont::ArrayPortalToIteratorEnd(arrays[i].ReadPortal()),
              outp);
    using DifferenceType = typename std::iterator_traits<decltype(outp)>::difference_type;
    std::advance(outp, static_cast<DifferenceType>(arrays[i].GetNumberOfValues()));
  }

  return out;
}
}

namespace viskores
{
namespace io
{

VTKPolyDataReader::VTKPolyDataReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKPolyDataReader::VTKPolyDataReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKPolyDataReader::Read()
{
  if (this->DataFile->Structure != viskores::io::internal::DATASET_POLYDATA)
  {
    throw viskores::io::ErrorIO("Incorrect DataSet type");
  }

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag == "FIELD")
  {
    this->ReadGlobalFields();
    this->DataFile->Stream >> tag;
  }

  // Read the points
  internal::parseAssert(tag == "POINTS");
  this->ReadPoints();

  viskores::Id numPoints = this->DataSet.GetNumberOfPoints();

  // Read the cellset
  std::vector<viskores::cont::ArrayHandle<viskores::Id>> connectivityArrays;
  std::vector<viskores::cont::ArrayHandle<viskores::IdComponent>> numIndicesArrays;
  std::vector<viskores::UInt8> shapesBuffer;
  while (!this->DataFile->Stream.eof())
  {
    viskores::UInt8 shape = viskores::CELL_SHAPE_EMPTY;
    this->DataFile->Stream >> tag;
    if (tag == "VERTICES")
    {
      shape = viskores::io::internal::CELL_SHAPE_POLY_VERTEX;
    }
    else if (tag == "LINES")
    {
      shape = viskores::io::internal::CELL_SHAPE_POLY_LINE;
    }
    else if (tag == "POLYGONS")
    {
      shape = viskores::CELL_SHAPE_POLYGON;
    }
    else if (tag == "TRIANGLE_STRIPS")
    {
      shape = viskores::io::internal::CELL_SHAPE_TRIANGLE_STRIP;
    }
    else
    {
      this->DataFile->Stream.seekg(-static_cast<std::streamoff>(tag.length()), std::ios_base::cur);
      break;
    }

    viskores::cont::ArrayHandle<viskores::Id> cellConnectivity;
    viskores::cont::ArrayHandle<viskores::IdComponent> cellNumIndices;
    this->ReadCells(cellConnectivity, cellNumIndices);

    connectivityArrays.push_back(cellConnectivity);
    numIndicesArrays.push_back(cellNumIndices);
    shapesBuffer.insert(
      shapesBuffer.end(), static_cast<std::size_t>(cellNumIndices.GetNumberOfValues()), shape);
  }

  viskores::cont::ArrayHandle<viskores::Id> connectivity =
    ConcatinateArrayHandles(connectivityArrays);
  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices =
    ConcatinateArrayHandles(numIndicesArrays);
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;
  shapes.Allocate(static_cast<viskores::Id>(shapesBuffer.size()));
  std::copy(shapesBuffer.begin(),
            shapesBuffer.end(),
            viskores::cont::ArrayPortalToIteratorBegin(shapes.WritePortal()));

  viskores::cont::ArrayHandle<viskores::Id> permutation;
  viskores::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
  this->SetCellsPermutation(permutation);

  if (viskores::io::internal::IsSingleShape(shapes))
  {
    viskores::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(
      numPoints, shapes.ReadPortal().Get(0), numIndices.ReadPortal().Get(0), connectivity);
    this->DataSet.SetCellSet(cellSet);
  }
  else
  {
    auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numIndices);
    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(numPoints, shapes, connectivity, offsets);
    this->DataSet.SetCellSet(cellSet);
  }

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace viskores::io
