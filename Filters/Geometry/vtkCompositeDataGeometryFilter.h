/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataGeometryFilter - extract geometry from multi-group data
// .SECTION Description
// vtkCompositeDataGeometryFilter applies vtkGeometryFilter to all
// leaves in vtkCompositeDataSet. Place this filter at the end of a
// pipeline before a polydata consumer such as a polydata mapper to extract
// geometry from all blocks and append them to one polydata object.

#ifndef __vtkCompositeDataGeometryFilter_h
#define __vtkCompositeDataGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTKFILTERSGEOMETRY_EXPORT vtkCompositeDataGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkCompositeDataGeometryFilter *New();
  vtkTypeMacro(vtkCompositeDataGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkCompositeDataGeometryFilter();
  ~vtkCompositeDataGeometryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  virtual int RequestCompositeData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*);

private:
  vtkCompositeDataGeometryFilter(const vtkCompositeDataGeometryFilter&);  // Not implemented.
  void operator=(const vtkCompositeDataGeometryFilter&);  // Not implemented.
};

#endif


