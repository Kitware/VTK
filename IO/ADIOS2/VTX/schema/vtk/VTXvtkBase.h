// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXvtkBase.h : abstract class for schemas of type
 *                  [VTK XML file formats schemas]
 *                  (https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf)
 *                  Provide common functionality.
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkBase_h
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkBase_h

#include "VTX/common/VTXTypes.h"
#include "VTX/schema/VTXSchema.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace vtx
{
namespace schema
{
VTK_ABI_NAMESPACE_BEGIN

class VTXvtkBase : public VTXSchema
{
public:
  VTXvtkBase(
    const std::string& type, const std::string& schema, adios2::IO& io, adios2::Engine& engine);

  // can't use = default, due to forward class not defined
  ~VTXvtkBase() override;

protected:
  std::vector<types::Piece> Pieces;

  const static std::set<std::string> TIMENames;
  const static std::set<std::string> SpecialNames;
  const static std::map<types::DataSetType, std::string> DataSetTypes;

  void DoFill(vtkMultiBlockDataSet* multiBlock, size_t step) override = 0;
  void ReadPiece(size_t step, size_t pieceID) override = 0;

  bool ReadDataSets(types::DataSetType type, size_t step, size_t pieceID);

  void Init() override = 0;
  void InitTimes() final;

  std::string DataSetType(types::DataSetType type) const noexcept;
};

VTK_ABI_NAMESPACE_END
} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkBase_h */
