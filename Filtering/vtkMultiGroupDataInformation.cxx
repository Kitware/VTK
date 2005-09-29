/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataInformation.h"

#include "vtkMultiGroupDataInformation.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkMultiGroupDataInformation, "1.1");
vtkStandardNewMacro(vtkMultiGroupDataInformation);

#include <vtkstd/vector>

struct vtkMultiGroupDataInformationInternal
{
  typedef vtkstd::vector<vtkSmartPointer<vtkInformation> > GroupInformationType;
  typedef vtkstd::vector<GroupInformationType> DataInformationType;

  DataInformationType DataInformation;
};

//----------------------------------------------------------------------------
vtkMultiGroupDataInformation::vtkMultiGroupDataInformation()
{
  this->Internal = new vtkMultiGroupDataInformationInternal;
}

//----------------------------------------------------------------------------
vtkMultiGroupDataInformation::~vtkMultiGroupDataInformation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataInformation::Clear()
{
  this->Internal->DataInformation.clear();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataInformation::DeepCopy(
  vtkMultiGroupDataInformation* from)
{
  unsigned int numGroups = from->GetNumberOfGroups(); 
  this->SetNumberOfGroups(numGroups);
  for (unsigned int i=0; i<numGroups; i++)
    {
    unsigned int numDataSets = from->GetNumberOfDataSets(i);
    this->SetNumberOfDataSets(i, numDataSets);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      vtkInformation* toInf   = this->GetInformation(i, j);
      vtkInformation* fromInf = from->GetInformation(i, j);
      toInf->Copy(fromInf);
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkMultiGroupDataInformation::GetNumberOfGroups()
{
  return this->Internal->DataInformation.size();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataInformation::SetNumberOfGroups(unsigned int numGroups)
{
  if (numGroups <= this->GetNumberOfGroups())
    {
    return;
    }
  this->Internal->DataInformation.resize(numGroups);

  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkMultiGroupDataInformation::GetNumberOfDataSets(
  unsigned int group)
{
  if (this->Internal->DataInformation.size() <= group)
    {
    return 0;
    }

  vtkMultiGroupDataInformationInternal::GroupInformationType& linf = 
    this->Internal->DataInformation[group];

  return linf.size();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataInformation::SetNumberOfDataSets(
  unsigned int group, unsigned int numDataSets)
{
  if (numDataSets <= this->GetNumberOfDataSets(group))
    {
    return;
    }
  // Make sure that there is a vector allocated for this group
  if (this->Internal->DataInformation.size() <= group)
    {
    this->SetNumberOfGroups(group+1);
    }

  vtkMultiGroupDataInformationInternal::GroupInformationType& linf = 
    this->Internal->DataInformation[group];
  linf.resize(numDataSets);

  this->Modified();
}


//----------------------------------------------------------------------------
int vtkMultiGroupDataInformation::HasInformation(
  unsigned int group, unsigned int id)
{
  if (this->Internal->DataInformation.size() <= group)
    {
    return 0;
    }

  vtkMultiGroupDataInformationInternal::GroupInformationType& linf = 
    this->Internal->DataInformation[group];
  if (linf.size() <= id)
    {
    return 0;
    }

  vtkInformation* inf = linf[id].GetPointer();
  if (!inf)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkInformation* vtkMultiGroupDataInformation::GetInformation(
  unsigned int group, unsigned int id)
{
  if (this->Internal->DataInformation.size() <= group)
    {
    return 0;
    }

  vtkMultiGroupDataInformationInternal::GroupInformationType& linf = 
    this->Internal->DataInformation[group];
  if (linf.size() <= id)
    {
    return 0;
    }

  vtkInformation* inf = linf[id].GetPointer();
  if (!inf)
    {
    inf = vtkInformation::New();
    linf[id] = inf;
    inf->Delete();
    }

  return inf;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

