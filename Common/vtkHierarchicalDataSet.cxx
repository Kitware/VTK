/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataSet.h"

#include "vtkHierarchicalDataIterator.h"
#include "vtkHierarchicalDataSetInternal.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataSet, "1.1");

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::vtkHierarchicalDataSet()
{
  this->Internal = new vtkHierarchicalDataSetInternal;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::~vtkHierarchicalDataSet()
{
  this->InitializeDataSets();
  delete this->Internal;
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
  // We need to delete all nodes since we manage memory for them.
  vtkHierarchicalDataSetInternal::DataSetsIterator idx;
  for(idx  = this->Internal->DataSets.begin();
      idx != this->Internal->DataSets.end()  ;
      ++idx)
    {
    vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = *idx;
    vtkHierarchicalDataSetInternal::LevelDataSetsIterator ldx;
    for (ldx = ldataSets.begin(); ldx != ldataSets.end(); ldx++)
      {
      delete *ldx;
      }
    idx->clear();
    }
  this->Internal->DataSets.clear();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::Initialize()
{
  this->Superclass::Initialize();
  this->InitializeDataSets();
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataSet::GetNumberOfLevels()
{
  return this->Internal->DataSets.size();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::SetNumberOfLevels(unsigned int numLevels)
{
  if (numLevels == this->GetNumberOfLevels())
    {
    return;
    }
  unsigned int curNumLevels = this->Internal->DataSets.size();
  if (curNumLevels > numLevels)
    {
    // We need to delete all extra nodes since we manage memory for them.
    for(unsigned int i=numLevels; i<curNumLevels; i++)
      {
      vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
        this->Internal->DataSets[i];
      vtkHierarchicalDataSetInternal::LevelDataSetsIterator ldx;
      for (ldx = ldataSets.begin(); ldx != ldataSets.end(); ldx++)
        {
        delete *ldx;
        }
      }
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
  if (curNumDataSets > numDataSets)
    {
    for (unsigned int i=numDataSets; i<curNumDataSets; i++)
      {
      delete ldataSets[i];
      }
    }
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

  if (ldataSets[id])
    {
    ldataSets[id]->DisconnectAll(vtkHDSNodeRef(level, id), 
                                 this->Internal->DataSets);
    ldataSets[id]->DataSet = 0;
    }
  else
    {
    ldataSets[id] = this->NewNode();
    }
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
  
  if (!ldataSets[id])
    {
    ldataSets[id] = this->NewNode();
    }

  ldataSets[id]->DataSet = ds;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataSet::IsNodePresent(unsigned int level, 
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

  return 1;
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

  return ldataSets[id]->DataSet.GetPointer();
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
  this->Modified();

  vtkHierarchicalDataSet* from = vtkHierarchicalDataSet::SafeDownCast(src);
  if (from)
    {
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
  this->Modified();

  vtkHierarchicalDataSet* from = vtkHierarchicalDataSet::SafeDownCast(src);
  if (from)
    {
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
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

