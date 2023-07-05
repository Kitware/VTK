// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSelector.h
 * @brief   Computes the portion of a dataset which is inside a selection
 *
 * This is an abstract superclass for types of selection operations. Subclasses
 * generally only need to override `ComputeSelectedElements`.
 */

#ifndef vtkSelector_h
#define vtkSelector_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <set> // for std::set

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkDataObject;
class vtkSelectionNode;
class vtkSignedCharArray;
class vtkTable;
class vtkDataObjectTree;
class vtkUniformGridAMR;

class VTKFILTERSEXTRACTION_EXPORT vtkSelector : public vtkObject
{
public:
  vtkTypeMacro(vtkSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets the vtkSelectionNode used by this selection operator and initializes
   * the data structures in the selection operator based on the selection.
   *
   * (for example in the vtkFrustumSelector this creates the vtkPlanes
   * implicit function to represent the frustum).
   *
   * @param node The selection node that determines the behavior of this operator.
   */
  virtual void Initialize(vtkSelectionNode* node);

  /**
   * Does any cleanup of objects created in Initialize
   */
  virtual void Finalize() {}

  /**
   * Given an input and the vtkSelectionNode passed into the Initialize() method, add to the
   * output a vtkSignedChar attribute array indicating whether each element is inside (1)
   * or outside (0) the selection. The attribute (point data or cell data) is determined
   * by the vtkSelection that owns the vtkSelectionNode set in Initialize(). The insidedness
   * array is named with the value of InsidednessArrayName. If input is a vtkCompositeDataSet,
   * the insidedness array is added to each block.
   *
   */
  virtual void Execute(vtkDataObject* input, vtkDataObject* output);

  ///@{
  /**
   * Get/Set the name of the array to use for the insidedness array to add to
   * the output in `Execute` call.
   */
  vtkSetMacro(InsidednessArrayName, std::string);
  vtkGetMacro(InsidednessArrayName, std::string);
  ///@}
protected:
  vtkSelector();
  ~vtkSelector() override;

  // Contains the selection criteria.
  vtkSelectionNode* Node = nullptr;

  // Name of the insidedness array added to the output when the selection criteria is
  // evaluated by this operator.
  std::string InsidednessArrayName;

  /**
   * This method computes whether or not each element in the dataset is inside the selection
   * and populates the given array with 0 (outside the selection) or 1 (inside the selection).
   *
   * The vtkDataObject passed in will never be a `vtkCompositeDataSet` subclass.
   *
   * What type of elements are operated over is determined by the vtkSelectionNode's
   * field association. The insidednessArray passed in should have the correct number of elements
   * for that field type or it will be resized.
   *
   * Returns true for successful completion. The operator should only return false
   * when it cannot operate on the inputs. In which case, it is assumed that the
   * insidednessArray may have been left untouched by this method and the calling code
   * will fill it with 0.
   */
  virtual bool ComputeSelectedElements(
    vtkDataObject* input, vtkSignedCharArray* insidednessArray) = 0;

  enum SelectionMode
  {
    INCLUDE,
    EXCLUDE,
    INHERIT
  };

  /**
   * Returns whether the AMR block is to be processed. Return `INCLUDE` to
   * indicate it must be processed or `EXCLUDE` to indicate it must not be
   * processed. If the selector cannot make an exact determination for the given
   * level, index it should return `INHERIT`. If the selection did not specify
   * which AMR block to extract, then too return `INHERIT`.
   */
  virtual SelectionMode GetAMRBlockSelection(unsigned int level, unsigned int index);

  /**
   * Returns whether the block is to be processed. Return `INCLUDE` to
   * indicate it must be processed or `EXCLUDE` to indicate it must not be
   * processed. If the selector cannot make an exact determination for the given
   * level and index, it should return `INHERIT`. Note, returning `INCLUDE` or
   * `EXCLUDE` has impact on all nodes in the subtree unless any of the node
   * explicitly overrides the block selection mode. isDataObjectTree is true for vtkDataObjectTree
   * and false for vtkUniformGridAMR. When isDataObjectTree == true, we treat compositeIndex == 0
   * differently.
   */
  virtual SelectionMode GetBlockSelection(
    unsigned int compositeIndex, bool isDataObjectTree = true);

  /**
   * Creates an array suitable for storing insideness. The array is named using
   * this->InsidednessArrayName and is sized to exactly `numElems` values.
   */
  vtkSmartPointer<vtkSignedCharArray> CreateInsidednessArray(vtkIdType numElems);

  /**
   * Given a data object and selected points, return an array indicating the
   * insidedness of cells that contain at least one of the selected points.
   */
  vtkSmartPointer<vtkSignedCharArray> ComputeCellsContainingSelectedPoints(
    vtkDataObject* data, vtkSignedCharArray* selectedPoints);

  /**
   * Handle expanding to connected cells or point, if requested. This method is
   * called in `Execute`. Subclass that override `Execute` should ensure they
   * call this method to handle expanding to connected elements, as requested.
   *
   * Note: this method will modify `output`.
   */
  void ExpandToConnectedElements(vtkDataObject* output);

private:
  vtkSelector(const vtkSelector&) = delete;
  void operator=(const vtkSelector&) = delete;

  void ProcessBlock(vtkDataObject* inputBlock, vtkDataObject* outputBlock, bool forceFalse);
  void ProcessAMR(vtkUniformGridAMR* input, vtkCompositeDataSet* output);
  void ProcessDataObjectTree(vtkDataObjectTree* input, vtkDataObjectTree* output,
    SelectionMode inheritedSelectionMode, unsigned int compositeIndex = 0);
  void ProcessSelectors(vtkCompositeDataSet* input);

  std::set<unsigned int> SubsetCompositeIds;
};

VTK_ABI_NAMESPACE_END
#endif
