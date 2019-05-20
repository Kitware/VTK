/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2Helper.cxx
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <fstream>
#include <numeric> //std::accumulate
#include <sstream>

#include "ADIOS2Helper.h"
#include "ADIOS2Helper.txx"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessController.h"

namespace adios2vtk
{
namespace helper
{

MPI_Comm MPIGetComm()
{
  MPI_Comm comm = nullptr;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  vtkMPICommunicator* vtkComm = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  if (vtkComm)
  {
    if (vtkComm->GetMPIComm())
    {
      comm = *(vtkComm->GetMPIComm()->GetHandle());
    }
  }

  if (comm == nullptr || comm == MPI_COMM_NULL)
  {
    throw std::runtime_error("ERROR: ADIOS2 requires MPI communicator for parallel reads\n");
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
  const std::string& input, const bool debugMode, const std::string& hint, const bool isFile)
{
  pugi::xml_document document;

  pugi::xml_parse_result result = isFile
    ? document.load_file(input.c_str())
    : document.load_buffer(const_cast<char*>(input.data()), input.size());

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

    // not mandatory
    const pugi::xml_attribute xmlNumberOfComponents =
      XMLAttribute("NumberOfComponents", dataArrayNode, true,
        "when parsing NumberOfComponents attribute in ADIOS2 VTK XML schema", false);

    const std::string name(xmlName.value());
    if (!xmlNumberOfComponents && specialNames.count(name) == 0)
    {
      continue;
    }

    // these are node_pcdata
    for (const pugi::xml_node& componentNode : dataArrayNode)
    {
      if (componentNode.type() != pugi::node_pcdata)
      {
        throw std::runtime_error("ERROR: NumberOfComponents attribute found, but component " +
          std::string(componentNode.name()) + " in node " + std::string(dataArrayNode.value()) +
          " is not of plain data type in ADIOS2 VTK XML schema\n");
      }

      std::string variablePCData(componentNode.value());
      variablePCData.erase(0, variablePCData.find_first_not_of(" \n\r\t"));
      variablePCData.erase(variablePCData.find_last_not_of(" \n\r\t") + 1);

      dataArray.Vector.emplace(variablePCData, vtkSmartPointer<vtkDataArray>());
    }

    if (xmlNumberOfComponents)
    {
      const size_t components = static_cast<size_t>(std::stoull(xmlNumberOfComponents.value()));
      if (dataArray.Vector.size() != components)
      {
        throw std::runtime_error("ERROR: NumberOfComponents " + std::to_string(components) +
          " and variable names found " + std::to_string(dataArray.Vector.size()) +
          " inside DataArray node " + std::string(xmlName.name()) + " in ADIOS2 VTK XML schema");
      }
    }
  }

  return dataSet;
}

std::string FileToString(const std::string& fileName)
{
  std::ifstream file(fileName);
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

// allowed types
template std::vector<size_t> StringToVector<size_t>(const std::string&);
template std::vector<int> StringToVector<int>(const std::string&);
template std::vector<double> StringToVector<double>(const std::string&);

std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept
{
  return std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<std::size_t>());
}

// allowed types
template vtkSmartPointer<vtkDataArray> NewDataArray<float>();
template vtkSmartPointer<vtkDataArray> NewDataArray<double>();

adios2::Box<adios2::Dims> PartitionCart1D(const adios2::Dims& shape)
{
  adios2::Box<adios2::Dims> selection({ adios2::Dims(shape.size(), 0), shape });

  const size_t mpiRank = static_cast<size_t>(MPIGetRank());
  const size_t mpiSize = static_cast<size_t>(MPIGetSize());

  // fastest index, VTK is column-major
  if (shape[2] >= mpiSize)
  {
    const size_t elements = shape[2] / mpiSize;
    // start
    selection.first[0] = mpiRank * elements;
    // count
    selection.second[0] = (mpiRank == mpiSize - 1) ? elements + shape[2] % mpiSize : elements;
  }

  return selection;
}

} // end helper namespace
} // end adios2vtk namespace
