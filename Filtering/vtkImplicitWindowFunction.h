/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.h
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
// .NAME vtkImplicitWindowFunction - implicit function maps another implicit function to lie within a specified range
// .SECTION Description
// vtkImplicitWindowFunction is used to modify the output of another
// implicit function to lie within a specified "window", or function
// range. This can be used to add "thickness" to cutting or clipping
// functions. 
//
// This class works as follows. First, it evaluates the function value of the 
// user-specified implicit function. Then, based on the window range specified,
// it maps the function value into the window values specified. 
//

// .SECTION See Also
// vtkImplicitFunction

#ifndef __vtkImplicitWindowFunction_h
#define __vtkImplicitWindowFunction_h

#include <math.h>
#include "vtkImplicitFunction.h"

class VTK_FILTERING_EXPORT vtkImplicitWindowFunction : public vtkImplicitFunction
{
public:
  vtkTypeRevisionMacro(vtkImplicitWindowFunction,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with window range (0,1) and window values (0,1).
  static vtkImplicitWindowFunction *New();

  // Description
  // Evaluate window function.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate window function gradient. Just return implicit function gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify an implicit function to operate on.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Specify the range of function values which are considered to lie within
  // the window. WindowRange[0] is assumed to be less than WindowRange[1].
  vtkSetVector2Macro(WindowRange,float);
  vtkGetVectorMacro(WindowRange,float,2);

  // Description:
  // Specify the range of output values that the window range is mapped
  // into. This is effectively a scaling and shifting of the original
  // function values.
  vtkSetVector2Macro(WindowValues,float);
  vtkGetVectorMacro(WindowValues,float,2);

  // Description:
  // Override modified time retrieval because of object dependencies.
  unsigned long GetMTime();

protected:
  vtkImplicitWindowFunction();
  ~vtkImplicitWindowFunction();

  vtkImplicitFunction *ImplicitFunction;
  float WindowRange[2];
  float WindowValues[2];

private:
  vtkImplicitWindowFunction(const vtkImplicitWindowFunction&);  // Not implemented.
  void operator=(const vtkImplicitWindowFunction&);  // Not implemented.
};

#endif


