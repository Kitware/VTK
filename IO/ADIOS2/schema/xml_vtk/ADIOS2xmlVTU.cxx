/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2xmlVTU.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2xmlVTU.cxx
 *
 *  Created on: June 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2xmlVTU.h"
#include "ADIOS2xmlVTU.txx"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkType.h"
#include "vtkUnsignedIntArray.h"

#include "vtkDoubleArray.h"

#include "ADIOS2Helper.h"

namespace adios2vtk
{
namespace schema
{

ADIOS2xmlVTU::ADIOS2xmlVTU(const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : ADIOS2xmlVTK("VTU", schema, io, engine)
{
  Init();
  InitTimes();
}

ADIOS2xmlVTU::~ADIOS2xmlVTU() {}

// PRIVATE
void ADIOS2xmlVTU::DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  ReadPiece(step, 0); // just read piece 0 for now

  const unsigned int rank = static_cast<unsigned int>(helper::MPIGetRank());

  vtkNew<vtkMultiPieceDataSet> pieces;
  pieces->SetPiece(rank, this->UnstructuredGrid);
  multiBlock->SetBlock(0, pieces);
}

void ADIOS2xmlVTU::ReadPiece(const size_t step, const size_t pieceID)
{
  const bool hasCells =
    ReadDataSets(types::DataSetType::Cells, step, pieceID, " in UnstructuredGrid VTK XML Schema\n");

  const bool hasPoints = ReadDataSets(
    types::DataSetType::Points, step, pieceID, " in UnstructuredGrid VTK XML Schema\n");

  const bool hasPointData = ReadDataSets(
    types::DataSetType::PointData, step, pieceID, " in UnstructuredGrid VTK XML Schema\n");

  this->Engine.PerformGets();

  // TODO CellData

  // Associate PointData
  {
    types::DataSet& dataSet = this->Pieces[pieceID][types::DataSetType::PointData];
    for (auto& dataArrayPair : dataSet)
    {
      const std::string& variableName = dataArrayPair.first;
      if (this->TIMENames.count(variableName) == 1)
      {
        continue;
      }
      types::DataArray& dataArray = dataArrayPair.second;
      this->UnstructuredGrid->GetPointData()->AddArray(dataArray.Data.GetPointer());
    }
  }

  // Set Grid
  // Associate Points
  std::vector<vtkIdType> nodeSizes;
  {
    types::DataSet& dataSet = this->Pieces[pieceID][types::DataSetType::Points];
    types::DataArray& dataArray = dataSet.begin()->second;

    // save nodeIdOffsets
    nodeSizes.reserve(dataArray.BlockCounts.size());
    for (const auto& bCount : dataArray.BlockCounts)
    {
      nodeSizes.push_back(bCount.second.front());
    }

    // TODO check if tuple is necessary
    auto nodesSize = dataArray.Data->GetDataSize();
    auto tuples = dataArray.Data->GetNumberOfTuples();
    auto components = dataArray.Data->GetNumberOfComponents();

    vtkDoubleArray* nodes = vtkDoubleArray::SafeDownCast(dataArray.Data.GetPointer());
    double* pnodes = nodes->GetPointer(0);

    vtkNew<vtkPoints> points;
    points->SetData(dataArray.Data.GetPointer());

    this->UnstructuredGrid->SetPoints(points);
  }

  // Associate Cells
  {
    types::DataSet& dataSet = this->Pieces[pieceID][types::DataSetType::Cells];

    types::DataArray& connectivity = dataSet.at("connectivity");
    vtkIdTypeArray* iconnectivity = vtkIdTypeArray::SafeDownCast(connectivity.Data.GetPointer());
    vtkIdType* pconn = iconnectivity->GetPointer(0);

    // increase the connectivity offsets to match the local block point id
    vtkIdType blockOffset = 0;
    size_t linearOffset = 0;

    // update with block offsets (squashed blocks)
    auto itBlocks = connectivity.BlockCounts.begin();
    size_t n = 0;

    for (const auto& blockPair : connectivity.BlockCounts)
    {
      const adios2::Dims& blockCount = blockPair.second;

      // through elements
      for (auto e = 0; e < blockCount[0]; ++e)
      {
        const auto nPoints = pconn[linearOffset];
        for (auto p = 0; p < nPoints; ++p)
        {
          const size_t index = linearOffset + p + 1;
          pconn[index] += blockOffset;
        }
        linearOffset += nPoints + 1; // 1 for nPoints itself
      }

      blockOffset += nodeSizes[n];
      ++n;
      ++itBlocks;
    }

    vtkIdType size = connectivity.Data->GetSize();
    vtkNew<vtkCellArray> cellArray;

    cellArray->SetCells(size, iconnectivity);

    types::DataArray& types = dataSet.at("types");

    // single type cells
    if (types.Data->GetSize() == 1)
    {
      int type = -1;

      if (types.Data->GetDataType() == VTK_UNSIGNED_INT)
      {
        vtkUnsignedIntArray* itypes = vtkUnsignedIntArray::SafeDownCast(types.Data.GetPointer());
        type = static_cast<int>(itypes->GetValue(0));
      }
      else if (types.Data->GetDataType() == VTK_INT)
      {
        vtkIntArray* itypes = vtkIntArray::SafeDownCast(types.Data.GetPointer());
        type = itypes->GetValue(0);
      }
      else
      {
        throw std::invalid_argument(
          "ERROR: types data array must be an int32_t or uint32_t type\n");
      }
      this->UnstructuredGrid->SetCells(type, cellArray);
    }
  }
}

void ADIOS2xmlVTU::Init()
{
  auto lf_InitPieceDataSetType = [&](types::Piece& piece, const types::DataSetType type,
                                   const pugi::xml_node& pieceNode) {
    const std::string nodeName = DataSetType(type);
    const pugi::xml_node dataSetNode = helper::XMLNode(
      nodeName, pieceNode, true, "when reading " + nodeName + " node in ImageData", false);
    types::DataSet dataSet = helper::XMLInitDataSet(dataSetNode, this->SpecialNames);
    piece[type] = dataSet;
  };

  // BODY OF FUNCTION STARTS HERE
  const pugi::xml_document xmlDocument =
    adios2vtk::helper::XMLDocument(this->Schema, true, "when reading xml vtu schema");

  const pugi::xml_node xmlVTKFileNode = adios2vtk::helper::XMLNode(
    "VTKFile", xmlDocument, true, "when reading VTKFile type=UnstructuredGrid node", true, true);

  const pugi::xml_node xmlUnstructuredGridNode = adios2vtk::helper::XMLNode(
    "UnstructuredGrid", xmlVTKFileNode, true, "when reading UnstructuredGrid node", true, true);

  size_t pieces = 0;
  for (const pugi::xml_node& xmlPieceNode : xmlUnstructuredGridNode.children("Piece"))
  {
    types::Piece piece;
    lf_InitPieceDataSetType(piece, types::DataSetType::PointData, xmlPieceNode);
    lf_InitPieceDataSetType(piece, types::DataSetType::Cells, xmlPieceNode);
    lf_InitPieceDataSetType(piece, types::DataSetType::Points, xmlPieceNode);

    this->Pieces.push_back(piece);
    ++pieces;
  }
  if (pieces == 0)
  {
    throw std::invalid_argument(
      "ERROR: could not find Piece XML-node when reading UnstructuredGrid XML-node "
      "in ADIOS2 VTU XML Schema source\n");
  }
}

#define declare_type(T)                                                                            \
  void ADIOS2xmlVTU::SetBlocks(                                                                    \
    adios2::Variable<T> variable, types::DataArray& dataArray, const size_t step)                  \
  {                                                                                                \
    SetBlocksCommon(variable, dataArray, step);                                                    \
  }
ADIOS2_VTK_ARRAY_TYPE(declare_type)
#undef declare_type

} // end namespace schema
} // end namespace adios2vtk
