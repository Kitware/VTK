/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValueSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkValueSelector
 * @brief selects elements matching chosen values.
 *
 * vtkValueSelector is a vtkSelector that can select elements matching
 * values. This can handle a wide array of vtkSelectionNode::SelectionContent types.
 * These include vtkSelectionNode::GLOBALIDS, vtkSelectionNode::PEDIGREEIDS,
 * vtkSelectionNode::VALUES, vtkSelectionNode::INDICES, and
 * vtkSelectionNode::THRESHOLDS.
 *
 * A few things to note:
 *
 * * vtkSelectionNode::SelectionList must be 2-component array for
 *   content-type = vtkSelectionNode::THRESHOLDS and 1-component array for all
 *   other support content-types. For 1-component selection list, this will
 *   match items where the field array (or index) value matches any value in the
 *   selection list. For 2-component selection list, this will match those items
 *   with values in inclusive-range specified by the two components.
 *
 * * For vtkSelectionNode::VALUES or vtkSelectionNode::THRESHOLDS, the field
 *   array to select on is defined by the name given the SelectionList itself.
 *   If the SelectionList has no name (or is an empty string), then the active
 *   scalars from the dataset will be chosen.
 */

#ifndef vtkValueSelector_h
#define vtkValueSelector_h

#include "vtkSelector.h"

#include <memory> // unique_ptr

class VTKFILTERSEXTRACTION_EXPORT vtkValueSelector : public vtkSelector
{
public:
  static vtkValueSelector* New();
  vtkTypeMacro(vtkValueSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName) override;
  void Finalize() override;

protected:
  vtkValueSelector();
  ~vtkValueSelector() override;

  bool ComputeSelectedElementsForBlock(vtkDataObject* input,
    vtkSignedCharArray* insidednessArray, unsigned int compositeIndex,
    unsigned int amrLevel, unsigned int amrIndex) override;

private:
  vtkValueSelector(const vtkValueSelector&) = delete;
  void operator=(const vtkValueSelector&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
