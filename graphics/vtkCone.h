/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.h
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
// .NAME vtkCone - implicit function for a cone
// .SECTION Description
// vtkCone computes the implicit function and function gradient for a cone.
// vtkCone is a concrete implementation of vtkImplicitFunction. The cone vertex
// is located at the origin with axis of rotation coincident with z-axis. (Use
// the superclass' vtkImplicitFunction transformation matrix if necessary to 
// reposition.) The angle specifies the angle between the axis of rotation 
// and the side of the cone.
// .SECTION Caveats
// The cone is infinite in extent. To truncate the cone use the 
// vtkImplicitBoolean in combination with clipping planes.

#ifndef __vtkCone_h
#define __vtkCone_h

#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkCone : public vtkImplicitFunction
{
public:
  vtkCone();
  vtkCone *New() {return new vtkCone;};
  char *GetClassName() {return "vtkCone";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get the cone angle (expressed in degrees).
  vtkSetClampMacro(Angle,float,0,89.0);
  vtkGetMacro(Angle,float);

protected:
  float Angle;

};

#endif


