/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxToHierarchicalBoxFilter.h
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
// .NAME vtkHierarchicalBoxToHierarchicalBoxFilter - abstract filter class
// .SECTION Description
// vtkHierarchicalBoxToHierarchicalBoxFilter is an abstract filter class
// whose subclasses take as input vtkHierarchicalBoxDataSet and generate
// vtkHierarchicalBoxDataSet data on output.

#ifndef __vtkHierarchicalBoxToHierarchicalBoxFilter_h
#define __vtkHierarchicalBoxToHierarchicalBoxFilter_h

#include "vtkHierarchicalBoxSource.h"
 
class vtkHierarchicalBoxDataSet;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxToHierarchicalBoxFilter : public vtkHierarchicalBoxSource
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalBoxToHierarchicalBoxFilter,
                       vtkHierarchicalBoxSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkHierarchicalBoxDataSet *input);
  vtkHierarchicalBoxDataSet *GetInput();
  
protected:
  vtkHierarchicalBoxToHierarchicalBoxFilter(){this->NumberOfRequiredInputs = 1;};
  ~vtkHierarchicalBoxToHierarchicalBoxFilter() {};
  
private:
  vtkHierarchicalBoxToHierarchicalBoxFilter(const vtkHierarchicalBoxToHierarchicalBoxFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxToHierarchicalBoxFilter&);  // Not implemented.
};

#endif


