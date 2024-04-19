// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXvtkVTI.h : class that supports ImageData schema in VTK XML format .vti
 *                  extends abstract ADIOS2xmlVTK
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h

#include <map>
#include <string>
#include <vector>

#include "VTX/schema/vtk/VTXvtkBase.h"

#include "vtkImageData.h"
#include "vtkNew.h"

namespace vtx
{
namespace schema
{
VTK_ABI_NAMESPACE_BEGIN
class VTXvtkVTI : public VTXvtkBase
{
public:
  VTXvtkVTI(const std::string& schema, adios2::IO& io, adios2::Engine& engine);
  ~VTXvtkVTI() override;

private:
  /** Could be extended in a container, this is a per-rank ImageData */
  vtkNew<vtkImageData> ImageData;
  /** Store the Whole Extent in physical dimensions, row-major */
  adios2::Dims WholeExtent;

  adios2::Dims GetShape(types::DataSetType type);
  adios2::Box<adios2::Dims> GetSelection(types::DataSetType type);

  void DoFill(vtkMultiBlockDataSet* multiBlock, size_t step) final;
  void ReadPiece(size_t step, size_t pieceID) final;

  void Init() final;

#define declare_type(T)                                                                            \
  void SetDimensions(adios2::Variable<T> variable, const types::DataArray& dataArray, size_t step) \
    final;
  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

  template <class T>
  void SetDimensionsCommon(
    adios2::Variable<T> variable, const types::DataArray& dataArray, size_t step);
};

VTK_ABI_NAMESPACE_END
} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h */
