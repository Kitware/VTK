/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSelection
 * @brief data object that represents a "selection" in VTK.
 *
 * vtkSelection is a data object that represents a selection definition. It is
 * used to define the elements that are selected. The criteria of the selection
 * is defined using one or more vtkSelectionNode instances. Parameters of the
 * vtkSelectionNode define what kind of elements are being selected
 * (vtkSelectionNode::GetFieldType), how the selection criteria is defined
 * (vtkSelectionNode::GetContentType), etc.
 *
 * Filters like vtkExtractSelection, vtkExtractDataArraysOverTime can be used to
 * extract the selected elements from a dataset.
 *
 * @section CombiningSelection Combining Selections
 *
 * When a vtkSelection contains multiple vtkSelectionNode instances, the
 * selection defined is a union of all the elements identified by each of the
 * nodes.
 *
 * Optionally, one can use `vtkSelection::SetExpression` to define a boolean
 * expression to build arbitrarily complex combinations. The expression can be
 * defined using names assigned to the selection nodes when the nodes are added
 * to vtkSelection (either explicitly or automatically).
 *
 * @sa
 * vtkSelectionNode
*/

#ifndef vtkSelection_h
#define vtkSelection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkSmartPointer.h" // for  vtkSmartPointer.

#include <string> // for string.
#include <memory> // for unique_ptr.

class vtkSelectionNode;
class vtkSignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT vtkSelection : public vtkDataObject
{
public:
  vtkTypeMacro(vtkSelection,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSelection* New();

  /**
   * Restore data object to initial state,
   */
  void Initialize() override;

  /**
   * Returns VTK_SELECTION enumeration value.
   */
  int GetDataObjectType() override  {return VTK_SELECTION;}

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

  //@{
  /**
   * Removes a selection node.
   */
  virtual void RemoveNode(unsigned int idx);
  virtual void RemoveNode(const std::string& name);
  virtual void RemoveNode(vtkSelectionNode*);
  //@}

  /**
   * Removes all selection nodes.
   */
  virtual void RemoveAllNodes();

  //@{
  /**
   * Get/Set the expression that defines the boolean expression to combine the
   * selection nodes. Expression consists of node name identifiers, `|` for
   * boolean-or, '&' for boolean and, '!' for boolean not, and parenthesis `(`
   * and `)`. If the expression consists of a node name identifier that is not
   * assigned any `vtkSelectionNode` (using `SetNode`) then it is evaluates to
   * `false`.
   *
   * `SetExpression` does not validate the expression. It will be validated in
   * `Evaluate` call.
   */
  vtkSetMacro(Expression, std::string);
  vtkGetMacro(Expression, std::string);
  //@}

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

  //@{
  /**
   * Dumps the contents of the selection, giving basic information only.
   */
  virtual void Dump();
  virtual void Dump(ostream& os);
  //@}

  //@{
  /**
   * Retrieve a vtkSelection stored inside an invormation object.
   */
  static vtkSelection* GetData(vtkInformation* info);
  static vtkSelection* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * Evaluates the expression for each element in the values. The order
   * matches the order of the selection nodes. If not expression is set or if
   * it's an empty string, then an expression that simply combines all selection
   * nodes in an binary-or is assumed.
   */
  vtkSmartPointer<vtkSignedCharArray> Evaluate(
    vtkSignedCharArray* const* values, unsigned int num_values) const;

  /**
   * Convenience method to pass a map of vtkSignedCharArray ptrs (or
   * vtkSmartPointers).
   */
  template <typename MapType>
  vtkSmartPointer<vtkSignedCharArray> Evaluate(const MapType& values_map) const;

protected:
  vtkSelection();
  ~vtkSelection() override;

  std::string Expression;

private:
  vtkSelection(const vtkSelection&) = delete;
  void operator=(const vtkSelection&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

//----------------------------------------------------------------------------
template <typename MapType>
inline vtkSmartPointer<vtkSignedCharArray> vtkSelection::Evaluate(const MapType& values_map) const
{
  const unsigned int num_nodes = this->GetNumberOfNodes();
  std::unique_ptr<vtkSignedCharArray* []> values(new vtkSignedCharArray*[num_nodes]);
  for (unsigned int cc = 0; cc < num_nodes; ++cc)
  {
    auto iter = values_map.find(this->GetNodeNameAtIndex(cc));
    values[cc] = iter != values_map.end() ? iter->second : nullptr;
  }
  return this->Evaluate(&values[0], num_nodes);
}

#endif
