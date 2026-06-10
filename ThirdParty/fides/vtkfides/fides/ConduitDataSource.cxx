//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/ConduitDataSource.h>
#include <fides/ExternalDataRegistry.h>

#include <algorithm>
#include <numeric>

#include <iostream>

#include "conduit.hpp"

namespace
{
enum class ConduitNodeType
{
  Shape,
  Data,
  DataType,
  Dimensions,
  Attributes,
};

std::string GetConduitPath(ConduitNodeType nodeType,
                           const std::string& varName,
                           const std::shared_ptr<rapidjson::Document> doc)
{
  std::string key;
  std::string defaultPath;

  switch (nodeType)
  {
    case ConduitNodeType::Shape:
      key = "shape_path";
      defaultPath = "variables/" + varName + "/shape";
      break;
    case ConduitNodeType::Data:
      key = "data_path";
      defaultPath = "variables/" + varName + "/data";
      break;
    case ConduitNodeType::DataType:
      key = "dtype_path";
      defaultPath = "variables/" + varName + "/dtype";
      break;
    case ConduitNodeType::Dimensions:
      key = "dimensions_path";
      defaultPath = "variables/" + varName + "/dimensions";
      break;
    case ConduitNodeType::Attributes:
      key = "attributes_path";
      defaultPath = "variables/" + varName + "/attributes";
      break;
    default:
      throw std::runtime_error("Unrecognized conduit node type");
      break;
  }

  auto m = doc->GetObject().begin();
  const auto root = m->value.GetObject();

  if (root.HasMember("variables"))
  {
    auto& val = root["variables"];
    if (val.IsObject() && val.HasMember(varName.c_str()))
    {
      auto& vargroup = val[varName.c_str()];
      if (vargroup.IsObject() && vargroup.HasMember(key.c_str()) &&
          vargroup[key.c_str()].IsString())
      {
        return vargroup[key.c_str()].GetString();
      }
    }
  }

  return defaultPath;
}
}


namespace fides
{
namespace io
{

class ConduitDataSource::InternalImpl
{
public:
  std::shared_ptr<conduit::Node> Node;
  std::string token;
};

ConduitDataSource::ConduitDataSource()
  : Internals(std::make_unique<InternalImpl>())
{
#if FIDES_USE_MPI
  this->Comm = MPI_COMM_WORLD;
#endif
}

#if FIDES_USE_MPI
ConduitDataSource::ConduitDataSource(MPI_Comm comm)
  : Internals(std::make_unique<InternalImpl>())
{
  this->Comm = comm;
}
#endif

ConduitDataSource::~ConduitDataSource() = default;

void ConduitDataSource::OpenSource(const std::unordered_map<std::string, std::string>& tokens,
                                   const std::string& dataSourceName,
                                   bool useMPI)
{
  this->OpenSource(tokens.at(dataSourceName), useMPI);
}

void ConduitDataSource::OpenSource(const std::string& token, bool fidesNotUsed(useMPI))
{
  // This gets called by everyone, but the results should never change, so just do it once
  if (this->Internals->Node == nullptr)
  {
    this->Internals->token = token;

    // Retrieve the conduit node from the registry and store for other methods
    std::shared_ptr<DataContainer> container =
      fides::io::ExternalDataRegistry::Instance().Get(token);

    if (container == nullptr)
    {
      throw std::runtime_error("No data object associated with token " + token);
    }

    this->Internals->Node = fides::GetAsConduit(*container);
  }
}

std::vector<fides::RawArray> ConduitDataSource::GetVariableDimensions(
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections),
  fides::FieldAssociation fidesNotUsed(association))
{
  std::string shapePath = GetConduitPath(ConduitNodeType::Shape, varName, this->SchemaDocument);

  if (!this->Internals->Node)
  {
    throw std::runtime_error("Conduit node pointer is null for variable " + varName);
  }

  const conduit::Node& n = *this->Internals->Node;

  if (!n.has_path(shapePath))
  {
    throw std::runtime_error("Missing expected shape node " + shapePath + " for variable " +
                             varName);
  }

  const conduit::Node& nodeShape = n[shapePath];
  if (nodeShape.dtype().is_empty())
  {
    // Return an empty RawArray if there is no shape data
    return std::vector<fides::RawArray>();
  }

  size_t numShapeElements = nodeShape.dtype().number_of_elements();
  // expects [dim0, dim1, ..., start0, start1, ...]
  size_t totalElements = numShapeElements * 2;

  // Allocate a 1D RawArray of size_t to hold shapes and starts
  auto raw = fides::AllocateRawArray<size_t>(totalElements, 1);
  size_t* buffer = raw.template GetWritePointer<size_t>();

  const conduit::int64* shape = nodeShape.value();

  // Fill the first half with dimensions
  for (size_t i = 0; i < numShapeElements; ++i)
  {
    buffer[i] = static_cast<size_t>(shape[i]);
  }

  // Fill the second half with starts (always 0 for Conduit)
  for (size_t i = 0; i < numShapeElements; ++i)
  {
    buffer[numShapeElements + i] = 0;
  }

  return std::vector<fides::RawArray>{ std::move(raw) };
}

#define conduitTemplateMacro(call) \
  switch (conduitType[0])          \
  {                                \
    case 'i':                      \
    {                              \
      using conduit_TT = int64_t;  \
      return call;                 \
      break;                       \
    }                              \
    case 'u':                      \
    {                              \
      using conduit_TT = uint64_t; \
      return call;                 \
      break;                       \
    }                              \
    case 'f':                      \
    {                              \
      using conduit_TT = double;   \
      return call;                 \
      break;                       \
    }                              \
  }

#define fidesTemplateMacro(call)                 \
  switch (type[0])                               \
  {                                              \
    case 'c':                                    \
    {                                            \
      using fides_TT = char;                     \
      conduitTemplateMacro(call);                \
      break;                                     \
    }                                            \
    case 'f':                                    \
    {                                            \
      if (type == "float32")                     \
      {                                          \
        using fides_TT = float;                  \
        conduitTemplateMacro(call);              \
        break;                                   \
      }                                          \
      if (type == "float64")                     \
      {                                          \
        using fides_TT = double;                 \
        conduitTemplateMacro(call);              \
        break;                                   \
      }                                          \
      using fides_TT = float;                    \
      conduitTemplateMacro(call);                \
      break;                                     \
    }                                            \
    case 'd':                                    \
    {                                            \
      using fides_TT = double;                   \
      conduitTemplateMacro(call);                \
      break;                                     \
    }                                            \
    case 'i':                                    \
      if (type == "int")                         \
      {                                          \
        using fides_TT = int;                    \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int8_t")                 \
      {                                          \
        using fides_TT = int8_t;                 \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int16_t")                \
      {                                          \
        using fides_TT = int16_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int32_t")                \
      {                                          \
        using fides_TT = int32_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int64_t")                \
      {                                          \
        using fides_TT = int64_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int8")                   \
      {                                          \
        using fides_TT = int8_t;                 \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int16")                  \
      {                                          \
        using fides_TT = int16_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int32")                  \
      {                                          \
        using fides_TT = int32_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "int64")                  \
      {                                          \
        using fides_TT = int64_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      break;                                     \
    case 'l':                                    \
      if (type == "long long int")               \
      {                                          \
        using fides_TT = long long int;          \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "long int")               \
      {                                          \
        using fides_TT = long int;               \
        conduitTemplateMacro(call);              \
      }                                          \
      break;                                     \
    case 's':                                    \
      if (type == "short")                       \
      {                                          \
        using fides_TT = short;                  \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "signed char")            \
      {                                          \
        using fides_TT = signed char;            \
        conduitTemplateMacro(call);              \
      }                                          \
      break;                                     \
    case 'u':                                    \
      if (type == "unsigned char")               \
      {                                          \
        using fides_TT = unsigned char;          \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "unsigned int")           \
      {                                          \
        using fides_TT = unsigned int;           \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "unsigned long int")      \
      {                                          \
        using fides_TT = unsigned long int;      \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "unsigned long long int") \
      {                                          \
        using fides_TT = unsigned long long int; \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint8_t")                \
      {                                          \
        using fides_TT = uint8_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint16_t")               \
      {                                          \
        using fides_TT = uint16_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint32_t")               \
      {                                          \
        using fides_TT = uint32_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint64_t")               \
      {                                          \
        using fides_TT = uint64_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint8")                  \
      {                                          \
        using fides_TT = uint8_t;                \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint16")                 \
      {                                          \
        using fides_TT = uint16_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint32")                 \
      {                                          \
        using fides_TT = uint32_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      else if (type == "uint64")                 \
      {                                          \
        using fides_TT = uint64_t;               \
        conduitTemplateMacro(call);              \
      }                                          \
      break;                                     \
  }


template <typename ConduitType, typename VariableType>
std::vector<fides::RawArray> GetScalarVariableInternal(std::shared_ptr<conduit::Node> rootNode,
                                                       const std::string& dataPath)
{
  if (!rootNode)
  {
    throw std::runtime_error("Invalid Conduit node pointer provided to GetScalarVariableInternal.");
  }

  const conduit::Node& root = *rootNode;
  const conduit::Node& nodeData = root[dataPath];

  const ConduitType* vecData = nodeData.value();

  // A scalar is just an array with 1 element and 1 component.
  auto raw = fides::AllocateRawArray<VariableType>(1, 1);
  VariableType* buffer = raw.template GetWritePointer<VariableType>();

  // Directly assign the casted value into the allocated RawArray buffer
  *buffer = static_cast<VariableType>(*vecData);

  return std::vector<fides::RawArray>{ std::move(raw) };
}

std::vector<fides::RawArray> ConduitDataSource::GetScalarVariable(
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::string dataPath = GetConduitPath(ConduitNodeType::Data, varName, this->SchemaDocument);
  std::string dataTypePath =
    GetConduitPath(ConduitNodeType::DataType, varName, this->SchemaDocument);

  if (!this->Internals->Node)
  {
    throw std::runtime_error("Conduit node pointer is null for variable " + varName);
  }

  const conduit::Node& n = (*this->Internals->Node);

  if (!n.has_path(dataPath))
  {
    throw std::runtime_error("Missing expected data node " + dataPath + " for variable " + varName);
  }

  if (!n.has_path(dataTypePath))
  {
    throw std::runtime_error("Missing expected dtype node " + dataTypePath + " for variable " +
                             varName);
  }

  const conduit::Node& nodeData = n[dataPath];
  const conduit::Node& nodeDataType = n[dataTypePath];

  const std::string& conduitType = nodeData.dtype().name();
  const std::string& type = nodeDataType.as_string();

  fidesTemplateMacro(
    (GetScalarVariableInternal<conduit_TT, fides_TT>(this->Internals->Node, dataPath)));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<fides::RawArray> ConduitDataSource::GetTimeArray(
  const std::string& fidesNotUsed(varName),
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  // Only called when schema json has "step_information", currently not
  // implemented for Conduit.
  throw std::runtime_error("ConduitDataSource::GetTimeArray not implemented");
}

template <typename ConduitType, typename VariableType>
std::vector<fides::RawArray> ReadVariableInternal(std::shared_ptr<conduit::Node> rootNode,
                                                  const std::string& shapePath,
                                                  const std::string& dataPath,
                                                  IsVector isit)
{
  if (!rootNode)
  {
    throw std::runtime_error("Invalid Conduit node pointer provided to ReadVariableInternal.");
  }

  const conduit::Node& root = *rootNode;

  const conduit::Node& nodeShape = root[shapePath];
  std::vector<size_t> shape;
  if (!nodeShape.dtype().is_empty())
  {
    const conduit::int64* shapeVal = nodeShape.value();
    shape.assign(shapeVal, shapeVal + nodeShape.dtype().number_of_elements());
  }

  const conduit::Node& nodeData = root[dataPath];
  size_t totalElements = nodeData.dtype().number_of_elements();

  // Determine vector detection
  bool isVector;
  if (isit == IsVector::Auto)
  {
    isVector = shape.size() == 2;
  }
  else
  {
    isVector = isit == IsVector::Yes;
  }

  // Calculate components and values based on shape
  int numComponents = 1;
  size_t numValues = totalElements;
  if (isVector && !shape.empty())
  {
    const size_t nDims = shape.size();
    numComponents = static_cast<int>(shape[nDims - 1]);
    numValues = totalElements / static_cast<size_t>(numComponents);
  }

  const ConduitType* vecData = nodeData.value();

  if constexpr (std::is_same_v<ConduitType, VariableType>)
  {
    // Zero-copy path: The requested type matches the Conduit memory type.
    // We wrap the Conduit pointer in a shared_ptr with a custom deleter
    // that captures rootNode to extend its lifetime
    std::shared_ptr<void> buf(const_cast<VariableType*>(vecData), [rootNode](void*) {
      // We do not delete the raw memory pointer here because it is owned
      // by the Conduit Node. The only point of the deleter is to capture
      // rootNode in this lambda, to guarantee the Conduit node stays alive
      // as long as this RawArray does.
    });

    return std::vector<fides::RawArray>{ fides::RawArray(
      std::move(buf), numValues, numComponents, fides::GetDataType<VariableType>()) };
  }
  else
  {
    // Type mismatch path: Allocate a new buffer and perform the cast/copy.
    auto raw = fides::AllocateRawArray<VariableType>(numValues, numComponents);
    VariableType* buffer = raw.template GetWritePointer<VariableType>();

    std::transform(vecData, vecData + totalElements, buffer, [](ConduitType val) {
      return static_cast<VariableType>(val);
    });

    return std::vector<fides::RawArray>{ std::move(raw) };
  }
}

std::vector<fides::RawArray> ConduitDataSource::ReadVariable(
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections),
  IsVector isit)
{
  std::string shapePath = GetConduitPath(ConduitNodeType::Shape, varName, this->SchemaDocument);
  std::string dataPath = GetConduitPath(ConduitNodeType::Data, varName, this->SchemaDocument);
  std::string dataTypePath =
    GetConduitPath(ConduitNodeType::DataType, varName, this->SchemaDocument);

  // It's an error if the shared_ptr is empty
  if (!this->Internals->Node)
  {
    throw std::runtime_error("Conduit node pointer is null for variable " + varName);
  }

  const conduit::Node& n = (*this->Internals->Node);

  if (!n.has_path(shapePath))
  {
    throw std::runtime_error("Missing expected shape node " + shapePath + " for variable " +
                             varName);
  }

  if (!n.has_path(dataPath))
  {
    throw std::runtime_error("Missing expected data node " + dataPath + " for variable " + varName);
  }

  if (!n.has_path(dataTypePath))
  {
    throw std::runtime_error("Missing expected dtype node " + dataTypePath + " for variable " +
                             varName);
  }

  const conduit::Node& nodeData = n[dataPath];
  const conduit::Node& nodeDataType = n[dataTypePath];

  const std::string& conduitType = nodeData.dtype().name();
  const std::string& type = nodeDataType.as_string();

  fidesTemplateMacro(
    (ReadVariableInternal<conduit_TT, fides_TT>(this->Internals->Node, shapePath, dataPath, isit)));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<fides::RawArray> ConduitDataSource::ReadMultiBlockVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections)
{
  // Conduit does not have blocks, so just ReadVariable
  return ReadVariable(varName, selections, IsVector::Auto);
}

std::vector<size_t> ConduitDataSource::GetVariableShape(std::string& varName)
{
  return this->GetVariableShape(varName, "");
}

std::vector<size_t> ConduitDataSource::GetVariableShape(std::string& varName,
                                                        const std::string& fidesNotUsed(group))
{
  std::string shapePath = GetConduitPath(ConduitNodeType::Shape, varName, this->SchemaDocument);
  const conduit::Node& n = (*this->Internals->Node);

  if (!n.has_path(shapePath))
  {
    throw std::runtime_error("Missing expected shape node " + shapePath + " for variable " +
                             varName);
  }

  const conduit::Node& nodeShape = n[shapePath];

  std::vector<size_t> shape;
  if (nodeShape.dtype().is_empty())
  {
    return shape;
  }

  const conduit::int64* shapeVal = nodeShape.value();
  shape.assign(shapeVal, shapeVal + nodeShape.dtype().number_of_elements());
  return shape;
}

std::vector<std::string> ConduitDataSource::ReadStringAttribute(const std::string& name)
{
  if (!this->Internals->Node)
  {
    return {};
  }
  const conduit::Node& root = *this->Internals->Node;
  if (!root.has_path(name))
  {
    return {};
  }
  const conduit::Node& n = root[name];
  // A list-of-strings (Conduit list with string children) is the
  // analogue of an ADIOS2 string-array attribute -- used today for
  // cell_types. Return one std::string per child.
  if (n.dtype().is_list())
  {
    std::vector<std::string> result;
    result.reserve(static_cast<size_t>(n.number_of_children()));
    for (conduit::index_t i = 0; i < n.number_of_children(); i++)
    {
      const conduit::Node& child = n.child(i);
      if (child.dtype().is_string())
      {
        result.push_back(child.as_string());
      }
    }
    return result;
  }
  if (n.dtype().is_string())
  {
    return { n.as_string() };
  }
  return {};
}

std::vector<std::int32_t> ConduitDataSource::ReadInt32Attribute(const std::string& name)
{
  if (!this->Internals->Node)
  {
    return {};
  }
  const conduit::Node& root = *this->Internals->Node;
  if (!root.has_path(name))
  {
    return {};
  }
  const conduit::Node& n = root[name];
  // Lists of ints are symmetric with the string case but not exercised
  // today; handle them anyway so future schema additions don't have
  // to revisit this code.
  if (n.dtype().is_list())
  {
    std::vector<std::int32_t> result;
    result.reserve(static_cast<size_t>(n.number_of_children()));
    for (conduit::index_t i = 0; i < n.number_of_children(); i++)
    {
      const conduit::Node& child = n.child(i);
      if (child.dtype().is_integer())
      {
        result.push_back(child.to_int32());
      }
    }
    return result;
  }
  if (n.dtype().is_integer())
  {
    return { n.to_int32() };
  }
  return {};
}

std::set<std::string> ConduitDataSource::GetAttributeNames(const std::string& prefix)
{
  std::set<std::string> result;
  if (!this->Internals->Node)
  {
    return result;
  }
  const conduit::Node& root = *this->Internals->Node;
  const conduit::Node* node = nullptr;
  if (prefix.empty())
  {
    node = &root;
  }
  else if (root.has_path(prefix))
  {
    node = &root[prefix];
  }
  if (!node)
  {
    return result;
  }
  for (const auto& name : node->child_names())
  {
    result.insert(name);
  }
  return result;
}

}
}
