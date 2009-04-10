/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayData.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArray.h"
#include "vtkArrayData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/vector>

//
// Standard functions
//

vtkCxxRevisionMacro(vtkArrayData, "1.5");
vtkStandardNewMacro(vtkArrayData);

class vtkArrayData::implementation
{
public:
  vtkstd::vector<vtkArray*> Arrays;
};

//----------------------------------------------------------------------------

vtkArrayData::vtkArrayData() :
  Implementation(new implementation())
{
}

//----------------------------------------------------------------------------

vtkArrayData::~vtkArrayData()
{
  this->ClearArrays();
  delete this->Implementation;
}

//----------------------------------------------------------------------------

void vtkArrayData::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for(unsigned int i = 0; i != this->Implementation->Arrays.size(); ++i)
    {
    os << indent << "Array: " << this->Implementation->Arrays[i] << endl;
    this->Implementation->Arrays[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkArrayData* vtkArrayData::GetData(vtkInformation* info)
{
  return info? vtkArrayData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

vtkArrayData* vtkArrayData::GetData(vtkInformationVector* v, int i)
{
  return vtkArrayData::GetData(v->GetInformationObject(i));
}

void vtkArrayData::AddArray(vtkArray* array)
{
  if(!array)
    {
    vtkErrorMacro(<< "Cannot add NULL array.");
    return;
    }
    
  if(vtkstd::count(this->Implementation->Arrays.begin(), this->Implementation->Arrays.end(), array))
    {
    vtkErrorMacro(<< "Cannot add array twice.");
    return;
    }

  this->Implementation->Arrays.push_back(array);
  array->Register(0);
}

void vtkArrayData::ClearArrays()
{
  for(unsigned int i = 0; i != this->Implementation->Arrays.size(); ++i)
    this->Implementation->Arrays[i]->Delete();

  this->Implementation->Arrays.clear();
}

vtkIdType vtkArrayData::GetNumberOfArrays()
{
  return this->Implementation->Arrays.size();
}

vtkArray* vtkArrayData::GetArray(vtkIdType index)
{
  if(index < 0 || index >= this->Implementation->Arrays.size())
    {
    vtkErrorMacro(<< "Array index out-of-range.");
    return 0;
    }
    
  return this->Implementation->Arrays[index];
}

