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
/**
 * @class   vtkPassThrough
 * @brief   Shallow copies the input into the output
 *
 *
 * The output type is always the same as the input object type.
*/

#ifndef vtkPassThrough_h
#define vtkPassThrough_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkPassThrough : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPassThrough* New();
  vtkTypeMacro(vtkPassThrough, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Specify the first input port as optional
   */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  //@{
  /**
   * Whether or not to deep copy the input. This can be useful if you
   * want to create a copy of a data object. You can then disconnect
   * this filter's input connections and it will act like a source.
   * Defaults to OFF.
   */
  vtkSetMacro(DeepCopyInput, int);
  vtkGetMacro(DeepCopyInput, int);
  vtkBooleanMacro(DeepCopyInput, int);
  //@}

protected:
  vtkPassThrough();
  ~vtkPassThrough() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  int DeepCopyInput;

private:
  vtkPassThrough(const vtkPassThrough&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPassThrough&) VTK_DELETE_FUNCTION;
};

#endif

