// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSelectionSource
 * @brief   Generate selection from given set of ids
 *
 * vtkSelectionSource generates a vtkSelection from a set of
 * (piece id, cell id) pairs. It will only generate the selection values
 * that match UPDATE_PIECE_NUMBER (i.e. piece == UPDATE_PIECE_NUMBER).
 * vtkSelectionSource can generate a vtkSelection with one or many
 * vtkSelectionNodes.
 *
 * To generate only one vtkSelectionNode, use the functions which don't pass a nodeId
 * to set/get the node information. To generate more than one vtkSelectionNode, use the
 * SetNumberOfNodes/RemoveNode to manipulate the number of nodes,
 * and use the functions that pass the nodeId to set the node information
 * The different nodes can have different contentType per node but the
 * fieldType/elementType is common across all nodes.
 *
 * To define the relation between the nodes you can use SetExpression. If non-empty,
 * the expression is a boolean expression that defines how the selection nodes present
 * in the selection are to be combined together to form the selection. If no expression
 * is specified and there are multiple selection nodes, then the default
 * expression simply combines all the selection nodes using an `or` operator.
 *
 * User-supplied, application-specific selections (with a ContentType of
 * vtkSelectionNode::USER) are not supported.
 */

#ifndef vtkSelectionSource_h
#define vtkSelectionSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"
#include "vtkSelectionNode.h" // For FieldType

#include <memory> // for std::shared_ptr
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkSelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionSource* New();
  vtkTypeMacro(vtkSelectionSource, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the number of nodes that will be created for the generated selection.
   *
   * The default is 1.
   */
  void SetNumberOfNodes(unsigned int numberOfNodes);
  unsigned int GetNumberOfNodes() { return static_cast<unsigned int>(this->NodesInfo.size()); }
  ///@}

  ///@{
  /**
   * Remove a selection node.
   */
  void RemoveNode(unsigned int idx);
  void RemoveNode(const char* name);
  ///@}

  /**
   * Remove all selection nodes.
   */
  virtual void RemoveAllNodes();

  ///@{
  /**
   * Set/Get the expression that defines the boolean expression to combine the
   * selection nodes. Expression consists of node name identifiers, `|` for
   * boolean-or, '^' for boolean-xor, '&' for boolean and, '!' for boolean not,
   * and parenthesis `(` and `)`. If you want to set the expression, be sure to
   * define the node names. If no expression is specified and there are multiple
   * selection nodes, then the default expression simply combines all the selection
   * nodes using an `or` operator.
   *
   * `SetExpression` does not validate the expression. It will be validated in
   * `internally.
   */
  vtkSetStdStringFromCharMacro(Expression);
  vtkGetCharFromStdStringMacro(Expression);
  ///@}

  ///@{
  /**
   * Set/Get FieldTypeOption which is used to specify the selection field type for the selection.
   *
   * If FIELD_TYPE is defined, set FieldType using accepted values as defined in
   * vtkSelectionNode::SelectionField.
   *
   * If ELEMENT_TYPE is defined, set ElementType using accepted values as defined in
   * `vtkDataObject::AttributeTypes`. Note, `vtkDataObject::FIELD` and
   * `vtkDataObject::POINT_THEN_CELL` are not supported. ELEMENT_TYPE will be converted
   * to FIELD_TYPE, internally, since vtkSelectionNode only accepts FIELD_TYPE.
   *
   * The default is FIELD_TYPE.
   */
  enum FieldTypeOptions
  {
    FIELD_TYPE,
    ELEMENT_TYPE
  };
  vtkSetClampMacro(FieldTypeOption, int, FIELD_TYPE, ELEMENT_TYPE);
  virtual void SetFieldTypeOptionToFieldType() { this->SetFieldTypeOption(FIELD_TYPE); }
  virtual void SetFieldTypeOptionToElementType() { this->SetFieldTypeOption(ELEMENT_TYPE); }
  vtkGetMacro(FieldTypeOption, int);
  ///@}

  ///@{
  /**
   * Set/Get the field type for the generated selection.
   * Possible values are as defined by
   * vtkSelectionNode::SelectionField.
   *
   * The default is vtkSelectionNode::SelectionField::CELL.
   */
  vtkSetClampMacro(FieldType, int, vtkSelectionNode::CELL, vtkSelectionNode::ROW);
  vtkGetMacro(FieldType, int);
  ///@}

  ///@{
  /**
   * Set/Get which types of elements are being selected.
   * Accepted values are defined in `vtkDataObject::AttributeTypes`. Note,
   * `vtkDataObject::FIELD` and `vtkDataObject::POINT_THEN_CELL` are not
   * supported.
   *
   * The default is vtkDataObject::AttributeTypes::Cell.
   */
  vtkSetClampMacro(ElementType, int, vtkDataObject::POINT, vtkDataObject::ROW);
  vtkGetMacro(ElementType, int);
  ///@}

  ///@{
  /**
   * Set/Get which process to limit the selection to. `-1` is treated as
   * all processes.
   *
   * The default is -1.
   */
  vtkSetClampMacro(ProcessID, int, -1, VTK_INT_MAX);
  vtkGetMacro(ProcessID, int);
  ///@}

  //------------------------------------------------------------------------------
  // Functions to manipulate the information of each selection node.
  //------------------------------------------------------------------------------

  ///@{
  /**
   * Set/Get the node name.
   *
   * If you want to set the Expression, be sure to define the node names.
   *
   * If the node name is not defined, a default node name is created atomically
   * at each execution of the filter to guarantee uniqueness. GetNodeName()
   * will return a non-empty name only if you have defined it before.
   */
  void SetNodeName(unsigned int nodeId, const char* name);
  void SetNodeName(const char* name) { this->SetNodeName(0, name); }
  const char* GetNodeName(unsigned int nodeId);
  const char* GetNodeName() { return this->GetNodeName(0); }
  ///@}

  ///@{
  /**
   * Add a (piece, id) to the selection set. The source will generate
   * only the ids for which piece == UPDATE_PIECE_NUMBER.
   * If piece == -1, the id applies to all pieces.
   */
  void AddID(unsigned int nodeId, vtkIdType piece, vtkIdType id);
  void AddID(vtkIdType piece, vtkIdType id) { this->AddID(0, piece, id); }
  void AddStringID(unsigned int nodeId, vtkIdType piece, const char* id);
  void AddStringID(vtkIdType piece, const char* id) { this->AddStringID(0, piece, id); }
  ///@}

  ///@{
  /**
   * Add a point in world space to probe at.
   */
  void AddLocation(unsigned int nodeId, double x, double y, double z);
  void AddLocation(double x, double y, double z) { this->AddLocation(0, x, y, z); }
  ///@}

  ///@{
  /**
   * Add a value range to threshold within.
   */
  void AddThreshold(unsigned int nodeId, double min, double max);
  void AddThreshold(double min, double max) { this->AddThreshold(0, min, max); }
  ///@}

  ///@{
  /**
   * Set a frustum to choose within.
   */
  void SetFrustum(unsigned int nodeId, double* vertices);
  void SetFrustum(double* vertices) { this->SetFrustum(0, vertices); }
  ///@}

  ///@{
  /**
   * Add the flat-index/composite index for a block.
   */
  void AddBlock(unsigned int nodeId, vtkIdType block);
  void AddBlock(vtkIdType blockno) { this->AddBlock(0, blockno); }
  ///@}

  ///@{
  /**
   * Add/Remove block-selectors to make selections with
   * vtkSelectionNode::BLOCK_SELECTORS as the content-type.
   */
  void AddBlockSelector(unsigned int nodeId, const char* block);
  void AddBlockSelector(const char* selector) { this->AddBlockSelector(0, selector); }
  void RemoveAllBlockSelectors(unsigned int nodeId);
  void RemoveAllBlockSelectors() { this->RemoveAllBlockSelectors(0); }
  ///@}

  ///@{
  /**
   * Removes all IDs.
   */
  void RemoveAllIDs(unsigned int nodeId);
  void RemoveAllIDs() { this->RemoveAllIDs(0); }
  void RemoveAllStringIDs(unsigned int nodeId);
  void RemoveAllStringIDs() { this->RemoveAllStringIDs(0); }
  ///@}

  ///@{
  /**
   * Remove all thresholds added with AddThreshold.
   */
  void RemoveAllThresholds(unsigned int nodeId);
  void RemoveAllThresholds() { this->RemoveAllThresholds(0); }
  ///@}

  ///@{
  /**
   * Remove all locations added with AddLocation.
   */
  void RemoveAllLocations(unsigned int nodeId);
  void RemoveAllLocations() { this->RemoveAllLocations(0); }
  ///@}

  ///@{
  /**
   * Remove all blocks added with AddBlock.
   */
  void RemoveAllBlocks(unsigned int nodeId);
  void RemoveAllBlocks() { this->RemoveAllBlocks(0); }
  ///@}

  ///@{
  /**
   * Set/Get the content type.
   * Possible values are as defined by
   * vtkSelectionNode::SelectionContent.
   *
   * The default is vtkSelectionNode::SelectionContent::INDICES.
   */
  void SetContentType(unsigned int nodeId, int type);
  void SetContentType(int contentType) { this->SetContentType(0, contentType); }
  int GetContentTypeMinValue() { return vtkSelectionNode::SelectionContent::GLOBALIDS; }
  int GetContentTypeMaxValue() { return vtkSelectionNode::SelectionContent::USER; }
  int GetContentType(unsigned int nodeId);
  int GetContentType() { return this->GetContentType(0); }
  ///@}

  ///@{
  /**
   * When extracting by points, extract the cells that contain the
   * passing points.
   *
   * The default is false.
   */
  void SetContainingCells(unsigned int nodeId, vtkTypeBool containingCells);
  void SetContainingCells(vtkTypeBool containingCells)
  {
    this->SetContainingCells(0, containingCells);
  }
  vtkTypeBool GetContainingCells(unsigned int nodeId);
  vtkTypeBool GetContainingCells() { return this->GetContainingCells(0); }
  ///@}

  ///@{
  /**
   * Set/Get the number of layers to extract connected to the selected elements.
   *
   * The default is 0.
   */
  void SetNumberOfLayers(unsigned int nodeId, int numberOfLayers);
  void SetNumberOfLayers(int numberOfLayers) { this->SetNumberOfLayers(0, numberOfLayers); }
  int GetNumberOfLayersMinValue() { return 0; }
  int GetNumberOfLayersMaxValue() { return VTK_INT_MAX; }
  int GetNumberOfLayers(unsigned int nodeId);
  int GetNumberOfLayers() { return this->GetNumberOfLayers(0); }
  ///@}

  ///@{
  /**
   * Set/Get the number of layers related flag to remove seed selection
   *
   * The default is false.
   */
  void SetRemoveSeed(unsigned int nodeId, bool RemoveSeed);
  void SetRemoveSeed(bool RemoveSeed) { this->SetRemoveSeed(0, RemoveSeed); }
  bool GetRemoveSeed(unsigned int nodeId);
  bool GetRemoveSeed() { return this->GetRemoveSeed(0); }
  ///@}

  ///@{
  /**
   * Set/Get the number of layers related flag to remove intermediate layers
   *
   * The default is false.
   */
  void SetRemoveIntermediateLayers(unsigned int nodeId, bool RemoveIntermediateLayers);
  void SetRemoveIntermediateLayers(bool RemoveIntermediateLayers)
  {
    this->SetRemoveIntermediateLayers(0, RemoveIntermediateLayers);
  }
  bool GetRemoveIntermediateLayers(unsigned int nodeId);
  bool GetRemoveIntermediateLayers() { return this->GetRemoveIntermediateLayers(0); }
  ///@}

  ///@{
  /**
   * Determines whether the selection describes what to include or exclude.
   *
   * The Default is 0, meaning include.
   */
  void SetInverse(unsigned int nodeId, vtkTypeBool inverse);
  void SetInverse(vtkTypeBool inverse) { this->SetInverse(0, inverse); }
  vtkTypeBool GetInverse(unsigned int nodeId);
  vtkTypeBool GetInverse() { return this->GetInverse(0); }
  ///@}

  ///@{
  /**
   * Get/Set the name used for the SelectionList in the generated
   * vtkSelectionNode.
   */
  void SetArrayName(unsigned int nodeId, const char* name);
  void SetArrayName(const char* name) { this->SetArrayName(0, name); }
  const char* GetArrayName(unsigned int nodeId);
  const char* GetArrayName() { return this->GetArrayName(0); }
  ///@}

  ///@{
  /**
   * Set/Get the component number for the array specified by ArrayName.
   *
   * The default is component 0. Use -1 for magnitude.
   */
  void SetArrayComponent(unsigned int nodeId, int component);
  void SetArrayComponent(int component) { this->SetArrayComponent(0, component); }
  int GetArrayComponent(unsigned int nodeId);
  int GetArrayComponent() { return this->GetArrayComponent(0); }
  ///@}

  ///@{
  /**
   * Set/Get the CompositeIndex.
   * If CompositeIndex < 0 then COMPOSITE_INDEX() is not added to the output.
   *
   * The default is -1.
   */
  void SetCompositeIndex(unsigned int nodeId, int index);
  void SetCompositeIndex(int compositeIndex) { this->SetCompositeIndex(0, compositeIndex); }
  int GetCompositeIndex(unsigned int nodeId);
  int GetCompositeIndex() { return this->GetCompositeIndex(0); }
  ///@}

  ///@{
  /**
   * Set/Get the Hierarchical/HierarchicalIndex.
   * If HierarchicalLevel or HierarchicalIndex < 0 , then HIERARCHICAL_LEVEL()
   * and HIERARCHICAL_INDEX() keys are not added to the output.
   *
   * The default for both is -1.
   */
  void SetHierarchicalLevel(unsigned int nodeId, int level);
  void SetHierarchicalLevel(int level) { this->SetHierarchicalLevel(0, level); }
  int GetHierarchicalLevel(unsigned int nodeId);
  int GetHierarchicalLevel() { return this->GetHierarchicalLevel(0); }
  void SetHierarchicalIndex(unsigned int nodeId, int index);
  void SetHierarchicalIndex(int index) { this->SetHierarchicalIndex(0, index); }
  int GetHierarchicalIndex(unsigned int nodeId);
  int GetHierarchicalIndex() { return this->GetHierarchicalIndex(0); }
  ///@}

  ///@{
  /**
   * For selector-based selection qualification. Note, this should not
   * be confused with block-selectors used to select blocks using selectors.
   * These here are qualifiers i.e. they limit the selected items.
   */
  void SetAssemblyName(unsigned int nodeId, const char* name);
  void SetAssemblyName(const char* name) { this->SetAssemblyName(0, name); }
  const char* GetAssemblyName(unsigned int nodeId);
  const char* GetAssemblyName() { return this->GetAssemblyName(0); }
  void AddSelector(unsigned int nodeId, const char* selector);
  void AddSelector(const char* selector) { this->AddSelector(0, selector); }
  void RemoveAllSelectors(unsigned int nodeId);
  void RemoveAllSelectors() { this->RemoveAllSelectors(0); }
  ///@}

  ///@{
  /**
   * Set/Get the query expression string.
   */
  void SetQueryString(unsigned int nodeId, const char* queryString);
  void SetQueryString(const char* query) { this->SetQueryString(0, query); }
  const char* GetQueryString(unsigned int nodeId);
  const char* GetQueryString() { return this->GetQueryString(0); }
  ///@}

protected:
  vtkSelectionSource();
  ~vtkSelectionSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::string Expression;
  int FieldTypeOption;
  int FieldType;
  int ElementType;
  int ProcessID;
  struct NodeInformation;
  std::vector<std::shared_ptr<NodeInformation>> NodesInfo;

private:
  vtkSelectionSource(const vtkSelectionSource&) = delete;
  void operator=(const vtkSelectionSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
