/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataInformation.h"

#include "vtkHierarchicalDataInformation.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkHierarchicalDataInformation, "1.3");
vtkStandardNewMacro(vtkHierarchicalDataInformation);

#include <vtkstd/vector>

struct vtkHierarchicalDataInformationInternal
{
  typedef vtkstd::vector<vtkSmartPointer<vtkInformation> > LevelInformationType;
  typedef vtkstd::vector<LevelInformationType> DataInformationType;

  DataInformationType DataInformation;
};

//----------------------------------------------------------------------------
vtkHierarchicalDataInformation::vtkHierarchicalDataInformation()
{
  this->Internal = new vtkHierarchicalDataInformationInternal;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataInformation::~vtkHierarchicalDataInformation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataInformation::Clear()
{
  this->Internal->DataInformation.clear();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataInformation::DeepCopy(
  vtkHierarchicalDataInformation* from)
{
  unsigned int numLevels = from->GetNumberOfLevels(); 
  this->SetNumberOfLevels(numLevels);
  for (unsigned int i=0; i<numLevels; i++)
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
unsigned int vtkHierarchicalDataInformation::GetNumberOfLevels()
{
  return this->Internal->DataInformation.size();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataInformation::SetNumberOfLevels(unsigned int numLevels)
{
  if (numLevels <= this->GetNumberOfLevels())
    {
    return;
    }
  this->Internal->DataInformation.resize(numLevels);

  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataInformation::GetNumberOfDataSets(
  unsigned int level)
{
  if (this->Internal->DataInformation.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataInformationInternal::LevelInformationType& linf = 
    this->Internal->DataInformation[level];

  return linf.size();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataInformation::SetNumberOfDataSets(
  unsigned int level, unsigned int numDataSets)
{
  if (numDataSets <= this->GetNumberOfDataSets(level))
    {
    return;
    }
  // Make sure that there is a vector allocated for this level
  if (this->Internal->DataInformation.size() <= level)
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkHierarchicalDataInformationInternal::LevelInformationType& linf = 
    this->Internal->DataInformation[level];
  linf.resize(numDataSets);

  this->Modified();
}


//----------------------------------------------------------------------------
int vtkHierarchicalDataInformation::HasInformation(
  unsigned int level, unsigned int id)
{
  if (this->Internal->DataInformation.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataInformationInternal::LevelInformationType& linf = 
    this->Internal->DataInformation[level];
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
vtkInformation* vtkHierarchicalDataInformation::GetInformation(
  unsigned int level, unsigned int id)
{
  if (this->Internal->DataInformation.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataInformationInternal::LevelInformationType& linf = 
    this->Internal->DataInformation[level];
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
void vtkHierarchicalDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

