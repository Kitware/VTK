/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkScalars.hh"
#include "vtkFloatScalars.hh"
#include "vtkShortScalars.hh"
#include "vtkIdList.hh"
#include "vtkLookupTable.hh"

vtkScalars::vtkScalars()
{
  this->Range[0] = this->Range[2] = this->Range[4] = this->Range[6] = 0.0;
  this->Range[1] = this->Range[3] = this->Range[5] = this->Range[7] = 1.0;

  this->LookupTable = NULL;
}

vtkScalars::~vtkScalars()
{
  if ( this->LookupTable ) this->LookupTable->UnRegister(this);
}

// Description:
// Given a list of pt ids, return an array of scalar values.
void vtkScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,this->GetScalar(ptId.GetId(i)));
    }
}


// Description:
// Return all the scalar values as a short scalar
vtkShortScalars *vtkScalars::GetAllShortScalars()
{
  int num = this->GetNumberOfScalars();
  vtkShortScalars *result = new vtkShortScalars(num);
  short *arr;
  
  arr = result->WritePtr(0,num);
    
  for (int i=0; i< num; i++)
    {
    arr[i] = (short)(256.0*this->GetScalar(i));
    }
  
  return result;
}

// Description:
// Determine (rmin,rmax) range of scalar values.
void vtkScalars::ComputeRange()
{
  int i;
  float s;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Range[0] =  VTK_LARGE_FLOAT;
    this->Range[1] =  -VTK_LARGE_FLOAT;
    for (i=0; i<this->GetNumberOfScalars(); i++)
      {
      s = this->GetScalar(i);
      if ( s < this->Range[0] ) this->Range[0] = s;
      if ( s > this->Range[1] ) this->Range[1] = s;
      }

    this->ComputeTime.Modified();
    }
}

// Description:
// Return the range of scalar values. Data returned as pointer to float array
// of length 2.
float *vtkScalars::GetRange()
{
  this->ComputeRange();
  return this->Range;
}

// Description:
// Return the range of scalar values. Range copied into array provided.
void vtkScalars::GetRange(float range[2])
{
  this->ComputeRange();
  range[0] = this->Range[0];
  range[1] = this->Range[1];
}

void vtkScalars::CreateDefaultLookupTable()
{
  if ( this->LookupTable ) this->LookupTable->UnRegister(this);
  this->LookupTable = new vtkLookupTable;
  this->LookupTable->Register(this);
}

void vtkScalars::SetLookupTable(vtkLookupTable *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable ) this->LookupTable->UnRegister(this);
    this->LookupTable = lut;
    this->LookupTable->Register(this);
    this->Modified();
    }
}

void vtkScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  float *range;

  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Scalars: " << this->GetNumberOfScalars() << "\n";
  range = this->GetRange();
  os << indent << "Range: (" << range[0] << ", " << range[1] << ")\n";
  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }
}
