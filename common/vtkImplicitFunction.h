/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.h
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
// .NAME vtkImplicitFunction - abstract interface for implicit functions
// .SECTION Description
// vtkImplicitFunction specifies an abstract interface for implicit 
// functions. Implicit functions are of the form F(x,y,z) = 0. Two primitive 
// operations are required: the ability to evaluate the function, and the 
// function gradient at a given point.
//
// Implicit functions are very powerful. It is possible to represent almost
// any type of geometry with implicit functions, especially if you use 
// boolean combinations implicit functions (see vtkImplicitBoolean).
//
// vtkImplicitFunction provides a mechanism to transform the implicit
// function(s) via a transformation matrix. This capability can be used to 
// translate, orient, or scale implicit functions. For example, a sphere 
// implicit function can be transformed into an oriented ellipse. This is 
// accomplished by using an instance of vtkTransform.

// .SECTION Caveats
// The transformation matrix transforms a point into the space of the implicit
// function (i.e., the model space). Typically we want to transform the 
// implicit model into world coordinates. In this case the inverse of the 
// transformation matrix is required.

// .SECTION See Also
// vtkTransform
// vtkSphere vtkCylinder vtkImplicitBoolean vtkPlane vtkPlanes
// vtkQuadric vtkImplicitVolume
// vtkSampleFunction vtkCutter vtkClipPolyData

#ifndef __vtkImplicitFunction_h
#define __vtkImplicitFunction_h

#include "vtkObject.h"
#include "vtkTransform.h"

class vtkImplicitFunction : public vtkObject
{
public:
  vtkImplicitFunction();
  char *GetClassName() {return "vtkImplicitFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();
  float FunctionValue(float x[3]);
  void FunctionGradient(float x[3], float g[3]);

  // Description:
  // Evaluate function at position x-y-z and return value. Must be implemented
  // by derived class.
  virtual float EvaluateFunction(float x[3]) = 0;

  // Description:
  // Evaluate function gradient at position x-y-z and pass back vector. Must
  // be implemented by derived class.
  virtual void EvaluateGradient(float x[3], float g[3]) = 0;

  // Description:
  // Set/Get transformation matrix to transform implicit function.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

protected:
  vtkTransform *Transform;

};

#endif
