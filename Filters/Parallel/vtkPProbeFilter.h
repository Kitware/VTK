/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPProbeFilter
 * @brief   probe dataset in parallel
 *
 * This filter works correctly only if the whole geometry dataset
 * (that specify the point locations used to probe input) is available on all
 * nodes.
*/

#ifndef vtkPProbeFilter_h
#define vtkPProbeFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkCompositeDataProbeFilter.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPProbeFilter : public vtkCompositeDataProbeFilter
{
public:
  vtkTypeMacro(vtkPProbeFilter,vtkCompositeDataProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPProbeFilter *New();

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPProbeFilter();
  ~vtkPProbeFilter() override;

  enum
  {
    PROBE_COMMUNICATION_TAG=1970
  };

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  vtkMultiProcessController* Controller;

private:
  vtkPProbeFilter(const vtkPProbeFilter&) = delete;
  void operator=(const vtkPProbeFilter&) = delete;

};

#endif
