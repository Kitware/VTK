/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelector.h
 * @brief   Computes the portion of a dataset which is inside a selection
 *
 * This is an abstract superclass for types of selection operations.
 */

#ifndef vtkSelector_h
#define vtkSelector_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkCompositeDataSet;
class vtkDataObject;
class vtkSelectionNode;
class vtkSignedCharArray;
class vtkTable;

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
   * @param insidednessArrayName The name of the insidedness array to add to the output
   *        from this operator.
   */
  virtual void Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName);

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
   */
  virtual bool ComputeSelectedElements(vtkDataObject* input, vtkDataObject* output);

protected:
  vtkSelector();
  virtual ~vtkSelector() override;

  // Contains the selection criteria.
  vtkSelectionNode* Node = nullptr;

  // Name of the insidedness array added to the output when the selection criteria is
  // evaluated by this operator.
  std::string InsidednessArrayName;

  /**
   * This method computes whether or not each element in the dataset is inside the selection
   * and populates the given array with 0 (outside the selection) or 1 (inside the selection).
   *
   * The vtkDataObject passed in should be a non-composite data object.
   *
   * What type of elements are operated over is determined by the vtkSelectionNode's
   * field association. The insidednessArray passed in should have the correct number of elements
   * for that field type or it will be resized. The last three parameters give the
   * data object's composite index, AMR level or AMR index.
   *
   * Returns true for successful completion. The operator should only return false
   * when it cannot operate on the inputs.
   *
   */
  virtual bool ComputeSelectedElementsForBlock(vtkDataObject* input,
    vtkSignedCharArray* insidednessArray, unsigned int compositeIndex,
    unsigned int amrLevel, unsigned int amrIndex) = 0;

  /**
   * Computes whether each element in the dataset is inside the selection and populates the
   * given array with 0 (outside the selection) or 1 (inside the selection).
   *
   * This methods operates on an input vtkCompositeDataSet. It stores results in arrays
   * associated with the element data in a copy of the input data object.
   *
   * What type of elements are operated over is determined by the vtkSelectionNode's
   * field association.
   *
   * Returns true after successful completion. The operator should only return false
   * when it cannot operate on the inputs. Selectors that do not apply to
   * vtkCompositeDataSets should do nothing and return true.
   */
  virtual bool ComputeSelectedElementsForCompositeDataSet(
    vtkCompositeDataSet* input, vtkCompositeDataSet* output);

  /**
   * Subclasses can call this to check if the block should be skipped.
   */
  bool SkipBlock(unsigned int compositeIndex, unsigned int amrLevel, unsigned int amrIndex);

  /**
   * Creates an array suitable for storing insideness.
   */
  vtkSmartPointer<vtkSignedCharArray> CreateInsidednessArray(vtkIdType numElems);

  /**
   * Given a data object and selected points, return an array indicating the
   * insidedness of cells that contain at least one of the selected points.*/
  vtkSmartPointer<vtkSignedCharArray> ComputeCellsContainingSelectedPoints(
    vtkDataObject* data, vtkSignedCharArray* selectedPoints);

private:
  vtkSelector(const vtkSelector&) = delete;
  void operator=(const vtkSelector&) = delete;
};

#endif
