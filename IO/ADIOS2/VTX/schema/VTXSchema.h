// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXSchema.h : abstract class from which all supported adios2 schemas
 *                  derive from. Provide common functionality.
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTXSchema_h
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTXSchema_h

#include <map>
#include <string>

#include "vtkMultiBlockDataSet.h"

#include <adios2.h>

#include "VTX/common/VTXDataArray.h"
#include "VTX/common/VTXTypes.h"

namespace vtx
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * Abstract common class to supported ADIOS2 schemas
 */
class VTXSchema
{
public:
  /** carries schema type from derived class */
  const std::string Type;

  /** schema contents as a single string */
  std::string Schema;

  /**
   * Stored times and corresponding steps
   * <pre>
   *   key: physical times
   *   value: adios2 step
   * </pre>
   */
  std::map<double, size_t> Times;

  /**
   * Generic base constructor
   * @param type from derived class
   * @param schema as input
   * @param io manages IO input containing variable information
   * @param engine manages stream input
   */
  VTXSchema(
    const std::string& type, const std::string& schema, adios2::IO& io, adios2::Engine& engine);

  // can't use = default, due to forward class not defined
  virtual ~VTXSchema();

  /**
   * Fills multiblock data from request steps
   * @param multiBlock output structure
   * @param step input adios2 step
   */
  void Fill(vtkMultiBlockDataSet* multiBlock, size_t step = 0);

protected:
  adios2::IO& IO;
  adios2::Engine& Engine;

  virtual void Init() = 0;
  virtual void InitTimes() = 0;

  virtual void DoFill(vtkMultiBlockDataSet* multiBlock, size_t step) = 0;
  virtual void ReadPiece(size_t step, size_t pieceID) = 0;

  void GetTimes(const std::string& variableName = "");
  void GetDataArray(const std::string& variableName, types::DataArray& dataArray, size_t step = 0);

#define declare_type(T)                                                                            \
  virtual void SetDimensions(                                                                      \
    adios2::Variable<T> variable, const types::DataArray& dataArray, size_t step);                 \
                                                                                                   \
  virtual void SetBlocks(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);

  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

private:
  template <class T>
  void GetDataArrayCommon(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);

  template <class T>
  void GetDataArrayGlobal(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);

  template <class T>
  void GetDataArrayLocal(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);

  template <class T>
  void GetDataValueGlobal(adios2::Variable<T> variable, types::DataArray& dataArray, size_t step);

  template <class T>
  void GetTimesCommon(const std::string& variableName);

  template <class T>
  void InitDataArray(
    const std::string& name, size_t elements, size_t components, types::DataArray& dataArray);
};

VTK_ABI_NAMESPACE_END
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTXSchema_h */
