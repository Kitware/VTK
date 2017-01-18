/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPassThroughFilter
 * @brief   Filter which shallow copies it's input to it's output
 *
 * This filter shallow copies it's input to it's output. It is normally
 * used by PVSources with multiple outputs as the VTK filter in the
 * dummy connection objects at each output.
*/

#ifndef vtkPassThroughFilter_h
#define vtkPassThroughFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkFieldData;

class VTKFILTERSPARALLEL_EXPORT vtkPassThroughFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPassThroughFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Create a new vtkPassThroughFilter.
   */
  static vtkPassThroughFilter *New();


protected:

  vtkPassThroughFilter() {}
  ~vtkPassThroughFilter() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkPassThroughFilter(const vtkPassThroughFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPassThroughFilter&) VTK_DELETE_FUNCTION;
};

#endif


