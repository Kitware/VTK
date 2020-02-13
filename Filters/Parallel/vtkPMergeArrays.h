/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMergeArrays.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPMergeArrays
 * @brief   Multiple inputs with one output, parallel version
 *
 * Like it's super class, this filter tries to combine all arrays from
 * inputs into one output.
 *
 * @sa
 * vtkMergeArrays
 */

#ifndef vtkPMergeArrays_h
#define vtkPMergeArrays_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMergeArrays.h"

class VTKFILTERSPARALLEL_EXPORT vtkPMergeArrays : public vtkMergeArrays
{
public:
  vtkTypeMacro(vtkPMergeArrays, vtkMergeArrays);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPMergeArrays* New();

protected:
  vtkPMergeArrays();
  ~vtkPMergeArrays() override {}

  int MergeDataObjectFields(vtkDataObject* input, int idx, vtkDataObject* output) override;

private:
  vtkPMergeArrays(const vtkPMergeArrays&) = delete;
  void operator=(const vtkPMergeArrays&) = delete;
};

#endif
