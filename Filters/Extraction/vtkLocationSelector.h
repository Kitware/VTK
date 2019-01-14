/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocationSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkLocationSelector
 * @brief selects cells containing or points near chosen point locations.
 *
 * vtkLocationSelector is vtkSelector that can select elements
 * containing or near matching elements. It handles vtkSelectionNode::LOCATIONS
 */

#ifndef vtkLocationSelector_h
#define vtkLocationSelector_h

#include "vtkSelector.h"

#include <memory> // unique_ptr

class VTKFILTERSEXTRACTION_EXPORT vtkLocationSelector : public vtkSelector
{
public:
  static vtkLocationSelector* New();
  vtkTypeMacro(vtkLocationSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName) override;
  void Finalize() override;

protected:
  vtkLocationSelector();
  ~vtkLocationSelector() override;

  bool ComputeSelectedElementsForBlock(vtkDataObject* input,
    vtkSignedCharArray* insidednessArray, unsigned int compositeIndex,
    unsigned int amrLevel, unsigned int amrIndex) override;

private:
  vtkLocationSelector(const vtkLocationSelector&) = delete;
  void operator=(const vtkLocationSelector&) = delete;

  class vtkInternals;
  class vtkInternalsForPoints;
  class vtkInternalsForCells;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
