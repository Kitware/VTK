/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXHelper.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXHelper.h : collection of helper function needed by VTK::IOADIOS2 module
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_h
#define VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_h

#include "VTXTypes.h"

#include <cstddef> //std::size_t
#include <set>
#include <string>
#include <utility> // std::pair
#include <vector>

#include <vtk_mpi.h>

#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkSmartPointer.h"

#include <vtk_pugixml.h>

#include <adios2.h>

namespace vtx
{
namespace helper
{

/** Get current MPI global communicator from VTK */
MPI_Comm MPIGetComm();

/** Get current MPI rank from MPIGetComm */
int MPIGetRank();

/** Get current MPI size from MPIGetComm */
int MPIGetSize();

/**
 * Get safely a pugi::xml_document from XML as a string
 * @param input entire XML contents as a string or file, depending on bool
 * isFile
 * @param debugMode true: safe mode throws exceptions
 * @param hint add extra information on exceptions
 * @return xml as pugi object
 * @throws std::invalid_argument
 */
pugi::xml_document XMLDocument(
  const std::string& input, const bool debugMode, const std::string& hint);

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
 * Overloaded version that gets a XML node from inside another node called
 * upperNode
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

/**
 * Translate file contents to string
 * @param fileName input
 * @return file contents as a single string
 */
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
template <class T>
std::vector<T> StringToVector(const std::string& input) noexcept;

/**
 * Return product of all dimensions components to get the total number of elements
 * @param dimensions input
 * @return total number of elements
 */
std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept;

/**
 * Initialize DataSet structure from parsing a pugi::xml_node, loops through
 * DataArray nodes
 * @param dataSetNode input
 * @param specialNames input check for vector components even if
 * NumberOfComponents wasn't declared
 * @return initialiazed DataSet
 */
types::DataSet XMLInitDataSet(
  const pugi::xml_node& dataSetNode, const std::set<std::string>& specialNames);

/**
 * Return a derived class of vtkDataArray specialized for supported types
 * @return specialized vtkDataArray
 */
template <class T>
vtkSmartPointer<vtkDataArray> NewDataArray();

/**
 * Special type for vtkIdTypeArray
 * @return smart pointer of type vtkIdTypeArray
 */
vtkSmartPointer<vtkIdTypeArray> NewDataArrayIdType();

/**
 * Simple partition to load balance shape across viz processes
 * @param shape input
 * @return selection first=start second=count
 */
adios2::Box<adios2::Dims> PartitionCart1D(const adios2::Dims& shape);

/**
 * Map's keys to a vector
 * @param input map
 * @return vector with keys only
 */
template <class T, class U>
std::vector<T> MapKeysToVector(const std::map<T, U>& input) noexcept;

/**
 * Print a vector with an associated name. For debugging purposes.
 * @param input vector data
 * @param name input name
 */
template <class T>
void Print(const std::vector<T>& input, const std::string& name);

/**
 * Linear index for a point within a box with shape dimensions from zero origin
 * @param shape input shape box
 * @param point input point
 * @return linearized point inside shape
 */
size_t LinearizePoint(const adios2::Dims& shape, const adios2::Dims& point) noexcept;

/**
 * Set the appropriate file name based on recognized user input
 * @param fileName input from user selected file
 * @return file name to be used by adios2 engine
 */
std::string GetFileName(const std::string& fileName) noexcept;

/**
 * Set the appropriate engine type based on recognized user input
 * @param fileName input from user selected file
 * @return engine type to be used by adios2 engine
 */
std::string GetEngineType(const std::string& fileName) noexcept;

/**
 * Check if input ends with a certain (ends) string
 * @param input string input
 * @param ends ending string to check in input
 * @return  true: input ends with "ends", false: it doesn't end with ends
 */
bool EndsWith(const std::string& input, const std::string& ends) noexcept;

} // end namespace helper
} // end namespace vtx

#include "VTXHelper.inl"

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_h */
