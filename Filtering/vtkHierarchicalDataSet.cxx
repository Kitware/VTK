/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataSet.h"

#include "vtkDataSet.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkHierarchicalDataIterator.h"
#include "vtkHierarchicalDataSetInternal.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataSet, "1.7");
vtkStandardNewMacro(vtkHierarchicalDataSet);

vtkCxxSetObjectMacro(vtkHierarchicalDataSet,HierarchicalDataInformation,vtkHierarchicalDataInformation);

vtkInformationKeyMacro(vtkHierarchicalDataSet,LEVEL,Integer);

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::vtkHierarchicalDataSet()
{
  this->Internal = new vtkHierarchicalDataSetInternal;
  this->HierarchicalDataInformation = vtkHierarchicalDataInformation::New();
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::~vtkHierarchicalDataSet()
{
  this->InitializeDataSets();
  delete this->Internal;
  this->SetHierarchicalDataInformation(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkHierarchicalDataSet::NewIterator()
{
  vtkHierarchicalDataIterator* iter = vtkHierarchicalDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
vtkHDSNode* vtkHierarchicalDataSet::NewNode()
{
  return new vtkHDSNode;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::InitializeDataSets()
{
  this->Internal->DataSets.clear();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::Initialize()
{
  this->Superclass::Initialize();
  this->InitializeDataSets();
  this->SetHierarchicalDataInformation(0);
  this->HierarchicalDataInformation = vtkHierarchicalDataInformation::New();
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataSet::GetNumberOfLevels()
{
  return this->Internal->DataSets.size();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::SetNumberOfLevels(unsigned int numLevels)
{
  this->HierarchicalDataInformation->SetNumberOfLevels(numLevels);
  if (numLevels == this->GetNumberOfLevels())
    {
    return;
    }
  this->Internal->DataSets.resize(numLevels);
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataSet::GetNumberOfDataSets(unsigned int level)
{
  if (this->Internal->DataSets.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];

  return ldataSets.size();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::SetNumberOfDataSets(unsigned int level, 
                                                 unsigned int numDataSets)
{
  this->HierarchicalDataInformation->SetNumberOfDataSets(level, numDataSets);
  if (numDataSets == this->GetNumberOfDataSets(level))
    {
    return;
    }
  // Make sure that there is a vector allocated for this level
  if (this->Internal->DataSets.size() <= level)
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  
  // We need to delete all extra nodes since we manage memory for them.
  unsigned int curNumDataSets = ldataSets.size();
  ldataSets.resize(numDataSets);

  // Assign NULL to all new pointers. We use this later to figure out
  // whether a node allocated for a particular entry.
  if (curNumDataSets < numDataSets)
    {
    for (unsigned int i=curNumDataSets; i<numDataSets; i++)
      {
      ldataSets[i] = 0;
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::InitializeNode(unsigned int level, 
                                            unsigned int id)
{

  // Make sure that there is a vector allocated for this level
  if (this->Internal->DataSets.size() <= level)
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  
  // Make sure that the size of the vector for this level is big enough.
  if (ldataSets.size() <= id)
    {
    this->SetNumberOfDataSets(level, id+1);
    }

  ldataSets[id] = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::SetDataSet(unsigned int level, 
                                        unsigned int id, 
                                        vtkDataObject* ds)
{
  // Make sure that there is a vector allocated for this level
  if (this->Internal->DataSets.size() <= level)
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  
  // Make sure that the size of the vector for this level is big enough.
  if (ldataSets.size() <= id)
    {
    this->SetNumberOfDataSets(level, id+1);
    }
  
  ldataSets[id] = ds;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::AddDataSet(vtkInformation* index, vtkDataObject* dobj)
{
  if (index->Has(INDEX()) && index->Has(LEVEL()))
    {
    this->SetDataSet(index->Get(LEVEL()), index->Get(INDEX()), dobj);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHierarchicalDataSet::GetDataSet(unsigned int level, 
                                                  unsigned int id)
{
  if (this->Internal->DataSets.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  if (ldataSets.size() <= id)
    {
    return 0;
    }

  if (!ldataSets[id])
    {
    return 0;
    }

  return ldataSets[id];
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHierarchicalDataSet::GetDataSet(vtkInformation* index)
{
  if (index->Has(INDEX()) && index->Has(LEVEL()))
    {
    return this->GetDataSet(index->Get(LEVEL()), index->Get(INDEX()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::ShallowCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Superclass::ShallowCopy(src);

  vtkHierarchicalDataSet* from = vtkHierarchicalDataSet::SafeDownCast(src);
  if (from)
    {
    this->SetHierarchicalDataInformation(from->HierarchicalDataInformation);
    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        this->SetDataSet(i, j, from->GetDataSet(i,j));
        }
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::DeepCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Superclass::ShallowCopy(src);
  this->SetHierarchicalDataInformation(0);
  this->HierarchicalDataInformation = vtkHierarchicalDataInformation::New();

  vtkHierarchicalDataSet* from = vtkHierarchicalDataSet::SafeDownCast(src);
  if (from)
    {
    this->HierarchicalDataInformation->DeepCopy(
      from->HierarchicalDataInformation);
    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkDataObject* ds = from->GetDataSet(i,j);
        if (ds)
          {
          vtkDataObject* copy = ds->NewInstance();
          copy->DeepCopy(ds);
          this->SetDataSet(i, j, copy);
          }
        }
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkHierarchicalDataSet::GetNumberOfPoints()
{
  vtkIdType numPts = 0;

  vtkCompositeDataIterator* iterator = this->NewIterator();
  iterator->InitTraversal();
  while (!iterator->IsDoneWithTraversal())
    {
    vtkDataObject* dObj = iterator->GetCurrentDataObject();
    if (dObj)
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(dObj);
      if (ds)
        {
        numPts += ds->GetNumberOfPoints();
        }
      else
        {
        vtkHierarchicalDataSet* hds = 
          vtkHierarchicalDataSet::SafeDownCast(dObj);
        if (hds)
          {
          numPts += hds->GetNumberOfPoints();
          }
        }
      }
    iterator->GoToNextItem();
    }
  iterator->Delete();

  return numPts;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "HierarchicalDataInformation: ";
  if (this->HierarchicalDataInformation)
    {
    os << endl;
    this->HierarchicalDataInformation->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

