/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.cxx
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
#include "vtkImplicitFunction.h"

vtkImplicitFunction::vtkImplicitFunction()
{
  this->Transform = NULL;
}

// Description:
// Evaluate function at position x-y-z and return value. Point x[3] is
// transformed through transform (if provided).
float vtkImplicitFunction::FunctionValue(float x[3])
{
  if ( ! this->Transform )
    {
    return this->EvaluateFunction(x);
    }

  else //pass point through transform
    {
    float pt[4];
    int i;

    pt[0] = x[0];
    pt[1] = x[1];
    pt[2] = x[2];
    pt[3] = 1.0;
    this->Transform->PointMultiply(pt,pt);
    if (pt[3] != 1.0 ) for (i=0; i<3; i++) pt[i] /= pt[3];

    return this->EvaluateFunction((float *)pt);
    }
}

// Description:
// Evaluate function gradient at position x-y-z and pass back vector. Point
// x[3] is transformed through transform (if provided).
void vtkImplicitFunction::FunctionGradient(float x[3], float g[3])
{
  if ( ! this->Transform )
    {
    this->EvaluateGradient(x,g);
    }

  else //pass point through transform
    {
    }
}

// Description:
// Overload standard modified time function. If Transform is modified,
// then this object is modified as well.
unsigned long vtkImplicitFunction::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long TransformMTime;

  if ( this->Transform != NULL )
    {
    TransformMTime = this->Transform->GetMTime();
    mTime = ( TransformMTime > mTime ? TransformMTime : mTime );
    }

  return mTime;
}

void vtkImplicitFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    os << indent << "Transform: (None)\n";
}

