/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataGeometryFilter - extract geometry from multi-group data
// .SECTION Description
// vtkMultiGroupDataGeometryFilter applies vtkGeometryFilter to all
// groups in vtkMultiGroupData. Place this filter at the end of a
// pipeline before a polydata consumer such as a polydata mapper to extract
// geometry from all blocks and append them to one polydata object.

#ifndef __vtkMultiGroupDataGeometryFilter_h
#define __vtkMultiGroupDataGeometryFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkMultiGroupDataGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkMultiGroupDataGeometryFilter *New();
  vtkTypeRevisionMacro(vtkMultiGroupDataGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

protected:
  vtkMultiGroupDataGeometryFilter();
  ~vtkMultiGroupDataGeometryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int RequestCompositeData(vtkInformation*, 
                                   vtkInformationVector**, 
                                   vtkInformationVector*);

private:
  vtkMultiGroupDataGeometryFilter(const vtkMultiGroupDataGeometryFilter&);  // Not implemented.
  void operator=(const vtkMultiGroupDataGeometryFilter&);  // Not implemented.
};

#endif


