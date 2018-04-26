/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionOperator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectionOperator.h
 * @brief   Computes the portion of a dataset which is inside a selection
 *
 * This is an abstract superclass for types of selection operations.
 */

#ifndef vtkSelectionOperator_h
#define vtkSelectionOperator_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkObject.h"

class vtkDataObject;
class vtkSelectionNode;
class vtkSignedCharArray;
class vtkTable;

class VTKFILTERSEXTRACTION_EXPORT vtkSelectionOperator : public vtkObject
{
  public:
  vtkTypeMacro(vtkSelectionOperator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets the vtkSelectionNode used by this selection operator an initializes
   * the datastructures in the selection operator based on the selection.
   *
   * (for example in the vtkFrustumSelector this creates the vtkPlanes
   * implicit function to represent the frustum)
   */
  virtual void Initialize(vtkSelectionNode* node) = 0;
  /**
   * Does any cleanup of objects created in Initialize
   */
  virtual void Finalize() {}

  /**
   * This method computes whether or not each element in the dataset is inside the selection
   * and populates the given array with 0 (outside the selection) or 1 (inside the selection).
   *
   * The vtkDataObject passed in should be a non-composite data object.
   *
   * What type of elements are operated over is determined by the vtkSelectionNode's
   * field association.  The array passed in should have the correct number of elements
   * for that field type or it will be resized.
   *
   * Returns true for successful completion.  The operator should only return false
   * when it cannot operate on the inputs.
   */
  virtual bool ComputeSelectedElements(vtkDataObject* input, vtkSignedCharArray* elementInside) = 0;

protected:
  vtkSelectionOperator();
  virtual ~vtkSelectionOperator() override;
private:
  vtkSelectionOperator(const vtkSelectionOperator&) = delete;
  void operator=(const vtkSelectionOperator&) = delete;
};

#endif
