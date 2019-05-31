/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2xmlVTK.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2xmlVTK.h : abstract class for schemas of type
 *                  [VTK XML file formats schemas]
 *                  (https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf)
 *                  Provide common functionality.
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_SCHEMA_XML_VTK_ADIOS2XMLVTK_H_
#define VTK_IO_ADIOS2_SCHEMA_XML_VTK_ADIOS2XMLVTK_H_

#include "ADIOS2Types.h"
#include "schema/ADIOS2Schema.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace adios2vtk
{
namespace schema
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
  const static std::map<types::DataSetType, std::string> m_DataSetTypes;

  virtual void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) = 0;
  virtual void ReadPiece(const size_t step, const size_t pieceID) = 0;

#define declare_type(T)                                                                            \
  virtual void SetDimensions(                                                                      \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step) = 0;
  ADIOS2_VTK_ARRAY_TYPE(declare_type)
#undef declare_type

  bool ReadDataSets(const types::DataSetType type, const size_t step, const size_t pieceID,
    const std::string& hint);

  virtual void Init() = 0;
  void InitTimes() final;

  std::string DataSetType(const types::DataSetType type) const noexcept;
};

} // end namespace schema
} // end namespace adios2vtk

#endif
