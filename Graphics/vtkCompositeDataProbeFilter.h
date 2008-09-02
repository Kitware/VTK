/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataProbeFilter - subclass of vtkProbeFilter which supports
// composite datasets in the input.
// .SECTION Description
// vtkCompositeDataProbeFilter supports probing into multi-group datasets.
// It sequentially probes through each concrete dataset within the composite 
// probing at only those locations at which there were no hits when probing
// earlier datasets. For Hierarchical datasets, this traversal through leaf
// datasets is done in reverse order of levels i.e. highest level first.
#ifndef __vtkCompositeDataProbeFilter_h
#define __vtkCompositeDataProbeFilter_h

#include "vtkProbeFilter.h"

class vtkCompositeDataSet;
class VTK_GRAPHICS_EXPORT vtkCompositeDataProbeFilter : public vtkProbeFilter
{
public:
  static vtkCompositeDataProbeFilter* New();
  vtkTypeRevisionMacro(vtkCompositeDataProbeFilter, vtkProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkCompositeDataProbeFilter();
  ~vtkCompositeDataProbeFilter();

  // Description:
  // Change input information to accept composite datasets as the input which
  // is probed into.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Builds the field list using the composite dataset source.
  int BuildFieldList(vtkCompositeDataSet* source);

  // Description:
  // Handle composite input.
  virtual int RequestData(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

private:
  vtkCompositeDataProbeFilter(const vtkCompositeDataProbeFilter&); // Not implemented.
  void operator=(const vtkCompositeDataProbeFilter&); // Not implemented.
//ETX
};

#endif


