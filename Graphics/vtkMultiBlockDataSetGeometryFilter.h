/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSetGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockDataSetGeometryFilter - extract geometry from multiblock data
// .SECTION Description
// vtkMultiBlockDataSetGeometryFilter applies vtkGeometryFilter to all
// blocks in vtkMultiBlockDataSet. Place this filter at the end of a
// pipeline before a polydata consumer such as a polydata mapper to extract
// geometry from all blocks and append them to one polydata object.

// .SECTION See Also

#ifndef __vtkMultiBlockDataSetGeometryFilter_h
#define __vtkMultiBlockDataSetGeometryFilter_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkMultiBlockDataSetGeometryFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMultiBlockDataSetGeometryFilter *New();
  vtkTypeRevisionMacro(vtkMultiBlockDataSetGeometryFilter,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the polygonal data output
  vtkPolyData* GetOutput();

protected:
  vtkMultiBlockDataSetGeometryFilter();
  ~vtkMultiBlockDataSetGeometryFilter();

  vtkPolyData* GetOutput(int port);

  virtual int FillOutputPortInformation(int, vtkInformation *);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int RequestCompositeData(vtkInformation*, 
                                   vtkInformationVector**, 
                                   vtkInformationVector*);

private:
  vtkMultiBlockDataSetGeometryFilter(const vtkMultiBlockDataSetGeometryFilter&);  // Not implemented.
  void operator=(const vtkMultiBlockDataSetGeometryFilter&);  // Not implemented.
};

#endif


