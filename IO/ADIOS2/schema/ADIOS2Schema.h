/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2Schema.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2Schema.h : abstract class from which all supported adios2 schemas
 *                  derive from. Provide common functionality.
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_SCHEMA_ADIOS2SCHEMA_H_
#define VTK_IO_ADIOS2_SCHEMA_ADIOS2SCHEMA_H_

#include "ADIOS2DataArray.h"
#include "ADIOS2Types.h"

#include <map>
#include <string>

#include "vtkMultiBlockDataSet.h"

#include <adios2.h>

namespace adios2vtk
{
/**
 * Abstract common class to supported ADIOS2 schemas
 */
class ADIOS2Schema
{
public:
  /** carries schema type from derived class */
  const std::string m_Type;

  /** schema contents as a single string */
  std::string m_Schema;

  /**
   * Stored times and corresponding steps
   * <pre>
   * 	key: physical times
   * 	value: adios2 step
   * </pre>
   */
  std::map<double, size_t> m_Times;

  /**
   * Generic base constructor
   * @param type from derived class
   * @param schema as input
   * @param io manages IO input containing variable information
   * @param engine manages stream input
   */
  ADIOS2Schema(
    const std::string type, const std::string& schema, adios2::IO* io, adios2::Engine* engine);

  // can't use = default, due to forward class not defined
  virtual ~ADIOS2Schema();

  /**
   * Fills multiblock data from request steps
   * @param multiBlock output structure
   * @param step input adios2 step
   */
  void Fill(vtkMultiBlockDataSet* multiBlock, const size_t step = 0);

protected:
  adios2::IO* m_IO = nullptr;
  adios2::Engine* m_Engine = nullptr;

  virtual void Init() = 0;
  virtual void InitTimes() = 0;

  virtual void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) = 0;
  virtual void ReadPiece(const size_t step, const size_t pieceID) = 0;

  void GetTimes(const std::string& variableName = "");
  void GetDataArray(const std::string& variableName, types::DataArray& dataArray,
    const size_t step = 0, const std::string mode = "deferred");

protected:
#define declare_type(T)                                                                            \
  virtual void SetDimensions(                                                                      \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step) = 0;
  ADIOS2_VTK_ARRAY_TYPE(declare_type)
#undef declare_type

private:
  template<class T>
  void GetTimesCommon(const std::string& variableName);

  template<class T>
  void GetDataArrayCommon(adios2::Variable<T> variable, types::DataArray& dataArray,
    const size_t step, const std::string mode);
};

} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_ADIOS2SCHEMA_H_ */
