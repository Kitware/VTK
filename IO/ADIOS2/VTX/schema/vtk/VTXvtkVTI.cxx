/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXvtkVTI.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXvtkVTI.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXvtkVTI.h"
#include "VTXvtkVTI.txx"

#include <algorithm>
#include <utility>

#include "vtkCellData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"

#include <vtk_pugixml.h>

#include "VTX/common/VTXHelper.h"

#include <adios2.h>

namespace vtx
{
namespace schema
{

VTXvtkVTI::VTXvtkVTI(const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : VTXvtkBase("vti", schema, io, engine)
{
  Init();
  InitTimes();
}

VTXvtkVTI::~VTXvtkVTI() {}

// PRIVATE
void VTXvtkVTI::DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  ReadPiece(step, 0); // just read piece 0 for now

  const unsigned int rank = static_cast<unsigned int>(helper::MPIGetRank());

  vtkNew<vtkMultiPieceDataSet> pieces;
  pieces->SetPiece(rank, this->ImageData);
  multiBlock->SetBlock(0, pieces);
}

void VTXvtkVTI::ReadPiece(const size_t step, const size_t pieceID)
{
  const bool hasCellData = ReadDataSets(types::DataSetType::CellData, step, pieceID);
  const bool hasPointData = ReadDataSets(types::DataSetType::PointData, step, pieceID);

  this->Engine.PerformGets();

  // CellData
  if (hasCellData)
  {
    types::DataSet& dataSet = this->Pieces[pieceID][types::DataSetType::CellData];
    for (auto& dataArrayPair : dataSet)
    {
      const std::string& variableName = dataArrayPair.first;
      if (this->TIMENames.count(variableName) == 1)
      {
        continue;
      }

      types::DataArray& dataArray = dataArrayPair.second;
      this->ImageData->GetCellData()->AddArray(dataArray.Data.GetPointer());
    }
  }

  // Point Data
  if (hasPointData)
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
      this->ImageData->GetPointData()->AddArray(dataArray.Data.GetPointer());
    }
  }
}

// PRIVATE
void VTXvtkVTI::Init()
{
  auto lf_InitPieceDataSetType = [&](types::Piece& piece, const types::DataSetType type,
                                   const pugi::xml_node& pieceNode) {
    const std::string nodeName = DataSetType(type);
    const pugi::xml_node dataSetNode = helper::XMLNode(
      nodeName, pieceNode, true, "when reading " + nodeName + " node in ImageData", false);
    types::DataSet dataSet = helper::XMLInitDataSet(dataSetNode, this->TIMENames);

    for (auto& dataArrayPair : dataSet)
    {
      types::DataArray& dataArray = dataArrayPair.second;
      dataArray.Shape = GetShape(type);

      adios2::Box<adios2::Dims> selection = GetSelection(type);
      dataArray.Start = selection.first;
      dataArray.Count = selection.second;
    }
    piece[type] = dataSet;
  };

  auto lf_InitExtent = [&](const pugi::xml_node& extentNode) {
    // Spacing
    const pugi::xml_attribute spacingXML = vtx::helper::XMLAttribute(
      "Spacing", extentNode, true, "when reading Spacing in ImageData", true);
    const std::vector<double> spacingV = vtx::helper::StringToVector<double>(spacingXML.value());
    if (spacingV.size() != 3)
    {
      throw std::runtime_error(
        "ERROR: incorrect Spacing attribute in ImageData from " + this->Engine.Name());
    }
    this->ImageData->SetSpacing(spacingV.data());

    // Origin
    const pugi::xml_attribute originXML = vtx::helper::XMLAttribute(
      "Origin", extentNode, true, "when reading Origin in ImageData", true);
    const std::vector<double> originV = vtx::helper::StringToVector<double>(originXML.value());
    if (originV.size() != 3)
    {
      throw std::runtime_error(
        "ERROR: incorrect Origin attribute in ImageData from " + this->Engine.Name());
    }
    this->ImageData->SetOrigin(originV.data());

    // TODO: allow varying mesh over time by assigning domain extent to
    // variables

    // Whole Extent is where piece partition is taken into account
    const pugi::xml_attribute wholeExtentXML = vtx::helper::XMLAttribute(
      "WholeExtent", extentNode, true, "when reading WholeExtent in ImageData", true);

    this->WholeExtent = vtx::helper::StringToVector<std::size_t>(wholeExtentXML.value());
    if (this->WholeExtent.size() != 6)
    {
      throw std::runtime_error("ERROR: incorrect WholeExtent attribute, must have 6 elements, "
                               "in ImageData from " +
        this->Engine.Name());
    }

    // partition is cell data based
    const adios2::Box<adios2::Dims> cellSelection = GetSelection(types::DataSetType::CellData);
    const adios2::Dims& start = cellSelection.first;
    const adios2::Dims& count = cellSelection.second;

    std::vector<int> extent(6);
    // set extent transforming to VTK's Column Major representation
    // extent indices are point-based
    for (size_t i = 0; i < 3; ++i)
    {
      extent[2 * i] = static_cast<int>(start[2 - i]);
      extent[2 * i + 1] = static_cast<int>(start[2 - i] + count[2 - i]);
    }
    this->ImageData->SetExtent(extent.data());
  };

  // BODY OF FUNCTION STARTS HERE
  const pugi::xml_document xmlDocument =
    vtx::helper::XMLDocument(this->Schema, true, "when reading xml vti schema");

  const pugi::xml_node xmlVTKFileNode = vtx::helper::XMLNode(
    "VTKFile", xmlDocument, true, "when reading VTKFile type=ImageData node", true, true);

  const pugi::xml_node xmlImageDataNode = vtx::helper::XMLNode(
    "ImageData", xmlVTKFileNode, true, "when reading ImageData node", true, true);

  lf_InitExtent(xmlImageDataNode);

  size_t pieces = 0;
  for (const pugi::xml_node& xmlPieceNode : xmlImageDataNode.children("Piece"))
  {
    types::Piece piece;
    lf_InitPieceDataSetType(piece, types::DataSetType::CellData, xmlPieceNode);
    lf_InitPieceDataSetType(piece, types::DataSetType::PointData, xmlPieceNode);
    this->Pieces.push_back(piece);
    ++pieces;
  }
  if (pieces == 0)
  {
    throw std::invalid_argument("ERROR: could not find Piece XML-node when "
                                "reading ImageData XML-node "
                                "in ADIOS2 VTK XML Schema source\n");
  }
}

adios2::Dims VTXvtkVTI::GetShape(const types::DataSetType type)
{
  adios2::Dims shape(3);
  size_t add = 0;
  if (type == types::DataSetType::CellData)
  {
    add = 0;
  }
  else if (type == types::DataSetType::PointData)
  {
    add = 1;
  }

  for (size_t i = 0; i < 3; ++i)
  {
    shape[i] = this->WholeExtent[2 * i + 1] - this->WholeExtent[2 * i] + add;
  }

  return shape;
}

adios2::Box<adios2::Dims> VTXvtkVTI::GetSelection(const types::DataSetType type)
{
  // partition is always cell data based
  const adios2::Dims shape = GetShape(types::DataSetType::CellData);
  adios2::Box<adios2::Dims> selection = helper::PartitionCart1D(shape);

  // slowest
  if (type == types::DataSetType::PointData)
  {
    for (auto& dim : selection.second)
    {
      ++dim;
    }
  }

  return selection;
}

#define declare_type(T)                                                                            \
  void VTXvtkVTI::SetDimensions(                                                                   \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step)            \
  {                                                                                                \
    SetDimensionsCommon(variable, dataArray, step);                                                \
  }
VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

} // end namespace schema
} // end namespace vtx
