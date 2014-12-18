/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericOutlineFilter - create wireframe outline for arbitrary
// generic data set
//
// .SECTION Description
// vtkGenericOutlineFilter is a filter that generates a wireframe outline of
// any generic data set. The outline consists of the twelve edges of the
// generic dataset bounding box.
//
// .SECTION See Also
// vtkGenericDataSet

#ifndef vtkGenericOutlineFilter_h
#define vtkGenericOutlineFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkOutlineSource;

class VTKFILTERSGENERIC_EXPORT vtkGenericOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGenericOutlineFilter *New();
  vtkTypeMacro(vtkGenericOutlineFilter,vtkPolyDataAlgorithm);

protected:
  vtkGenericOutlineFilter();
  ~vtkGenericOutlineFilter();

  vtkOutlineSource *OutlineSource;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int FillInputPortInformation(int, vtkInformation*);

private:
  vtkGenericOutlineFilter(const vtkGenericOutlineFilter&);  // Not implemented.
  void operator=(const vtkGenericOutlineFilter&);  // Not implemented.
};

#endif
