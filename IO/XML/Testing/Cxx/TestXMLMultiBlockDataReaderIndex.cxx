// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests that vtkXMLMultiBlockDataReader places children of a .vtm file at the
// right block, both when the "index" attribute is missing and when it differs
// from the position of the element in the XML (see #20162).

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkTesting.h"
#include "vtkXMLMultiBlockDataReader.h"

#include "vtksys/FStream.hxx"

#include <iostream>
#include <string>

namespace
{
const char* TriangleVTU = R"(<VTKFile type="UnstructuredGrid" version="0.1">
<UnstructuredGrid><Piece NumberOfPoints="3" NumberOfCells="1">
<Points><DataArray type="Float32" NumberOfComponents="3" format="ascii">0 0 0 1 0 0 0 1 0</DataArray></Points>
<Cells>
<DataArray type="Int64" Name="connectivity" format="ascii">0 1 2</DataArray>
<DataArray type="Int64" Name="offsets" format="ascii">3</DataArray>
<DataArray type="UInt8" Name="types" format="ascii">5</DataArray>
</Cells>
</Piece></UnstructuredGrid></VTKFile>
)";

void WriteFile(const std::string& fileName, const std::string& contents)
{
  vtksys::ofstream file(fileName.c_str());
  file << contents;
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadVTM(
  const std::string& dir, const std::string& name, const std::string& body)
{
  const std::string fileName = dir + "/" + name + ".vtm";
  WriteFile(fileName,
    "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\"><vtkMultiBlockDataSet>" + body +
      "</vtkMultiBlockDataSet></VTKFile>");

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  return vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
}

const char* GetBlockName(vtkMultiBlockDataSet* mb, unsigned int index)
{
  return mb->HasMetaData(index) ? mb->GetMetaData(index)->Get(vtkCompositeDataSet::NAME())
                                : nullptr;
}

// Checks that `mb` has `numBlocks` blocks and that block `index` is named
// `name`.
bool CheckBlock(const std::string& testName, vtkMultiBlockDataSet* mb, unsigned int numBlocks,
  unsigned int index, const std::string& name)
{
  if (!mb || mb->GetNumberOfBlocks() != numBlocks)
  {
    std::cerr << testName << ": expected " << numBlocks << " blocks, got "
              << (mb ? mb->GetNumberOfBlocks() : 0) << std::endl;
    return false;
  }
  const char* blockName = GetBlockName(mb, index);
  if (!blockName || name != blockName)
  {
    std::cerr << testName << ": expected block " << index << " to be named '" << name << "', got '"
              << (blockName ? blockName : "(none)") << "'" << std::endl;
    return false;
  }
  return true;
}

// Checks that block `index` of `mb` is named `name` and holds the triangle.
bool CheckLeaf(const std::string& testName, vtkMultiBlockDataSet* mb, unsigned int numBlocks,
  unsigned int index, const std::string& name)
{
  if (!CheckBlock(testName, mb, numBlocks, index, name))
  {
    return false;
  }
  auto ds = vtkDataSet::SafeDownCast(mb->GetBlock(index));
  if (!ds || ds->GetNumberOfPoints() != 3)
  {
    std::cerr << testName << ": expected block " << index << " to hold a 3-point dataset"
              << std::endl;
    return false;
  }
  return true;
}
}

int TestXMLMultiBlockDataReaderIndex(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  const std::string dir = testing->GetTempDirectory();
  WriteFile(dir + "/tri.vtu", TriangleVTU);

  bool success = true;

  // A single DataSet without index.
  {
    auto mb = ReadVTM(dir, "NoIndex", R"(<DataSet name="tri" file="tri.vtu"/>)");
    success &= CheckLeaf("NoIndex", mb, 1, 0, "tri");
  }

  // Several DataSets without index.
  {
    auto mb = ReadVTM(dir, "NoIndexMany",
      R"(<DataSet name="a" file="tri.vtu"/><DataSet name="b" file="tri.vtu"/>)");
    success &= CheckLeaf("NoIndexMany", mb, 2, 0, "a");
    success &= CheckLeaf("NoIndexMany", mb, 2, 1, "b");
  }

  // A DataSet without index after one with an index goes after it.
  {
    auto mb = ReadVTM(dir, "MixedIndex",
      R"(<DataSet index="2" name="a" file="tri.vtu"/><DataSet name="b" file="tri.vtu"/>)");
    success &= CheckLeaf("MixedIndex", mb, 4, 2, "a");
    success &= CheckLeaf("MixedIndex", mb, 4, 3, "b");
  }

  // Nested Block without index.
  {
    auto mb = ReadVTM(dir, "NestedNoIndex",
      R"(<DataSet name="a" file="tri.vtu"/>)"
      R"(<Block name="blk"><DataSet name="c" file="tri.vtu"/></Block>)");
    success &= CheckLeaf("NestedNoIndex", mb, 2, 0, "a");
    if (CheckBlock("NestedNoIndex", mb, 2, 1, "blk"))
    {
      auto child = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(1));
      success &= CheckLeaf("NestedNoIndex/blk", child, 1, 0, "c");
    }
    else
    {
      success = false;
    }
  }

  // Nested Block whose index differs from its position in the XML.
  {
    auto mb = ReadVTM(dir, "NestedIndex",
      R"(<Block index="3" name="blk"><DataSet name="c" file="tri.vtu"/></Block>)");
    if (CheckBlock("NestedIndex", mb, 4, 3, "blk"))
    {
      auto child = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(3));
      success &= CheckLeaf("NestedIndex/blk", child, 1, 0, "c");
    }
    else
    {
      success = false;
    }
  }

  // Piece with DataSets without index.
  {
    auto mb = ReadVTM(dir, "PieceNoIndex",
      R"(<Piece name="mp"><DataSet file="tri.vtu"/><DataSet file="tri.vtu"/></Piece>)");
    if (CheckBlock("PieceNoIndex", mb, 1, 0, "mp"))
    {
      auto mp = vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(0));
      if (!mp || mp->GetNumberOfPieces() != 2 || !mp->GetPiece(0) || !mp->GetPiece(1))
      {
        std::cerr << "PieceNoIndex: expected 2 non-empty pieces, got "
                  << (mp ? mp->GetNumberOfPieces() : 0) << " pieces" << std::endl;
        success = false;
      }
    }
    else
    {
      success = false;
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
