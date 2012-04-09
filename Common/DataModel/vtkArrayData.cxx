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

vtkStandardNewMacro(vtkArrayData);

class vtkArrayData::implementation
{
public:
  std::vector<vtkArray*> Arrays;
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

  // See http://developers.sun.com/solaris/articles/cmp_stlport_libCstd.html
  // Language Feature: Partial Specializations 
  // Workaround
  
  int n=0;
#ifdef _RWSTD_NO_CLASS_PARTIAL_SPEC
  std::count(this->Implementation->Arrays.begin(),
                this->Implementation->Arrays.end(),array,n);
#else
  n=std::count(this->Implementation->Arrays.begin(),
                  this->Implementation->Arrays.end(),array);
#endif
  
  if(n!=0)
    {
    vtkErrorMacro(<< "Cannot add array twice.");
    return;
    }

  this->Implementation->Arrays.push_back(array);
  array->Register(0);

  this->Modified();
}

void vtkArrayData::ClearArrays()
{
  for(unsigned int i = 0; i != this->Implementation->Arrays.size(); ++i)
    {
    this->Implementation->Arrays[i]->Delete();
    }

  this->Implementation->Arrays.clear();

  this->Modified();
}

vtkIdType vtkArrayData::GetNumberOfArrays()
{
  return static_cast<vtkIdType>(this->Implementation->Arrays.size());
}

vtkArray* vtkArrayData::GetArray(vtkIdType index)
{
  if(index < 0 ||
     static_cast<size_t>(index) >= this->Implementation->Arrays.size())
    {
    vtkErrorMacro(<< "Array index out-of-range.");
    return 0;
    }
    
  return this->Implementation->Arrays[static_cast<size_t>(index)];
}

vtkArray* vtkArrayData::GetArrayByName(const char *name)
{
  if(!name || name[0]=='\0')
    {
    vtkErrorMacro(<< "No name passed into routine.");
    return 0;
    }
    
  vtkArray *temp = 0;
  for (vtkIdType ctr=0; ctr<this->GetNumberOfArrays(); ctr++)
    {
    temp = this->GetArray(ctr);
    if (temp && !strcmp(name, temp->GetName()))
      {
      break;
      }
    temp = 0;
    }
  return temp;
}

void vtkArrayData::ShallowCopy(vtkDataObject* other)
{
  if(vtkArrayData* const array_data = vtkArrayData::SafeDownCast(other))
    {
    this->ClearArrays();
    this->Implementation->Arrays = array_data->Implementation->Arrays;
    for(size_t i = 0;i != this->Implementation->Arrays.size();++i)
      {
      this->Implementation->Arrays[i]->Register(this);
      }
    this->Modified();
    }

  Superclass::ShallowCopy(other);
}

void vtkArrayData::DeepCopy(vtkDataObject* other)
{
  if(vtkArrayData* const array_data = vtkArrayData::SafeDownCast(other))
    {
    this->ClearArrays();
    for(size_t i = 0;i != array_data->Implementation->Arrays.size();++i)
      {
      this->Implementation->Arrays.push_back(
        array_data->Implementation->Arrays[i]->DeepCopy());
      }
    this->Modified();
    }

  Superclass::DeepCopy(other);
}

