/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionSource.h
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
// .NAME vtkPiecewiseFunctionSource - abstract class whose subclasses generate piecewise functions
// .SECTION Description
// vtkPiecewiseFunctionSource is an abstract class whose subclasses generate
// piecewise functions

#ifndef __vtkPiecewiseFunctionSource_h
#define __vtkPiecewiseFunctionSource_h

#include "vtkSource.h"
#include "vtkPiecewiseFunction.h"

class VTK_FILTERING_EXPORT vtkPiecewiseFunctionSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkPiecewiseFunctionSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkPiecewiseFunction *GetOutput();
  vtkPiecewiseFunction *GetOutput(int idx)
    {return (vtkPiecewiseFunction *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPiecewiseFunction *output);

protected:
  vtkPiecewiseFunctionSource();
  ~vtkPiecewiseFunctionSource() {};
  
private:
  vtkPiecewiseFunctionSource(const vtkPiecewiseFunctionSource&);  // Not implemented.
  void operator=(const vtkPiecewiseFunctionSource&);  // Not implemented.
};

#endif





