/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXHelper.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXHelper.cxx
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXHelper.h"
#include "VTXHelper.txx"

#include <fstream>
#include <numeric> //std::accumulate
#include <sstream>

#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessController.h"

#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

namespace vtx
{
namespace helper
{

MPI_Comm MPIGetComm()
{
  MPI_Comm comm = MPI_COMM_NULL;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  vtkMPICommunicator* vtkComm = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  if (vtkComm)
  {
    if (vtkComm->GetMPIComm())
    {
      comm = *(vtkComm->GetMPIComm()->GetHandle());
    }
  }
  return comm;
}

int MPIGetRank()
{
  MPI_Comm comm = MPIGetComm();
  int rank;
  MPI_Comm_rank(comm, &rank);
  return rank;
}

int MPIGetSize()
{
  MPI_Comm comm = MPIGetComm();
  int size;
  MPI_Comm_size(comm, &size);
  return size;
}

pugi::xml_document XMLDocument(
  const std::string& input, const bool debugMode, const std::string& hint)
{
  pugi::xml_document document;

  pugi::xml_parse_result result =
    document.load_buffer(const_cast<char*>(input.data()), input.size());

  if (debugMode)
  {
    if (!result)
    {
      throw std::invalid_argument(
        "ERROR: XML: parse error in XML string, description: " + std::string(result.description()) +
        ", check with any XML editor if format is ill-formed, " + hint + "\n");
    }
  }
  return document;
}

pugi::xml_node XMLNode(const std::string nodeName, const pugi::xml_document& xmlDocument,
  const bool debugMode, const std::string& hint, const bool isMandatory, const bool isUnique)
{
  const pugi::xml_node node = xmlDocument.child(nodeName.c_str());

  if (debugMode)
  {
    if (isMandatory && !node)
    {
      throw std::invalid_argument("ERROR: XML: no <" + nodeName + "> element found, " + hint);
    }

    if (isUnique)
    {
      const size_t nodes = std::distance(xmlDocument.children(nodeName.c_str()).begin(),
        xmlDocument.children(nodeName.c_str()).end());
      if (nodes > 1)
      {
        throw std::invalid_argument("ERROR: XML only one <" + nodeName +
          "> element can exist inside " + std::string(xmlDocument.name()) + ", " + hint + "\n");
      }
    }
  }
  return node;
}

pugi::xml_node XMLNode(const std::string nodeName, const pugi::xml_node& upperNode,
  const bool debugMode, const std::string& hint, const bool isMandatory, const bool isUnique)
{
  const pugi::xml_node node = upperNode.child(nodeName.c_str());

  if (debugMode)
  {
    if (isMandatory && !node)
    {
      throw std::invalid_argument("ERROR: XML: no <" + nodeName + "> element found, inside <" +
        std::string(upperNode.name()) + "> element " + hint);
    }

    if (isUnique)
    {
      const size_t nodes = std::distance(
        upperNode.children(nodeName.c_str()).begin(), upperNode.children(nodeName.c_str()).end());
      if (nodes > 1)
      {
        throw std::invalid_argument("ERROR: XML only one <" + nodeName +
          "> element can exist inside <" + std::string(upperNode.name()) + "> element, " + hint +
          "\n");
      }
    }
  }
  return node;
}

pugi::xml_attribute XMLAttribute(const std::string attributeName, const pugi::xml_node& node,
  const bool debugMode, const std::string& hint, const bool isMandatory)
{
  const pugi::xml_attribute attribute = node.attribute(attributeName.c_str());

  if (debugMode)
  {
    if (isMandatory && !attribute)
    {
      const std::string nodeName(node.name());

      throw std::invalid_argument("ERROR: XML: No attribute " + attributeName + " found on <" +
        nodeName + "> element" + hint);
    }
  }
  return attribute;
}

types::DataSet XMLInitDataSet(
  const pugi::xml_node& dataSetNode, const std::set<std::string>& specialNames)
{
  types::DataSet dataSet;

  for (const pugi::xml_node& dataArrayNode : dataSetNode)
  {
    const pugi::xml_attribute xmlName = XMLAttribute(
      "Name", dataArrayNode, true, "when parsing Name attribute in ADIOS2 VTK XML schema", true);
    auto result = dataSet.emplace(xmlName.value(), types::DataArray());
    types::DataArray& dataArray = result.first->second;

    // handle special names
    const std::string name(xmlName.value());
    auto itSpecialName = specialNames.find(name);
    const bool isSpecialName = (itSpecialName != specialNames.end()) ? true : false;
    if (isSpecialName)
    {
      const std::string specialName = *itSpecialName;
      if (specialName == "connectivity")
      {
        dataArray.IsIdType = true;
        dataArray.Persist = true;
      }
      else if (specialName == "vertices")
      {
        dataArray.HasTuples = true;
        dataArray.Persist = true;

        const pugi::xml_attribute xmlOrder = XMLAttribute("Ordering", dataArrayNode, true,
          "when parsing vertices \"Order\" attribute in ADIOS2 VTK XML schema", false);
        const std::string order(xmlOrder.value());
        // XXXX, YYYY, ZZZZ struct of arrays
        if (order == "SOA")
        {
          dataArray.IsSOA = true;
        }
      }
      else if (specialName == "types")
      {
        dataArray.Persist = true;
      }
    }

    // not mandatory
    const pugi::xml_attribute xmlNumberOfComponents =
      XMLAttribute("NumberOfComponents", dataArrayNode, true,
        "when parsing NumberOfComponents attribute in ADIOS2 "
        "VTK XML schema",
        false);

    // TODO enable vector support
    if (!xmlNumberOfComponents && !isSpecialName)
    {
      continue;
    }

    // these are node_pcdata
    for (const pugi::xml_node& componentNode : dataArrayNode)
    {
      if (componentNode.type() != pugi::node_pcdata)
      {
        throw std::runtime_error("ERROR: NumberOfComponents attribute found, but "
                                 "component " +
          std::string(componentNode.name()) + " in node " + std::string(dataArrayNode.value()) +
          " is not of plain data type in ADIOS2 VTK XML schema\n");
      }
      // TRIM
      std::string variablePCData(componentNode.value());
      variablePCData.erase(0, variablePCData.find_first_not_of(" \n\r\t"));
      variablePCData.erase(variablePCData.find_last_not_of(" \n\r\t") + 1);

      dataArray.VectorVariables.push_back(variablePCData);
    }

    if (xmlNumberOfComponents)
    {
      const size_t components = static_cast<size_t>(std::stoull(xmlNumberOfComponents.value()));
      if (dataArray.VectorVariables.size() != components)
      {
        throw std::runtime_error("ERROR: NumberOfComponents " + std::to_string(components) +
          " and variable names found " + std::to_string(dataArray.VectorVariables.size()) +
          " inside DataArray node " + std::string(xmlName.name()) + " in ADIOS2 VTK XML schema");
      }
    }

    if (dataArray.IsScalar() && (name == "TIME" || name == "CYCLE"))
    {
      throw std::invalid_argument("ERROR: data array " + name +
        " expected to have a least one component, in ADIOS2 VTK XML "
        "schema\n");
    }
  }

  return dataSet;
}

std::string FileToString(const std::string& fileName)
{
  vtksys::ifstream file(fileName.c_str());
  std::stringstream schemaSS;
  schemaSS << file.rdbuf();
  return schemaSS.str();
}

std::string SetToCSV(const std::set<std::string>& input) noexcept
{
  std::string csv = "{ ";
  for (const std::string& el : input)
  {
    csv += el + ", ";
  }
  if (input.size() > 0)
  {
    csv.pop_back();
    csv.pop_back();
    csv += " }";
  }
  return csv;
}

std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept
{
  return std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<std::size_t>());
}

// allowed types
template vtkSmartPointer<vtkDataArray> NewDataArray<int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<unsigned int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<long int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<unsigned long int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<long long int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<unsigned long long int>();
template vtkSmartPointer<vtkDataArray> NewDataArray<float>();
template vtkSmartPointer<vtkDataArray> NewDataArray<double>();

adios2::Box<adios2::Dims> PartitionCart1D(const adios2::Dims& shape)
{
  adios2::Box<adios2::Dims> selection({ adios2::Dims(shape.size(), 0), shape });

  const size_t mpiRank = static_cast<size_t>(MPIGetRank());
  const size_t mpiSize = static_cast<size_t>(MPIGetSize());

  // slowest index
  if (shape[0] >= mpiSize)
  {
    const size_t elements = shape[0] / mpiSize;
    // start
    selection.first[0] = mpiRank * elements;
    // count
    selection.second[0] = (mpiRank == mpiSize - 1) ? elements + shape[0] % mpiSize : elements;
  }

  return selection;
}

size_t LinearizePoint(const adios2::Dims& shape, const adios2::Dims& point) noexcept
{
  const size_t i = point[0];
  const size_t j = point[1];
  const size_t k = point[2];

  const size_t Ny = shape[1];
  const size_t Nz = shape[2];

  return i * Ny * Nz + j * Nz + k;
}

vtkSmartPointer<vtkIdTypeArray> NewDataArrayIdType()
{
  return vtkSmartPointer<vtkIdTypeArray>::New();
}

std::string GetFileName(const std::string& fileName) noexcept
{
  const std::string output =
    EndsWith(fileName, ".bp.dir") ? fileName.substr(0, fileName.size() - 4) : fileName;
  return output;
}

std::string GetEngineType(const std::string& fileName) noexcept
{
  const std::string engineType = vtksys::SystemTools::FileIsDirectory(fileName) ? "BP4" : "BP3";
  return engineType;
}

bool EndsWith(const std::string& input, const std::string& ends) noexcept
{
  if (input.length() >= ends.length())
  {
    return (!input.compare(input.length() - ends.length(), ends.length(), ends));
  }
  return false;
}

} // end helper namespace
} // end adios2vtk namespace
