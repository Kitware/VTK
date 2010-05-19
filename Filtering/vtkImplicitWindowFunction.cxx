/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitWindowFunction.h"

#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImplicitWindowFunction);
vtkCxxSetObjectMacro(vtkImplicitWindowFunction,ImplicitFunction,vtkImplicitFunction);

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
double vtkImplicitWindowFunction::EvaluateFunction(double x[3])
{
  static int beenWarned=0;
  double value, diff1, diff2, scaledRange;

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
void vtkImplicitWindowFunction::EvaluateGradient(double x[3], double n[3])
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
  this->Superclass::PrintSelf(os,indent);

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

//----------------------------------------------------------------------------
void vtkImplicitWindowFunction::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkImplicitWindowFunction::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void
vtkImplicitWindowFunction
::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->ImplicitFunction,
                            "ImplicitFunction");
}
