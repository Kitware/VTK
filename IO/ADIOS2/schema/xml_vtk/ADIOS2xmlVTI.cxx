/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2xmlVTI.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2xmlVTI.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2xmlVTI.h"
#include "ADIOS2xmlVTI.txx"

#include <algorithm>
#include <utility>

#include "ADIOS2Helper.h"

#include "vtkCellData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"

#include <vtk_pugixml.h>

#include <adios2.h>

namespace adios2vtk
{
namespace schema
{

ADIOS2xmlVTI::ADIOS2xmlVTI(const std::string& schema, adios2::IO* io, adios2::Engine* engine)
  : ADIOS2xmlVTK("vti", schema, io, engine)
{
  Init();
  InitTimes();
}

ADIOS2xmlVTI::~ADIOS2xmlVTI() {}

// PRIVATE
void ADIOS2xmlVTI::DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  ReadPiece(step, 0); // just read piece 0 for now

  const unsigned int rank = static_cast<unsigned int>(helper::MPIGetRank());

  vtkNew<vtkMultiPieceDataSet> pieces;
  pieces->SetPiece(rank, m_ImageData);
  multiBlock->SetBlock(0, pieces);
}

void ADIOS2xmlVTI::ReadPiece(const size_t step, const size_t pieceID)
{
  const bool hasCellData =
    ReadDataSets(types::DataSetType::CellData, step, pieceID, " in ImageData VTK XML Schema\n");
  const bool hasPointData =
    ReadDataSets(types::DataSetType::PointData, step, pieceID, " in ImageData VTK XML Schema\n");

  m_Engine->PerformGets();

  // CellData
  if (hasCellData)
  {
    types::DataSet& dataSet = m_Pieces[pieceID][types::DataSetType::CellData];
    for (auto& dataArrayPair : dataSet)
    {
      const std::string& variableName = dataArrayPair.first;
      if (m_TIMENames.count(variableName) == 1)
      {
        continue;
      }

      types::DataArray& dataArray = dataArrayPair.second;
      m_ImageData->GetCellData()->AddArray(dataArray.m_vtkDataArray.GetPointer());
    }
  }

  // Point Data
  if (hasPointData)
  {
    types::DataSet& dataSet = m_Pieces[pieceID][types::DataSetType::PointData];
    for (auto& dataArrayPair : dataSet)
    {
      const std::string& variableName = dataArrayPair.first;
      if (m_TIMENames.count(variableName) == 1)
      {
        continue;
      }
      types::DataArray& dataArray = dataArrayPair.second;
      m_ImageData->GetPointData()->AddArray(dataArray.m_vtkDataArray.GetPointer());
    }
  }
}

// PRIVATE
void ADIOS2xmlVTI::Init()
{
  auto lf_InitPieceDataSetType = [&](types::Piece& piece, const types::DataSetType type,
                                   const pugi::xml_node& pieceNode) {
    const std::string nodeName = DataSetType(type);
    const pugi::xml_node dataSetNode = helper::XMLNode(
      nodeName, pieceNode, true, "when reading " + nodeName + " node in ImageData", false);
    types::DataSet dataSet = helper::XMLInitDataSet(dataSetNode, m_TIMENames);

    for (auto& dataArrayPair : dataSet)
    {
      types::DataArray& dataArray = dataArrayPair.second;
      dataArray.m_Shape = GetShape(type);
      const auto& selection = GetSelection(type);
      dataArray.m_Start = selection.first;
      dataArray.m_Count = selection.second;
    }
    piece[type] = dataSet;
  };

  auto lf_InitExtent = [&](const pugi::xml_node& extentNode) {
    // Spacing
    const pugi::xml_attribute spacingXML = adios2vtk::helper::XMLAttribute(
      "Spacing", extentNode, true, "when reading Spacing in ImageData", true);
    const std::vector<double> spacingV =
      adios2vtk::helper::StringToVector<double>(spacingXML.value());
    if (spacingV.size() != 3)
    {
      throw std::runtime_error(
        "ERROR: incorrect Spacing attribute in ImageData from " + m_Engine->Name());
    }
    m_ImageData->SetSpacing(spacingV.data());

    // Origin
    const pugi::xml_attribute originXML = adios2vtk::helper::XMLAttribute(
      "Origin", extentNode, true, "when reading Origin in ImageData", true);
    const std::vector<double> originV =
      adios2vtk::helper::StringToVector<double>(originXML.value());
    if (originV.size() != 3)
    {
      throw std::runtime_error(
        "ERROR: incorrect Origin attribute in ImageData from " + m_Engine->Name());
    }
    m_ImageData->SetOrigin(originV.data());

    // TODO: allow varying mesh over time by assigning domain extent to variables

    // Whole Extent is where piece partition is taken into account
    const pugi::xml_attribute wholeExtentXML = adios2vtk::helper::XMLAttribute(
      "WholeExtent", extentNode, true, "when reading WholeExtent in ImageData", true);

    m_WholeExtent = adios2vtk::helper::StringToVector<size_t>(wholeExtentXML.value());
    if (m_WholeExtent.size() != 6)
    {
      throw std::runtime_error(
        "ERROR: incorrect WholeExtent attribute, must have 6 elements, in ImageData from " +
        m_Engine->Name());
    }

    // set extent
    const adios2::Box<adios2::Dims> cellSelection = GetSelection(types::DataSetType::CellData);
    const adios2::Dims& start = cellSelection.first;
    const adios2::Dims& count = cellSelection.second;

    std::vector<int> extent(6);
    for (size_t i = 0; i < 3; ++i)
    {
      extent[2 * i] = static_cast<int>(start[2 - i]);
      extent[2 * i + 1] = static_cast<int>(start[2 - i] + count[2 - i]);
    }
    m_ImageData->SetExtent(extent.data());
  };

  // BODY OF FUNCTION STARTS HERE
  const pugi::xml_document xmlDocument =
    adios2vtk::helper::XMLDocument(m_Schema, true, "when reading xml vti schema");

  const pugi::xml_node xmlVTKFileNode = adios2vtk::helper::XMLNode(
    "VTKFile", xmlDocument, true, "when reading VTKFile type=ImageData node", true, true);

  const pugi::xml_node xmlImageDataNode = adios2vtk::helper::XMLNode(
    "ImageData", xmlVTKFileNode, true, "when reading ImageData node", true, true);

  lf_InitExtent(xmlImageDataNode);

  for (const pugi::xml_node& xmlPieceNode : xmlImageDataNode.children("Piece"))
  {
    types::Piece piece;
    lf_InitPieceDataSetType(piece, types::DataSetType::CellData, xmlPieceNode);
    lf_InitPieceDataSetType(piece, types::DataSetType::PointData, xmlPieceNode);
    m_Pieces.push_back(piece);
  }
}

adios2::Dims ADIOS2xmlVTI::GetShape(const types::DataSetType type)
{
  if (type == types::DataSetType::CellData)
  {
    adios2::Dims shape(3);
    for (size_t i = 0; i < 3; ++i)
    {
      shape[2 - i] = m_WholeExtent[2 * i + 1] - m_WholeExtent[2 * i] - 1;
    }
    return shape;
  }
  else if (type == types::DataSetType::PointData)
  {
    adios2::Dims shape(3);
    for (size_t i = 0; i < 3; ++i)
    {
      shape[2 - i] = m_WholeExtent[2 * i + 1] - m_WholeExtent[2 * i];
    }
    return shape;
  }

  return adios2::Dims();
}

adios2::Box<adios2::Dims> ADIOS2xmlVTI::GetSelection(const types::DataSetType type)
{
  const adios2::Dims cellShape = GetShape(types::DataSetType::CellData);
  adios2::Box<adios2::Dims> selection = helper::PartitionCart1D(cellShape);

  // modify count if point data
  if (type == types::DataSetType::PointData)
  {
    adios2::Dims& count = selection.second;
    std::for_each(count.begin(), count.end(), [](size_t& dim) { dim += 1; });
  }

  return selection;
}

#define declare_type(T)                                                                            \
  void ADIOS2xmlVTI::SetDimensions(                                                                \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step)            \
  {                                                                                                \
    SetDimensionsCommon(variable, dataArray, step);                                                \
  }
ADIOS2_VTK_ARRAY_TYPE(declare_type)
#undef declare_type

} // end namespace schema
} // end namespace adios2vtk
