/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXvtkBase.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXvtkBase.cxx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXvtkBase.h"

#include <adios2.h>

namespace vtx
{
namespace schema
{
const std::set<std::string> VTXvtkBase::TIMENames = { "TIME", "CYCLE" };
const std::set<std::string> VTXvtkBase::SpecialNames = { "TIME", "CYCLE", "connectivity", "types",
  "vertices" };

const std::map<types::DataSetType, std::string> VTXvtkBase::DataSetTypes = {
  { types::DataSetType::CellData, "CellData" }, { types::DataSetType::PointData, "PointData" },
  { types::DataSetType::Points, "Points" }, { types::DataSetType::Coordinates, "Coordinates" },
  { types::DataSetType::Cells, "Cells" }, { types::DataSetType::Verts, "Verts" },
  { types::DataSetType::Lines, "Lines" }, { types::DataSetType::Strips, "Strips" },
  { types::DataSetType::Polys, "Polys" }
};

VTXvtkBase::VTXvtkBase(
  const std::string& type, const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : VTXSchema(type, schema, io, engine)
{
}

VTXvtkBase::~VTXvtkBase() {}

bool VTXvtkBase::ReadDataSets(
  const types::DataSetType type, const size_t step, const size_t pieceID)
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

void VTXvtkBase::InitTimes()
{
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
          return;
        }
      }
    }
  }

  // ADIOS2 will just use steps
  GetTimes();
}

std::string VTXvtkBase::DataSetType(const types::DataSetType type) const noexcept
{
  return this->DataSetTypes.at(type);
}

} // end namespace schema
} // end namespace adios2vtk
