/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImplicitWindowFunction.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImplicitWindowFunction* vtkImplicitWindowFunction::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitWindowFunction");
  if(ret)
    {
    return (vtkImplicitWindowFunction*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitWindowFunction;
}




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
  this->SetImplicitFunction(NULL);
}

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
  if ( scaledRange == 0.0 )
    {
    scaledRange = 1.0;
    }

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
    if ( fMtime > mtime )
      {
      mtime = fMtime;
      }
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
