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

// .SECTION See Also

#ifndef __vtkHierarchicalDataSetGeometryFilter_h
#define __vtkHierarchicalDataSetGeometryFilter_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataSetGeometryFilter : public vtkHierarchicalDataSetAlgorithm
{
public:
  static vtkHierarchicalDataSetGeometryFilter *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataSetGeometryFilter,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the polygonal data output
  vtkPolyData* GetOutput();

protected:
  vtkHierarchicalDataSetGeometryFilter();
  ~vtkHierarchicalDataSetGeometryFilter();

  vtkPolyData* GetOutput(int port);

  virtual int FillOutputPortInformation(int, vtkInformation *);

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


