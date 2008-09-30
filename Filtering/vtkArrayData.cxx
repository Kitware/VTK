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

vtkCxxRevisionMacro(vtkArrayData, "1.1");
vtkStandardNewMacro(vtkArrayData);

vtkArrayData::vtkArrayData() :
  Array(0)
{
}

vtkArrayData::~vtkArrayData()
{
  if(this->Array)
    {
    this->Array->Delete();
    }
}

void vtkArrayData::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Array: " << (this->Array ? "" : "(none)") << endl;
  if(this->Array)
    {
    this->Array->PrintSelf(os, indent.GetNextIndent());
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

