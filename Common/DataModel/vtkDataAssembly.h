// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataAssembly
 * @brief hierarchical representation to use with
 * vtkPartitionedDataSetCollection
 *
 * vtkDataAssembly is a mechanism to represent hierarchical organization of
 * items (or vtkPartitionedDataSet instances) in a vtkPartitionedDataSetCollection.
 * vtkPartitionedDataSetCollection is similar to a vtkMultiBlockDataSet since it
 * provides a means for putting together multiple non-composite datasets.
 * However, vtkPartitionedDataSetCollection itself doesn't provide any mechanism
 * to define relationships between items in the collections. That is done using
 * vtkDataAssembly.
 *
 * @section Overview Overview
 *
 * At its core, vtkDataAssembly is simply a tree of nodes starting
 * with the root node. Each node has a unique id and a string name (names need not
 * be unique). On initialization with `vtkDataAssembly::Initialize`, an empty tree
 * with a root node is created. The root node's id and name can be obtained
 * using `vtkDataAssembly::GetRootNode` and `vtkDataAssembly::GetRootNodeName`.
 * The root node's id is fixed (vtkDataAssembly::GetRootNode), however the name
 * can be changed using `vtkDataAssembly::SetRootNodeName`.
 *
 * Child nodes can be added using `vtkDataAssembly::AddNode` or
 * `vtkDataAssembly::AddNodes`, each of which returns the ids for every child
 * node added. A non-root node can be removed using `vtkDataAssembly::RemoveNode`.
 *
 * Each node in the tree (including the root node) can have associated dataset
 * indices. For a vtkDataAssembly associated with a
 * vtkPartitionedDataSetCollection, these indices refer to the item index, or
 * partitioned-dataset-index for items in the collection. Dataset indices can be
 * specified using `vtkDataAssembly::AddDataSetIndex`,
 * `vtkDataAssembly::AddDataSetIndices` and removed using `vtkDataAssembly::RemoveDataSetIndex`,
 * `vtkDataAssembly::RemoveAllDataSetIndices`.
 * `vtkDataAssembly::GetDataSetIndices` provides a mechanism to get the
 * database indices associated with a node, and optionally, the entire subtree
 * rooted at the chosen node.
 *
 * @section Searching Searching
 *
 * Each node in the vtkDataAssembly is assigned a unique id.
 * `vtkDataAssembly::FindFirstNodeWithName` and
 * `vtkDataAssembly::FindNodesWithName` can be used to get the id(s) for
 * node(s) with given name.
 *
 * `vtkDataAssembly::SelectNodes` provides a
 * more flexible mechanism to find nodes using name-based queries. Section
 * @ref DataAssemblyPathQueries covers supported queries.
 *
 * @section Traversal Traversal
 *
 * `vtkDataAssemblyVisitor` defines a visitor API. An instance of a concretized
 * `vtkDataAssemblyVisitor` subclass can be passed to `vtkDataAssembly::Visit`
 * to traverse the data-assembly hierarchy either in depth-first or
 * breadth-first order.
 *
 * @section DataAssemblyPathQueries Supported Path Queries
 *
 * `vtkDataAssembly::SelectNodes` can be used find nodes that match the
 * specified query (or queries) using XPath 1.0 syntax.
 *
 * For example:
 *
 * * '/' is used as the path separator. If a node name has a `/` it must be
 * escaped using `\\` in the query. Note, escaping is not necessary when using
 * `SetNodeName`/`GetNodeName`.
 *
 * * '/' selects the root node.
 *
 * * '/nodename' selects all child nodes of the root with the name 'nodename'.
 *
 * * '//nodename' selects all nodes with 'nodename' that are descendants of the
 *   root; thus, this this will traverse the entire tree.
 *
 * * '/nodename/' selects all child nodes of all nodes named 'nodename' under
 *   the root; thus, ending a query with '/' selects the children of the found
 *   nodes rather than the nodes themselves.
 *
 * * '/nodename1/nodename2' selects all nodes named 'nodename2' which are
 *   children of nodes with name 'nodename1' that are themselves children of
 *   the root node.
 *
 * * '//nodename1/nodename2' finds all nodes in the tree named 'nodename1' and
 *   then selects all children of these found nodes that are named 'nodename2'.
 *
 * @section Applications Applications
 *
 * The separation of dataset storage (vtkPartitionedDataSetCollection) and
 * organization (vtkDataAssembly) enables development of algorithms that can
 * expose APIs that are not tightly coupled to dataset storage. Together,
 * vtkPartitionedDataSetCollection and vtkDataAssembly can be thought of as a
 * different way of organizing data that was previously organized as a
 * vtkMultiBlockDataSet. The advantage of the this newer approach is that
 * filters can support specifying parameters using paths or path queries
 * rather than composite indices. The composite indices suffered from the fact
 * that they made no sense except for the specific vtkMultiBlockDataSet they
 * were applied too. Thus, if the filters input was changed, the composite ids
 * rarely made any sense and needed to be updated. Paths and path queries,
 * however, do not suffer from this issue.
 *
 */

#ifndef vtkDataAssembly_h
#define vtkDataAssembly_h

#include "vtkCommonDataModelModule.h" // for export macros
#include "vtkObject.h"

#include <map>    // for std::map
#include <memory> // for std::unique_ptr
#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssemblyVisitor;

class VTKCOMMONDATAMODEL_EXPORT vtkDataAssembly : public vtkObject
{
public:
  static vtkDataAssembly* New();
  vtkTypeMacro(vtkDataAssembly, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the data-assembly. When a new vtkDataAssembly instance is
   * created, it is in initialized form and it is not required to call this
   * method to initialize it.
   */
  void Initialize();

  ///@{
  /**
   * Initializes a data-assembly using an XML representation of the assembly.
   * Returns true if the initialization was successful, otherwise the assembly
   * is set a clean state and returns false.
   */
  bool InitializeFromXML(const char* xmlcontents);
  ///@}

  ///@{
  /**
   * Saves the data-assembly as a XML.
   */
  std::string SerializeToXML(vtkIndent indent) const;
  ///@}

  /**
   * Returns the ID for the root node.
   * This always returns 0.
   */
  static int GetRootNode() { return 0; }

  ///@{
  /**
   * Get/Set root node name. Defaults to DataAssembly.
   */
  void SetRootNodeName(const char* name)
  {
    this->SetNodeName(vtkDataAssembly::GetRootNode(), name);
  }
  const char* GetRootNodeName() const { return this->GetNodeName(vtkDataAssembly::GetRootNode()); }
  ///@}

  /**
   * Adds a node to the assembly with the given name and returns its id. `parent` is the id for
   * the parent node which defaults to the root node id (i.e. `GetRootNode`).
   *
   * If `parent` is invalid, the add will fail.
   *
   * @returns id of the newly added node on success, else -1.
   */
  int AddNode(const char* name, int parent = 0);

  /**
   * Same as `AddNode` except allows adding multiple nodes in one go.
   *
   * If `parent` is invalid, the add will fail.
   *
   * @returns vectors of corresponding indices for each of the `names` added.
   */
  std::vector<int> AddNodes(const std::vector<std::string>& names, int parent = 0);

  /**
   * Add a subtree by copy the nodes from another tree starting with the
   * specified parent index.
   */
  int AddSubtree(int parent, vtkDataAssembly* other, int otherParent = 0);

  /**
   * Removes a node from the assembly. The node identified by the id and all its
   * children are removed.
   *
   * Root node cannot be removed.
   *
   * @returns true if removal was successful, false if the id is invalid or root
   * node id.
   */
  bool RemoveNode(int id);

  ///@{
  /**
   * Get/Set a node's name. If node id is invalid, `SetNodeName` will raise an
   * error; `GetNodeName` will also raise an error and return nullptr.
   *
   * `SetNodeName` will raise an error if the name is not valid. Name cannot be
   * empty or nullptr.
   */
  void SetNodeName(int id, const char* name);
  const char* GetNodeName(int id) const;
  ///@}

  /**
   * Returns the path for a node.
   */
  std::string GetNodePath(int id) const;

  /**
   * Return a node id given the path. Returns `-1` if path is not valid.
   */
  int GetFirstNodeByPath(const char* path) const;

  /**
   * Add a dataset index to a node. The node id can refer to any
   * valid node in the assembly, including the root.
   *
   * While the same dataset can be added multiple times in the assembly,
   * it cannot be added multiple times to the same node. Additional adds
   * will fail.
   *
   * @returns true if addition succeeded else false.
   */
  bool AddDataSetIndex(int id, unsigned int dataset_index);

  /**
   * Same as `AddDataSetIndex` except supports adding multiple dataset indices
   * in one go. Note, a dataset index only gets added once.
   *
   * @returns true if any dataset index was successfully added.
   */
  bool AddDataSetIndices(int id, const std::vector<unsigned int>& dataset_indices);

  /**
   * Same as `AddDataSetIndices` except this supports adding a contiguous range of dataset
   * indices in one go.
   *
   * @ returns true if any dataset index was successfully added.
   */
  bool AddDataSetIndexRange(int id, unsigned int index_start, int count);

  /**
   * Removes a dataset index from a node.
   *
   * @returns true if the removal was successful, else returns false.
   */
  bool RemoveDataSetIndex(int id, unsigned int dataset_index);

  /**
   * Clears all dataset indices from the node.
   *
   * If `traverse_subtree` is true (default), recursively removes all
   * dataset indices from all the child nodes.
   *
   * @returns true on success, else returns false.
   */
  bool RemoveAllDataSetIndices(int id, bool traverse_subtree = true);

  enum TraversalOrder
  {
    DepthFirst = 0,
    BreadthFirst
  };

  /**
   * Finds first node that is encountered in a breadth first traversal
   * of the assembly with the given name.
   *
   * @returns the if of the node if found, else -1.
   */
  int FindFirstNodeWithName(
    const char* name, int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;

  /**
   * Finds all nodes with the given name. The nodes can be ordered
   * depth first or breadth first, based on the `sort_order` flag.
   */
  std::vector<int> FindNodesWithName(
    const char* name, int sort_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;

  /**
   * Returns ids for all child nodes.
   *
   * If `traverse_subtree` is true (default), recursively builds the child node
   * list. The traversal order can be specified using `traversal_order` flag;
   * defaults to depth-first.
   *
   * @sa vtkDataAssembly::Visit, vtkDataAssemblyVisitor
   *
   */
  std::vector<int> GetChildNodes(int parent, bool traverse_subtree = true,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;

  /**
   * Returns the number of child nodes.
   *
   * @returns the number of child nodes for the parent node or 0 if the parent
   * is invalid.
   */
  int GetNumberOfChildren(int parent) const;

  /**
   * Returns the id for a child not at the given index, if valid, otherwise -1.
   */
  int GetChild(int parent, int index) const;

  /**
   * Returns the index for a child under a given. -1 if invalid.
   */
  int GetChildIndex(int parent, int child) const;

  /**
   * Returns the id for the parent node, if any.
   * Returns -1 if the node is invalid or has no parent (i.e. is the root node).
   */
  int GetParent(int id) const;

  /**
   * Returns true if attribute with the given name is present
   * on the chosen node.
   */
  bool HasAttribute(int id, const char* name) const;

  ///@{
  /**
   * Set an attribute. Will replace an existing attribute with the same name if
   * present.
   */
  void SetAttribute(int id, const char* name, const char* value);
  void SetAttribute(int id, const char* name, int value);
  void SetAttribute(int id, const char* name, unsigned int value);
#if VTK_ID_TYPE_IMPL != VTK_INT
  void SetAttribute(int id, const char* name, vtkIdType value);
#endif
  ///@}

  ///@{
  /**
   * Get an attribute value. Returns true if a value was provided else false.
   */
  bool GetAttribute(int id, const char* name, const char*& value) const;
  bool GetAttribute(int id, const char* name, int& value) const;
  bool GetAttribute(int id, const char* name, unsigned int& value) const;
#if VTK_ID_TYPE_IMPL != VTK_INT
  bool GetAttribute(int id, const char* name, vtkIdType& value) const;
#endif
  ///@}

  ///@{
  /**
   * Get an attribute value. Returns the value associated with the node or the
   * provided default value.
   */
  const char* GetAttributeOrDefault(int id, const char* name, const char* default_value) const;
  int GetAttributeOrDefault(int id, const char* name, int default_value) const;
  unsigned int GetAttributeOrDefault(int id, const char* name, unsigned int default_value) const;
#if VTK_ID_TYPE_IMPL != VTK_INT
  vtkIdType GetAttributeOrDefault(int id, const char* name, vtkIdType default_value) const;
#endif
  ///@}

  ///@{
  /**
   * Visit each node in the assembly for processing. The traversal order can be
   * specified using `traversal_order` which defaults to depth-first.
   */
  void Visit(vtkDataAssemblyVisitor* visitor,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const
  {
    this->Visit(0, visitor, traversal_order);
  }
  void Visit(int id, vtkDataAssemblyVisitor* visitor,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;
  ///@}

  ///@{
  /**
   * Returns the dataset indices associated with the node.
   *
   * If `traverse_subtree` is true (default), recursively builds the dataset
   * indices list for the node and all its child nodes.
   * Note, a dataset index will only appear once in the output even if it is
   * encountered on multiple nodes in the subtree.
   *
   * When `traverse_subtree` is true, the traversal order can be specified
   * using `traversal_order`. Defaults to depth-first.
   *
   */
  std::vector<unsigned int> GetDataSetIndices(int id, bool traverse_subtree = true,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;
  std::vector<unsigned int> GetDataSetIndices(const std::vector<int>& ids,
    bool traverse_subtree = true,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;
  ///@}

  /**
   * Returns ids for nodes matching the path_queries. See Section
   * @ref DataAssemblyPathQueries for supported query expressions.
   *
   * Will return an empty vector is no nodes match the requested query.
   *
   * @returns node ids matching the query in traversal order chosen using
   *          `traversal_order`.
   */
  std::vector<int> SelectNodes(const std::vector<std::string>& path_queries,
    int traversal_order = vtkDataAssembly::TraversalOrder::DepthFirst) const;

  /**
   * Remap dataset indices. `mapping` is map where the key is the old index and
   * value is the new index. If `remove_unmapped` is true, then any dataset not
   * in the map will be removed.
   */
  bool RemapDataSetIndices(
    const std::map<unsigned int, unsigned int>& mapping, bool remove_unmapped);

  /**
   * Create a deep-copy of other by only passing the chosen branches. All other
   * branches of the tree will be pruned. Note this method never affects the
   * depth of the selected branches or dataset indices attached to any of the nodes
   * in pruned output.
   */
  void SubsetCopy(vtkDataAssembly* other, const std::vector<int>& selected_branches);

  /**
   * Deep copy the `other`.
   */
  void DeepCopy(vtkDataAssembly* other);

  /**
   * Validates a node name.
   */
  static bool IsNodeNameValid(const char* name);

  /**
   * Converts any string to a string that is a valid node name.
   * This is done by simply discarding any non-supported character.
   * Additionally, if the first character is not a "_" or an alphabet, then
   * the "_" is prepended.
   */
  static std::string MakeValidNodeName(const char* name);

  /**
   * Returns true for node names that are reserved.
   */
  static bool IsNodeNameReserved(const char* name);

protected:
  vtkDataAssembly();
  ~vtkDataAssembly() override;

private:
  vtkDataAssembly(const vtkDataAssembly&) = delete;
  void operator=(const vtkDataAssembly&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
