/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDummyGPUInfoList.h"

#include "vtkGPUInfoListArray.h"

#include "vtkObjectFactory.h"
#include <assert.h>

vtkStandardNewMacro(vtkDummyGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkDummyGPUInfoList::Probe()
{
  if(!this->Probed)
    {
    this->Probed=true;
    this->Array=new vtkGPUInfoListArray;
    this->Array->v.resize(0); // no GPU.
    }
  assert("post: probed" && this->IsProbed());
}

// ----------------------------------------------------------------------------
vtkDummyGPUInfoList::vtkDummyGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkDummyGPUInfoList::~vtkDummyGPUInfoList()
{
}

// ----------------------------------------------------------------------------
void vtkDummyGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
