/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class vtkImplicitWindowFunction : public vtkImplicitFunction
{
public:
  vtkImplicitWindowFunction();
  ~vtkImplicitWindowFunction();
  char *GetClassName() {return "vtkImplicitWindowFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // implicit function interface
  float EvaluateFunction(float x[3]);
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
  // Specify the range of output values that the window range is mapped into. This
  // is effectively a scaling and shifting of the original function values.
  vtkSetVector2Macro(WindowValues,float);
  vtkGetVectorMacro(WindowValues,float,2);

  // Override modified time retrieval because of object dependencies.
  unsigned long int GetMTime();

protected:
  vtkImplicitFunction *ImplicitFunction;
  float WindowRange[2];
  float WindowValues[2];

};

#endif


