/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXSchemaManager.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXSchemaManager.h : reusable class that manages a reader that
 *                         is a derived type of VTXSchema
 *
 *  Created on: May 31, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_VTXSchemaManager_H_
#define VTK_IO_ADIOS2_VTX_VTXSchemaManager_H_

#include <memory>
#include <set>
#include <string>

#include "vtkMultiBlockDataSet.h"

#include "schema/VTXSchema.h"
#include <adios2.h>

namespace vtx
{

class VTXSchemaManager
{
public:
  /** current time*/
  double Time = 0.;
  /** current adios2 step */
  size_t Step = 0;

  /** managed polymorphic reader, could be extended in a container */
  std::unique_ptr<VTXSchema> Reader;

  VTXSchemaManager() = default;
  ~VTXSchemaManager() = default;

  /**
   * Updates metadata if stream is changed
   * @param streamName input current stream name
   * @param step input current step
   * @param schemaName schema name to look for either as attribute or separate
   * file
   */
  void Update(const std::string& streamName, const size_t step = 0,
    const std::string& schemaName = "vtk.xml");

  /**
   * Fill multiblock data
   * @param multiblock output structure to be filled by m_Reader
   * @param step input data for one step at a time
   */
  void Fill(vtkMultiBlockDataSet* multiblock, const size_t step = 0);

private:
  /** Current stream name */
  std::string StreamName;

  /** Single ADIOS object alive during the entire run */
  std::unique_ptr<adios2::ADIOS> ADIOS;

  /** Current ADIOS2 IO used for getting variables */
  adios2::IO IO;

  /** Current ADIOS2 Engine doing the heavy work */
  adios2::Engine Engine;

  /** carries the schema information */
  std::string SchemaName;

  static const std::set<std::string> SupportedTypes;

  /** we can extend this to add more schemas */
  void InitReader();

  /** Called within InitReader */
  bool InitReaderXMLVTK();
};

} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_VTX_VTXSchemaManager_h */
