/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2VTKHelper.h
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2HELPER_H_
#define VTK_IO_ADIOS2_ADIOS2HELPER_H_

#include "ADIOS2Types.h"

#include <cstddef> //std::size_t
#include <set>
#include <string>
#include <utility> // std::pair
#include <vector>

#include <mpi.h>

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include <vtk_pugixml.h>

namespace adios2vtk
{
namespace helper
{

/**
 * Get current MPI global communicator from VTK
 */
MPI_Comm MPIGetComm();

/** Get current MPI rank */
int MPIGetRank();

/**
 * Get safely a pugi::xml_document from XML as a string
 * @param input entire XML contents as a string or file, depending on bool isFile
 * @param debugMode true: safe mode throws exceptions
 * @param hint add extra information on exceptions
 * @param isFile false: input is string, true: input is fileName
 * @return xml as pugi object
 * @throws std::invalid_argument
 */
pugi::xml_document XMLDocument(const std::string& input, const bool debugMode,
  const std::string& hint, const bool isFile = false);

/**
 * Get safely a pugi::xml_document from a pugmi::xml_document
 * @param nodeName input node to be found
 * @param xmlDocument input document
 * @param debugMode true: safe mode throws exceptions
 * @param hint add extra information on exceptions
 * @param isMandatory true: throws exception if node is not found
 * @param isUnique true: throws exception if node exist more than once
 * @return node if found, empty node if not mandatory
 * @throws std::invalid_argument
 */
pugi::xml_node XMLNode(const std::string nodeName, const pugi::xml_document& xmlDocument,
  const bool debugMode, const std::string& hint, const bool isMandatory = true,
  const bool isUnique = false);

/**
 * Overloaded version that gets a XML node from inside another node called upperNode
 * @param nodeName input node to be found
 * @param upperNode input node to search inside for nodeName
 * @param debugMode true: safe mode throws exceptions
 * @param hint add extra information on exceptions
 * @param isMandatory true: throws exception if node is not found
 * @param isUnique true: throws exception if node exist more than once
 * @return node if found, empty node if not mandatory
 * @throws std::invalid_argument
 */
pugi::xml_node XMLNode(const std::string nodeName, const pugi::xml_node& upperNode,
  const bool debugMode, const std::string& hint, const bool isMandatory = true,
  const bool isUnique = false);

std::string FileToString(const std::string& fileName);

/**
 * Get a node attribute identified by its key
 * @param attributeName input xml attribute to be found
 * @param node input node to search inside for attributeName
 * @param debugMode true: safe mode throws exceptions
 * @param hint add extra information on exceptions
 * @param isMandatory true: throws exception if node is not found
 * @return attribute if found, empty node if not mandatory
 * @throws std::invalid_argument
 */
pugi::xml_attribute XMLAttribute(const std::string attributeName, const pugi::xml_node& node,
  const bool debugMode, const std::string& hint, const bool isMandatory = true);

/**
 * Convert a set of strings into a csv "string1,string2,string3" string
 * @param input set of ordered strings
 * @return csv string
 */
std::string SetToCSV(const std::set<std::string>& input) noexcept;

/**
 * Converts a single string "s1 s2 s3" list to a vector
 * vector ={ "s1", "s2", "s3" };
 * @param input
 * @return
 */
template<class T>
std::vector<T> StringToVector(const std::string& input) noexcept;

std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept;

/**
 * Initialize DataSet structure from parsing a pugi::xml_node, loops through DataArray nodes
 * @param dataSetNode input
 * @return initialiazed DataSet
 */
types::DataSet XMLInitDataSet(const pugi::xml_node& dataSetNode);

template<class T>
vtkSmartPointer<vtkDataArray> NewDataArray();

} // end namespace helper
} // end namespace adiosvtk

#endif /* VTK_IO_ADIOS2_ADIOS2HELPER_H_ */
