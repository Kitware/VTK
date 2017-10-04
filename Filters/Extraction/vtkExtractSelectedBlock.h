/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedBlock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkExtractSelectedBlock
 * @brief Extract-Selection filter to extract blocks.
 *
 * vtkExtractSelectedBlock extracts blocks from a composite dataset on input 0
 * using a vtkSelection on input 1.
 *
 * IDs extracted can refer to leaf nodes or non-leaf nodes. When they refer to
 * non-leaf nodes, the entire subtree is extracted.
 *
 * Note: this filter uses `vtkCompositeDataSet::ShallowCopy`, as a result, datasets at
 * leaf nodes are simply passed through, rather than being shallow-copied
 * themselves.
*/

#ifndef vtkExtractSelectedBlock_h
#define vtkExtractSelectedBlock_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractSelectionBase.h"

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedBlock : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectedBlock* New();
  vtkTypeMacro(vtkExtractSelectedBlock, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExtractSelectedBlock();
  ~vtkExtractSelectedBlock() override;

  // Generate the output.
  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *) override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
private:
  vtkExtractSelectedBlock(const vtkExtractSelectedBlock&) = delete;
  void operator=(const vtkExtractSelectedBlock&) = delete;

};

#endif


