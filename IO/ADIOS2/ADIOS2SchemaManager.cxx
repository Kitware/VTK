/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2SchemaManager.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2SchemaManager.cxx
 *
 *  Created on: May 31, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2SchemaManager.h"

#include "ADIOS2Helper.h"

#include "schema/xml_vtk/ADIOS2xmlVTI.h"

#include <vtk_pugixml.h>
#include <vtksys/SystemTools.hxx>

namespace adios2vtk
{

// PUBLIC
void ADIOS2SchemaManager::Update(
  const std::string& streamName, const size_t step, const std::string& schemaName)
{
  // can't do it in the constructor as it need MPI initialized
  if (!m_ADIOS)
  {
    m_ADIOS.reset(new adios2::ADIOS(helper::MPIGetComm()));
  }

  if (!m_IO && !m_Engine)
  {
    m_StreamName = streamName;
    m_SchemaName = schemaName;
    m_IO = m_ADIOS->DeclareIO(m_StreamName);
    m_Engine = m_IO.Open(m_StreamName, adios2::Mode::Read);
    InitReader();
  }
  else
  {
    // TODO: check if variables changed
  }
}

void ADIOS2SchemaManager::Fill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  m_Reader->Fill(multiBlock, step);
}

// PRIVATE
const std::set<std::string> ADIOS2SchemaManager::m_SupportedTypes = { "ImageData" };
// TODO: , "StructuredGrid", "UnstructuredGrid" };

void ADIOS2SchemaManager::InitReader()
{
  if (InitReaderXMLVTK())
  {
    return;
  }
  // else if( InitReaderOther() ) {}
  // here we can make it extensible by trying to find other schema types
  // for now we stick with VTK XML schemas
}

bool ADIOS2SchemaManager::InitReaderXMLVTK()
{

  pugi::xml_document xmlDocument;
  std::string xmlContents;

  bool isSchemaFile = false;

  // auto lf_GetXMLDoc = [&]() {
  // check if it's file, not optimizing with MPI_Bcast
  std::string xmlFileName;
  if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name()))
  {
    xmlFileName = m_Engine.Name() + "/" + m_SchemaName;
  }
  else if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name() + ".dir"))
  {
    xmlFileName = m_Engine.Name() + ".dir/" + m_SchemaName;
  }

  if (!xmlFileName.empty())
  {
    if (vtksys::SystemTools::FileExists(xmlFileName))
    {
      xmlContents = adios2vtk::helper::FileToString(xmlFileName);
      xmlDocument =
        adios2vtk::helper::XMLDocument(xmlContents, true, "when reading " + m_SchemaName + " file");
      isSchemaFile = true;
    }
  }

  if (!isSchemaFile)
  {
    const adios2::Attribute<std::string> vtkXMLAttribute =
      m_IO.InquireAttribute<std::string>(m_SchemaName);
    const std::vector<std::string> vtkAttributes = vtkXMLAttribute.Data();
    if (vtkAttributes.empty())
    {
      throw std::runtime_error("ERROR: neither " + m_SchemaName +
        " file or bp attribute was found in " + m_Engine.Name() + "\n");
    }

    xmlContents = vtkAttributes.front();
    xmlDocument = adios2vtk::helper::XMLDocument(
      xmlContents, true, "when reading " + m_SchemaName + " attribute");
  }

  // auto lf_InitReader = [&]() {
  constexpr bool isDebug = true;
  constexpr bool isMandatory = true;
  constexpr bool isUnique = true;

  const pugi::xml_node vtkXMLFileNode = adios2vtk::helper::XMLNode("VTKFile", xmlDocument, isDebug,
    "when reading VTKFile node in " + m_Engine.Name(), isMandatory, isUnique);

  const pugi::xml_attribute typeXML = adios2vtk::helper::XMLAttribute("type", vtkXMLFileNode, true,
    "when reading type xml attribute in vtk.xml " + m_Engine.Name(), isMandatory);

  const std::string type = std::string(typeXML.value());

  if (m_SupportedTypes.count(type) == 0)
  {
    throw std::runtime_error(
      "ERROR: ADIOS2Reader only supports types= " + adios2vtk::helper::SetToCSV(m_SupportedTypes) +
      " when reading type xml attribute in " + m_SchemaName + " from " + m_Engine.Name() + "\n");
  }

  if (type == "ImageData")
  {
    m_Reader.reset(new adios2vtk::schema::ADIOS2xmlVTI(xmlContents, &m_IO, &m_Engine));
  }

  const bool success = m_Reader ? true : false;
  return success;
}

} // end adios2vtk
