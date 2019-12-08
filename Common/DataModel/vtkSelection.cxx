/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelection.h"

#include "vtkAbstractArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkTable.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <atomic>
#include <cassert>
#include <cctype>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
// since certain compilers don't support std::to_string yet
template <typename T>
std::string convert_to_string(const T& val)
{
  std::ostringstream str;
  str << val;
  return str.str();
}
}

//============================================================================
namespace parser
{
class Node
{
public:
  Node() = default;
  virtual ~Node() = default;
  virtual bool Evaluate(vtkIdType offset) const = 0;
  virtual void Print(ostream& os) const = 0;
};

class NodeVariable : public Node
{
  vtkSignedCharArray* Data;
  std::string Name;

public:
  NodeVariable(vtkSignedCharArray* data, const std::string& name)
    : Data(data)
    , Name(name)
  {
  }
  bool Evaluate(vtkIdType offset) const override
  {
    assert(this->Data == nullptr || this->Data->GetNumberOfValues() > offset);
    return this->Data ? (this->Data->GetValue(offset) != 0) : false;
  }
  void Print(ostream& os) const override { os << this->Name; }
};

class NodeNot : public Node
{
  std::shared_ptr<Node> Child;

public:
  NodeNot(const std::shared_ptr<Node>& node)
    : Child(node)
  {
  }
  bool Evaluate(vtkIdType offset) const override
  {
    assert(this->Child);
    return !this->Child->Evaluate(offset);
  }
  void Print(ostream& os) const override
  {
    os << "!";
    this->Child->Print(os);
  }
};

class NodeAnd : public Node
{
  std::shared_ptr<Node> ChildA;
  std::shared_ptr<Node> ChildB;

public:
  NodeAnd(const std::shared_ptr<Node>& nodeA, const std::shared_ptr<Node>& nodeB)
    : ChildA(nodeA)
    , ChildB(nodeB)
  {
  }
  bool Evaluate(vtkIdType offset) const override
  {
    assert(this->ChildA && this->ChildB);
    return this->ChildA->Evaluate(offset) && this->ChildB->Evaluate(offset);
  }
  void Print(ostream& os) const override
  {
    os << "(";
    this->ChildA->Print(os);
    os << " & ";
    this->ChildB->Print(os);
    os << ")";
  }
};

class NodeOr : public Node
{
  std::shared_ptr<Node> ChildA;
  std::shared_ptr<Node> ChildB;

public:
  NodeOr(const std::shared_ptr<Node>& nodeA, const std::shared_ptr<Node>& nodeB)
    : ChildA(nodeA)
    , ChildB(nodeB)
  {
  }
  bool Evaluate(vtkIdType offset) const override
  {
    assert(this->ChildA && this->ChildB);
    return this->ChildA->Evaluate(offset) || this->ChildB->Evaluate(offset);
  }
  void Print(ostream& os) const override
  {
    os << "(";
    this->ChildA->Print(os);
    os << " | ";
    this->ChildB->Print(os);
    os << ")";
  }
};
} // namespace parser

//============================================================================
class vtkSelection::vtkInternals
{
  // applies the operator on the "top" (aka back) of the op_stack to the
  // variables on the var_stack and pushes the result on the var_stack.
  bool ApplyBack(
    std::vector<char>& op_stack, std::vector<std::shared_ptr<parser::Node> >& var_stack) const
  {
    assert(!op_stack.empty());

    if (op_stack.back() == '!')
    {
      if (var_stack.empty())
      {
        // failed
        return false;
      }
      const auto a = var_stack.back();
      var_stack.pop_back();
      var_stack.push_back(std::make_shared<parser::NodeNot>(a));
      // pop the applied operator.
      op_stack.pop_back();
      return true;
    }
    else if (op_stack.back() == '|' || op_stack.back() == '&')
    {
      if (var_stack.size() < 2)
      {
        // failed!
        return false;
      }

      const auto b = var_stack.back();
      var_stack.pop_back();
      const auto a = var_stack.back();
      var_stack.pop_back();
      if (op_stack.back() == '|')
      {
        var_stack.push_back(std::make_shared<parser::NodeOr>(a, b));
      }
      else
      {
        var_stack.push_back(std::make_shared<parser::NodeAnd>(a, b));
      }
      // pop the applied operator.
      op_stack.pop_back();
      return true;
    }
    return false;
  }

  // higher the value, higher the precedence.
  inline int precedence(char op) const
  {
    switch (op)
    {
      case '|':
        return -15;
      case '&':
        return -14;
      case '!':
        return -3;
      case '(':
      case ')':
        return -1;
      default:
        return -100;
    }
  }

public:
  std::map<std::string, vtkSmartPointer<vtkSelectionNode> > Items;
  vtksys::RegularExpression RegExID;

  vtkInternals()
    : RegExID("^[a-zA-Z0-9]+$")
  {
  }

  std::shared_ptr<parser::Node> BuildExpressionTree(
    const std::string& expression, const std::map<std::string, vtkSignedCharArray*>& values_map)
  {
    // We don't use PEGTL since it does not support all supported compilers viz.
    // VS2013.
    std::string accumated_text;
    accumated_text.reserve(expression.size() + 64);

    std::vector<std::string> parts;
    for (auto ch : expression)
    {
      switch (ch)
      {
        case '(':
        case ')':
        case '|':
        case '&':
        case '!':
          if (!accumated_text.empty())
          {
            parts.push_back(accumated_text);
            accumated_text.clear();
          }
          parts.push_back(std::string(1, ch));
          break;

        default:
          if (std::isalnum(ch))
          {
            accumated_text.push_back(ch);
          }
          break;
      }
    }
    if (!accumated_text.empty())
    {
      parts.push_back(accumated_text);
    }

    std::vector<std::shared_ptr<parser::Node> > var_stack;
    std::vector<char> op_stack;
    for (const auto& term : parts)
    {
      if (term[0] == '(')
      {
        op_stack.push_back(term[0]);
      }
      else if (term[0] == ')')
      {
        // apply operators till we encounter the opening paren.
        while (!op_stack.empty() && op_stack.back() != '(' && this->ApplyBack(op_stack, var_stack))
        {
        }
        if (op_stack.empty())
        {
          // missing opening paren???
          return nullptr;
        }
        assert(op_stack.back() == '(');
        // pop the opening paren.
        op_stack.pop_back();
      }
      else if (term[0] == '&' || term[0] == '|' || term[0] == '!')
      {
        while (!op_stack.empty() && (precedence(term[0]) < precedence(op_stack.back())) &&
          this->ApplyBack(op_stack, var_stack))
        {
        }
        // push the boolean operator on stack to eval later.
        op_stack.push_back(term[0]);
      }
      else
      {
        auto iter = values_map.find(term);
        auto dataptr = iter != values_map.end() ? iter->second : nullptr;
        var_stack.push_back(std::make_shared<parser::NodeVariable>(dataptr, term));
      }
    }

    while (!op_stack.empty() && this->ApplyBack(op_stack, var_stack))
    {
    }
    return (op_stack.empty() && var_stack.size() == 1) ? var_stack.front() : nullptr;
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelection);

//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
  : Expression()
  , Internals(new vtkSelection::vtkInternals())
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkSelection::~vtkSelection()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSelection::Initialize()
{
  this->Superclass::Initialize();
  this->RemoveAllNodes();
  this->Expression.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkSelection::GetNumberOfNodes() const
{
  return static_cast<unsigned int>(this->Internals->Items.size());
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelection::GetNode(unsigned int idx) const
{
  const vtkInternals& internals = (*this->Internals);
  if (static_cast<unsigned int>(internals.Items.size()) > idx)
  {
    auto iter = std::next(internals.Items.begin(), static_cast<int>(idx));
    assert(iter != internals.Items.end());
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelection::GetNode(const std::string& name) const
{
  const vtkInternals& internals = (*this->Internals);
  auto iter = internals.Items.find(name);
  if (iter != internals.Items.end())
  {
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
std::string vtkSelection::AddNode(vtkSelectionNode* node)
{
  if (!node)
  {
    return std::string();
  }

  const vtkInternals& internals = (*this->Internals);

  // Make sure that node is not already added
  for (const auto& pair : internals.Items)
  {
    if (pair.second == node)
    {
      return pair.first;
    }
  }

  static std::atomic<uint64_t> counter(0U);
  std::string name = std::string("node") + convert_to_string(++counter);
  while (internals.Items.find(name) != internals.Items.end())
  {
    name = std::string("node") + convert_to_string(++counter);
  }

  this->SetNode(name, node);
  return name;
}

//----------------------------------------------------------------------------
void vtkSelection::SetNode(const std::string& name, vtkSelectionNode* node)
{
  vtkInternals& internals = (*this->Internals);
  if (!node)
  {
    vtkErrorMacro("`node` cannot be null.");
  }
  else if (!internals.RegExID.find(name))
  {
    vtkErrorMacro("`" << name << "` is not in the expected form.");
  }
  else if (internals.Items[name] != node)
  {
    internals.Items[name] = node;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
std::string vtkSelection::GetNodeNameAtIndex(unsigned int idx) const
{
  const vtkInternals& internals = (*this->Internals);
  if (static_cast<unsigned int>(internals.Items.size()) > idx)
  {
    auto iter = std::next(internals.Items.begin(), static_cast<int>(idx));
    assert(iter != internals.Items.end());
    return iter->first;
  }
  return std::string();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(unsigned int idx)
{
  vtkInternals& internals = (*this->Internals);
  if (static_cast<unsigned int>(internals.Items.size()) > idx)
  {
    auto iter = std::next(internals.Items.begin(), static_cast<int>(idx));
    assert(iter != internals.Items.end());
    internals.Items.erase(iter);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(const std::string& name)
{
  vtkInternals& internals = (*this->Internals);
  if (internals.Items.erase(name) == 1)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(vtkSelectionNode* node)
{
  vtkInternals& internals = (*this->Internals);
  for (auto iter = internals.Items.begin(); iter != internals.Items.end(); ++iter)
  {
    if (iter->second == node)
    {
      internals.Items.erase(iter);
      this->Modified();
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveAllNodes()
{
  vtkInternals& internals = (*this->Internals);
  if (!internals.Items.empty())
  {
    internals.Items.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  unsigned int numNodes = this->GetNumberOfNodes();
  os << indent << "Number of nodes: " << numNodes << endl;
  os << indent << "Nodes: " << endl;
  for (unsigned int i = 0; i < numNodes; i++)
  {
    os << indent << "Node #" << i << endl;
    this->GetNode(i)->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void vtkSelection::ShallowCopy(vtkDataObject* src)
{
  if (auto* ssrc = vtkSelection::SafeDownCast(src))
  {
    this->Expression = ssrc->Expression;
    this->Internals->Items = ssrc->Internals->Items;
    this->Superclass::ShallowCopy(src);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSelection::DeepCopy(vtkDataObject* src)
{
  if (auto* ssrc = vtkSelection::SafeDownCast(src))
  {
    this->Expression = ssrc->Expression;

    const auto& srcMap = ssrc->Internals->Items;
    auto& destMap = this->Internals->Items;
    destMap = srcMap;
    for (auto& apair : destMap)
    {
      vtkNew<vtkSelectionNode> clone;
      clone->DeepCopy(apair.second);
      apair.second = clone;
    }
    this->Superclass::DeepCopy(src);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelection* s)
{
  for (unsigned int n = 0; n < s->GetNumberOfNodes(); ++n)
  {
    this->Union(s->GetNode(n));
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelectionNode* node)
{
  bool merged = false;
  for (unsigned int tn = 0; tn < this->GetNumberOfNodes(); ++tn)
  {
    vtkSelectionNode* tnode = this->GetNode(tn);
    if (tnode->EqualProperties(node))
    {
      tnode->UnionSelectionList(node);
      merged = true;
      break;
    }
  }
  if (!merged)
  {
    vtkSmartPointer<vtkSelectionNode> clone = vtkSmartPointer<vtkSelectionNode>::New();
    clone->DeepCopy(node);
    this->AddNode(clone);
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Subtract(vtkSelection* s)
{
  for (unsigned int n = 0; n < s->GetNumberOfNodes(); ++n)
  {
    this->Subtract(s->GetNode(n));
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Subtract(vtkSelectionNode* node)
{
  bool subtracted = false;
  for (unsigned int tn = 0; tn < this->GetNumberOfNodes(); ++tn)
  {
    vtkSelectionNode* tnode = this->GetNode(tn);

    if (tnode->EqualProperties(node))
    {
      tnode->SubtractSelectionList(node);
      subtracted = true;
    }
  }
  if (!subtracted)
  {
    vtkErrorMacro("Could not subtract selections");
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSelection::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  const vtkInternals& internals = (*this->Internals);
  for (const auto& apair : internals.Items)
  {
    mtime = std::max(mtime, apair.second->GetMTime());
  }
  return mtime;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformation* info)
{
  return info ? vtkSelection::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformationVector* v, int i)
{
  return vtkSelection::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSignedCharArray> vtkSelection::Evaluate(
  vtkSignedCharArray* const* values, unsigned int num_values) const
{
  std::map<std::string, vtkSignedCharArray*> values_map;

  vtkIdType numVals = -1;
  unsigned int cc = 0;
  const vtkInternals& internals = (*this->Internals);
  for (const auto& apair : internals.Items)
  {
    vtkSignedCharArray* array = cc < num_values ? values[cc] : nullptr;
    if (array == nullptr)
    {
      // lets assume null means false.
    }
    else
    {
      if (array->GetNumberOfComponents() != 1)
      {
        vtkGenericWarningMacro("Only single-component arrays are supported!");
        return nullptr;
      }
      if (numVals != -1 && array->GetNumberOfTuples() != numVals)
      {
        vtkGenericWarningMacro("Mismatched number of tuples.");
        return nullptr;
      }
      numVals = array->GetNumberOfTuples();
    }
    values_map[apair.first] = array;
    cc++;
  }

  std::string expr = this->Expression;
  if (expr.empty())
  {
    bool add_separator = false;
    std::ostringstream stream;
    for (const auto& apair : internals.Items)
    {
      stream << (add_separator ? "|" : "") << apair.first;
      add_separator = true;
    }
    expr = stream.str();
  }

  auto tree = this->Internals->BuildExpressionTree(expr, values_map);
  if (tree && (!values_map.empty()))
  {
    auto result = vtkSmartPointer<vtkSignedCharArray>::New();
    result->SetNumberOfComponents(1);
    result->SetNumberOfTuples(numVals);
    vtkSMPTools::For(0, numVals, [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType idx = start; idx < end; ++idx)
      {
        result->SetTypedComponent(idx, 0, tree->Evaluate(idx));
      }
    });
    return result;
  }
  else if (!tree)
  {
    vtkGenericWarningMacro("Failed to parse expression: " << this->Expression);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSelection::Dump()
{
  this->Dump(cout);
}

//----------------------------------------------------------------------------
void vtkSelection::Dump(ostream& os)
{
  vtkSmartPointer<vtkTable> tmpTable = vtkSmartPointer<vtkTable>::New();
  cerr << "==Selection==" << endl;
  for (unsigned int i = 0; i < this->GetNumberOfNodes(); ++i)
  {
    os << "===Node " << i << "===" << endl;
    vtkSelectionNode* node = this->GetNode(i);
    os << "ContentType: ";
    switch (node->GetContentType())
    {
      case vtkSelectionNode::GLOBALIDS:
        os << "GLOBALIDS";
        break;
      case vtkSelectionNode::PEDIGREEIDS:
        os << "PEDIGREEIDS";
        break;
      case vtkSelectionNode::VALUES:
        os << "VALUES";
        break;
      case vtkSelectionNode::INDICES:
        os << "INDICES";
        break;
      case vtkSelectionNode::FRUSTUM:
        os << "FRUSTUM";
        break;
      case vtkSelectionNode::LOCATIONS:
        os << "LOCATIONS";
        break;
      case vtkSelectionNode::THRESHOLDS:
        os << "THRESHOLDS";
        break;
      case vtkSelectionNode::BLOCKS:
        os << "BLOCKS";
        break;
      case vtkSelectionNode::USER:
        os << "USER";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    os << endl;
    os << "FieldType: ";
    switch (node->GetFieldType())
    {
      case vtkSelectionNode::CELL:
        os << "CELL";
        break;
      case vtkSelectionNode::POINT:
        os << "POINT";
        break;
      case vtkSelectionNode::FIELD:
        os << "FIELD";
        break;
      case vtkSelectionNode::VERTEX:
        os << "VERTEX";
        break;
      case vtkSelectionNode::EDGE:
        os << "EDGE";
        break;
      case vtkSelectionNode::ROW:
        os << "ROW";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    os << endl;
    if (node->GetSelectionData())
    {
      tmpTable->SetRowData(node->GetSelectionData());
      tmpTable->Dump(10);
    }
  }
}
