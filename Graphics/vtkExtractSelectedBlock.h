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
// .NAME vtkExtractSelectedBlock 
// .SECTION Description

#ifndef __vtkExtractSelectedBlock_h
#define __vtkExtractSelectedBlock_h

#include "vtkExtractSelectionBase.h"

class VTK_GRAPHICS_EXPORT vtkExtractSelectedBlock : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectedBlock* New();
  vtkTypeMacro(vtkExtractSelectedBlock, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkExtractSelectedBlock();
  ~vtkExtractSelectedBlock();

  // Generate the output.
  virtual int RequestData(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Sets up empty output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
private:
  vtkExtractSelectedBlock(const vtkExtractSelectedBlock&); // Not implemented.
  void operator=(const vtkExtractSelectedBlock&); // Not implemented.
//ETX
};

#endif


