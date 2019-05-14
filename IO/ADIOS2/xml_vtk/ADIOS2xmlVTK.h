/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2xmlVTK.h
 *
 *  Created on: May 14, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_XML_VTK_ADIOS2XMLVTK_H_
#define VTK_IO_ADIOS2_XML_VTK_ADIOS2XMLVTK_H_

#include "ADIOS2Schema.h"
#include "ADIOS2Types.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace adios2vtk
{
namespace xml
{

class ADIOS2xmlVTK : public ADIOS2Schema
{
public:
  ADIOS2xmlVTK(
    const std::string type, const std::string& schema, adios2::IO* io, adios2::Engine* engine);

  // can't use = default, due to forward class not defined
  virtual ~ADIOS2xmlVTK();

protected:
  std::vector<types::Piece> m_Pieces;

  const static std::set<std::string> m_TIMENames;

  virtual void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) = 0;
  virtual void ReadPiece(const size_t step, const size_t pieceID) = 0;

  bool ReadDataSets(const types::DataSetType type, const size_t step, const size_t pieceID,
    const std::string& hint);

  void InitTimes() final;
};

} // end namespace xml
} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_XML_VTK_ADIOS2XMLVTK_H_ */
