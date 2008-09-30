/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFactoredArrayData.cxx
  
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
#include "vtkFactoredArrayData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/vector>

//
// Standard functions
//

vtkCxxRevisionMacro(vtkFactoredArrayData, "1.1");
vtkStandardNewMacro(vtkFactoredArrayData);

class vtkFactoredArrayData::implementation
{
public:
  vtkstd::vector<vtkArray*> Arrays;
};

//----------------------------------------------------------------------------

vtkFactoredArrayData::vtkFactoredArrayData() :
  Implementation(new implementation())
{
}

//----------------------------------------------------------------------------

vtkFactoredArrayData::~vtkFactoredArrayData()
{
  this->ClearArrays();
  delete this->Implementation;
}

//----------------------------------------------------------------------------

void vtkFactoredArrayData::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

vtkFactoredArrayData* vtkFactoredArrayData::GetData(vtkInformation* info)
{
  return info? vtkFactoredArrayData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

vtkFactoredArrayData* vtkFactoredArrayData::GetData(vtkInformationVector* v, int i)
{
  return vtkFactoredArrayData::GetData(v->GetInformationObject(i));
}

void vtkFactoredArrayData::AddArray(vtkArray* array)
{
  if(vtkstd::count(this->Implementation->Arrays.begin(), this->Implementation->Arrays.end(), array))
    {
    vtkErrorMacro(<< "vtkFactoredArrayData::AddArray() - cannot add array twice");
    return;
    }

  this->Implementation->Arrays.push_back(array);
  array->Register(0);
}

void vtkFactoredArrayData::ClearArrays()
{
  for(vtkIdType i = 0; i != this->Implementation->Arrays.size(); ++i)
    this->Implementation->Arrays[i]->Delete();

  this->Implementation->Arrays.clear();
}

vtkIdType vtkFactoredArrayData::GetNumberOfArrays()
{
  return this->Implementation->Arrays.size();
}

vtkArray* vtkFactoredArrayData::GetArray(vtkIdType index)
{
  return this->Implementation->Arrays[index];
}

