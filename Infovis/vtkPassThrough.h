/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThrough.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPassThrough - Shallow copies the input into the output
//
// .SECTION Description
// The output type is always the same as the input object type.

#ifndef __vtkPassThrough_h
#define __vtkPassThrough_h

#include "vtkPassInputTypeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkPassThrough : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPassThrough* New();
  vtkTypeRevisionMacro(vtkPassThrough, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPassThrough();
  ~vtkPassThrough();

  virtual int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkPassThrough(const vtkPassThrough&); // Not implemented
  void operator=(const vtkPassThrough&);   // Not implemented
};

#endif

