/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGeometryFilter - Geometry filter that does outlines for volumes.
// .SECTION Description
// This filter defaults to using the outline filter unless the input
// is a structured volume.

#ifndef __vtkPVGeometryFilter_h
#define __vtkPVGeometryFilter_h

#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"

class VTK_PARALLEL_EXPORT vtkPVGeometryFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkPVGeometryFilter *New();
  vtkTypeRevisionMacro(vtkPVGeometryFilter,vtkDataSetSurfaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This flag is set during the execute method.  It indicates
  // that the input was 3d and an outline representation was used.
  vtkGetMacro(OutlineFlag, int);

protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();

  void Execute();
  void ImageDataExecute(vtkImageData *input);
  void StructuredGridExecute(vtkStructuredGrid *input);
  void RectilinearGridExecute(vtkRectilinearGrid *input);
  void UnstructuredGridExecute(vtkUnstructuredGrid *input);

  int OutlineFlag;

private:
  vtkPVGeometryFilter(const vtkPVGeometryFilter&); // Not implemented
  void operator=(const vtkPVGeometryFilter&); // Not implemented
};

#endif


