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
#include <fides/Keys.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <type_traits>

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
  Blocks,
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
    case ConduitNodeType::Blocks:
      key = "blocks_path";
      defaultPath = "variables/" + varName + "/blocks";
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

/// The nodes that make up one block of a variable. \c Start is only set for a
/// block of a multi-block variable that provides one.
struct ConduitBlock
{
  const conduit::Node* Shape = nullptr;
  const conduit::Node* Data = nullptr;
  const conduit::Node* Start = nullptr;
};

/// The blocks of a variable. \c Exists is false if the variable is not in
/// the node at all. \c IsBlockList is true if the variable has a \c blocks
/// child, in which case \c Blocks has one entry per child of it, possibly
/// none (a rank that was handed no blocks of the variable).
struct ConduitVariable
{
  bool Exists = false;
  bool IsBlockList = false;
  std::vector<ConduitBlock> Blocks;
};

/// Returns the blocks of a variable. A variable with a \c blocks child (a list
/// or object whose children each hold \c shape and optionally \c data and
/// \c start) has one block per child. Otherwise the variable is a single block
/// at the (possibly schema-configured) shape and data paths. \c Data is null
/// for a block without data.
ConduitVariable GetVariable(const conduit::Node& root,
                            const std::string& varName,
                            const std::shared_ptr<rapidjson::Document> doc)
{
  ConduitVariable variable;
  const std::string blocksPath = GetConduitPath(ConduitNodeType::Blocks, varName, doc);
  if (root.has_path(blocksPath))
  {
    variable.Exists = true;
    variable.IsBlockList = true;
    const conduit::Node& blocksNode = root[blocksPath];
    for (conduit::index_t i = 0; i < blocksNode.number_of_children(); ++i)
    {
      const conduit::Node& block = blocksNode.child(i);
      if (!block.has_child("shape"))
      {
        throw std::runtime_error("Missing expected shape node in block " + std::to_string(i) +
                                 " of variable " + varName);
      }
      variable.Blocks.push_back({ &block["shape"],
                                  block.has_child("data") ? &block["data"] : nullptr,
                                  block.has_child("start") ? &block["start"] : nullptr });
    }
    return variable;
  }

  const std::string shapePath = GetConduitPath(ConduitNodeType::Shape, varName, doc);
  const std::string dataPath = GetConduitPath(ConduitNodeType::Data, varName, doc);
  const bool hasData = root.has_path(dataPath);
  if (!root.has_path(shapePath))
  {
    if (!hasData)
    {
      return variable;
    }
    throw std::runtime_error("Missing expected shape node " + shapePath + " for variable " +
                             varName);
  }
  variable.Exists = true;
  variable.Blocks.push_back({ &root[shapePath], hasData ? &root[dataPath] : nullptr, nullptr });
  return variable;
}

/// Returns the blocks of a variable, throwing if the variable does not exist.
ConduitVariable GetExistingVariable(const conduit::Node& root,
                                    const std::string& varName,
                                    const std::shared_ptr<rapidjson::Document> doc)
{
  ConduitVariable variable = GetVariable(root, varName, doc);
  if (!variable.Exists)
  {
    throw std::runtime_error("Missing expected shape node " +
                             GetConduitPath(ConduitNodeType::Shape, varName, doc) +
                             " for variable " + varName);
  }
  return variable;
}

/// Returns the indices of the blocks to read: the block selection if there is
/// one (possibly empty), otherwise all \c nBlocks blocks.
std::vector<size_t> GetSelectedBlocks(size_t nBlocks,
                                      const fides::metadata::MetaData& selections,
                                      const std::string& varName)
{
  if (!selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    std::vector<size_t> all(nBlocks);
    std::iota(all.begin(), all.end(), 0);
    return all;
  }
  const auto& selected =
    selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data;
  for (size_t blockId : selected)
  {
    if (blockId >= nBlocks)
    {
      throw std::runtime_error("Block " + std::to_string(blockId) + " selected for variable " +
                               varName + ", which has " + std::to_string(nBlocks) + " block(s)");
    }
  }
  return selected;
}

/// Returns the values of a shape (or start) node, which may have any integer
/// dtype.
std::vector<size_t> GetShapeValues(const conduit::Node* node)
{
  std::vector<size_t> values;
  if (!node || node->dtype().is_empty())
  {
    return values;
  }
  if (!node->dtype().is_integer())
  {
    throw std::runtime_error("Conduit shape and start nodes must have an integer dtype, not " +
                             node->dtype().name());
  }
  conduit::Node asInt64;
  node->to_int64_array(asInt64);
  const conduit::int64* ptr = asInt64.value();
  const auto n = asInt64.dtype().number_of_elements();
  for (conduit::index_t i = 0; i < n; ++i)
  {
    if (ptr[i] < 0)
    {
      throw std::runtime_error("Conduit shape and start nodes must not have negative values");
    }
    values.push_back(static_cast<size_t>(ptr[i]));
  }
  return values;
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
  const fides::metadata::MetaData& selections,
  fides::FieldAssociation fidesNotUsed(association))
{
  if (!this->Internals->Node)
  {
    throw std::runtime_error("Conduit node pointer is null for variable " + varName);
  }

  const ConduitVariable variable =
    GetExistingVariable(*this->Internals->Node, varName, this->SchemaDocument);
  const auto& blocks = variable.Blocks;

  std::vector<fides::RawArray> result;
  for (size_t blockId : GetSelectedBlocks(blocks.size(), selections, varName))
  {
    const auto shape = GetShapeValues(blocks[blockId].Shape);
    if (shape.empty())
    {
      if (!variable.IsBlockList)
      {
        // A single-block variable without shape data has no dimensions
        return std::vector<fides::RawArray>();
      }
      throw std::runtime_error("Block " + std::to_string(blockId) + " of variable " + varName +
                               " has an empty shape");
    }
    const auto start = GetShapeValues(blocks[blockId].Start);
    if (!start.empty() && start.size() != shape.size())
    {
      throw std::runtime_error("start and shape of variable " + varName +
                               " have different lengths");
    }

    // [dim0, dim1, ..., start0, start1, ...], with starts 0 unless given
    const size_t numShapeElements = shape.size();
    auto raw = fides::AllocateRawArray<size_t>(numShapeElements * 2, 1);
    size_t* buffer = raw.template GetWritePointer<size_t>();
    for (size_t i = 0; i < numShapeElements; ++i)
    {
      buffer[i] = shape[i];
      buffer[numShapeElements + i] = start.empty() ? 0 : start[i];
    }
    result.push_back(std::move(raw));
  }
  return result;
}

// Choose conduit_TT to match the *actual* width of the conduit buffer, so a
// 32-bit buffer is read as 32-bit rather than through an int64_t/double pointer
// (which Conduit's type-strict Node::value() would reject anyway). The output
// type is selected separately by fidesTemplateMacro (fides_TT), so a data model
// may still request a narrower output than the stored buffer.
#define conduitTemplateMacro(call) \
  do                               \
  {                                \
    if (conduitType == "int8")     \
    {                              \
      using conduit_TT = int8_t;   \
      return call;                 \
    }                              \
    if (conduitType == "int16")    \
    {                              \
      using conduit_TT = int16_t;  \
      return call;                 \
    }                              \
    if (conduitType == "int32")    \
    {                              \
      using conduit_TT = int32_t;  \
      return call;                 \
    }                              \
    if (conduitType == "int64")    \
    {                              \
      using conduit_TT = int64_t;  \
      return call;                 \
    }                              \
    if (conduitType == "uint8")    \
    {                              \
      using conduit_TT = uint8_t;  \
      return call;                 \
    }                              \
    if (conduitType == "uint16")   \
    {                              \
      using conduit_TT = uint16_t; \
      return call;                 \
    }                              \
    if (conduitType == "uint32")   \
    {                              \
      using conduit_TT = uint32_t; \
      return call;                 \
    }                              \
    if (conduitType == "uint64")   \
    {                              \
      using conduit_TT = uint64_t; \
      return call;                 \
    }                              \
    if (conduitType == "float32")  \
    {                              \
      using conduit_TT = float;    \
      return call;                 \
    }                              \
    if (conduitType == "float64")  \
    {                              \
      using conduit_TT = double;   \
      return call;                 \
    }                              \
  } while (0)

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

  // Cast the value into the allocated RawArray buffer, warning if a narrower
  // requested integer dtype truncated it: a round-trip cast that does not
  // recover the original value indicates lost data.
  const VariableType out = static_cast<VariableType>(*vecData);
  if constexpr (std::is_integral_v<ConduitType> && std::is_integral_v<VariableType>)
  {
    if (static_cast<ConduitType>(out) != *vecData)
    {
      std::cerr << "WARNING: Conduit variable '" << dataPath
                << "' was read into a narrower integer dtype that truncated its value."
                << std::endl;
    }
  }
  *buffer = out;

  return std::vector<fides::RawArray>{ std::move(raw) };
}

// clang-tidy flags "readability-function-size" here, but it is due to macro-generated
// numeric type dispatch (fidesTemplateMacro x conduitTemplateMacro), rather than logic
// complexity.
// NOLINTNEXTLINE(readability-function-size)
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

  const conduit::Node& nodeData = n[dataPath];
  const std::string& conduitType = nodeData.dtype().name();

  // The declared dtype node is optional. When present it is an explicit request
  // for the output type, which may differ from the stored buffer (for example a
  // float64 buffer declared "float" narrows to float on output). When absent,
  // honor the buffer's own type: a Conduit node is self-describing, so no
  // separate type declaration is required.
  std::string type = conduitType;
  if (n.has_path(dataTypePath))
  {
    type = n[dataTypePath].as_string();
  }

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
fides::RawArray ReadVariableInternal(std::shared_ptr<conduit::Node> rootNode,
                                     const conduit::Node& nodeShape,
                                     const conduit::Node& nodeData,
                                     const std::string& varName,
                                     IsVector isit)
{
  if (!rootNode)
  {
    throw std::runtime_error("Invalid Conduit node pointer provided to ReadVariableInternal.");
  }

  const std::vector<size_t> shape = GetShapeValues(&nodeShape);
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

    return fides::RawArray(
      std::move(buf), numValues, numComponents, fides::GetDataType<VariableType>());
  }
  else
  {
    // Type mismatch path: Allocate a new buffer and perform the cast/copy.
    auto raw = fides::AllocateRawArray<VariableType>(numValues, numComponents);
    VariableType* buffer = raw.template GetWritePointer<VariableType>();

    // Convert element-by-element, flagging any value that a narrower requested
    // integer dtype truncated: a round-trip cast that does not recover the
    // original value indicates lost data.
    bool truncated = false;
    std::transform(vecData, vecData + totalElements, buffer, [&truncated](ConduitType val) {
      const VariableType out = static_cast<VariableType>(val);
      if constexpr (std::is_integral_v<ConduitType> && std::is_integral_v<VariableType>)
      {
        if (static_cast<ConduitType>(out) != val)
        {
          truncated = true;
        }
      }
      return out;
    });
    if (truncated)
    {
      std::cerr << "WARNING: Conduit variable '" << varName
                << "' was read into a narrower integer dtype that truncated one or more values."
                << std::endl;
    }

    return raw;
  }
}

// Reads one block of a variable. \c type is the requested output type: the
// declared dtype if the variable has one, otherwise the block's own type.
//
// clang-tidy flags "readability-function-size" here, but it is due to macro-generated
// numeric type dispatch (fidesTemplateMacro x conduitTemplateMacro), rather than logic
// complexity.
// NOLINTNEXTLINE(readability-function-size)
fides::RawArray ReadBlock(std::shared_ptr<conduit::Node> rootNode,
                          const conduit::Node& nodeShape,
                          const conduit::Node& nodeData,
                          const std::string& varName,
                          const std::string& type,
                          IsVector isit)
{
  const std::string& conduitType = nodeData.dtype().name();
  fidesTemplateMacro(
    (ReadVariableInternal<conduit_TT, fides_TT>(rootNode, nodeShape, nodeData, varName, isit)));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<fides::RawArray> ConduitDataSource::ReadVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  IsVector isit,
  ReadMode fidesNotUsed(mode))
{
  // It's an error if the shared_ptr is empty
  if (!this->Internals->Node)
  {
    throw std::runtime_error("Conduit node pointer is null for variable " + varName);
  }

  const conduit::Node& n = (*this->Internals->Node);
  const auto blocks = GetExistingVariable(n, varName, this->SchemaDocument).Blocks;

  // The declared dtype node is optional. When present it is an explicit request
  // for the output type, which may differ from the stored buffer (for example a
  // float64 buffer declared "float" narrows to float on output). When absent,
  // honor the buffer's own type: a Conduit node is self-describing, so no
  // separate type declaration is required.
  const std::string dataTypePath =
    GetConduitPath(ConduitNodeType::DataType, varName, this->SchemaDocument);
  const bool hasDeclaredType = n.has_path(dataTypePath);

  // Without a declared dtype, every block must store the same type, as the
  // blocks of an ADIOS2 variable do. All blocks are checked, not only the
  // selected ones, so that whether a read succeeds and the type it returns do
  // not depend on the block selection.
  if (!hasDeclaredType)
  {
    const conduit::Node* firstData = nullptr;
    for (const ConduitBlock& block : blocks)
    {
      if (!block.Data)
      {
        continue;
      }
      if (!firstData)
      {
        firstData = block.Data;
      }
      else if (block.Data->dtype().id() != firstData->dtype().id())
      {
        throw std::runtime_error("Blocks of variable " + varName + " store different types (" +
                                 firstData->dtype().name() + " and " + block.Data->dtype().name() +
                                 "); declare the variable's dtype to convert them to one type");
      }
    }
  }

  std::vector<fides::RawArray> arrays;
  for (size_t blockId : GetSelectedBlocks(blocks.size(), selections, varName))
  {
    const ConduitBlock& block = blocks[blockId];
    if (!block.Data)
    {
      throw std::runtime_error(
        "Missing expected data node " +
        GetConduitPath(ConduitNodeType::Data, varName, this->SchemaDocument) + " for variable " +
        varName);
    }
    const std::string type =
      hasDeclaredType ? n[dataTypePath].as_string() : block.Data->dtype().name();
    arrays.push_back(
      ReadBlock(this->Internals->Node, *block.Shape, *block.Data, varName, type, isit));
  }
  return arrays;
}

std::vector<fides::RawArray> ConduitDataSource::ReadMultiBlockVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  ReadMode mode)
{
  // Read the selected blocks and, like the ADIOS data source, return them as a
  // single contiguous array.
  auto arrays = this->ReadVariable(varName, selections, IsVector::Auto, mode);
  if (arrays.size() <= 1)
  {
    return arrays;
  }

  const fides::RawArray& first = arrays.front();
  size_t numValues = 0;
  size_t numBytes = 0;
  for (const auto& array : arrays)
  {
    if (array.Type != first.Type || array.NumComponents != first.NumComponents)
    {
      throw std::runtime_error("Blocks of variable " + varName +
                               " differ in type or number of components");
    }
    numValues += array.NumValues;
    numBytes += array.GetNumberOfBytes();
  }

  std::shared_ptr<void> buffer(new char[numBytes], [](void* p) { delete[] static_cast<char*>(p); });
  char* dest = static_cast<char*>(buffer.get());
  for (const auto& array : arrays)
  {
    std::memcpy(dest, array.Data.get(), array.GetNumberOfBytes());
    dest += array.GetNumberOfBytes();
  }
  return std::vector<fides::RawArray>{ fides::RawArray(
    std::move(buffer), numValues, first.NumComponents, first.Type) };
}

size_t ConduitDataSource::GetNumberOfBlocks(const std::string& varName)
{
  return this->GetNumberOfBlocks(varName, "");
}

size_t ConduitDataSource::GetNumberOfBlocks(const std::string& varName,
                                            const std::string& fidesNotUsed(group))
{
  if (!this->Internals->Node)
  {
    return 0;
  }
  return GetVariable(*this->Internals->Node, varName, this->SchemaDocument).Blocks.size();
}

std::vector<size_t> ConduitDataSource::GetVariableShape(std::string& varName)
{
  return this->GetVariableShape(varName, "");
}

std::vector<size_t> ConduitDataSource::GetVariableShape(std::string& varName,
                                                        const std::string& fidesNotUsed(group))
{
  // A variable-level shape takes precedence. Without one, a multi-block
  // variable reports the shape of its first block.
  const conduit::Node& n = (*this->Internals->Node);
  const std::string shapePath =
    GetConduitPath(ConduitNodeType::Shape, varName, this->SchemaDocument);
  if (n.has_path(shapePath))
  {
    return GetShapeValues(&n[shapePath]);
  }

  const auto blocks = GetExistingVariable(n, varName, this->SchemaDocument).Blocks;
  if (blocks.empty())
  {
    // A variable with no blocks on this rank
    return std::vector<size_t>();
  }
  return GetShapeValues(blocks.front().Shape);
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
