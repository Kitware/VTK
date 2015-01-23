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
// .NAME vtkPProbeFilter - probe dataset in parallel
// .SECTION Description

#ifndef vtkPProbeFilter_h
#define vtkPProbeFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkCompositeDataProbeFilter.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPProbeFilter : public vtkCompositeDataProbeFilter
{
public:
  vtkTypeMacro(vtkPProbeFilter,vtkCompositeDataProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPProbeFilter *New();

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
protected:
  vtkPProbeFilter();
  ~vtkPProbeFilter();

  enum
    {
    PROBE_COMMUNICATION_TAG=1970
    };

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkMultiProcessController* Controller;

private:
  vtkPProbeFilter(const vtkPProbeFilter&);  // Not implemented.
  void operator=(const vtkPProbeFilter&);  // Not implemented.
//ETX
};

#endif
