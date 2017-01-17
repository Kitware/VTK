/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPOutlineFilter
 * @brief   create wireframe outline for arbitrary data set
 *
 * vtkPOutlineFilter works like vtkOutlineFilter, but it looks for data
 * partitions in other processes.  It assumes the filter is operated
 * in a data parallel pipeline.
*/

#ifndef vtkPOutlineFilter_h
#define vtkPOutlineFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
class vtkOutlineSource;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkPOutlineFilter *New();
  vtkTypeMacro(vtkPOutlineFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPOutlineFilter();
  ~vtkPOutlineFilter() VTK_OVERRIDE;

  vtkMultiProcessController* Controller;
  vtkOutlineSource *OutlineSource;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkPOutlineFilter(const vtkPOutlineFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPOutlineFilter&) VTK_DELETE_FUNCTION;
};
#endif
