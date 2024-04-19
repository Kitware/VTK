// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractSelection
 * @brief   extract a subset from a vtkDataSet.
 *
 * vtkExtractSelection extracts some subset of cells and points from
 * its input data object. The data object is given on its first input port.
 * The subset is described by the contents of the vtkSelection on its
 * second input port.  Depending on the contents of the vtkSelection
 * this will create various vtkSelectors to identify the
 * selected elements.
 *
 * This filter supports vtkCompositeDataSet, vtkDataSet, vtkHyperTreeGrid and vtkTable.
 *
 * 1. If preserve topology is on, the output type is the same as the input.
 * 2. If preserve topology is on.
 *    1. If input is a subclass of vtkDataObjectTree, the output is the same subclass.
 *    2. If input is vtkUniformGridAMR, the output is vtkPartitionedDataSetCollection.
 *    3. If input is vtkDataSet, the output is vtkUnstructuredGrid.
 *    4. If input is vtkHyperTreeGrid, the output is vtkHyperTreeGrid or vtkUnstructuredGrid
 *       depending on the HyperTreeGridToUnstructuredGrid flag.
 *    5. If input is vtkTable, the output is vtkTable.
 *
 * Other types of input are not processed and the corresponding output is a
 * default constructed object of the input type.
 *
 * @sa
 * vtkSelection vtkSelector vtkSelectionNode
 */

#ifndef vtkExtractSelection_h
#define vtkExtractSelection_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro

#include "vtkSelectionNode.h" // for vtkSelectionNode::SelectionContent
#include "vtkSmartPointer.h"  // for smart pointer

VTK_ABI_NAMESPACE_BEGIN
class vtkUnsignedCharArray;
class vtkSignedCharArray;
class vtkSelection;
class vtkSelectionNode;
class vtkSelector;
class vtkUnstructuredGrid;
class vtkTable;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelection : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractSelection* New();
  vtkTypeMacro(vtkExtractSelection, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  ///@{
  /**
   * This flag tells the extraction filter not to extract a subset of the
   * data, but instead to produce a vtkInsidedness array and add it to the
   * input dataset. Default value is false(0).
   */
  vtkSetMacro(PreserveTopology, bool);
  vtkGetMacro(PreserveTopology, bool);
  vtkBooleanMacro(PreserveTopology, bool);
  ///@}

  ///@{
  /**
   * Set/Get a flag controlling whether to output an Unstructured Grid (true) or an HyperTreeGrid
   * (false) when input is a HyperTreeGrid
   *
   * Default is set to false
   */
  vtkGetMacro(HyperTreeGridToUnstructuredGrid, bool);
  vtkSetMacro(HyperTreeGridToUnstructuredGrid, bool);
  vtkBooleanMacro(HyperTreeGridToUnstructuredGrid, bool);
  ///@}

protected:
  vtkExtractSelection();
  ~vtkExtractSelection() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  /**
   * Sets up empty output dataset
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Gets the attribute association of the selection.  Currently we support ROW, POINT, and CELL.
  // If the selection types are mismatched the boolean parameter will be set to false, otherwise
  // it will be true after the function returns.
  vtkDataObject::AttributeTypes GetAttributeTypeOfSelection(vtkSelection* sel, bool& sane);

  /**
   * Creates a new vtkSelector for the given content type.
   * May return null if not supported.
   */
  virtual vtkSmartPointer<vtkSelector> NewSelectionOperator(
    vtkSelectionNode::SelectionContent type);

  enum class EvaluationResult
  {
    INVALID,
    NONE,
    MIXED,
    ALL
  };
  /**
   * Evaluates the selection for the given content type for a data object and returns
   * the evaluation result.
   */
  EvaluationResult EvaluateSelection(vtkDataObject* dataObject,
    vtkDataObject::AttributeTypes association, vtkSelection* selection,
    std::map<std::string, vtkSmartPointer<vtkSelector>>& selectors);

  /**
   * Initialize and populate outputColorArray as cell array depending on vtkSelectionData available
   * in the selection. dataObject and association are used to find the vtkInsidedness array, it's
   * used to know if a point or cell is inside the selection.
   *
   * @note When several selections select the same point/cell, the color chosen will be that of the
   * last selection.
   */
  vtkSmartPointer<vtkUnsignedCharArray> EvaluateColorArrayInSelection(
    vtkDataObject* dataObject, vtkDataObject::AttributeTypes association, vtkSelection* selection);

  /**
   * Add colorArray has cell array on the dataObject.
   */
  void AddColorArrayOnObject(vtkDataObject* dataObject, vtkUnsignedCharArray* colorArray);

  /**
   * Given a non-composite input data object (either a block of a larger composite
   * or the whole input), along with the element type being extracted and the
   * computed insidedness array this method either copies the input and adds the
   * insidedness array (if PreserveTopology is on) or returns a new data object
   * containing only the elements to be extracted.
   */
  vtkSmartPointer<vtkDataObject> ExtractElements(vtkDataObject* inputBlock,
    vtkDataObject::AttributeTypes elementType, EvaluationResult evaluationResult,
    vtkDataObject* outputBlock);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Given a vtkDataSet and an array of which cells to extract, this populates
   * the given vtkUnstructuredGrid with the selected cells.
   */
  void ExtractSelectedCells(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkSignedCharArray* cellInside, bool extractAll);
  /**
   * Given a vtkDataSet and an array of which points to extract, the populates
   * the given vtkUnstructuredGrid with the selected points and a cell of type vertex
   * for each point.
   */
  void ExtractSelectedPoints(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkSignedCharArray* pointInside, bool extractAll);
  /**
   * Given an input vtkTable and an array of which rows to extract, this populates
   * the output table with the selected rows.
   */
  void ExtractSelectedRows(
    vtkTable* input, vtkTable* output, vtkSignedCharArray* rowsInside, bool extractAll);

  bool PreserveTopology = false;

private:
  vtkExtractSelection(const vtkExtractSelection&) = delete;
  void operator=(const vtkExtractSelection&) = delete;

  /// Boolean controlling whether to extract HTG input as a UG (true) or a masked HTG (false)
  bool HyperTreeGridToUnstructuredGrid = false;
};

VTK_ABI_NAMESPACE_END
#endif
