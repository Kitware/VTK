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
 * @class   vtkExtractSelectedBlock
 *
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
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkExtractSelectedBlock();
  ~vtkExtractSelectedBlock();

  // Generate the output.
  virtual int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  /**
   * Sets up empty output dataset
   */
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
private:
  vtkExtractSelectedBlock(const vtkExtractSelectedBlock&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractSelectedBlock&) VTK_DELETE_FUNCTION;

};

#endif


