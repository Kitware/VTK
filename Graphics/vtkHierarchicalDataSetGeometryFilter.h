/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSetGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataSetGeometryFilter - extract geometry from hierarchical data
// .SECTION Description
// vtkHierarchicalDataSetGeometryFilter applies vtkGeometryFilter to all
// blocks in vtkHierarchicalDataSet. Place this filter at the end of a
// pipeline before a polydata consumer such as a polydata mapper to extract
// geometry from all blocks and append them to one polydata object.

#ifndef __vtkHierarchicalDataSetGeometryFilter_h
#define __vtkHierarchicalDataSetGeometryFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataSetGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkHierarchicalDataSetGeometryFilter *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataSetGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

protected:
  vtkHierarchicalDataSetGeometryFilter();
  ~vtkHierarchicalDataSetGeometryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int RequestCompositeData(vtkInformation*, 
                                   vtkInformationVector**, 
                                   vtkInformationVector*);

private:
  vtkHierarchicalDataSetGeometryFilter(const vtkHierarchicalDataSetGeometryFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSetGeometryFilter&);  // Not implemented.
};

#endif


