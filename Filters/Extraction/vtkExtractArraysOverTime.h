/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractArraysOverTime
 * @brief   extracts a selection over time.
 *
 * @deprecated in VTK 8.2. Replaced by vtkExtractSelectedArraysOverTime. Please
 * use vtkExtractSelectedArraysOverTime instead.
 *
*/
#ifndef vtkExtractArraysOverTime_h
#define vtkExtractArraysOverTime_h

#include "vtkExtractSelectedArraysOverTime.h"

#ifndef VTK_LEGACY_REMOVE
class vtkSelection;
class vtkDataSet;
class vtkTable;
class vtkExtractSelection;
class vtkDataSetAttributes;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractArraysOverTime : public vtkExtractSelectedArraysOverTime
{
public:
  VTK_LEGACY(static vtkExtractArraysOverTime* New());
  vtkTypeMacro(vtkExtractArraysOverTime, vtkExtractSelectedArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExtractArraysOverTime();
  ~vtkExtractArraysOverTime() override;

private:
  vtkExtractArraysOverTime(const vtkExtractArraysOverTime&) = delete;
  void operator=(const vtkExtractArraysOverTime&) = delete;
};

#endif // VTK_LEGACY_REMOVE
#endif
