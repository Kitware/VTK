/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataLevelFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataLevelFilter - generate scalars from levels
// .SECTION Description
// vtkHierarchicalDataLevelFilter is a filter to that generates scalars 
// using hiearchical data level information.

#ifndef __vtkHierarchicalDataLevelFilter_h
#define __vtkHierarchicalDataLevelFilter_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataLevelFilter : public vtkHierarchicalDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataLevelFilter,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with PointIds and CellIds on; and ids being generated
  // as scalars.
  static vtkHierarchicalDataLevelFilter *New();

protected:
  vtkHierarchicalDataLevelFilter();
  ~vtkHierarchicalDataLevelFilter();

  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

private:
  vtkHierarchicalDataLevelFilter(const vtkHierarchicalDataLevelFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalDataLevelFilter&);  // Not implemented.
};

#endif


