/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourValues.cxx
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
#include "vtkContourValues.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkContourValues* vtkContourValues::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkContourValues");
  if(ret)
    {
    return (vtkContourValues*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkContourValues;
}




// Construct object with a single contour value at 0.0.
vtkContourValues::vtkContourValues()
{
  this->Contours = vtkFloatArray::New();
  this->Contours->Allocate(64);
  this->Contours->InsertValue(0,0.0);
}

vtkContourValues::~vtkContourValues()
{
  this->Contours->Delete();
}

// Set the ith contour value.
void vtkContourValues::SetValue(int i, float value) 
{
  int numContours=this->Contours->GetMaxId()+1;
  i = (i < 0 ? 0 : i);

  if ( i >= numContours || value != this->Contours->GetValue(i) )
    {
    this->Modified();
    this->Contours->InsertValue(i,value);
    }
}

// Get the ith contour value. The return value will be clamped if the
// index i is out of range.
float vtkContourValues::GetValue(int i) 
{
  i = (i < 0 ? 0 : i);
  i = (i > this->Contours->GetMaxId() ? this->Contours->GetMaxId() : i);
  return this->Contours->GetValue(i);
}

// Return a pointer to a list of contour values. The contents of the
// list will be garbage if the number of contours <= 0.
float *vtkContourValues::GetValues() 
{
  return this->Contours->GetPointer(0);
}

// Fill a supplied list with contour values. Make sure you've
// allocated memory of size GetNumberOfContours().
void vtkContourValues::GetValues(float *contourValues)
{
  int i, numContours=this->Contours->GetMaxId()+1;

  for ( i=0; i < numContours; i++ )
    {
    contourValues[i] = this->Contours->GetValue(i);
    }
}

// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void vtkContourValues::SetNumberOfContours(const int number)
{
  int    currentNumber = this->Contours->GetMaxId()+1;
  int    n = ( number < 0 ? 0 : number);
  int    i;
  float  *oldValues = NULL;

  if ( n != currentNumber )
    {
    this->Modified();

    // Keep a copy of the old values
    if ( currentNumber > 0 )
      {
      oldValues = new float[currentNumber];
      for ( i = 0; i < currentNumber; i++ )
	{
	oldValues[i] = this->Contours->GetValue(i);
	}
      }

    this->Contours->SetNumberOfValues(n);

    // Copy them back in since the array may have been re-allocated
    if ( currentNumber > 0 )
      {
      int limit = (currentNumber < n)?(currentNumber):(n);
      for ( i = 0; i < limit; i++ )
	{
	this->Contours->SetValue( i, oldValues[i] );
	}
      delete [] oldValues;
      }

    }
  // Set the new contour values to 0.0
  if (n > currentNumber)
    {
    for ( i = currentNumber; i < n; i++ )
      {
      this->Contours->SetValue (i, 0.0);
      }
    }
}

// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkContourValues::GenerateValues(int numContours, float rangeStart, 
				     float rangeEnd)
{
  float range[2];

  range[0] = rangeStart;
  range[1] = rangeEnd;
  this->GenerateValues(numContours,range);
}

// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkContourValues::GenerateValues(int numContours, float range[2])
{
  float val, incr;
  int i;

  this->SetNumberOfContours(numContours);
  if (numContours == 1)
    {
    incr = 0;
    }
  else
    {
    incr = (range[1] - range[0]) / (numContours-1);
    }
  for (i=0, val=range[0]; i < numContours; i++, val+=incr)
    {
    this->SetValue(i,val);
    }
}

// Return the number of contours in the
int vtkContourValues::GetNumberOfContours() 
{
  return this->Contours->GetMaxId()+1;
}

void vtkContourValues::PrintSelf(ostream& os, vtkIndent indent)
{
  int i, numContours=this->Contours->GetMaxId() + 1;

  os << indent << "Contour Values: \n";
  for ( i=0; i < numContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Contours->GetValue(i) << "\n";
    }
}
