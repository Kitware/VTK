/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataAssembly.h"

#include "vtkDataAssemblyVisitor.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <vtksys/RegularExpression.hxx>

#include <vtk_pugixml.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

static constexpr const char* DATASET_NODE_NAME = "dataset";

//============================================================================
//** vtkDataAssemblyVisitor **
//============================================================================
class vtkDataAssemblyVisitor::vtkInternals
{
public:
  pugi::xml_node CurrentNode;
};

//------------------------------------------------------------------------------
vtkDataAssemblyVisitor::vtkDataAssemblyVisitor()
  : Internals(new vtkDataAssemblyVisitor::vtkInternals())
  , Assembly(nullptr)
  , TraversalOrder(vtkDataAssembly::DepthFirst)
{
}

//------------------------------------------------------------------------------
vtkDataAssemblyVisitor::~vtkDataAssemblyVisitor() = default;
//------------------------------------------------------------------------------
const char* vtkDataAssemblyVisitor::GetCurrentNodeName() const
{
  return this->Internals->CurrentNode.name();
}

//------------------------------------------------------------------------------
std::vector<unsigned int> vtkDataAssemblyVisitor::GetCurrentDataSetIndices() const
{
  std::vector<unsigned int> indices;
  for (auto child : this->Internals->CurrentNode.children(DATASET_NODE_NAME))
  {
    indices.push_back(child.attribute("id").as_uint());
  }
  return indices;
}

//------------------------------------------------------------------------------
void vtkDataAssemblyVisitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{

//------------------------------------------------------------------------------
bool IsAssemblyNode(const pugi::xml_node& node)
{
  return (!vtkDataAssembly::IsNodeNameReserved(node.name()));
}

//------------------------------------------------------------------------------
bool IsDataSetNode(const pugi::xml_node& node)
{
  return strcmp(node.name(), DATASET_NODE_NAME) == 0;
}

//------------------------------------------------------------------------------
struct ValidationAndInitializationWalker : public pugi::xml_tree_walker
{
  std::unordered_map<int, pugi::xml_node>& NodeMap;
  int& MaxUniqueId;

  ValidationAndInitializationWalker(std::unordered_map<int, pugi::xml_node>& map, int& id)
    : NodeMap(map)
    , MaxUniqueId(id)
  {
  }
  bool for_each(pugi::xml_node& node) override
  {
    if (IsAssemblyNode(node))
    {
      if (auto attr = node.attribute("id"))
      {
        if (auto id = attr.as_int())
        {
          this->MaxUniqueId = std::max(this->MaxUniqueId, id);
          this->NodeMap[id] = node;
        }
        else
        {
          vtkLogF(ERROR, "Invalid required attribute, id='%s' on '%s'", attr.value(),
            node.path().c_str());
          return false;
        }
      }
      else
      {
        vtkLogF(ERROR, "Missing required attribute 'id' on node '%s'", node.path().c_str());
        return false;
      }
    }
    else if (IsDataSetNode(node))
    {
      if (auto attr = node.attribute("id"))
      {
        auto id = attr.as_uint(VTK_UNSIGNED_INT_MAX);
        if (id == VTK_UNSIGNED_INT_MAX)
        {
          vtkLogF(ERROR, "Invalid required attribute, id='%s'", attr.value());
          return false;
        }
      }
      else
      {
        vtkLogF(ERROR, "Missing required attribute 'id' on 'dataset'.");
        return false;
      }
    }
    else
    {
      vtkLogF(ERROR, "Invalid node with name '%s'", node.name());
      return false;
    }
    return true;
  }
};

//------------------------------------------------------------------------------
// Walker used to offset all node "id"s.
struct OffsetIdWalker : public pugi::xml_tree_walker
{
  int Offset;
  OffsetIdWalker(int offset)
    : Offset(offset)
  {
  }

  void update(pugi::xml_node& node) const
  {
    if (IsAssemblyNode(node))
    {
      auto attr = node.attribute("id");
      auto id = attr.as_uint(VTK_UNSIGNED_INT_MAX);
      if (id != VTK_UNSIGNED_INT_MAX)
      {
        attr.set_value(id + this->Offset);
      }
    }
  }

  bool begin(pugi::xml_node& node) override
  {
    this->update(node);
    return true;
  }
  bool for_each(pugi::xml_node& node) override
  {
    this->update(node);
    return true;
  }
};

//------------------------------------------------------------------------------
class FindNodesWithNameVisitor : public vtkDataAssemblyVisitor
{
public:
  static FindNodesWithNameVisitor* New();
  vtkTypeMacro(FindNodesWithNameVisitor, vtkDataAssemblyVisitor);

  const char* Name = nullptr;
  bool FindFirstMatch = false;

  std::vector<int> Matches;

  struct interrupt : public std::exception
  {
  };

protected:
  FindNodesWithNameVisitor() = default;
  ~FindNodesWithNameVisitor() override = default;

  void Visit(int nodeid) override
  {
    auto name = this->GetCurrentNodeName();
    if (strcmp(name, this->Name) == 0)
    {
      this->Matches.push_back(nodeid);
      if (this->FindFirstMatch)
      {
        // stop traversal, we're done.
        throw interrupt{};
      }
    }
  }

private:
  FindNodesWithNameVisitor(const FindNodesWithNameVisitor&) = delete;
  void operator=(const FindNodesWithNameVisitor&) = delete;
};
vtkStandardNewMacro(FindNodesWithNameVisitor);

//------------------------------------------------------------------------------
class GetChildNodesVisitor : public vtkDataAssemblyVisitor
{
public:
  static GetChildNodesVisitor* New();
  vtkTypeMacro(GetChildNodesVisitor, vtkDataAssemblyVisitor);

  int Root = 0;
  bool TraverseSubtree = true;
  std::vector<int> Children;

  void Visit(int nodeid) override
  {
    // skip the node whose children we're searching for
    if (this->Root != nodeid)
    {
      this->Children.push_back(nodeid);
    }
  }
  bool GetTraverseSubtree(int nodeid) override
  {
    return this->TraverseSubtree || (nodeid == this->Root);
  }

protected:
  GetChildNodesVisitor() = default;
  ~GetChildNodesVisitor() override = default;
};
vtkStandardNewMacro(GetChildNodesVisitor);

//------------------------------------------------------------------------------
class GetDataSetIndicesVisitor : public vtkDataAssemblyVisitor
{
public:
  static GetDataSetIndicesVisitor* New();
  vtkTypeMacro(GetDataSetIndicesVisitor, vtkDataAssemblyVisitor);

  bool TraverseSubtree = true;
  int Root = 0;
  std::vector<unsigned int> DataSetIndices;

  void Visit(int) override
  {
    const auto curids = this->GetCurrentDataSetIndices();
    std::copy(curids.begin(), curids.end(), std::back_inserter(this->DataSetIndices));
  }
  bool GetTraverseSubtree(int vtkNotUsed(nodeid)) override { return this->TraverseSubtree; }

protected:
  GetDataSetIndicesVisitor() = default;
  ~GetDataSetIndicesVisitor() override = default;
};
vtkStandardNewMacro(GetDataSetIndicesVisitor);

//------------------------------------------------------------------------------
class SelectNodesVisitor : public vtkDataAssemblyVisitor
{
public:
  static SelectNodesVisitor* New();
  vtkTypeMacro(SelectNodesVisitor, vtkDataAssemblyVisitor);

  std::unordered_set<int> UnorderedSelectedNodes;
  std::vector<int> SelectedNodes;

  void Visit(int id) override
  {
    if (this->UnorderedSelectedNodes.find(id) != this->UnorderedSelectedNodes.end())
    {
      this->SelectedNodes.push_back(id);
    }
  }

protected:
  SelectNodesVisitor() = default;
  ~SelectNodesVisitor() override = default;
};
vtkStandardNewMacro(SelectNodesVisitor);
}

//============================================================================
class vtkDataAssembly::vtkInternals
{
public:
  pugi::xml_document Document;
  std::unordered_map<int, pugi::xml_node> NodeMap;
  int MaxUniqueId = 0;
  bool Parse(const char* xmlcontents, vtkDataAssembly* self);

  bool ParseDocument(vtkDataAssembly* self)
  {
    auto& doc = this->Document;
    this->NodeMap.clear();
    this->MaxUniqueId = 0;

    ValidationAndInitializationWalker walker{ this->NodeMap, this->MaxUniqueId };
    auto root = doc.first_child();
    if (::IsAssemblyNode(root) && root.attribute("version").as_float() == 1.0f &&
      root.attribute("id").as_int(-1) == 0 &&
      strcmp(root.attribute("type").as_string(), "vtkDataAssembly") == 0 && root.traverse(walker))
    {
      this->NodeMap[0] = root;
      return true;
    }
    else
    {
      vtkErrorWithObjectMacro(self, "Not a vtkDataAssembly XML.");
      return false;
    }
  }

  pugi::xml_node FindNode(int id) const
  {
    auto iter = this->NodeMap.find(id);
    return iter != this->NodeMap.end() ? iter->second : pugi::xml_node();
  }
};

//------------------------------------------------------------------------------
bool vtkDataAssembly::vtkInternals::Parse(const char* xmlcontents, vtkDataAssembly* self)
{
  auto& doc = this->Document;
  auto result = doc.load_string(xmlcontents);
  if (!result)
  {
    vtkErrorWithObjectMacro(self,
      "Invalid xml provided. \n"
        << "  Error description: " << result.description() << "\n"
        << "  Error offset: " << result.offset << " (error at [..." << (xmlcontents + result.offset)
        << "])");
    return false;
  }
  return this->ParseDocument(self);
}

//============================================================================
vtkStandardNewMacro(vtkDataAssembly);
//------------------------------------------------------------------------------
vtkDataAssembly::vtkDataAssembly()
  : Internals(nullptr)
{
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkDataAssembly::~vtkDataAssembly() = default;

//------------------------------------------------------------------------------
bool vtkDataAssembly::IsNodeNameValid(const char* name)
{
  if (name == nullptr || name[0] == '\0' || vtkDataAssembly::IsNodeNameReserved(name))
  {
    return false;
  }

  if ((name[0] < 'a' || name[0] > 'z') && (name[0] < 'A' || name[0] > 'Z') && name[0] != '_')
  {
    // names must start with a letter or underscore.
    return false;
  }

  vtksys::RegularExpression regEx("[^a-zA-Z0-9_.-]");
  if (regEx.find(name))
  {
    // found a non-acceptable character; names can contain letters,
    // digits, hyphens, underscores, and periods.
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
std::string vtkDataAssembly::MakeValidNodeName(const char* name)
{
  if (name == nullptr || name[0] == '\0')
  {
    vtkLog(ERROR, "cannot convert empty string to a valid name");
    return std::string();
  }

  if (vtkDataAssembly::IsNodeNameReserved(name))
  {
    vtkLogF(ERROR, "'%s' is a reserved name.", name);
    return std::string();
  }

  const char sorted_valid_chars[] =
    ".-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
  const auto sorted_valid_chars_len = strlen(sorted_valid_chars);

  std::string result;
  result.reserve(strlen(name));
  for (size_t cc = 0, max = strlen(name); cc < max; ++cc)
  {
    if (std::binary_search(
          sorted_valid_chars, sorted_valid_chars + sorted_valid_chars_len, name[cc]))
    {
      result += name[cc];
    }
  }

  if (result.empty() ||
    ((result[0] < 'a' || result[0] > 'z') && (result[0] < 'A' || result[0] > 'Z') &&
      result[0] != '_'))
  {
    return "_" + result;
  }
  return result;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::IsNodeNameReserved(const char* name)
{
  return name ? strcmp(name, DATASET_NODE_NAME) == 0 : false;
}

//------------------------------------------------------------------------------
void vtkDataAssembly::Initialize()
{
  this->Internals.reset(new vtkDataAssembly::vtkInternals());
  this->Internals->Parse("<assembly type='vtkDataAssembly' version='1.0' id='0' />", this);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::InitializeFromXML(const char* xmlcontents)
{
  this->Initialize();
  if (xmlcontents == nullptr || xmlcontents[0] == '\0')
  {
    return true;
  }

  if (!this->Internals->Parse(xmlcontents, this))
  {
    this->Initialize();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
std::string vtkDataAssembly::SerializeToXML(vtkIndent indent) const
{
  std::ostringstream indent_str;
  indent_str << indent;

  std::ostringstream stream;
  this->Internals->Document.save(stream, indent_str.str().c_str());
  return stream.str();
}

//------------------------------------------------------------------------------
void vtkDataAssembly::DeepCopy(vtkDataAssembly* other)
{
  if (other)
  {
    this->Internals.reset(new vtkDataAssembly::vtkInternals());
    // TODO: am sure there's a better way than serialize/deserialize
    std::ostringstream str;
    other->Internals->Document.save(str);
    this->Internals->Parse(str.str().c_str(), this);
    this->Modified();
  }
  else
  {
    this->Initialize();
  }
}

//------------------------------------------------------------------------------
int vtkDataAssembly::AddSubtree(int parent, vtkDataAssembly* other, int otherParent)
{
  if (!other)
  {
    vtkErrorMacro("'other' cannot be nullptr.");
    return -1;
  }

  auto& internals = (*this->Internals);
  auto node = internals.FindNode(parent);
  if (!node)
  {
    vtkErrorMacro("Parent node with id=" << parent << " not found.");
    return -1;
  }

  auto& ointernals = (*other->Internals);
  auto onode = ointernals.FindNode(otherParent);
  if (!onode)
  {
    vtkErrorMacro("Note node with id=" << parent << " not found on 'other'");
    return -1;
  }

  auto subtree = node.append_copy(onode);
  if (otherParent == 0)
  {
    // remove type and version attributes.
    subtree.remove_attribute(subtree.attribute("type"));
    subtree.remove_attribute(subtree.attribute("version"));
  }

  // now update node ids on the copied subtree.
  OffsetIdWalker walker(internals.MaxUniqueId + 1);
  subtree.traverse(walker);

  // reset internal datastructure (and also validate it)
  return internals.ParseDocument(this);
}

//------------------------------------------------------------------------------
int vtkDataAssembly::AddNode(const char* name, int parent)
{
  if (!vtkDataAssembly::IsNodeNameValid(name))
  {
    vtkErrorMacro("Invalid name specified '" << (name ? name : "(nullptr)"));
    return -1;
  }

  auto& internals = (*this->Internals);
  auto pnode = internals.FindNode(parent);
  if (!pnode)
  {
    vtkErrorMacro("Parent node with id=" << parent << " not found.");
    return -1;
  }

  auto child = ++internals.MaxUniqueId;
  auto cnode = pnode.append_child(name);
  cnode.append_attribute("id") = child;
  internals.NodeMap[child] = cnode;
  this->Modified();
  return child;
}

//------------------------------------------------------------------------------
std::vector<int> vtkDataAssembly::AddNodes(const std::vector<std::string>& names, int parent)
{
  auto& internals = (*this->Internals);
  auto pnode = internals.FindNode(parent);
  if (!pnode)
  {
    vtkErrorMacro("Parent node with id=" << parent << " not found.");
    return std::vector<int>{};
  }

  // validate names first to avoid partial additions.
  for (const auto& name : names)
  {
    if (!vtkDataAssembly::IsNodeNameValid(name.c_str()))
    {
      vtkErrorMacro("Invalid name specified '" << name.c_str() << "'.");
      return std::vector<int>{};
    }
  }

  std::vector<int> ids;
  for (const auto& name : names)
  {
    auto child = ++internals.MaxUniqueId;
    auto cnode = pnode.append_child(name.c_str());
    cnode.append_attribute("id") = child;
    internals.NodeMap[child] = cnode;
    ids.push_back(child);
  }
  if (!ids.empty())
  {
    this->Modified();
  }
  return ids;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::RemoveNode(int id)
{
  if (id == 0)
  {
    vtkErrorMacro("Cannot remove root node.");
    return false;
  }

  auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return false;
  }

  // cleanup nodemap.
  for (auto childid : this->GetChildNodes(id))
  {
    internals.NodeMap.erase(childid);
  }
  internals.NodeMap.erase(id);
  node.parent().remove_child(node);
  this->Modified();
  return true;
}

//------------------------------------------------------------------------------
void vtkDataAssembly::SetNodeName(int id, const char* name)
{
  if (!vtkDataAssembly::IsNodeNameValid(name))
  {
    vtkErrorMacro("Invalid name specified '" << (name ? name : "(nullptr)") << "'.");
    return;
  }

  auto& internals = (*this->Internals);
  if (auto node = internals.FindNode(id))
  {
    node.set_name(name);
    this->Modified();
  }
  else
  {
    vtkErrorMacro("Invalid id='" << id << "'");
  }
}

//------------------------------------------------------------------------------
const char* vtkDataAssembly::GetNodeName(int id) const
{
  const auto& internals = (*this->Internals);
  if (auto node = internals.FindNode(id))
  {
    return node.name();
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
std::string vtkDataAssembly::GetNodePath(int id) const
{
  const auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return std::string{};
  }

  return node.path();
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetFirstNodeByPath(const char* path) const
{
  const auto& internals = (*this->Internals);
  auto node = internals.FindNode(vtkDataAssembly::GetRootNode()).first_element_by_path(path);
  return node ? node.attribute("id").as_int() : -1;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::AddDataSetIndex(int id, unsigned int dataset_index)
{
  auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return false;
  }
  auto current_datasets = this->GetDataSetIndices(id, /*traverse_subtree=*/false);
  if (std::find(current_datasets.begin(), current_datasets.end(), dataset_index) !=
    current_datasets.end())
  {
    // already present, no need to add again.
    return true;
  }

  auto dsnode = node.append_child(DATASET_NODE_NAME);
  dsnode.append_attribute("id") = dataset_index;
  this->Modified();
  return true;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::AddDataSetIndices(int id, const std::vector<unsigned int>& dataset_indices)
{
  auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return false;
  }
  auto current_datasets = this->GetDataSetIndices(id, /*traverse_subtree=*/false);
  std::unordered_set<unsigned int> set(current_datasets.begin(), current_datasets.end());
  bool modified = false;
  for (const auto& idx : dataset_indices)
  {
    if (set.find(idx) == set.end())
    {
      set.insert(idx);
      auto dsnode = node.append_child(DATASET_NODE_NAME);
      dsnode.append_attribute("id") = idx;
      modified = true;
    }
  }
  if (modified)
  {
    this->Modified();
  }
  return modified;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::AddDataSetIndexRange(int id, unsigned int index_start, int count)
{
  // for now, we're doing this easy thing..at some point we may want to add
  // support for storing ranges compactly.
  std::vector<unsigned int> indices(count);
  std::iota(indices.begin(), indices.end(), index_start);
  return this->AddDataSetIndices(id, indices);
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::RemoveDataSetIndex(int id, unsigned int dataset_index)
{
  auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return false;
  }

  for (auto child : node.children(DATASET_NODE_NAME))
  {
    if (child.attribute("id").as_uint() == dataset_index)
    {
      child.parent().remove_child(child);
      this->Modified();
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::RemoveAllDataSetIndices(int id, bool traverse_subtree /*=true*/)
{
  auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  if (!node)
  {
    return false;
  }

  std::vector<pugi::xml_node> to_remove;

  struct Walker : pugi::xml_tree_walker
  {
    std::vector<pugi::xml_node>* ToRemove = nullptr;
    bool for_each(pugi::xml_node& nnode) override
    {
      if (strcmp(nnode.name(), DATASET_NODE_NAME) == 0)
      {
        this->ToRemove->push_back(nnode);
      }
      return true;
    }
  };

  if (traverse_subtree)
  {
    Walker walker;
    walker.ToRemove = &to_remove;
    node.traverse(walker);
  }
  else
  {
    for (const auto& dschild : node.children(DATASET_NODE_NAME))
    {
      to_remove.push_back(dschild);
    }
  }

  for (auto& dsnode : to_remove)
  {
    dsnode.parent().remove_child(dsnode);
  }

  if (!to_remove.empty())
  {
    this->Modified();
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetNumberOfChildren(int parent) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(parent);
  if (!node)
  {
    return 0;
  }

  auto range = node.children();
  const int count = std::count_if(range.begin(), range.end(), ::IsAssemblyNode);
  return count;
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetChild(int parent, int index) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(parent);
  int cur_child = 0;
  for (const auto& cnode : node.children())
  {
    if (::IsAssemblyNode(cnode))
    {
      if (cur_child == index)
      {
        return cnode.attribute("id").as_int(-1);
      }
      ++cur_child;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetChildIndex(int parent, int child) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(parent);
  int index = 0;
  for (const auto& cnode : node.children())
  {
    if (::IsAssemblyNode(cnode))
    {
      if (cnode.attribute("id").as_int(-1) == child)
      {
        return index;
      }
      ++index;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetParent(int id) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return node.parent().attribute("id").as_int(-1);
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::HasAttribute(int id, const char* name) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return (node.attribute(name));
}

//------------------------------------------------------------------------------
void vtkDataAssembly::SetAttribute(int id, const char* name, const char* value)
{
  const auto& internals = (*this->Internals);
  auto node = internals.FindNode(id);
  auto attr = node.attribute(name);
  if (!attr)
  {
    attr = node.append_attribute(name);
  }
  attr.set_value(value);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataAssembly::SetAttribute(int id, const char* name, int value)
{
  this->SetAttribute(id, name, std::to_string(value).c_str());
}

//------------------------------------------------------------------------------
void vtkDataAssembly::SetAttribute(int id, const char* name, unsigned int value)
{
  this->SetAttribute(id, name, std::to_string(value).c_str());
}

//------------------------------------------------------------------------------
#if VTK_ID_TYPE_IMPL != VTK_INT
void vtkDataAssembly::SetAttribute(int id, const char* name, vtkIdType value)
{
  this->SetAttribute(id, name, std::to_string(value).c_str());
}
#endif

//------------------------------------------------------------------------------
bool vtkDataAssembly::GetAttribute(int id, const char* name, const char*& value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  if (auto attr = node.attribute(name))
  {
    value = attr.as_string();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::GetAttribute(int id, const char* name, int& value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  if (auto attr = node.attribute(name))
  {
    value = attr.as_int();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::GetAttribute(int id, const char* name, unsigned int& value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  if (auto attr = node.attribute(name))
  {
    value = attr.as_uint();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
#if VTK_ID_TYPE_IMPL != VTK_INT
bool vtkDataAssembly::GetAttribute(int id, const char* name, vtkIdType& value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  if (auto attr = node.attribute(name))
  {
    value = static_cast<vtkIdType>(attr.as_llong());
    return true;
  }
  return false;
}
#endif

//------------------------------------------------------------------------------
const char* vtkDataAssembly::GetAttributeOrDefault(
  int id, const char* name, const char* default_value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return node.attribute(name).as_string(default_value);
}

//------------------------------------------------------------------------------
int vtkDataAssembly::GetAttributeOrDefault(int id, const char* name, int default_value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return node.attribute(name).as_int(default_value);
}

//------------------------------------------------------------------------------
unsigned int vtkDataAssembly::GetAttributeOrDefault(
  int id, const char* name, unsigned int default_value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return node.attribute(name).as_uint(default_value);
}

#if VTK_ID_TYPE_IMPL != VTK_INT
//------------------------------------------------------------------------------
vtkIdType vtkDataAssembly::GetAttributeOrDefault(
  int id, const char* name, vtkIdType default_value) const
{
  const auto& internals = (*this->Internals);
  const auto node = internals.FindNode(id);
  return static_cast<vtkIdType>(node.attribute(name).as_llong(default_value));
}
#endif

//------------------------------------------------------------------------------
int vtkDataAssembly::FindFirstNodeWithName(const char* name, int traversal_order) const
{
  vtkNew<FindNodesWithNameVisitor> visitor;
  visitor->FindFirstMatch = true;
  visitor->Name = name;
  try
  {
    this->Visit(visitor, traversal_order);
  }
  catch (FindNodesWithNameVisitor::interrupt&)
  {
    // catch the interrupt exception.
  }
  return !visitor->Matches.empty() ? visitor->Matches.front() : -1;
}

//------------------------------------------------------------------------------
std::vector<int> vtkDataAssembly::FindNodesWithName(const char* name, int traversal_order) const
{
  vtkNew<FindNodesWithNameVisitor> visitor;
  visitor->FindFirstMatch = false;
  visitor->Name = name;
  this->Visit(visitor, traversal_order);
  return visitor->Matches;
}

//------------------------------------------------------------------------------
std::vector<int> vtkDataAssembly::GetChildNodes(
  int parent, bool traverse_subtree, int traversal_order) const
{
  vtkNew<GetChildNodesVisitor> visitor;
  visitor->TraverseSubtree = traverse_subtree;
  visitor->Root = parent;
  this->Visit(parent, visitor,
    traverse_subtree ? traversal_order : vtkDataAssembly::TraversalOrder::BreadthFirst);
  return visitor->Children;
}

//------------------------------------------------------------------------------
std::vector<unsigned int> vtkDataAssembly::GetDataSetIndices(
  int id, bool traverse_subtree, int traversal_order) const
{
  std::vector<int> ids;
  ids.push_back(id);
  return this->GetDataSetIndices(ids, traverse_subtree, traversal_order);
}

//------------------------------------------------------------------------------
std::vector<unsigned int> vtkDataAssembly::GetDataSetIndices(
  const std::vector<int>& ids, bool traverse_subtree, int traversal_order) const
{
  vtkNew<GetDataSetIndicesVisitor> visitor;
  visitor->TraverseSubtree = traverse_subtree;
  for (const auto& nodeid : ids)
  {
    visitor->Root = nodeid;
    this->Visit(nodeid, visitor,
      traverse_subtree ? traversal_order : vtkDataAssembly::TraversalOrder::BreadthFirst);
  }

  // uniquify dataset indices.
  auto& indices = visitor->DataSetIndices;
  std::unordered_set<unsigned int> helper;
  auto end = std::remove_if(indices.begin(), indices.end(),
    [&helper](const unsigned int& idx) { return !helper.insert(idx).second; });
  indices.erase(end, indices.end());
  return indices;
}

//------------------------------------------------------------------------------
void vtkDataAssembly::Visit(int id, vtkDataAssemblyVisitor* visitor, int traversal_order) const
{
  const auto& internals = (*this->Internals);
  if (visitor == nullptr || internals.NodeMap.find(id) == internals.NodeMap.end())
  {
    vtkErrorMacro("Invalid parameters.");
    return;
  }

  visitor->TraversalOrder = traversal_order;
  visitor->Assembly = this;
  auto& vinternals = *(visitor->Internals);

  if (traversal_order == vtkDataAssembly::TraversalOrder::DepthFirst)
  {
    std::function<void(const pugi::xml_node&)> iterate;
    iterate = [&](const pugi::xml_node& node) {
      const auto cid = node.attribute("id").as_int(-1);
      vinternals.CurrentNode = node;
      visitor->Visit(cid);

      // Do subtree, if enabled.
      if (visitor->GetTraverseSubtree(cid))
      {
        visitor->BeginSubTree(cid);
        for (const auto& child : node.children())
        {
          if (::IsAssemblyNode(child))
          {
            vinternals.CurrentNode = child;
            iterate(child);
          }
        }
        vinternals.CurrentNode = node;
        visitor->EndSubTree(cid);
      }
    };
    iterate(internals.NodeMap.at(id));
  }
  else
  {
    // breadth-first traversal.
    std::deque<pugi::xml_node> fifo_visited;
    vinternals.CurrentNode = internals.NodeMap.at(id);
    visitor->Visit(id);

    fifo_visited.push_back(internals.NodeMap.at(id));
    while (!fifo_visited.empty())
    {
      auto node = fifo_visited.front();
      fifo_visited.pop_front();

      const auto cid = node.attribute("id").as_int(-1);
      vinternals.CurrentNode = node;
      if (visitor->GetTraverseSubtree(cid))
      {
        visitor->BeginSubTree(cid);
        for (const auto& child : node.children())
        {
          if (::IsAssemblyNode(child))
          {
            vinternals.CurrentNode = child;
            visitor->Visit(child.attribute("id").as_int(-1));
            fifo_visited.push_back(child);
          }
        }
        vinternals.CurrentNode = node;
        visitor->EndSubTree(cid);
      }
    }
  }

  vinternals.CurrentNode = pugi::xml_node();
  visitor->TraversalOrder = vtkDataAssembly::TraversalOrder::DepthFirst;
  visitor->Assembly = nullptr;
}

//------------------------------------------------------------------------------
std::vector<int> vtkDataAssembly::SelectNodes(
  const std::vector<std::string>& path_queries, int traversal_order) const
{
  const auto& internals = (*this->Internals);
  vtkNew<SelectNodesVisitor> visitor;
  for (const auto& query : path_queries)
  {
    vtkLogF(TRACE, "query='%s'", query.c_str());
    if (query.empty())
    {
      continue;
    }
    try
    {
      auto set = internals.Document.select_nodes(query.c_str());

      auto notUsed = std::accumulate(set.begin(), set.end(), &visitor->UnorderedSelectedNodes,
        [&internals](std::unordered_set<int>* result, const pugi::xpath_node& xnode) {
          if (xnode.node() == internals.Document)
          {
            // note: if xpath matches the document, the xnode is the document and not the
            // first-child and the attribute request fails.
            result->insert(0);
          }
          else if (::IsAssemblyNode(xnode.node()))
          {
            result->insert(xnode.node().attribute("id").as_int(-1));
          }
          return result;
        });
      (void)notUsed;
    }
    catch (pugi::xpath_exception& exp)
    {
      vtkLogF(TRACE, "xpath exception: %s", exp.what());
    }
  }

  this->Visit(visitor, traversal_order);
  return visitor->SelectedNodes;
}

//------------------------------------------------------------------------------
bool vtkDataAssembly::RemapDataSetIndices(
  const std::map<unsigned int, unsigned int>& mapping, bool remove_unmapped)
{
  bool modified = false;
  auto& internals = (*this->Internals);
  for (const auto& xpath_node : internals.Document.select_nodes("//dataset"))
  {
    const auto id = xpath_node.node().attribute("id").as_uint();
    auto iter = mapping.find(id);
    if (iter != mapping.end())
    {
      if (iter->second != id)
      {
        xpath_node.node().attribute("id").set_value(iter->second);
        modified = true;
      }
    }
    else if (remove_unmapped)
    {
      xpath_node.node().parent().remove_child(xpath_node.node());
      modified = true;
    }
  }

  if (modified)
  {
    this->Modified();
  }
  return modified;
}

//------------------------------------------------------------------------------
void vtkDataAssembly::SubsetCopy(vtkDataAssembly* other, const std::vector<int>& selected_branches)
{
  this->Initialize();
  if (other == nullptr)
  {
    return;
  }

  auto& internals = (*this->Internals);
  const auto& ointernals = (*other->Internals);

  if (selected_branches.empty())
  {
    auto src = ointernals.Document.first_child();
    auto dest = internals.Document.first_child();

    dest.set_name(src.name());
    for (const auto& attribute : src.attributes())
    {
      dest.append_copy(attribute);
    }
    return;
  }

  std::unordered_set<int> complete_subtree;
  std::unordered_set<int> partial_subtree;

  for (const auto& id : selected_branches)
  {
    auto node = ointernals.FindNode(id);
    if (node)
    {
      complete_subtree.insert(id);
    }
    while ((node = node.parent()))
    {
      if (!partial_subtree.insert(node.attribute("id").as_int(-1)).second)
      {
        // parent already inserted, short-cut traversal.
        break;
      }
    }
  }

  if (complete_subtree.find(0) != complete_subtree.end())
  {
    this->DeepCopy(other);
    return;
  }

  // ensure root is always in the partial_subtree
  assert(partial_subtree.find(0) != partial_subtree.end());

  std::function<void(const pugi::xml_node&, pugi::xml_node)> subset_copier;
  subset_copier = [&partial_subtree, &complete_subtree, &subset_copier](
                    const pugi::xml_node& src, pugi::xml_node dest) -> void {
    // first, copy src attributes over.
    for (const auto& attribute : src.attributes())
    {
      dest.append_copy(attribute);
    }
    for (const auto& child : src.children())
    {
      if (::IsAssemblyNode(child))
      {
        const auto id = child.attribute("id").as_int(-1);
        if (complete_subtree.find(id) != complete_subtree.end())
        {
          dest.append_copy(child);
        }
        else if (partial_subtree.find(id) != partial_subtree.end())
        {
          auto dest_child = dest.append_child(child.name());
          subset_copier(child, dest_child);
        }
      }
      else
      {
        assert(::IsDataSetNode(child));
        // copy dataset nodes.
        dest.append_copy(child);
      }
    }
  };

  subset_copier(ointernals.Document.first_child(), internals.Document.first_child());
  internals.ParseDocument(this);
}

//------------------------------------------------------------------------------
void vtkDataAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XML Representation : " << endl << endl;
  this->Internals->Document.save(os, "    ");
}
