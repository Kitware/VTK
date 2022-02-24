/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkBlockSelector
 * @brief selector for blocks
 *
 * Selects cells or points contained in a block as defined in the
 * vtkSelectionNode used to initialize this operator.
 *
 * This selector supports vtkSelectionNode::BLOCKS and
 * vtkSelectionNode::BLOCK_SELECTORS.
 */

#ifndef vtkBlockSelector_h
#define vtkBlockSelector_h

#include "vtkSelector.h"

class VTKFILTERSEXTRACTION_EXPORT vtkBlockSelector : public vtkSelector
{
public:
  static vtkBlockSelector* New();
  vtkTypeMacro(vtkBlockSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node) override;

  /**
   * Overridden to handle `BLOCK_SELECTORS`. We need the data to convert
   * selector expressions to composite indices for quick check if block is
   * selected. We do that here.
   */
  void Execute(vtkDataObject* input, vtkDataObject* output) override;

protected:
  vtkBlockSelector();
  ~vtkBlockSelector() override;

  bool ComputeSelectedElements(vtkDataObject* input, vtkSignedCharArray* insidednessArray) override;
  SelectionMode GetAMRBlockSelection(unsigned int level, unsigned int index) override;
  SelectionMode GetBlockSelection(
    unsigned int compositeIndex, bool isDataObjectTree = true) override;

private:
  vtkBlockSelector(const vtkBlockSelector&) = delete;
  void operator=(const vtkBlockSelector&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
