/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.cxx
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
#include "vtkImplicitWindowFunction.h"

// Description:
// Construct object with window range (0,1) and window values (0,1).
vtkImplicitWindowFunction::vtkImplicitWindowFunction()
{
  this->ImplicitFunction = NULL;

  this->WindowRange[0] = 0.0;
  this->WindowRange[1] = 1.0;

  this->WindowValues[0] = 0.0;
  this->WindowValues[1] = 1.0;
}

vtkImplicitWindowFunction::~vtkImplicitWindowFunction()
{
}

// Description
// Evaluate window function.
float vtkImplicitWindowFunction::EvaluateFunction(float x[3])
{
  static int beenWarned=0;
  float value, diff1, diff2, scaledRange;

  if ( ! this->ImplicitFunction && ! beenWarned )
    {
    vtkErrorMacro(<<"Implicit function must be defined");
    beenWarned = 1;
    return 0.0;
    }

  value = this->ImplicitFunction->EvaluateFunction(x);

  diff1 = value - this->WindowRange[0];
  diff2 = value - this->WindowRange[1];

  scaledRange = (this->WindowValues[1] - this->WindowValues[0]) / 2.0;
  if ( scaledRange == 0.0 ) scaledRange = 1.0;

  if ( diff1 >= 0.0 && diff2 <= 0.0 ) //within window
    {
    if ( diff1 <= (-diff2) )
      {
      value = diff1 / scaledRange + this->WindowValues[0];
      }
    else
      {
      value = (-diff2) / scaledRange + this->WindowValues[0];
      }
    }

  else if ( diff1 < 0.0 ) //below window
    {
    value = diff1 / scaledRange + this->WindowValues[0];
    }

  else //above window
    {
    value = -diff2 / scaledRange + this->WindowValues[0];
    }

  return value;
}

// Description
// Evaluate window function gradient. Just return implicit function gradient.
void vtkImplicitWindowFunction::EvaluateGradient(float x[3], float n[3])
{
    if ( this->ImplicitFunction )
      {
      this->ImplicitFunction->EvaluateGradient(x,n);
      }
}

unsigned long int vtkImplicitWindowFunction::GetMTime()
{
  unsigned long int fMtime;
  unsigned long int mtime = this->vtkImplicitFunction::GetMTime();

  if ( this->ImplicitFunction )
  {
    fMtime = this->ImplicitFunction->GetMTime();
    if ( fMtime > mtime ) mtime = fMtime;
  }
  return mtime;
}

void vtkImplicitWindowFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  if ( this->ImplicitFunction )
    {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
    }
  else
    {
    os << indent << "No implicit function defined.\n";
    }

  os << indent << "Window Range: (" << this->WindowRange[0] 
     << ", " << this->WindowRange[1] << ")\n";

  os << indent << "Window Values: (" << this->WindowValues[0] 
     << ", " << this->WindowValues[1] << ")\n";

}
