//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataContainer.h>
#include <fides/FidesTypes.h>

#if FIDES_USE_VISKORES
#include <viskores/cont/PartitionedDataSet.h>
#endif

#if FIDES_USE_VTK
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkSmartPointer.h>
#endif

#if FIDES_USE_CONDUIT
#include <conduit/conduit.hpp>
#endif

namespace fides
{

const char* const kAutoNamesAssemblySubtree = "__fides_auto_names";

CellShape ConvertStringToCellShape(const std::string& cellShapeName)
{
  if (cellShapeName == "vertex")
  {
    return CellShape::Vertex;
  }
  else if (cellShapeName == "line")
  {
    return CellShape::Line;
  }
  else if (cellShapeName == "triangle")
  {
    return CellShape::Triangle;
  }
  else if (cellShapeName == "quad")
  {
    return CellShape::Quad;
  }
  else if (cellShapeName == "tetrahedron")
  {
    return CellShape::Tetra;
  }
  else if (cellShapeName == "hexahedron")
  {
    return CellShape::Hexahedron;
  }
  else if (cellShapeName == "wedge")
  {
    return CellShape::Wedge;
  }
  else if (cellShapeName == "pyramid")
  {
    return CellShape::Pyramid;
  }
  else
  {
    throw std::runtime_error("Unsupported cell shape: " + cellShapeName);
  }
}

std::string ConvertCellShapeToString(CellShape shape)
{
  switch (shape)
  {
    case CellShape::Vertex:
      return "vertex";
    case CellShape::Line:
      return "line";
    case CellShape::Triangle:
      return "triangle";
    case CellShape::Quad:
      return "quad";
    case CellShape::Tetra:
      return "tetrahedron";
    case CellShape::Hexahedron:
      return "hexahedron";
    case CellShape::Wedge:
      return "wedge";
    case CellShape::Pyramid:
      return "pyramid";
    default:
      throw std::runtime_error("Unknown CellShape for string conversion");
  }
}

std::string ConvertDataTypeToString(DataType type)
{
  switch (type)
  {
    case DataType::Float32:
      return "float";
    case DataType::Float64:
      return "double";
    case DataType::Int8:
      return "int8_t";
    case DataType::Int16:
      return "int16_t";
    case DataType::Int32:
      return "int32_t";
    case DataType::Int64:
      return "int64_t";
    case DataType::UInt8:
      return "uint8_t";
    case DataType::UInt16:
      return "uint16_t";
    case DataType::UInt32:
      return "uint32_t";
    case DataType::UInt64:
      return "uint64_t";
    default:
      throw std::runtime_error("Unknown DataType for string conversion");
  }
}

int GetCellShapeNumberOfPoints(CellShape shape)
{
  switch (shape)
  {
    case CellShape::Vertex:
      return 1;
    case CellShape::Line:
      return 2;
    case CellShape::Triangle:
      return 3;
    case CellShape::Quad:
      return 4;
    case CellShape::Tetra:
      return 4;
    case CellShape::Hexahedron:
      return 8;
    case CellShape::Wedge:
      return 6;
    case CellShape::Pyramid:
      return 5;
    default:
      throw std::runtime_error("Unknown CellShape");
  }
}

// Runtime backend support detection methods

bool HasVTKSupport()
{
#if FIDES_USE_VTK
  return true;
#else
  return false;
#endif
}

bool HasViskoresSupport()
{
#if FIDES_USE_VISKORES
  return true;
#else
  return false;
#endif
}

bool HasConduitSupport()
{
#if FIDES_USE_CONDUIT
  return true;
#else
  return false;
#endif
}

//-----------------------------------------------------------------------------
// The container should hold a std::vector<viskores::cont::DataSet, we extract
// that and use it create and return  a viskores::cont::PartitionedDataSet
fides::ViskoresCollection GetAsViskoresPDS(fides::DataContainer& container)
{
#if FIDES_USE_VISKORES
  if (auto* vec = GetDataAs<std::vector<viskores::cont::DataSet>>(container))
  {
    return viskores::cont::PartitionedDataSet(*vec);
  }
  throw std::runtime_error("Container does not hold a std::vector<viskores::cont::DataSet>");
#endif
  (void)container;
  throw std::runtime_error("Fides was not compiled with Viskores support.");
}

//-----------------------------------------------------------------------------
// The container should hold a vtkSmartPointer<vtkPartitionedDataSet> which we
// just extract and return
fides::VTKCollection GetAsVTKPDS(fides::DataContainer& container)
{
#if FIDES_USE_VTK
  if (auto* vec = GetDataAs<vtkSmartPointer<vtkPartitionedDataSet>>(container))
  {
    return *vec;
  }
  throw std::runtime_error("Container does not hold a vtkSmartPointer<vtkPartitionedDataSet>");
#endif
  (void)container;
  throw std::runtime_error("Fides was not compiled with VTK support.");
}

//-----------------------------------------------------------------------------
// Multi-dataset (PDC) result: the container holds a
// vtkSmartPointer<vtkPartitionedDataSetCollection>.
fides::VTKPDC GetAsVTKPDC(fides::DataContainer& container)
{
#if FIDES_USE_VTK
  if (auto* pdc = GetDataAs<vtkSmartPointer<vtkPartitionedDataSetCollection>>(container))
  {
    return *pdc;
  }
  throw std::runtime_error(
    "Container does not hold a vtkSmartPointer<vtkPartitionedDataSetCollection>. "
    "Use GetAsVTKPDC only on a multi-dataset / multi-group read; otherwise use GetAsVTKPDS.");
#endif
  (void)container;
  throw std::runtime_error("Fides was not compiled with VTK support.");
}

//-----------------------------------------------------------------------------
// Multi-dataset (PDC) result on the Viskores side: a vector of
// PartitionedDataSets, one per collection item.
fides::ViskoresPartitionedCollection GetAsViskoresCollection(fides::DataContainer& container)
{
#if FIDES_USE_VISKORES
  if (auto* vec = GetDataAs<std::vector<viskores::cont::PartitionedDataSet>>(container))
  {
    return *vec;
  }
  throw std::runtime_error(
    "Container does not hold a std::vector<viskores::cont::PartitionedDataSet>. "
    "Use GetAsViskoresCollection only on a multi-dataset / multi-group read.");
#endif
  (void)container;
  throw std::runtime_error("Fides was not compiled with Viskores support.");
}

//-----------------------------------------------------------------------------
// The container should hold a std::shared_ptr<conduit::Node>, since that is
// what we put in, regardless of which method (Wrap vs WrapExternal) was used
// to wrap it.
fides::ConduitCollection GetAsConduit(fides::DataContainer& container)
{
#if FIDES_USE_CONDUIT
  if (auto* nodePtr = GetDataAs<fides::ConduitCollection>(container))
  {
    return *nodePtr;
  }
  throw std::runtime_error("Container does not hold a std::shared_ptr<conduit::Node>.");
#else
  (void)container;
  throw std::runtime_error("Fides was not compiled with Conduit support.");
#endif
}

//-----------------------------------------------------------------------------
std::shared_ptr<fides::DataContainer> Wrap(const fides::VTKCollection& dataset)
{
#if FIDES_USE_VTK
  // Copy the smart pointer into the wrapper to increment the ref count
  return std::make_shared<ConcreteDataWrapper<fides::VTKCollection>>(dataset);
#else
  (void)dataset;
  throw std::runtime_error("Fides was not compiled with VTK support.");
#endif
}

//-----------------------------------------------------------------------------
std::shared_ptr<fides::DataContainer> Wrap(const fides::ViskoresCollection& dataset)
{
#if FIDES_USE_VISKORES
  // Shallow the Viskores handle into the wrapper
  return std::make_shared<ConcreteDataWrapper<fides::ViskoresCollection>>(dataset);
#else
  (void)dataset;
  throw std::runtime_error("Fides was not compiled with Viskores support.");
#endif
}

//-----------------------------------------------------------------------------
std::shared_ptr<fides::DataContainer> Wrap(const fides::ConduitCollection& node)
{
#if FIDES_USE_CONDUIT
  // Just copy the shared_ptr into the wrapper
  return std::make_shared<ConcreteDataWrapper<fides::ConduitCollection>>(node);
#else
  (void)node;
  throw std::runtime_error("Fides was not compiled with Conduit support.");
#endif
}

//-----------------------------------------------------------------------------
std::shared_ptr<fides::DataContainer> WrapExternal(fides::ConduitNode* node)
{
#if FIDES_USE_CONDUIT
  if (node == nullptr)
  {
    throw std::invalid_argument("WrapExternal received a null fides::ConduitNode pointer.");
  }

  // Wrap the raw pointer in the collection type (shared_ptr) with a no-op deleter
  fides::ConduitCollection shared_node(node, [](conduit::Node*) {});

  // Now store it like Wrap does
  return std::make_shared<ConcreteDataWrapper<fides::ConduitCollection>>(shared_node);
#else
  (void)node;
  throw std::runtime_error("Fides was not compiled with Conduit support.");
#endif
}

}
