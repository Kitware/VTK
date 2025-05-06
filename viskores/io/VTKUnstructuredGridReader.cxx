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

#include <viskores/io/VTKUnstructuredGridReader.h>

#include <viskores/io/internal/VTKDataSetCells.h>

#include <viskores/cont/ConvertNumComponentsToOffsets.h>

namespace viskores
{
namespace io
{

VTKUnstructuredGridReader::VTKUnstructuredGridReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKUnstructuredGridReader::VTKUnstructuredGridReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKUnstructuredGridReader::Read()
{
  if (this->DataFile->Structure != viskores::io::internal::DATASET_UNSTRUCTURED_GRID)
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
  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;

  this->DataFile->Stream >> tag;
  internal::parseAssert(tag == "CELLS");

  this->ReadCells(connectivity, numIndices);
  this->ReadShapes(shapes);

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
}
