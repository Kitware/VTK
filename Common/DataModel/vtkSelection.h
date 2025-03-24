// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSelection
 * @brief data object that represents a "selection" in VTK.
 *
 * vtkSelection defines a selection. A selection is a data-object that defines
 * which entities from another data-object are to treated as "selected". Filters like
 * `vtkExtractSelection` or `vtkExtractDataArraysOverTime` can then be used to
 * extract these selected entities from the *other* data-object.
 *
 * vtkSelection comprises of `vtkSelectionNode`s and optionally, an expression
 * specified using `vtkSelection::SetExpression`. If non-empty, the expression
 * is a boolean expression that defines now the selection nodes present in the
 * selection are to be combined together to form the selection. If no expression
 * is specified and there are multiple selection nodes, then the default
 * expression simply combines all the selection nodes using an `or` operator.
 *
 * Each vtkSelectionNode is used to define the selection criteria.
 * vtkSelectionNode API lets one select what kind of entities are being selected
 * (vtkSelectionNode::FieldType) and how they are being selected
 * (vtkSelectionNode::ContentType).
 *
 * @sa
 * vtkSelectionNode
 */

#ifndef vtkSelection_h
#define vtkSelection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkSmartPointer.h" // for  vtkSmartPointer.

#include <array>  // for array.
#include <string> // for string.
#include <vector> // for vector.

VTK_ABI_NAMESPACE_BEGIN
class vtkSelectionNode;
class vtkSignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT vtkSelection : public vtkDataObject
{
public:
  vtkTypeMacro(vtkSelection, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSelection* New();

  /**
   * Restore data object to initial state,
   */
  void Initialize() override;

  /**
   * Returns VTK_SELECTION enumeration value.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_SELECTION; }

  /**
   * Returns the number of nodes in this selection.
   * Each node contains information about part of the selection.
   */
  unsigned int GetNumberOfNodes() const;

  /**
   * Returns a node given it's index. Performs bound checking
   * and will return nullptr if out-of-bounds.
   */
  virtual vtkSelectionNode* GetNode(unsigned int idx) const;

  /**
   * Returns a node with the given name, if present, else nullptr is returned.
   */
  virtual vtkSelectionNode* GetNode(const std::string& name) const;

  /**
   * Adds a selection node. Assigns the node a unique name and returns that
   * name. This API is primarily provided for backwards compatibility and
   * `SetNode` is the preferred method.
   */
  virtual std::string AddNode(vtkSelectionNode*);

  /**
   * Adds a vtkSelectionNode and assigns it the specified name. The name
   * must be a non-empty string. If an item with the same name
   * has already been added, it will be removed.
   */
  virtual void SetNode(const std::string& name, vtkSelectionNode*);

  /**
   * Returns the name for a node at the given index.
   */
  virtual std::string GetNodeNameAtIndex(unsigned int idx) const;

  ///@{
  /**
   * Removes a selection node.
   */
  virtual void RemoveNode(unsigned int idx);
  virtual void RemoveNode(const std::string& name);
  virtual void RemoveNode(vtkSelectionNode*);
  ///@}

  /**
   * Removes all selection nodes.
   */
  virtual void RemoveAllNodes();

  ///@{
  /**
   * Get/Set the expression that defines the boolean expression to combine the
   * selection nodes. Expression consists of node name identifiers, `|` for
   * boolean-or, '^' for boolean-xor, '&' for boolean and, '!' for boolean not,
   * and parenthesis `(` and `)`. If the expression consists of a node name identifier
   * that is not assigned any `vtkSelectionNode` (using `SetNode`) then it is evaluates
   * to `false`.
   *
   * `SetExpression` does not validate the expression. It will be validated in
   * `Evaluate` call.
   */
  vtkSetMacro(Expression, std::string);
  vtkGetMacro(Expression, std::string);
  ///@}

  /**
   * Copy selection nodes of the input.
   */
  void DeepCopy(vtkDataObject* src) override;

  /**
   * Copy selection nodes of the input.
   * This is a shallow copy: selection lists and pointers in the
   * properties are passed by reference.
   */
  void ShallowCopy(vtkDataObject* src) override;

  /**
   * Union this selection with the specified selection.
   * Attempts to reuse selection nodes in this selection if properties
   * match exactly. Otherwise, creates new selection nodes.
   */
  virtual void Union(vtkSelection* selection);

  /**
   * Union this selection with the specified selection node.
   * Attempts to reuse a selection node in this selection if properties
   * match exactly. Otherwise, creates a new selection node.
   */
  virtual void Union(vtkSelectionNode* node);

  /**
   * Remove the nodes from the specified selection from this selection.
   * Assumes that selection node internal arrays are vtkIdTypeArrays.
   */
  virtual void Subtract(vtkSelection* selection);

  /**
   * Remove the nodes from the specified selection from this selection.
   * Assumes that selection node internal arrays are vtkIdTypeArrays.
   */
  virtual void Subtract(vtkSelectionNode* node);

  /**
   * Return the MTime taking into account changes to the properties
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Dumps the contents of the selection, giving basic information only.
   */
  virtual void Dump();
  virtual void Dump(ostream& os);
  ///@}

  ///@{
  /**
   * Retrieve a vtkSelection stored inside an invormation object.
   */
  static vtkSelection* GetData(vtkInformation* info);
  static vtkSelection* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Evaluates the expression for each element in the values and extracts the range.
   * The order matches the order of the selection nodes. If not expression is set or
   * if it's an empty string, then an expression that simply combines all selection
   * nodes in an binary-or is assumed.
   */
  vtkSmartPointer<vtkSignedCharArray> Evaluate(
    vtkSignedCharArray* const* values, unsigned int num_values) const
  {
    std::array<signed char, 2> range;
    return this->Evaluate(values, num_values, range);
  }
  vtkSmartPointer<vtkSignedCharArray> Evaluate(vtkSignedCharArray* const* values,
    unsigned int num_values, std::array<signed char, 2>& range) const;
  ///@}

  ///@{
  /**
   * Convenience method to pass a map of vtkSignedCharArray ptrs (or
   * vtkSmartPointers) and range.
   */
  template <typename MapType>
  vtkSmartPointer<vtkSignedCharArray> Evaluate(const MapType& values_map) const
  {
    std::array<signed char, 2> range;
    return this->Evaluate(values_map, range);
  }
  template <typename MapType>
  vtkSmartPointer<vtkSignedCharArray> Evaluate(
    const MapType& values_map, std::array<signed char, 2>& range) const;
  ///@}

protected:
  vtkSelection();
  ~vtkSelection() override;

  std::string Expression;

private:
  vtkSelection(const vtkSelection&) = delete;
  void operator=(const vtkSelection&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  struct EvaluateFunctor;
};

//----------------------------------------------------------------------------
template <typename MapType>
inline vtkSmartPointer<vtkSignedCharArray> vtkSelection::Evaluate(
  const MapType& values_map, std::array<signed char, 2>& range) const
{
  const unsigned int num_nodes = this->GetNumberOfNodes();
  std::vector<vtkSignedCharArray*> values(num_nodes, nullptr);
  for (unsigned int cc = 0; cc < num_nodes; ++cc)
  {
    auto iter = values_map.find(this->GetNodeNameAtIndex(cc));
    values[cc] = iter != values_map.end() ? iter->second : nullptr;
  }
  return this->Evaluate(values.data(), num_nodes, range);
}

VTK_ABI_NAMESPACE_END
#endif
