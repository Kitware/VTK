/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourValues.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourValues.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkContourValues);

// Construct object with a single contour value at 0.0.
vtkContourValues::vtkContourValues()
{
  this->Contours = vtkDoubleArray::New();
  this->Contours->Allocate(64);
  this->Contours->InsertValue(0,0.0);
}

vtkContourValues::~vtkContourValues()
{
  this->Contours->Delete();
}

// Set the ith contour value.
void vtkContourValues::SetValue(int i, double value) 
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
double vtkContourValues::GetValue(int i) 
{
  i = (i < 0 ? 0 : i);
  i = (i > this->Contours->GetMaxId() ? this->Contours->GetMaxId() : i);
  return this->Contours->GetValue(i);
}

// Return a pointer to a list of contour values. The contents of the
// list will be garbage if the number of contours <= 0.
double *vtkContourValues::GetValues() 
{
  return this->Contours->GetPointer(0);
}

// Fill a supplied list with contour values. Make sure you've
// allocated memory of size GetNumberOfContours().
void vtkContourValues::GetValues(double *contourValues)
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
  double  *oldValues = NULL;

  if ( n != currentNumber )
    {
    this->Modified();

    // Keep a copy of the old values
    if ( currentNumber > 0 )
      {
      oldValues = new double[currentNumber];
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
void vtkContourValues::GenerateValues(int numContours, double rangeStart, 
                                     double rangeEnd)
{
  double range[2];

  range[0] = rangeStart;
  range[1] = rangeEnd;
  this->GenerateValues(numContours,range);
}

// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkContourValues::GenerateValues(int numContours, double range[2])
{
  int i;

  this->SetNumberOfContours(numContours);
  if (numContours == 1)
    {
    this->SetValue(0, range[0]);
    }
  else
    {
    for (i= 0; i < numContours; ++i)
      {
      // no we cannot factorize the ratio
      // (range[1] - range[0])/(numContours - 1) out of the loop.
      // we want the whole expression to be evaluated on the FPU.
      // see bug discussion: http://www.vtk.org/Bug/view.php?id=7887
      this->SetValue(i, range[0] + i*(range[1] - range[0])/(numContours - 1));
      }
    }
}

// Return the number of contours in the
int vtkContourValues::GetNumberOfContours() 
{
  return this->Contours->GetMaxId()+1;
}

void vtkContourValues::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  int i, numContours=this->Contours->GetMaxId() + 1;

  os << indent << "Contour Values: \n";
  for ( i=0; i < numContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Contours->GetValue(i) << "\n";
    }
}
