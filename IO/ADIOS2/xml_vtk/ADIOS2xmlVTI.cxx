/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2xmlVTI.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2xmlVTI.h"

#include <iostream>
#include <utility>

#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"

#include <vtk_pugixml.h>

#include "ADIOS2Helper.h"

#include <adios2.h>

namespace adios2vtk
{
namespace xml
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

  // TODO MPI decomposition
  const unsigned int rank = static_cast<unsigned int>(helper::MPIGetRank());

  // TODO debugging
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

      if (dataArray.Vector.empty()) // is Scalar
      {
        m_ImageData->GetCellData()->AddArray(dataArray.Scalar.GetPointer());
      }
      else
      {
        // TODO treat as vector
        for (auto& vPair : dataArray.Vector)
        {
          m_ImageData->GetCellData()->AddArray(vPair.second.GetPointer());
        }
      }
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

      if (dataArray.Vector.empty()) // is Scalar
      {
        m_ImageData->GetPointData()->AddArray(dataArray.Scalar.GetPointer());
      }
      else
      {
        // TODO treat as vector
        for (auto& vPair : dataArray.Vector)
        {
          m_ImageData->GetPointData()->AddArray(vPair.second.GetPointer());
        }
      }
    }
  }
}

// PRIVATE
void ADIOS2xmlVTI::Init()
{
  auto lf_InitPiece = [&](const pugi::xml_node& pieceNode) {
    types::Piece piece;
    const pugi::xml_node cellDataNode = helper::XMLNode(
      "CellData", pieceNode, true, "when reading CellData node in ImageData", false);

    piece[types::DataSetType::CellData] = helper::XMLInitDataSet(cellDataNode, m_TIMENames);

    const pugi::xml_node pointDataNode = adios2vtk::helper::XMLNode(
      "PointData", pieceNode, true, "when reading CellData node in ImageData", false);

    piece[types::DataSetType::PointData] = helper::XMLInitDataSet(pointDataNode, m_TIMENames);

    m_Pieces.push_back(piece);
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

    const std::vector<size_t> wholeExtentV =
      adios2vtk::helper::StringToVector<size_t>(wholeExtentXML.value());
    if (wholeExtentV.size() != 6)
    {
      throw std::runtime_error(
        "ERROR: incorrect WholeExtent attribute in ImageData from" + m_Engine->Name());
    }

    adios2::Dims shape(3);
    for (size_t i = 0; i < 3; ++i)
    {
      shape[2 - i] = wholeExtentV[2 * i + 1] - wholeExtentV[2 * i] - 1;
      std::cout << "Shape " << i << " " << shape[i] << "\n";
    }

    const adios2::Box<adios2::Dims> selection = helper::PartitionCart1D(shape);
    const adios2::Dims& start = selection.first;
    const adios2::Dims& count = selection.second;

    std::vector<int> extent(6);
    for (size_t i = 0; i < 3; ++i)
    {
      extent[2 * i] = static_cast<int>(start[2 - i]);
      extent[2 * i + 1] = static_cast<int>(start[2 - i] + count[2 - i]);

      std::cout << "Extent " << i << " " << extent[2 * i] << " " << extent[2 * i + 1] << "\n";
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
    lf_InitPiece(xmlPieceNode);
  }
}

} // end namespace xml
} // end namespace adios2vtk
