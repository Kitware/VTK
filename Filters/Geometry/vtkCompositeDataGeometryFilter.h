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
/**
 * @class   vtkCompositeDataGeometryFilter
 * @brief   extract geometry from multi-group data
 *
 * vtkCompositeDataGeometryFilter applies vtkGeometryFilter to all
 * leaves in vtkCompositeDataSet. Place this filter at the end of a
 * pipeline before a polydata consumer such as a polydata mapper to extract
 * geometry from all blocks and append them to one polydata object.
*/

#ifndef vtkCompositeDataGeometryFilter_h
#define vtkCompositeDataGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTKFILTERSGEOMETRY_EXPORT vtkCompositeDataGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkCompositeDataGeometryFilter *New();
  vtkTypeMacro(vtkCompositeDataGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector) VTK_OVERRIDE;

protected:
  vtkCompositeDataGeometryFilter();
  ~vtkCompositeDataGeometryFilter() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  virtual int RequestCompositeData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*);

private:
  vtkCompositeDataGeometryFilter(const vtkCompositeDataGeometryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataGeometryFilter&) VTK_DELETE_FUNCTION;
};

#endif


