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

#ifndef __vtkMultiBlockDataSetGeometryFilter_h
#define __vtkMultiBlockDataSetGeometryFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkMultiBlockDataSetGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkMultiBlockDataSetGeometryFilter *New();
  vtkTypeRevisionMacro(vtkMultiBlockDataSetGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

protected:
  vtkMultiBlockDataSetGeometryFilter();
  ~vtkMultiBlockDataSetGeometryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

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


