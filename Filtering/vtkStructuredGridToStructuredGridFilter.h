/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToStructuredGridFilter.h
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
// .NAME vtkStructuredGridToStructuredGridFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToStructuredPointsFilter is an abstract filter class 
// whose subclasses take on input a structured grid  and generate a
// structured grid on output.

// .SECTION See Also
// vtkExtractGrid

#ifndef __vtkStructuredGridToStructuredGridFilter_h
#define __vtkStructuredGridToStructuredGridFilter_h

#include "vtkStructuredGridSource.h"

class VTK_FILTERING_EXPORT vtkStructuredGridToStructuredGridFilter : public vtkStructuredGridSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredGridToStructuredGridFilter,vtkStructuredGridSource);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkStructuredGrid *input);
  vtkStructuredGrid *GetInput();

protected:
  vtkStructuredGridToStructuredGridFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkStructuredGridToStructuredGridFilter() {};
private:
  vtkStructuredGridToStructuredGridFilter(const vtkStructuredGridToStructuredGridFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridToStructuredGridFilter&);  // Not implemented.
};

#endif


