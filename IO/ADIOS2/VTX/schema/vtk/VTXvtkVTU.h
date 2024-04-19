// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXvtkVTU.h : class that supports UnstructuredMesh schema in VTK XML
 * format .vtu extends abstract VTXvtkBase
 *
 *  Created on: June 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_h
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_h

#include <map>
#include <string>
#include <vector>

#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"

#include "VTX/schema/vtk/VTXvtkBase.h"

namespace vtx
{
namespace schema
{
VTK_ABI_NAMESPACE_BEGIN
class VTXvtkVTU : public VTXvtkBase
{
public:
  VTXvtkVTU(const std::string& schema, adios2::IO& io, adios2::Engine& engine);
  ~VTXvtkVTU() override;

private:
  /** Could be extended in a container, this is a per-rank ImageData */
  vtkNew<vtkUnstructuredGrid> UnstructuredGrid;

  /** BlockIDs carried by current rank */
  std::vector<size_t> BlockIDs;

  void DoFill(vtkMultiBlockDataSet* multiBlock, size_t step) final;
  void ReadPiece(size_t step, size_t pieceID) final;

  void Init() final;

#define declare_type(T)                                                                            \
  void SetBlocks(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step) final;
  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

  template <class T>
  void SetBlocksCommon(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);
};

VTK_ABI_NAMESPACE_END
} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_h */
