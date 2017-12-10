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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the first input port as optional
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  //@{
  /**
   * Whether or not to deep copy the input. This can be useful if you
   * want to create a copy of a data object. You can then disconnect
   * this filter's input connections and it will act like a source.
   * Defaults to OFF.
   */
  vtkSetMacro(DeepCopyInput, vtkTypeBool);
  vtkGetMacro(DeepCopyInput, vtkTypeBool);
  vtkBooleanMacro(DeepCopyInput, vtkTypeBool);
  //@}

  /**
   * Allow the filter to execute without error when no input connection is
   * specified. In this case, and empty vtkPolyData dataset will be created.
   * By default, this setting is false.
   * @{
   */
  vtkSetMacro(AllowNullInput, bool)
  vtkGetMacro(AllowNullInput, bool)
  vtkBooleanMacro(AllowNullInput, bool)
  /**@}*/

protected:
  vtkPassThrough();
  ~vtkPassThrough() override;

  int RequestDataObject(
      vtkInformation *request,
      vtkInformationVector **inVec,
      vtkInformationVector *outVec) override;
  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  vtkTypeBool DeepCopyInput;
  bool AllowNullInput;

private:
  vtkPassThrough(const vtkPassThrough&) = delete;
  void operator=(const vtkPassThrough&) = delete;
};

#endif

