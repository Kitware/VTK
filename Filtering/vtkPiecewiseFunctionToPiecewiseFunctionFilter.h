/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionToPiecewiseFunctionFilter.h
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
// .NAME vtkPiecewiseFunctionToPiecewiseFunctionFilter - abstract filter class
// .SECTION Description
// vtkPiecewiseFunctionToPiecewiseFunctionFilter is an abstract filter class
// whose subclasses take as input a piecewise function and generate a
// piecewise function on output.

#ifndef __vtkPiecewiseFunctionToPiecewiseFunctionFilter_h
#define __vtkPiecewiseFunctionToPiecewiseFunctionFilter_h

#include "vtkPiecewiseFunctionSource.h"
#include "vtkPiecewiseFunction.h"

class VTK_FILTERING_EXPORT vtkPiecewiseFunctionToPiecewiseFunctionFilter : public vtkPiecewiseFunctionSource
{
public:
  vtkTypeRevisionMacro(vtkPiecewiseFunctionToPiecewiseFunctionFilter,vtkPiecewiseFunctionSource);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkPiecewiseFunction *input);
  vtkPiecewiseFunction *GetInput();
  
protected:  
   vtkPiecewiseFunctionToPiecewiseFunctionFilter();
  ~vtkPiecewiseFunctionToPiecewiseFunctionFilter() {};

private:
  vtkPiecewiseFunctionToPiecewiseFunctionFilter(const vtkPiecewiseFunctionToPiecewiseFunctionFilter&);  // Not implemented.
  void operator=(const vtkPiecewiseFunctionToPiecewiseFunctionFilter&);  // Not implemented.
};

#endif


