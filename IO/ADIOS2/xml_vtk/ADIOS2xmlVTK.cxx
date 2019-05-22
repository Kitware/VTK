/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2xmlVTK.cxx
 *
 *  Created on: May 14, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2xmlVTK.h"

#include <adios2.h>

namespace adios2vtk
{
namespace xml
{

const std::set<std::string> ADIOS2xmlVTK::m_TIMENames = { "TIME", "CYCLE" };

ADIOS2xmlVTK::ADIOS2xmlVTK(
  const std::string type, const std::string& schema, adios2::IO* io, adios2::Engine* engine)
  : ADIOS2Schema(type, schema, io, engine)
{
}

ADIOS2xmlVTK::~ADIOS2xmlVTK() {}

bool ADIOS2xmlVTK::ReadDataSets(
  const types::DataSetType type, const size_t step, const size_t pieceID, const std::string& hint)
{
  if (m_Pieces.size() < pieceID)
  {
    throw std::invalid_argument(
      "ERROR: pieceID " + std::to_string(pieceID) + " not found " + hint + "\n");
  }

  types::Piece& piece = m_Pieces[pieceID];
  auto itDataSet = piece.find(type);
  if (itDataSet == piece.end())
  {
    return false;
  }

  types::DataSet& dataSet = itDataSet->second;
  for (auto& dataArrayPair : dataSet)
  {
    const std::string& name = dataArrayPair.first;
    types::DataArray& dataArray = dataArrayPair.second;

    if (dataArray.Vector.empty()) // is Scalar
    {
      // deferred
      GetDataArray(name, dataArray.Scalar, step);
    }
    else
    {
      for (auto& vPair : dataArray.Vector)
      {
        // deferred
        GetDataArray(vPair.first, vPair.second, step);
      }
    }
  }
  return true;
}

void ADIOS2xmlVTK::InitTimes()
{
  bool foundTime = false;

  for (types::Piece& piece : m_Pieces)
  {
    for (auto& itDataSet : piece)
    {
      for (auto& itDataArray : itDataSet.second)
      {
        const std::string& name = itDataArray.first;
        if (name == "TIME" || name == "CYCLE")
        {
          if (itDataArray.second.Vector.empty())
          {
            throw std::runtime_error(
              "ERROR: found time tag " + name + " but no variable associated with it\n");
          }
          const std::string& variableName = itDataArray.second.Vector.begin()->first;
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

} // end namespace xml
} // end namespace adios2vtk
