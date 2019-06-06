/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2xmlVTK.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2xmlVTK.cxx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2xmlVTK.h"

#include <adios2.h>

namespace adios2vtk
{
namespace schema
{

const std::set<std::string> ADIOS2xmlVTK::TIMENames = { "TIME", "CYCLE" };

const std::map<types::DataSetType, std::string> ADIOS2xmlVTK::DataSetTypes = {
  { types::DataSetType::CellData, "CellData" }, { types::DataSetType::PointData, "PointData" },
  { types::DataSetType::Points, "Points" }, { types::DataSetType::Coordinates, "Coordinates" },
  { types::DataSetType::Cells, "Cells" }, { types::DataSetType::Verts, "Verts" },
  { types::DataSetType::Lines, "Lines" }, { types::DataSetType::Strips, "Strips" },
  { types::DataSetType::Polys, "Polys" }
};

ADIOS2xmlVTK::ADIOS2xmlVTK(
  const std::string type, const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : ADIOS2Schema(type, schema, io, engine)
{
}

ADIOS2xmlVTK::~ADIOS2xmlVTK() {}

bool ADIOS2xmlVTK::ReadDataSets(
  const types::DataSetType type, const size_t step, const size_t pieceID, const std::string& hint)
{
  types::Piece& piece = this->Pieces.at(pieceID);
  types::DataSet& dataSet = piece.at(type);

  for (auto& dataArrayPair : dataSet)
  {
    const std::string& variableName = dataArrayPair.first;
    types::DataArray& dataArray = dataArrayPair.second;
    if (this->TIMENames.count(variableName) == 1)
    {
      continue;
    }
    GetDataArray(variableName, dataArray, step);
  }
  return true;
}

void ADIOS2xmlVTK::InitTimes()
{
  bool foundTime = false;

  for (types::Piece& piece : this->Pieces)
  {
    for (auto& itDataSet : piece)
    {
      for (auto& itDataArray : itDataSet.second)
      {
        const std::string& name = itDataArray.first;
        if (name == "TIME" || name == "CYCLE")
        {
          const std::vector<std::string>& vecComponents = itDataArray.second.VectorVariables;
          const std::string& variableName = vecComponents.front();
          GetTimes(variableName);
          foundTime = true;
          return;
        }
      }
    }
  }

  // ADIOS2 will just use steps
  if (!foundTime)
  {
    GetTimes();
  }
}

std::string ADIOS2xmlVTK::DataSetType(const types::DataSetType type) const noexcept
{
  return this->DataSetTypes.at(type);
}

} // end namespace schema
} // end namespace adios2vtk
