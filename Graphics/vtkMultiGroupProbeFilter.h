/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupProbeFilter - subclass of vtkProbeFilter which supports
// multigroup datasets in the input.
// .SECTION Description
// vtkMultiGroupProbeFilter supports probing into multi-group datasets.
// It sequentially probes through each concrete dataset within the multigroup
// probing at only those locations at which there were no hits when probing
// earlier datasets. For Hierarchical datasets, this traversal through leaf
// datasets is done in reverse order of levels i.e. highest level first.
#ifndef __vtkMultiGroupProbeFilter_h
#define __vtkMultiGroupProbeFilter_h

#include "vtkProbeFilter.h"

class VTK_GRAPHICS_EXPORT vtkMultiGroupProbeFilter : public vtkProbeFilter
{
public:
  static vtkMultiGroupProbeFilter* New();
  vtkTypeRevisionMacro(vtkMultiGroupProbeFilter, vtkProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkMultiGroupProbeFilter();
  ~vtkMultiGroupProbeFilter();

  // Description:
  // Change input information to accept multigroup datasets as the input which
  // is probed into.
  virtual int FillInputPortInformation(int port, vtkInformation* info);


  // Description:
  // Handle multigroup input.
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);


  // Description:
  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

private:
  vtkMultiGroupProbeFilter(const vtkMultiGroupProbeFilter&); // Not implemented.
  void operator=(const vtkMultiGroupProbeFilter&); // Not implemented.
//ETX
};

#endif


