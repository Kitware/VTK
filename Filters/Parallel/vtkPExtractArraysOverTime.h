/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtractArraysOverTime
 * @brief   extract point or cell data over time (parallel)
 *
 * @deprecated Deprecated in VTK 8.2. Replaced by
 * vtkPExtractSelectedArraysOverTime.
*/

#ifndef vtkPExtractArraysOverTime_h
#define vtkPExtractArraysOverTime_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPExtractSelectedArraysOverTime.h"

#ifndef VTK_LEGACY_REMOVE
class VTKFILTERSPARALLEL_EXPORT vtkPExtractArraysOverTime : public vtkPExtractSelectedArraysOverTime
{
public:
  VTK_LEGACY(static vtkPExtractArraysOverTime* New());
  vtkTypeMacro(vtkPExtractArraysOverTime, vtkPExtractSelectedArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExtractArraysOverTime();
  ~vtkPExtractArraysOverTime() override;

private:
  vtkPExtractArraysOverTime(const vtkPExtractArraysOverTime&) = delete;
  void operator=(const vtkPExtractArraysOverTime&) = delete;
};

#endif // VTK_LEGACY_REMOVE
#endif
