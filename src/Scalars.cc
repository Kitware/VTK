/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Scalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Scalars.hh"
#include "FScalars.hh"
#include "IdList.hh"
#include "Lut.hh"

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
// Determine (rmin,rmax) range of scalar values.
void vtkScalars::ComputeRange()
{
  int i;
  float s;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Range[0] =  LARGE_FLOAT;
    this->Range[1] =  -LARGE_FLOAT;
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
